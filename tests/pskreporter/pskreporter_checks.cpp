#include "../../widgets/PskReporterDialog.hpp"
#include "../../widgets/PskReporterHighlight.hpp"
#include <QApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QEventLoop>
#include <QDir>
#include <QFontDatabase>
#include <QSslSocket>
#include <QFile>
#include <QTemporaryDir>
#include <iostream>

static void require (bool condition, char const *message)
{
  if (!condition) { std::cerr << message << '\n'; std::exit (1); }
}

int main (int argc, char **argv)
{
  if (argc > 1 && QString::fromLocal8Bit (argv[1]) == "--package-check") {
    QCoreApplication app {argc, argv};
    require (QSslSocket::supportsSsl (), "Packaged Qt cannot load OpenSSL");
    std::cout << "Packaged Qt runtime and OpenSSL verified.\n";
    return 0;
  }
  QApplication app {argc, argv};
  QFontDatabase::addApplicationFont ("C:/Windows/Fonts/segoeui.ttf");
  app.setFont (QFont {"Segoe UI", 10});
  QTcpServer server;
  require (server.listen (QHostAddress::LocalHost), "Cannot start fixture server");
  QUrl endpoint {QString ("http://127.0.0.1:%1/query").arg (server.serverPort ())};
  QByteArray body;
  QByteArray request;
  int requests = 0;
  QObject::connect (&server, &QTcpServer::newConnection, [&] {
    auto socket = server.nextPendingConnection ();
    QObject::connect (socket, &QTcpSocket::readyRead, socket, [&, socket] {
      request += socket->readAll ();
      if (!request.contains ("\r\n\r\n")) return;
      ++requests;
      socket->write ("HTTP/1.1 200 OK\r\nContent-Type: application/xml\r\nContent-Length: "
                     + QByteArray::number (body.size ()) + "\r\nConnection: close\r\n\r\n" + body);
      socket->disconnectFromHost ();
    });
    QObject::connect (socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
  });
  PskReporterDialog::Context operating {"OE1TEST", "FT8", 14074000};
  auto ctx = [&] { return operating; };
  QString selectedCall, selectedGrid;
  QString callError;
  int calls = 0;
  auto select = [&] (QString const& call, QString const& grid, PskReporterDialog::Context const& c) {
    require (c.call == "OE1TEST" && c.dial == 14074000, "Wrong selection context");
    if (!callError.isEmpty ()) return callError;
    ++calls;
    selectedCall = call; selectedGrid = grid; return QString {};
  };
  auto report = [] (QString call, QString grid, qint64 time, quint64 frequency = 14075500, QString mode = "FT8") {
    return QString ("<receptionReport senderCallsign='OE1TEST' receiverCallsign='%1' receiverLocator='%2' sNR='-12' mode='%3' frequency='%4' flowStartSeconds='%5'/>")
      .arg (call, grid, mode).arg (frequency).arg (time);
  };
  auto now = QDateTime::currentSecsSinceEpoch ();
  body = ("<receptionReports>" + report ("DL1ABC", "JO62", now - 200)
          + report ("DL1ABC", "JO63", now - 20) + report ("G1XYZ", "IO91", now - 40)
          + report ("F1OLD", "JN18", now - 3600) + report ("K1BAD", "FN42", now - 10, 7074500)
          + report ("I1BAD", "JN45", now - 10, 14075500, "FT4") + "</receptionReports>").toUtf8 ();
  PskReporterDialog dialog {nullptr, ctx, select, endpoint};
  QTemporaryDir preferences;
  require (preferences.isValid (), "Cannot create settings fixture");
  QSettings settings {preferences.filePath ("custom.ini"), QSettings::IniFormat};
  settings.beginGroup ("TestProfile");
  settings.setValue ("ExistingRadioSetting", "unchanged");
  settings.sync ();
  dialog.setPreferences (settings);
  require (!settings.contains ("CustomPskReporter/Favorites"), "Opening the list wrote settings");
  int halts = 0;
  dialog.setHaltTxHandler ([&] { ++halts; });
  bool workedOnBand = false;
  dialog.setStyleProvider ([&] (QString const& call, QString const&, PskReporterDialog::Context const&) {
    if (call == "DL1ABC")
      return PskReporterDialog::StationStyle {QColor {"#c0c0c0"}, Qt::black, workedOnBand ? "Worked on band" : "Worked", "CQ", true, workedOnBand};
    return PskReporterDialog::StationStyle {QColor {"#ff80ff"}, Qt::black, "New callsign", "New DXCC", false, false};
  });
  QTextEdit activity;
  QString original {"120000 -12 0.1 1500 ~ CQ DL1ABC JO63\n"
                    "120000 -12 0.1 1800 ~ CQ G1XYZ IO91\n"
                    "120000 -12 0.1 1900 ~ CQ DL1ABC/P JO63\n"
                    "120000 -12 0.1 2000 ~ CQ F1OLD JN18"};
  activity.setPlainText (original);
  auto current = ctx ();
  PskReporterHighlight highlights {&activity, [&] { return current; }};
  dialog.setReportsUpdatedHandler ([&] (QMap<QString, qint64> const& stations, PskReporterDialog::Context const& context) {
    highlights.setReports (stations, context);
  });
  auto wait = [&] {
    QEventLoop loop;
    QTimer::singleShot (500, &loop, &QEventLoop::quit);
    loop.exec ();
  };
  dialog.show ();
  dialog.refresh ();
  wait ();
  auto table = dialog.findChild<QTableWidget *> ();
  require (table && table->rowCount () == 2, "Deduplication / age / frequency / mode filtering failed");
  require (activity.extraSelections ().size () == 2, "Matching Band Activity callsigns were not marked");
  require (activity.toPlainText () == original, "PSK markers changed decoded text");
  for (auto const& marker : activity.extraSelections ()) {
    require (marker.cursor.selectedText () == "DL1ABC" || marker.cursor.selectedText () == "G1XYZ", "A partial or unrelated callsign was marked");
    require (marker.format.toolTip ().contains ("received your signal"), "PSK marker has no reception tooltip");
  }
  activity.append ("120015 -10 0.1 1500 ~ CQ DL1ABC JO63");
  wait ();
  require (activity.extraSelections ().size () == 3, "New decoded lines were not marked");
  current.mode = "FT4";
  activity.append ("120030 -10 0.1 1500 + CQ DL1ABC JO63");
  wait ();
  require (activity.extraSelections ().isEmpty (), "Markers survived a mode change");
  current = ctx ();
  current.dial = 7074000;
  highlights.setReports ({{"DL1ABC", now}}, ctx ());
  require (activity.extraSelections ().isEmpty (), "Markers survived a frequency change");
  current = ctx ();
  highlights.setReports ({{"DL1ABC", now - 1801}}, ctx ());
  require (activity.extraSelections ().isEmpty (), "Expired reception evidence was marked");
  require (request.contains ("senderCallsign=OE1TEST") && request.contains ("mode=FT8")
           && request.contains ("frange=14074000-14079000"), "Wrong query parameters");
  require (table->item (0, 0)->text () == "DL1ABC" && table->item (0, 1)->text () == "JO63", "Latest report was not selected");
  require (table->item (0, 6)->text () == "Worked" && table->item (0, 0)->background ().color () == QColor {"#c0c0c0"}, "Worked status / station colors were not applied");
  require (table->item (1, 6)->text () == "New callsign" && table->item (1, 0)->background ().color () == QColor {"#ff80ff"}, "New station colors were not applied");
  workedOnBand = true;
  dialog.refreshStyles ();
  require (table->item (0, 6)->text () == "Worked on band", "Station styling did not update after a logbook change");
  table->selectRow (0);
  for (auto button : dialog.findChildren<QPushButton *> ())
    if (button->text () == "Call station") button->click ();
  require (selectedCall == "DL1ABC" && selectedGrid == "JO63", "Station call action failed");
  bool callingStatus = false;
  for (auto label : dialog.findChildren<QLabel *> ()) callingStatus |= label->text ().contains ("Calling DL1ABC: Enable Tx is on");
  require (callingStatus, "Station call did not display its TX status");
  auto button = [&] (QString const& text) {
    for (auto candidate : dialog.findChildren<QPushButton *> ())
      if (candidate->text () == text) return candidate;
    require (false, "Missing calling button");
    return static_cast<QPushButton *> (nullptr);
  };
  auto filter = dialog.findChild<QComboBox *> ("pskFilter");
  auto search = dialog.findChild<QLineEdit *> ("pskSearch");
  auto age = dialog.findChild<QComboBox *> ("pskAge");
  filter->setCurrentIndex (1);
  require (table->isRowHidden (0) && !table->isRowHidden (1), "New callsign filter disagrees with the logbook");
  filter->setCurrentIndex (2);
  require (table->isRowHidden (0) && !table->isRowHidden (1), "New band filter disagrees with the logbook");
  workedOnBand = false;
  dialog.refreshStyles ();
  require (!table->isRowHidden (0), "New band filter did not update with logbook status");
  filter->setCurrentIndex (0);
  search->setText ("jo63");
  require (!table->isRowHidden (0) && table->isRowHidden (1), "Grid search failed");
  search->setText ("g1xyz");
  require (table->isRowHidden (0) && !table->isRowHidden (1), "Callsign search failed");
  search->clear ();
  table->item (1, 4)->setData (Qt::UserRole, now - 600);
  age->setCurrentIndex (2);
  require (!table->isRowHidden (0) && table->isRowHidden (1), "Recent reception filter failed");
  age->setCurrentIndex (0);
  table->selectRow (0);
  button ("Favorite")->click ();
  filter->setCurrentIndex (3);
  require (!table->isRowHidden (0) && table->isRowHidden (1) && table->item (0, 7)->text () == "*", "Favorite filter failed");
  filter->setCurrentIndex (4);
  require (!table->isRowHidden (0) && table->isRowHidden (1), "Called before filter failed");
  button ("Repeat last (Ctrl+R)")->click ();
  require (calls == 2 && table->item (0, 8)->text () == "2", "Recall did not increment the attempt count");
  callError = "Already transmitting";
  button ("Repeat last (Ctrl+R)")->click ();
  require (calls == 2 && table->item (0, 8)->text () == "2", "Rejected calls changed call history");
  callError.clear ();
  operating.mode = "FT4";
  button ("Repeat last (Ctrl+R)")->click ();
  require (calls == 2, "Recall started a call in the wrong mode");
  operating.mode = "FT8";
  operating.dial = 7074000;
  button ("Repeat last (Ctrl+R)")->click ();
  require (calls == 2, "Recall started a call on the wrong band");
  operating.dial = 14074000;
  filter->setCurrentIndex (0);
  table->selectRow (1);
  button ("Call next (Ctrl+N)")->click ();
  require (calls == 3 && selectedCall == "G1XYZ", "Call next did not prefer an uncalled visible station");
  filter->setCurrentIndex (3);
  button ("Call next (Ctrl+N)")->click ();
  require (calls == 4 && selectedCall == "DL1ABC", "Call next ignored the favorite filter");
  button ("Halt Tx")->click ();
  require (halts == 1, "Halt Tx did not invoke the normal stop action");
  settings.sync ();
  require (settings.group () == "TestProfile" && settings.value ("ExistingRadioSetting") == "unchanged", "Calling changed unrelated profile settings");
  PskReporterDialog restored {nullptr, ctx, select, endpoint};
  restored.setPreferences (settings);
  auto recent = restored.findChild<QComboBox *> ("pskRecent");
  require (recent->count () == 3 && recent->itemText (1).contains ("DL1ABC") && recent->itemText (1).contains ("3 calls"), "Saved call history was not restored");
  for (auto candidate : restored.findChildren<QPushButton *> ())
    if (candidate->text () == "Repeat last (Ctrl+R)") candidate->click ();
  require (calls == 5 && selectedCall == "DL1ABC", "Saved station could not be recalled without a reception list");
  // Load a new settings handle to test disk persistence rather than shared QSettings memory.
  QSettings persisted {preferences.filePath ("custom.ini"), QSettings::IniFormat};
  persisted.beginGroup ("TestProfile");
  require (persisted.value ("CustomPskReporter/Favorites").toStringList () == QStringList {"DL1ABC"}, "Favorites were not saved to the current profile");
  filter->setCurrentIndex (0);
  workedOnBand = true;
  dialog.refreshStyles ();
  dialog.refresh ();
  wait ();
  require (requests == 1, "Five-minute request throttle failed");
  QDir {}.mkpath (".local");
  require (dialog.grab ().save (".local/psk-reporter-preview.png"), "Preview capture failed");
  dialog.hide ();
  request.clear ();
  body = "<receptionReports><receptionReport";
  PskReporterDialog malformed {nullptr, ctx, select, endpoint};
  malformed.refresh ();
  wait ();
  require (malformed.findChild<QTableWidget *> ()->rowCount () == 0, "Malformed XML produced stations");
  bool error = false;
  for (auto label : malformed.findChildren<QLabel *> ()) error |= label->text ().contains ("invalid response");
  require (error, "Malformed XML did not display an error");
  QFile live {".local/psk-reporter-live.xml"};
  if (live.open (QIODevice::ReadOnly)) {
    body = live.readAll ();
    QXmlStreamReader xml {body};
    while (!xml.atEnd ()) {
      xml.readNext ();
      if (!xml.isStartElement () || xml.name () != "receptionReport") continue;
      auto a = xml.attributes ();
      if (a.value ("flowStartSeconds").toLongLong () < QDateTime::currentSecsSinceEpoch () - 1800) continue;
      PskReporterDialog::Context liveContext {a.value ("senderCallsign").toString (), a.value ("mode").toString (),
                                             a.value ("frequency").toULongLong () / 1000 * 1000};
      request.clear ();
      PskReporterDialog liveDialog {nullptr, [liveContext] { return liveContext; },
                                   [] (QString const&, QString const&, PskReporterDialog::Context const&) { return QString {}; }, endpoint};
      liveDialog.refresh ();
      wait ();
      require (liveDialog.findChild<QTableWidget *> ()->rowCount () > 0, "Real API reception reports were not parsed");
      std::cout << "Saved live API response parsed successfully.\n";
      break;
    }
  }
  std::cout << "PSK Reporter checks passed: query, filters, newest report, call action, throttle, malformed XML, Band Activity markers, expiry, station colors, QSO status, favorites, history persistence, recall guards, next station and Halt Tx.\n";
}
