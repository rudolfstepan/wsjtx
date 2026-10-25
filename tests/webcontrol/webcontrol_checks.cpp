#include "../../Network/WebControlServer.hpp"
#include <QApplication>
#include <QTcpSocket>
#include <QHostAddress>
#include <QEventLoop>
#include <QTimer>
#include <QPushButton>
#include <QLineEdit>
#include <QIntValidator>
#include <QSpinBox>
#include <QComboBox>
#include <QTextEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTextCursor>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <iostream>
#include <stdexcept>

void check(bool condition, char const * message) { if (!condition) throw std::runtime_error(message); }

QByteArray request(WebControlServer& server, QByteArray method, QByteArray path, QByteArray body = {},
                   QByteArray key = {}, QByteArray extra = {}, bool split = false)
{
  QTcpSocket socket; QEventLoop loop; QTimer timeout; timeout.setSingleShot(true);
  QByteArray received;
  QObject::connect(&socket, &QTcpSocket::readyRead, [&] { received += socket.readAll(); });
  QObject::connect(&socket, &QTcpSocket::disconnected, &loop, &QEventLoop::quit);
  QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
  QObject::connect(&socket, &QTcpSocket::connected, [&] {
    QByteArray bytes = method + " " + path + " HTTP/1.1\r\nHost: localhost:" + QByteArray::number(server.serverPort()) + "\r\n";
    if (!key.isEmpty()) bytes += "Authorization: Bearer " + key + "\r\n";
    bytes += extra + "Content-Type: application/json\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body;
    if (split) {
      socket.write(bytes.left(bytes.size()-2));
      auto tail = bytes.right(2);
      QTimer::singleShot(20, &socket, [&socket, tail] { socket.write(tail); });
    } else socket.write(bytes);
  });
  timeout.start(2500); socket.connectToHost(QHostAddress::LocalHost, server.serverPort()); loop.exec();
  check(!received.isEmpty(), "HTTP response timeout"); return received;
}
QJsonObject json(QByteArray const& response) { return QJsonDocument::fromJson(response.mid(response.indexOf("\r\n\r\n")+4)).object(); }
bool status(QByteArray const& response, int code) { return response.startsWith("HTTP/1.1 " + QByteArray::number(code) + " "); }

int main(int argc, char ** argv)
{
  QApplication app(argc, argv);
  try {
    QWidget window, waterfall; QVBoxLayout layout(&window);
    QPushButton monitor("Monitor"), halt("Halt TX"), logButton("Log QSO");
    monitor.setObjectName("monitorButton"); monitor.setCheckable(true); halt.setObjectName("stopTxButton"); logButton.setObjectName("logQSOButton");
    layout.addWidget(&monitor); layout.addWidget(&halt); layout.addWidget(&logButton);
    QSpinBox frequency; frequency.setObjectName("RxFreqSpinBox"); frequency.setRange(200,5000); layout.addWidget(&frequency);
    QLineEdit call; call.setObjectName("dxCallEntry"); QIntValidator validator(1,999); call.setValidator(&validator); layout.addWidget(&call);
    QComboBox band; band.setObjectName("bandComboBox"); band.addItems({"14.074", "18.100"}); layout.addWidget(&band);
    QComboBox respond; respond.setObjectName("respondComboBox"); respond.addItems({"CQ: None", "CQ: First", "CQ: Max Dist"}); layout.addWidget(&respond);
    QLineEdit message("CQ TEST AA00"); message.setObjectName("tx1"); layout.addWidget(&message);
    QTextEdit decodes; decodes.setObjectName("decodedTextBrowser"); decodes.setPlainText("123000 -10 0.2 1234 ~ CQ TEST AA00"); layout.addWidget(&decodes);
    auto palette = decodes.palette(); palette.setColor(QPalette::Text, QColor("#e0e1e2")); palette.setColor(QPalette::Base, QColor("#182532")); decodes.setPalette(palette);
    auto cursor = decodes.textCursor();
    QTextBlockFormat blockFormat; blockFormat.setBackground(QColor("#357ac5")); cursor.setBlockFormat(blockFormat);
    auto callPosition = decodes.toPlainText().indexOf("TEST");
    cursor.setPosition(callPosition); cursor.setPosition(callPosition + 4, QTextCursor::KeepAnchor);
    QTextCharFormat highlight; highlight.setForeground(QColor("#551144")); highlight.setBackground(QColor("#12cafe")); highlight.setFontWeight(QFont::Bold); highlight.setFontUnderline(true); cursor.mergeCharFormat(highlight);
    cursor.movePosition(QTextCursor::End); cursor.insertBlock(QTextBlockFormat(), QTextCharFormat());
    cursor.insertText("<img src=x onerror=alert(1)> CQ TEST AA00");
    QTextEdit::ExtraSelection heard; heard.cursor = QTextCursor(decodes.document());
    auto gridPosition = decodes.toPlainText().indexOf("AA00");
    heard.cursor.setPosition(gridPosition); heard.cursor.setPosition(gridPosition + 4, QTextCursor::KeepAnchor);
    heard.format.setForeground(Qt::black); heard.format.setBackground(QColor("#7fe5df")); heard.format.setFontWeight(QFont::Bold); heard.format.setFontUnderline(true); heard.format.setToolTip("PSK Reporter test marker");
    decodes.setExtraSelections({heard});
    QDialog log; QVBoxLayout logLayout(&log); QLineEdit logCall; logCall.setObjectName("call"); logLayout.addWidget(&logCall);
    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel); buttons.setObjectName("buttonBox"); logLayout.addWidget(&buttons);
    QObject::connect(&buttons, &QDialogButtonBox::accepted, &log, &QDialog::accept);
    QObject::connect(&buttons, &QDialogButtonBox::rejected, &log, &QDialog::reject);
    QObject::connect(&logButton, &QPushButton::clicked, &log, &QDialog::show);
    int clicks=0, halts=0, selections=0, edits=0, decodesSelected=0;
    QObject::connect(&monitor, &QPushButton::clicked, [&] { ++clicks; });
    QObject::connect(&halt, &QPushButton::clicked, [&] { ++halts; });
    QObject::connect(&band, QOverload<int>::of(&QComboBox::activated), [&](int) { ++selections; });
    QObject::connect(&call, &QLineEdit::editingFinished, [&] { ++edits; });
    window.show(); waterfall.show(); app.processEvents();
    WebControlServer server(&window,&waterfall,&log);
    server.selectDecode = [&](QString const&,int) { ++decodesSelected; };
    check(server.start(0, app.arguments().contains("--serve") ? QString("0427") : QString()), "Listen failed"); auto key=server.token().toLatin1();
    if (app.arguments().contains("--serve")) {
      server.stationStatus = [] { return QJsonObject{{"callsign", "TEST"}, {"grid", "AA00"}, {"frequency", "14.074 000"}, {"mode", "FT8"}, {"utc", "12:30:00"}, {"monitoring", true}}; };
      std::cout << QJsonDocument(QJsonObject{{"port", server.serverPort()}, {"key", server.token()}}).toJson(QJsonDocument::Compact).constData() << std::endl;
      QTimer::singleShot(180000, &app, &QApplication::quit);
      return app.exec();
    }
    auto command = [&](QByteArray body) { return request(server,"POST","/api/command",body,key); };
    check(status(request(server,"GET","/"),200), "Embedded page missing");
    check(status(request(server,"GET","/?"),200), "Empty query rejected on login page");
    check(request(server,"GET","/?view=station").contains("id=\"login\""), "Query string hid login page");
    check(request(server,"GET","/app.js?v=1").contains("function render"), "Asset query string rejected");
    check(request(server,"GET","/app.js").contains("function render"), "Embedded JavaScript missing");
    check(request(server,"GET","/i18n.js").contains("Browser language"), "Embedded translations missing");
    check(status(request(server,"GET","/api/state"),401), "State accessible without key");
    check(status(request(server,"GET","/api/state?"),401), "API query bypassed authentication");
    check(status(request(server,"GET","/api/state?key=" + key),401), "URL key bypassed header authentication");
    check(status(request(server,"GET","/api/state?refresh=1",{},key),200), "Authenticated API query rejected");
    check(status(request(server,"GET","/api/state",{},"wrong"),401), "Wrong key accepted");
    check(status(request(server,"POST","/api/command","{\"id\":\"monitorButton\",\"value\":true}"),401)&&clicks==0,"Unauthenticated mutation");
    auto state=json(request(server,"GET","/api/state",{},key));
    check(state["controls"].toObject().contains("monitorButton"), "Control state missing");
    auto rows = state["decodedTextBrowser"].toArray();
    check(rows.size()==2,"Decode missing");
    check(rows[0].toObject()["style"].toObject()["background"]=="#357ac5", "Desktop row background lost");
    auto runs = rows[0].toObject()["runs"].toArray(); bool highlighted = false, heardMarked = false; QString complete;
    for (auto item : runs) {
      auto run = item.toObject(); auto style = run["style"].toObject(); complete += run["text"].toString();
      if (run["text"]=="TEST") highlighted = style["foreground"]=="#551144" && style["background"]=="#12cafe" && style["bold"].toBool() && style["underline"].toBool();
      if (run["text"]=="AA00") heardMarked = style["foreground"]=="#000000" && style["background"]=="#7fe5df" && style["bold"].toBool() && style["underline"].toBool() && style["tooltip"]=="PSK Reporter test marker";
    }
    check(highlighted && complete==rows[0].toObject()["text"].toString(), "Desktop callsign formatting lost");
    check(heardMarked, "PSK Reporter overlay lost");
    decodes.setExtraSelections({});
    check(json(request(server,"GET","/api/state",{},key))["decodedTextBrowser"].toArray()[0].toObject()["runs"] != rows[0].toObject()["runs"], "Overlay removal did not update decode state");
    auto crossing = heard; crossing.cursor.setPosition(callPosition + 2); crossing.cursor.setPosition(gridPosition + 2, QTextCursor::KeepAnchor);
    crossing.format.setBackground(QColor(127, 229, 223, 128)); decodes.setExtraSelections({crossing});
    auto crossingRuns = json(request(server,"GET","/api/state",{},key))["decodedTextBrowser"].toArray()[0].toObject()["runs"].toArray();
    QString crossedText;
    for (auto item : crossingRuns) {
      auto run = item.toObject();
      if (run["style"].toObject()["background"]=="#7fe5df80") crossedText += run["text"].toString();
    }
    check(crossedText=="ST AA", "Overlay boundaries or alpha channel lost across text fragments");
    decodes.setExtraSelections({heard});
    check(!rows[1].toObject()["style"].toObject().contains("background"), "Row background leaked into unhighlighted decode");
    auto colors = state["decodePalette"].toObject()["decodedTextBrowser"].toObject();
    check(colors["foreground"]=="#e0e1e2" && colors["background"]=="#182532", "Desktop palette lost");
    check(status(command("{\"id\":\"monitorButton\",\"value\":true}"),200)&&monitor.isChecked()&&clicks==1,"Monitor did not use desktop click");
    command("{\"id\":\"monitorButton\",\"value\":true}");check(clicks==1,"Desired state toggled on retry");
    check(status(command("{\"id\":\"monitorButton\",\"value\":1}"),409),"Invalid boolean accepted");
    monitor.setEnabled(false);check(status(command("{\"id\":\"monitorButton\",\"value\":false}"),409),"Disabled control accepted");monitor.setEnabled(true);
    check(status(command("{\"id\":\"RxFreqSpinBox\",\"value\":1500}"),200)&&frequency.value()==1500,"Frequency update failed");
    check(status(command("{\"id\":\"RxFreqSpinBox\",\"value\":99999}"),409)&&frequency.value()==1500,"Out of range frequency accepted");
    check(status(command("{\"id\":\"RxFreqSpinBox\",\"value\":1500.5}"),409),"Fractional frequency accepted");
    check(status(command("{\"id\":\"dxCallEntry\",\"value\":\"123\"}"),200)&&edits==1,"Editing signal missing");
    check(status(command("{\"id\":\"dxCallEntry\",\"value\":\"abc\"}"),409)&&call.text()=="123","Validator bypassed");
    check(status(command("{\"id\":\"bandComboBox\",\"value\":1}"),200)&&selections==1,"Band activation missing");
    check(status(command("{\"id\":\"notAllowed\"}"),409),"Unknown control accepted");
    check(status(command("{\"id\":\"decodedTextBrowser\",\"value\":0,\"text\":\"old\"}"),409)&&decodesSelected==0,"Stale decode accepted");
    QJsonObject select{{"id","decodedTextBrowser"},{"value",0},{"text",decodes.document()->firstBlock().text()}};
    check(status(command(QJsonDocument(select).toJson()),200)&&decodesSelected==1,"Decode selection failed");
    check(status(request(server,"POST","/api/command","{}",key,"Origin: http://evil.example\r\n"),403),"Cross origin command accepted");
    check(status(request(server,"POST","/api/command","{",key),400),"Malformed JSON accepted");
    check(status(request(server,"POST","/api/command","{}",key,"Transfer-Encoding: chunked\r\n"),400),"Ambiguous framing accepted");
    check(status(request(server,"POST","/api/command","{}",key,"Content-Length: 2\r\n"),400),"Duplicate length accepted");
    check(status(request(server,"POST","/api/command","{\"id\":\"RxFreqSpinBox\",\"value\":1600}",key,{},true),200)&&frequency.value()==1600,"Fragmented request failed");
    check(status(request(server,"GET","/api/waterfall",{},key),200),"Waterfall missing");
    waterfall.hide();check(status(request(server,"GET","/api/waterfall",{},key),404),"Hidden waterfall captured");
    command("{\"id\":\"logQSOButton\"}");check(log.isVisible(),"Log dialog not opened");
    check(status(command("{\"id\":\"log.call\",\"value\":\"TEST\"}"),200)&&logCall.text()=="TEST","Log edit failed");
    check(status(command("{\"id\":\"monitorButton\",\"value\":false}"),409),"Station editable during logging");
    check(status(command("{\"id\":\"log.accept\"}"),200)&&!log.isVisible(),"Log accept failed");
    QDialog modal(&window);modal.setModal(true);modal.show();app.processEvents();
    check(json(request(server,"GET","/api/state",{},key))["blocked"].toBool(),"Modal state missing");
    check(status(command("{\"id\":\"monitorButton\",\"value\":false}"),409),"Modal bypassed");
    check(status(command("{\"id\":\"stopTxButton\"}"),200)&&halts==1,"Halt TX unavailable during modal");modal.hide();
    check(server.start(0)&&server.token().toLatin1()!=key,"Key not rotated");
    check(status(request(server,"GET","/api/state",{},key),401),"Old key accepted");
    auto randomKey = server.token().toLatin1();
    check(server.start(0,"  My own station key!  ") && server.token()=="My own station key!", "Manual key not applied");
    check(status(request(server,"GET","/api/state",{},"My own station key!"),200), "Manual key rejected");
    check(status(request(server,"GET","/api/state",{},randomKey),401), "Old random key accepted after manual change");
    check(!server.start(0,"key\r\nInjected: true") && server.isListening() && server.token()=="My own station key!", "Invalid manual key changed the running server");
    check(!server.start(0,QString(129, 'x')) && !server.start(0,QString(QChar(0x20ac))), "Unsupported header key accepted");
    check(server.start(0," ") && server.token().size()==72, "Blank key did not generate a random key");
    bool settingsChecked = false;
    QTimer::singleShot(0, [&] {
      auto dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
      if (!dialog) return;
      auto keyEdit = dialog->findChild<QLineEdit *>("webControlKey");
      auto apply = dialog->findChild<QPushButton *>("webControlStart");
      auto random = dialog->findChild<QPushButton *>("webControlRandom");
      if (keyEdit && apply && random && !keyEdit->isReadOnly()) {
        keyEdit->setText("0427"); apply->click();
        bool applied = server.isListening() && server.token()=="0427" && status(request(server,"GET","/api/state",{},"0427"),200);
        random->click();
        settingsChecked = applied && server.token()!="0427" && keyEdit->text()==server.token();
      }
      dialog->accept();
    });
    server.showSettings();
    check(settingsChecked, "Settings dialog did not apply a manual key and generate a replacement");
    server.stop();check(!server.isListening(),"Server not stopped");
    std::cout << "Web control: HTTP, authentication, controls, validation, decodes, logging, waterfall and lifecycle passed\n";
  } catch(std::exception const& e) { std::cerr << e.what() << '\n';return 1; }
}
