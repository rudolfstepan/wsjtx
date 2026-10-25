#include "WebControlServer.hpp"

#include <QTcpSocket>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QApplication>
#include <QAbstractButton>
#include <QAction>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QSlider>
#include <QTextEdit>
#include <QTextBlock>
#include <QTextFragment>
#include <QTextCharFormat>
#include <QTextBlockFormat>
#include <QTimer>
#include <QUuid>
#include <QBuffer>
#include <QPixmap>
#include <QUrl>
#include <QValidator>
#include <QTabWidget>
#include <algorithm>
#include <cmath>

namespace {
QStringList const controls {
  "monitorButton", "stopButton", "DecodeButton", "EraseButton", "autoButton", "stopTxButton", "tuneButton",
  "logQSOButton", "dxCallEntry", "dxGridEntry", "lookupButton", "genStdMsgsPushButton",
  "TxFreqSpinBox", "RxFreqSpinBox", "rptSpinBox", "txFirstCheckBox", "cbAutoSeq", "cbHoldTxFreq", "cbCQonly",
  "pbR2T", "pbT2R", "bandComboBox", "outAttenuation", "tabWidget", "sbFtol", "cbTx6", "cbRxAll", "respondComboBox",
  "txb1", "txb2", "txb3", "txb4", "txb5", "txb6",
  "tx1", "tx2", "tx3", "tx4", "tx5", "tx6", "txrb1", "txrb2", "txrb3", "txrb4", "txrb5", "txrb6"
};
QStringList const modes {"actionFT8", "actionFT4", "actionFT2", "actionREIST16", "actionJT65", "actionJT9",
  "actionQ65", "actionMSK144", "actionWSPR", "actionFST4", "actionFST4W"};
QStringList const logFields {"call", "grid", "name", "sent", "rcvd", "txPower", "comments", "exchSent", "exchRcvd"};

bool validAccessKey (QString const& key)
{
  return key.size() <= 128 && std::all_of(key.begin(), key.end(), [] (QChar ch) {
    return ch.unicode() >= 0x20 && ch.unicode() <= 0x7e;
  });
}

QString cssColor (QColor const& color)
{
  // CSS puts the alpha channel last, whereas QColor::HexArgb puts it first.
  return color.name(QColor::HexRgb) + (color.alpha() == 255 ? QString() : QString("%1").arg(color.alpha(), 2, 16, QLatin1Char('0')));
}

QJsonObject textStyle (QTextCharFormat const& format)
{
  QJsonObject result;
  if (format.foreground().style() != Qt::NoBrush) result["foreground"] = cssColor(format.foreground().color());
  if (format.background().style() != Qt::NoBrush) result["background"] = cssColor(format.background().color());
  result["bold"] = format.fontWeight() >= QFont::Bold;
  result["italic"] = format.fontItalic();
  result["underline"] = format.underlineStyle() != QTextCharFormat::NoUnderline;
  result["strikeout"] = format.fontStrikeOut();
  if (!format.toolTip().isEmpty()) result["tooltip"] = format.toolTip();
  return result;
}

QJsonArray formattedRuns (QTextBlock const& block, QList<QTextEdit::ExtraSelection> const& selections)
{
  // PSK Reporter uses ExtraSelection overlays rather than changing the document.
  QList<QTextEdit::ExtraSelection> overlays;
  for (auto const& selection : selections)
    if (selection.cursor.selectionStart() < block.position() + block.length() - 1 &&
        selection.cursor.selectionEnd() > block.position()) overlays.append(selection);
  QJsonArray runs;
  for (auto it = block.begin(); !it.atEnd(); ++it) {
    auto fragment = it.fragment();
    if (!fragment.isValid()) continue;
    QVector<int> boundaries {fragment.position(), fragment.position() + fragment.length()};
    for (auto const& selection : overlays) {
      boundaries.append(qBound(boundaries.first(), selection.cursor.selectionStart(), fragment.position() + fragment.length()));
      boundaries.append(qBound(boundaries.first(), selection.cursor.selectionEnd(), fragment.position() + fragment.length()));
    }
    std::sort(boundaries.begin(), boundaries.end());
    boundaries.erase(std::unique(boundaries.begin(), boundaries.end()), boundaries.end());
    for (int i = 1; i < boundaries.size(); ++i) {
      auto format = fragment.charFormat();
      for (auto const& selection : overlays)
        if (selection.cursor.selectionStart() <= boundaries[i-1] && selection.cursor.selectionEnd() > boundaries[i-1]) format.merge(selection.format);
      runs.append(QJsonObject{{"text", fragment.text().mid(boundaries[i-1] - fragment.position(), boundaries[i] - boundaries[i-1])}, {"style", textStyle(format)}});
    }
  }
  return runs;
}

QJsonObject describe (QWidget * w)
{
  QJsonObject o {{"enabled", w->isEnabled()}, {"visible", w->isVisible()}};
  if (auto b = qobject_cast<QAbstractButton *>(w)) {
    o["kind"] = "button"; o["text"] = b->text().remove('&');
    o["checkable"] = b->isCheckable(); o["checked"] = b->isChecked();
  } else if (auto e = qobject_cast<QLineEdit *>(w)) {
    o["kind"] = "text"; o["value"] = e->text(); o["enabled"] = w->isEnabled() && !e->isReadOnly();
  } else if (auto s = qobject_cast<QSpinBox *>(w)) {
    o["kind"] = "number"; o["value"] = s->value(); o["min"] = s->minimum(); o["max"] = s->maximum();
  } else if (auto s = qobject_cast<QSlider *>(w)) {
    o["kind"] = "range"; o["value"] = s->value(); o["min"] = s->minimum(); o["max"] = s->maximum();
  } else if (auto t = qobject_cast<QTabWidget *>(w)) {
    o["kind"] = "select"; o["value"] = t->currentIndex();
    QJsonArray items;
    for (int i = 0; i < t->count(); ++i) items.append(t->tabText(i));
    o["items"] = items;
  } else if (auto c = qobject_cast<QComboBox *>(w)) {
    o["kind"] = "select"; o["value"] = c->currentIndex(); o["text"] = c->currentText();
    QJsonArray items;
    for (int i = 0; i < c->count(); ++i) items.append(c->itemText(i));
    o["items"] = items; o["editable"] = c->isEditable();
  }
  return o;
}

QString setControl (QWidget * w, QJsonValue const& v)
{
  if (!w || !w->isEnabled() || !w->isVisible()) return "Control unavailable";
  if (auto b = qobject_cast<QAbstractButton *>(w)) {
    if (b->isCheckable()) {
      if (!v.isBool()) return "Boolean required";
      if (b->isChecked() != v.toBool()) b->click();
    } else b->click();
  } else if (auto e = qobject_cast<QLineEdit *>(w)) {
    if (!v.isString() || e->isReadOnly() || v.toString().size() > qMin(128, e->maxLength())) return "Invalid text";
    QString text = v.toString(); int pos = text.size();
    if (e->validator() && e->validator()->validate(text, pos) != QValidator::Acceptable) return "Invalid value";
    e->setText(text);
    Q_EMIT e->textEdited(text);
    Q_EMIT e->editingFinished();
  } else if (auto s = qobject_cast<QSpinBox *>(w)) {
    if (!v.isDouble() || v.toDouble() != std::floor(v.toDouble()) || v.toDouble() < s->minimum() || v.toDouble() > s->maximum()) return "Invalid number";
    s->setValue(v.toInt());
  } else if (auto s = qobject_cast<QSlider *>(w)) {
    if (!v.isDouble() || v.toDouble() != std::floor(v.toDouble()) || v.toDouble() < s->minimum() || v.toDouble() > s->maximum()) return "Invalid number";
    s->setValue(v.toInt());
  } else if (auto t = qobject_cast<QTabWidget *>(w)) {
    if (!v.isDouble() || v.toDouble() != std::floor(v.toDouble()) || v.toDouble() < 0 || v.toDouble() >= t->count() || !t->isTabEnabled(v.toInt())) return "Invalid tab";
    t->setCurrentIndex(v.toInt());
  } else if (auto c = qobject_cast<QComboBox *>(w)) {
    if (v.isString() && c->isEditable() && c->objectName() != "bandComboBox") return setControl(c->lineEdit(), v);
    if (!v.isDouble() || v.toDouble() != std::floor(v.toDouble()) || v.toDouble() < 0 || v.toDouble() >= c->count()) return "Invalid selection";
    c->setCurrentIndex(v.toInt());
    Q_EMIT c->activated(v.toInt());
  } else return "Unsupported control";
  return {};
}
}

WebControlServer::WebControlServer (QWidget * window, QWidget * waterfall, QDialog * log)
  : QTcpServer(window), window_(window), waterfall_(waterfall), log_(log)
{
  setMaxPendingConnections(16);
  connect(this, &QTcpServer::newConnection, this, &WebControlServer::acceptConnections);
}

bool WebControlServer::start (quint16 port, QString const& accessKey)
{
  auto customKey = accessKey.trimmed();
  if (!validAccessKey(customKey)) return false;
  stop();
  token_ = customKey.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) + QUuid::createUuid().toString(QUuid::WithoutBraces) : customKey;
  return listen(QHostAddress::AnyIPv4, port);
}

void WebControlServer::stop ()
{
  close(); token_.clear();
  for (auto s : findChildren<QTcpSocket *>()) { s->abort(); s->deleteLater(); }
}

void WebControlServer::showSettings ()
{
  QDialog dialog(window_); dialog.setWindowTitle(tr("Web control (experimental)")); dialog.setObjectName("webControlSettings");
  QFormLayout layout(&dialog);
  QLabel description(tr("Experimental feature: operate this station from a browser on your LAN.\n"
    "The access key permits transmitting. Use a trusted LAN or VPN; HTTP is unencrypted.\n"
    "Web control is off after application restart. Losing the browser connection does not halt TX."));
  description.setWordWrap(true); layout.addRow(&description);
  QSpinBox port; port.setObjectName("webControlPort"); port.setRange(1024, 65535); port.setValue(isListening() ? serverPort() : 8080);
  layout.addRow(tr("Port"), &port);
  QLineEdit key; key.setObjectName("webControlKey"); key.setMaxLength(128);
  key.setPlaceholderText(tr("Enter your own key, or leave empty for a random key"));
  layout.addRow(tr("Access key"), &key);
  QLabel addresses; addresses.setTextInteractionFlags(Qt::TextSelectableByMouse); layout.addRow(&addresses);
  QPushButton startButton(tr("Start / apply key")), stopButton(tr("Stop web control")), randomButton(tr("Start / new random key"));
  startButton.setObjectName("webControlStart"); randomButton.setObjectName("webControlRandom");
  layout.addRow(&startButton, &stopButton);
  layout.addRow(&randomButton);
  auto refresh = [&] {
    key.setText(token_);
    QStringList urls;
    if (isListening()) {
      urls << QString("http://127.0.0.1:%1").arg(serverPort());
      for (auto a : QNetworkInterface::allAddresses())
        if (a.protocol() == QAbstractSocket::IPv4Protocol && !a.isLoopback()) urls << QString("http://%1:%2").arg(a.toString()).arg(serverPort());
    }
    addresses.setText(urls.isEmpty() ? tr("Web control is stopped") : urls.join('\n'));
  };
  connect(&startButton, &QPushButton::clicked, &dialog, [&] {
    if (!validAccessKey(key.text().trimmed())) {
      QMessageBox::warning(&dialog, tr("Web control (experimental)"), tr("Use up to 128 ASCII characters (letters, digits, spaces and punctuation), or leave the key empty to generate one."));
      return;
    }
    if (!start(quint16(port.value()), key.text())) QMessageBox::warning(&dialog, tr("Web control (experimental)"), errorString());
    refresh();
  });
  connect(&randomButton, &QPushButton::clicked, &dialog, [&] {
    if (!start(quint16(port.value()))) QMessageBox::warning(&dialog, tr("Web control (experimental)"), errorString());
    refresh();
  });
  connect(&stopButton, &QPushButton::clicked, &dialog, [&] { stop(); refresh(); });
  QDialogButtonBox buttons(QDialogButtonBox::Close); layout.addRow(&buttons);
  connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
  refresh(); dialog.exec();
}

void WebControlServer::acceptConnections ()
{
  while (hasPendingConnections()) {
    auto s = nextPendingConnection();
    if (findChildren<QTcpSocket *>().size() > 24) { s->abort(); s->deleteLater(); continue; }
    s->setReadBufferSize(32768);
    auto timeout = new QTimer(s); timeout->setSingleShot(true); timeout->start(5000);
    connect(timeout, &QTimer::timeout, s, [s] { s->abort(); });
    connect(s, &QTcpSocket::disconnected, s, &QObject::deleteLater);
    connect(s, &QTcpSocket::readyRead, this, [this, s] { readRequest(s); });
  }
}

void WebControlServer::respond (QTcpSocket * s, int status, QByteArray const& body, QByteArray const& type)
{
  s->setProperty("done", true);
  QByteArray headers = "HTTP/1.1 " + QByteArray::number(status) + (status == 200 ? " OK\r\n" : " Error\r\n");
  headers += "Content-Type: " + type + "\r\nContent-Length: " + QByteArray::number(body.size());
  headers += "\r\nConnection: close\r\nCache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\n"
    "Referrer-Policy: no-referrer\r\nContent-Security-Policy: default-src 'self'; script-src 'self'; style-src 'self'; img-src 'self' blob:; frame-ancestors 'none'; base-uri 'none'; form-action 'self'\r\n\r\n";
  s->write(headers); s->write(body); s->disconnectFromHost();
}

void WebControlServer::readRequest (QTcpSocket * s)
{
  if (s->property("done").toBool()) return;
  QByteArray data = s->property("request").toByteArray() + s->readAll();
  if (data.size() > 24576) { respond(s, 413, "{}"); return; }
  s->setProperty("request", data);
  int end = data.indexOf("\r\n\r\n");
  if (end < 0) { if (data.size() > 8192) respond(s, 431, "{}"); return; }
  if (end > 8192) { respond(s, 431, "{}"); return; }
  auto lines = data.left(end).split('\n');
  auto request = lines.takeFirst().trimmed().split(' ');
  if (request.size() != 3 || request[2] != "HTTP/1.1") { respond(s, 400, "{}"); return; }
  QMap<QByteArray, QByteArray> headers;
  for (auto line : lines) {
    int colon = line.indexOf(':');
    if (colon < 1) { respond(s, 400, "{}"); return; }
    auto name = line.left(colon).trimmed().toLower();
    if (headers.contains(name)) { respond(s, 400, "{}"); return; }
    headers[name] = line.mid(colon + 1).trimmed();
  }
  bool ok = true;
  int length = headers.contains("content-length") ? headers["content-length"].toInt(&ok) : 0;
  if (!ok || length < 0 || length > 16384 || headers.contains("transfer-encoding") || !headers.contains("host")) { respond(s, 400, "{}"); return; }
  if (data.size() < end + 4 + length) return;
  if (data.size() != end + 4 + length) { respond(s, 400, "{}"); return; }
  auto path = request[1]; auto method = request[0];
  // A query string does not change the resource path (including the common /?).
  // Authentication continues to use only the Authorization header below.
  int query = path.indexOf('?');
  if (query >= 0) path.truncate(query);
  if (method == "GET" && (path == "/" || path == "/app.js" || path == "/i18n.js" || path == "/style.css")) {
    QFile file(":/web/" + (path == "/" ? QString("index.html") : QString::fromLatin1(path.mid(1))));
    if (!file.open(QIODevice::ReadOnly)) { respond(s, 404, "{}"); return; }
    respond(s, 200, file.readAll(), path == "/" ? "text/html; charset=utf-8" : path.endsWith(".js") ? "application/javascript; charset=utf-8" : "text/css; charset=utf-8"); return;
  }
  if (token_.isEmpty() || headers.value("authorization") != "Bearer " + token_.toLatin1()) { respond(s, 401, "{\"error\":\"Access key required\"}"); return; }
  // No CORS; reject cross-origin browser requests even with a supplied key.
  if (headers.contains("origin") && headers["origin"] != "http://" + headers["host"]) { respond(s, 403, "{}"); return; }
  if (method == "GET" && path == "/api/state") { respond(s, 200, QJsonDocument(state()).toJson(QJsonDocument::Compact)); return; }
  if (method == "GET" && path == "/api/waterfall") {
    if (!waterfall_ || !waterfall_->isVisible()) { respond(s, 404, "{}"); return; }
    QByteArray png; QBuffer buffer(&png); buffer.open(QIODevice::WriteOnly); waterfall_->grab().save(&buffer, "PNG");
    respond(s, 200, png, "image/png"); return;
  }
  if (method == "POST" && path == "/api/command") {
    if (headers.value("content-type") != "application/json") { respond(s, 415, "{}"); return; }
    QJsonParseError error; auto doc = QJsonDocument::fromJson(data.mid(end + 4, length), &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) { respond(s, 400, "{}"); return; }
    // Mark handled before signals can enter a nested dialog event loop.
    s->setProperty("done", true);
    QPointer<QTcpSocket> guarded(s);
    auto result = command(doc.object());
    if (guarded && guarded->state() == QAbstractSocket::ConnectedState)
      respond(guarded, result.isEmpty() ? 200 : 409, QJsonDocument(QJsonObject{{"error", result}}).toJson(QJsonDocument::Compact));
    return;
  }
  respond(s, 404, "{}");
}

QJsonObject WebControlServer::state () const
{
  QJsonObject result = stationStatus ? stationStatus() : QJsonObject();
  result["title"] = window_->windowTitle();
  auto modal = QApplication::activeModalWidget();
  result["blocked"] = modal && modal != log_;
  result["dialog"] = modal ? modal->windowTitle() : QString();
  QJsonObject widgets;
  for (auto const& id : controls) if (auto w = window_->findChild<QWidget *>(id)) widgets[id] = describe(w);
  result["controls"] = widgets;
  QJsonObject actions;
  for (auto const& id : modes) if (auto a = window_->findChild<QAction *>(id))
    actions[id] = QJsonObject{{"text", a->text().remove('&')}, {"checked", a->isChecked()}, {"enabled", a->isEnabled()}};
  result["modes"] = actions;
  QJsonObject palettes;
  for (auto const& id : {"decodedTextBrowser", "decodedTextBrowser2"}) {
    QJsonArray rows;
    if (auto text = window_->findChild<QTextEdit *>(id)) {
      auto selections = text->extraSelections();
      auto palette = text->viewport()->palette();
      palettes[id] = QJsonObject{{"foreground", cssColor(palette.color(QPalette::Text))},
                                {"background", cssColor(palette.color(QPalette::Base))}};
      auto block = text->document()->lastBlock();
      for (int i = 0; block.isValid() && i < 250; ++i, block = block.previous()) {
        auto style = textStyle(block.charFormat());
        auto background = block.blockFormat().background();
        if (background.style() != Qt::NoBrush) style["background"] = cssColor(background.color());
        auto runs = formattedRuns(block, selections);
        rows.prepend(QJsonObject{{"line", block.blockNumber()}, {"text", block.text()}, {"style", style}, {"runs", runs}});
      }
    }
    result[id] = rows;
  }
  result["decodePalette"] = palettes;
  QJsonObject log;
  if (log_ && log_->isVisible()) for (auto const& id : logFields)
    if (auto w = log_->findChild<QWidget *>(id)) log[id] = describe(w);
  result["log"] = log;
  return result;
}

QString WebControlServer::command (QJsonObject const& cmd)
{
  auto id = cmd.value("id").toString(); auto value = cmd.value("value");
  auto modal = QApplication::activeModalWidget();
  if (id == "stopTxButton") {
    if (auto b = window_->findChild<QAbstractButton *>(id)) { b->click(); return {}; }
  }
  if (modal && modal != log_) return "Close the dialog on the station computer first";
  if (id == "waterfall.show") {
    if (!waterfall_) return "Waterfall unavailable";
    waterfall_->show(); return {};
  }
  if (id.startsWith("log.")) {
    if (!log_ || !log_->isVisible()) return "No QSO to log";
    auto field = id.mid(4);
    if (field == "accept" || field == "reject") {
      auto buttons = log_->findChild<QDialogButtonBox *>("buttonBox");
      auto button = buttons ? buttons->button(field == "accept" ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel) : nullptr;
      if (!button) return "Log action unavailable";
      button->click(); return {};
    }
    if (!logFields.contains(field)) return "Unknown log field";
    return setControl(log_->findChild<QWidget *>(field), value);
  }
  if (log_ && log_->isVisible()) return "Finish or cancel logging first";
  if (modes.contains(id)) {
    auto a = window_->findChild<QAction *>(id);
    if (!a || !a->isEnabled()) return "Mode unavailable";
    a->trigger(); return {};
  }
  if (id == "decodedTextBrowser" || id == "decodedTextBrowser2") {
    auto text = window_->findChild<QTextEdit *>(id);
    if (!text || !value.isDouble() || value.toDouble() != value.toInt()) return "Invalid decode";
    auto block = text->document()->findBlockByNumber(value.toInt());
    if (!block.isValid() || block.text().isEmpty() || block.text() != cmd.value("text").toString()) return "Decode changed; select again";
    if (selectDecode) selectDecode(id, value.toInt());
    return {};
  }
  if (!controls.contains(id)) return "Unknown control";
  return setControl(window_->findChild<QWidget *>(id), value);
}
