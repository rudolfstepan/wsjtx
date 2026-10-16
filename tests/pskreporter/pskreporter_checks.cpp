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
  if (argc == 7 && QString::fromLocal8Bit (argv[1]) == "--live-query") {
    PskReporterDialog::Context liveContext {QString::fromLocal8Bit (argv[2]), QString::fromLocal8Bit (argv[3]),
                                           QString::fromLocal8Bit (argv[4]).toULongLong ()};
    PskReporterDialog liveDialog {nullptr, [liveContext] { return liveContext; },
      [] (QString const&, QString const&, PskReporterDialog::Context const&) { return QString {"Diagnostic only"}; }};
    liveDialog.setBandRangeProvider ([&] (quint64) {
      return qMakePair (QString::fromLocal8Bit (argv[5]).toULongLong (), QString::fromLocal8Bit (argv[6]).toULongLong ());
    });
    QEventLoop loop;
    QTimer poll;
    QObject::connect (&poll, &QTimer::timeout, [&] {
      for (auto label : liveDialog.findChildren<QLabel *> ()) {
        auto status = label->text ();
        if (status.contains ("Updated") || status.contains ("request failed") || status.contains ("invalid response")) {
          std::cout << status.toStdString () << '\n';
          require (status.contains ("Updated"), "Live Qt HTTPS request failed");
          loop.quit ();
        }
      }
    });
    poll.start (100);
    QTimer::singleShot (35000, [&] { require (false, "Live query did not finish"); });
    liveDialog.refresh ();
    loop.exec ();
    return 0;
  }
  QFontDatabase::addApplicationFont ("C:/Windows/Fonts/segoeui.ttf");
  app.setFont (QFont {"Segoe UI", 10});
  auto retryClock = QDateTime {QDate {2026, 10, 16}, QTime {12, 0}, Qt::UTC};
  require (PskQueryThrottle::retryAfter ("1800", retryClock) == 1800000
           && PskQueryThrottle::retryAfter ("Fri, 16 Oct 2026 12:30:00 GMT", retryClock) == 1800000
           && PskQueryThrottle::retryAfter ("invalid", retryClock) == 0,
           "Retry-After seconds/date/invalid handling failed");
  QTemporaryDir throttleDir;
  QSettings throttleSettings {throttleDir.filePath ("throttle.ini"), QSettings::IniFormat};
  throttleSettings.beginGroup ("FirstProfile");
  PskQueryThrottle firstThrottle;
  firstThrottle.configure (throttleSettings);
  require (firstThrottle.reserve (), "Initial query reservation failed");
  throttleSettings.endGroup (); throttleSettings.beginGroup ("OtherProfile");
  PskQueryThrottle secondThrottle;
  secondThrottle.configure (throttleSettings);
  require (!secondThrottle.reserve () && secondThrottle.remaining () > 295000,
           "Restart/profile change bypassed the shared five-minute cooldown");
  secondThrottle.failed (429, {});
  require (secondThrottle.remaining () > 595000, "HTTP 429 did not back off for ten minutes");
  secondThrottle.failed (429, {});
  require (secondThrottle.remaining () > 1195000, "Repeated rate limit did not increase the backoff");
  secondThrottle.failed (503, "7200");
  PskQueryThrottle reopenedThrottle;
  reopenedThrottle.configure (throttleSettings);
  require (reopenedThrottle.remaining () > 7195000, "Server Retry-After did not survive restart");
  PskPropagation statistics;
  auto sample = QDateTime {QDate {2025, 10, 1}, QTime {23, 59}, Qt::UTC}.toSecsSinceEpoch ();
  statistics.add ("OE1TEST", "A", "FT8", "Japan", "20m", "JA1AAA", sample);
  statistics.add ("oe1test", "A", "FT8", "Japan", "20m", "JA1AAA", sample - 120);
  statistics.add ("OE1TEST", "A", "FT8", "Japan", "20m", "JA2BBB", sample);
  statistics.add ("OE1TEST", "A", "FT8", "Japan", "20m", "JA1AAA", sample + 86400);
  statistics.add ("OE1TEST", "A", "FT8", "Japan", "20m", "JA1AAA", sample + 120);
  statistics.add ("OE1TEST", "B", "FT8", "Japan", "20m", "JA1AAA", sample);
  statistics.add ("OTHER", "A", "FT8", "Japan", "20m", "JA1AAA", sample);
  statistics.add ("OE1TEST", "A", "FT4", "Japan", "20m", "JA1AAA", sample);
  PskPropagation reloaded;
  reloaded.load (statistics.save ());
  auto windows = reloaded.windows ("OE1TEST", "A", "FT8", 10);
  require (windows.size () == 2 && windows[0].hour == 23 && windows[0].days.size () == 2
           && windows[0].receivers.size () == 2 && windows[0].receiverDays == 3,
           "Propagation persistence, distinct days/receivers, UTC boundary or setup/call/mode isolation failed");
  require (reloaded.windows ("OE1TEST", "A", "FT8", 11).isEmpty (), "Propagation month filter failed");
  require (!statistics.add ("OE1TEST", "A", "FT8", "Japan", "20m", "JA1AAA", sample),
           "Repeated observations should not trigger another storage update");
  auto recentWindows = reloaded.windows ("OE1TEST", "A", "FT8", 10, QDate {2025, 10, 2});
  require (recentWindows.size () == 2 && recentWindows[0].days.size () == 1,
           "History date cutoff failed");
  auto hourWindow = windows[0];
  auto clock = QDateTime {QDate {2026, 10, 16}, QTime {23, 30}, Qt::UTC};
  require (PskPropagation::nextWindow (hourWindow, clock) == clock,
           "A window currently in progress was not recognized");
  hourWindow.hour = 1;
  require (PskPropagation::nextWindow (hourWindow, clock) == QDateTime (QDate {2026, 10, 17}, QTime {1, 0}, Qt::UTC),
           "Next UTC day scheduling failed");
  clock = QDateTime {QDate {2026, 12, 31}, QTime {23, 30}, Qt::UTC};
  hourWindow.month = 1;
  require (PskPropagation::nextWindow (hourWindow, clock) == QDateTime (QDate {2027, 1, 1}, QTime {1, 0}, Qt::UTC),
           "Year/month boundary scheduling failed");
  hourWindow.month = 12;
  require (!PskPropagation::nextWindow (hourWindow, clock).isValid (),
           "Historical month was wrongly scheduled in a different month");
  QVariantMap corrupt;
  corrupt.insert (QStringList {"OE1TEST", "A", "FT8", "Japan", "20m", "2025-10-01", "invalid"}.join (QChar {0x1f}),
                  QStringList {"JA1AAA"});
  PskPropagation invalid;
  invalid.load (corrupt);
  require (invalid.save ().isEmpty (), "Malformed saved hour was accepted as midnight");
  QTcpServer server;
  require (server.listen (QHostAddress::LocalHost), "Cannot start fixture server");
  QUrl endpoint {QString ("http://127.0.0.1:%1/query").arg (server.serverPort ())};
  QByteArray body;
  QByteArray request;
  int requests = 0;
  QByteArray responseStatus = "200 OK", retryHeader;
  QObject::connect (&server, &QTcpServer::newConnection, [&] {
    auto socket = server.nextPendingConnection ();
    QObject::connect (socket, &QTcpSocket::readyRead, socket, [&, socket] {
      request += socket->readAll ();
      if (!request.contains ("\r\n\r\n")) return;
      ++requests;
      socket->write ("HTTP/1.1 " + responseStatus + "\r\n" + retryHeader + "Content-Type: application/xml\r\nContent-Length: "
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
  dialog.findChild<QComboBox *> ("pskScope")->setCurrentIndex (0);
  QTemporaryDir preferences;
  require (preferences.isValid (), "Cannot create settings fixture");
  QSettings settings {preferences.filePath ("custom.ini"), QSettings::IniFormat};
  settings.beginGroup ("TestProfile");
  settings.setValue ("ExistingRadioSetting", "unchanged");
  settings.sync ();
  dialog.setPreferences (settings);
  dialog.setPropagationProviders ([] (QString const& call) { return call.startsWith ("DL") ? "Germany" : "England"; },
                                   [] (quint64) { return QString {"20m"}; });
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
  QSettings statsSaved {settings.fileName (), settings.format ()};
  statsSaved.beginGroup ("TestProfile");
  PskPropagation collected;
  collected.load (statsSaved.value ("CustomPskReporter/PropagationV1").toMap ());
  require (collected.windows ("OE1TEST", "Default", "FT8", 0).size () >= 2,
           "Validated API observations were not persisted");
  QTimer::singleShot (100, [&] {
    auto popup = qobject_cast<QDialog *> (QApplication::activeModalWidget ());
    require (popup != nullptr, "Personal DX windows did not open");
    auto summary = popup->findChild<QTableWidget *> ();
    require (summary && summary->rowCount () >= 2, "Personal DX windows did not display collected observations");
    require (summary->columnCount () == 10 && !summary->item (0, 8)->text ().isEmpty (),
             "Propagation observation date range is missing");
    int unfiltered = summary->rowCount ();
    auto minimum = popup->findChild<QSpinBox *> ("propagationMinimumDays");
    minimum->setValue (2);
    require (summary->rowCount () == 0, "Minimum reception-day filter failed");
    minimum->setValue (1);
    auto search = popup->findChild<QLineEdit *> ("propagationSearch");
    search->setText ("Germany");
    require (summary->rowCount () > 0 && summary->rowCount () < unfiltered, "Propagation country search failed");
    search->clear ();
    QDir {}.mkpath (".local");
    require (popup->grab ().save (".local/psk-propagation-preview.png"), "Cannot save propagation preview");
    popup->findChild<QComboBox *> ("propagationView")->setCurrentIndex (1);
    require (popup->findChild<QComboBox *> ("propagationMonth")->currentIndex () == 0,
             "Upcoming view must include next-month windows at a month boundary");
    int expectedUpcoming = 0;
    for (auto const& w : collected.windows ("OE1TEST", "Default", "FT8", 0))
      if (PskPropagation::nextWindow (w, QDateTime::currentDateTimeUtc ()).isValid ()) ++expectedUpcoming;
    require (summary->rowCount () == expectedUpcoming, "Upcoming view month/hour filtering failed");
    require (popup->grab ().save (".local/psk-propagation-upcoming-preview.png"), "Cannot save upcoming preview");
    popup->accept ();
  });
  for (auto button : dialog.findChildren<QPushButton *> ())
    if (button->text () == "Personal DX windows") button->click ();
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
  request.clear ();
  body = ("<receptionReports>" + report ("DL3BAND", "JO62", now - 10, 14074952)
          + report ("DL4MODE", "JO63", now - 10, 14085000, "FT2")
          + report ("DL5FREQ", "JO64", now - 10, 14085000) + "</receptionReports>").toUtf8 ();
  PskReporterDialog bandDialog {nullptr, ctx, select, endpoint};
  bandDialog.setBandRangeProvider ([] (quint64) { return qMakePair (quint64 {14000000}, quint64 {14350000}); });
  bandDialog.refresh ();
  wait ();
  auto bandTable = bandDialog.findChild<QTableWidget *> ();
  require (request.contains ("mode=FT8") && request.contains ("frange=14000000-14350000")
           && bandTable->rowCount () == 2, "Band query lost reports or included the wrong mode");
  for (int row = 0; row < bandTable->rowCount (); ++row) require (!bandTable->isRowHidden (row), "Band report hidden by default");
  int previousCalls = calls;
  bandTable->selectRow (0);
  for (auto candidate : bandDialog.findChildren<QPushButton *> ())
    if (candidate->text () == "Call station") candidate->click ();
  require (calls == previousCalls, "Called a station from another working frequency");
  bandDialog.findChild<QComboBox *> ("pskScope")->setCurrentIndex (0);
  int visibleBandRows = 0;
  for (int row = 0; row < bandTable->rowCount (); ++row) if (!bandTable->isRowHidden (row)) ++visibleBandRows;
  require (visibleBandRows == 1, "Narrow scope did not filter band reports");
  request.clear ();
  PskReporterDialog changedMode {nullptr, ctx, select, endpoint};
  changedMode.refresh ();
  operating.mode = "FT2";
  wait ();
  require (request.contains ("mode=FT8") && changedMode.findChild<QTableWidget *> ()->rowCount () == 0,
           "A response from the previous live mode was displayed");
  operating.mode = "FT8";
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
  operating.mode = "FT2";
  request.clear ();
  body = ("<receptionReports>" + report ("DL2FT2", "JO63", now - 10, 14075500, "FT2") + "</receptionReports>").toUtf8 ();
  PskReporterDialog ft2Dialog {nullptr, ctx, select, endpoint};
  ft2Dialog.refresh ();
  wait ();
  auto ft2Table = ft2Dialog.findChild<QTableWidget *> ();
  require (request.contains ("mode=FT2") && ft2Table->rowCount () == 1, "FT2 reception reports/query failed");
  ft2Table->selectRow (0);
  for (auto candidate : ft2Dialog.findChildren<QPushButton *> ())
    if (candidate->text ().startsWith ("Call station")) candidate->click ();
  require (selectedCall == "DL2FT2", "FT2 PSK station could not be selected for calling");
  responseStatus = "429 Too Many Requests";
  retryHeader = "Retry-After: 1800\r\n";
  request.clear ();
  QSettings limitedSettings {preferences.filePath ("limited.ini"), QSettings::IniFormat};
  PskReporterDialog limited {nullptr, ctx, select, endpoint};
  limited.setPreferences (limitedSettings);
  int beforeLimited = requests;
  limited.refresh (); wait ();
  bool rateMessage = false;
  for (auto label : limited.findChildren<QLabel *> ()) rateMessage |= label->text ().contains ("HTTP 429");
  require (requests == beforeLimited + 1 && rateMessage, "HTTP 429 did not show a specific query-service message");
  limited.refresh (); wait ();
  PskReporterDialog restarted {nullptr, ctx, select, endpoint};
  restarted.setPreferences (limitedSettings);
  restarted.refresh (); wait ();
  require (requests == beforeLimited + 1, "HTTP 429 cooldown was bypassed by refresh/restart");
  std::cout << "PSK Reporter checks passed: query, filters, newest report, call action, shared persistent throttle, Retry-After, HTTP 429 backoff, malformed XML, Band Activity markers, expiry, station colors, QSO status, favorites, history persistence, recall guards, next station, FT2 reports/calling and Halt Tx.\n";
}
