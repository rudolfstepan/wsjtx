#ifndef PSK_REPORTER_DIALOG_HPP
#define PSK_REPORTER_DIALOG_HPP

#include <QDialog>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QSettings>
#include <QShortcut>
#include <QSpinBox>
#include <QDateTime>
#include <QDesktopServices>
#include <QElapsedTimer>
#include <QHeaderView>
#include <QLabel>
#include <QMap>
#include <QSet>
#include <QHBoxLayout>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPushButton>
#include <QTableWidget>
#include <QTimer>
#include <QUrlQuery>
#include <QVBoxLayout>
#include <QXmlStreamReader>
#include <QRegularExpression>
#include <functional>
#include "PskPropagation.hpp"
#include "PskQueryThrottle.hpp"

// The reported frequency belongs to our transmission, not to the receiver.
// Reception reports cannot determine the receiver's TX frequency or sequence.
class PskReporterDialog final : public QDialog
{
public:
  struct Context { QString call; QString mode; quint64 dial; };
  using ContextProvider = std::function<Context ()>;
  using BandRange = std::function<QPair<quint64, quint64> (quint64)>;
  // Empty result means the call was started; otherwise display the reason here.
  using SelectStation = std::function<QString (QString const&, QString const&, Context const&)>;
  using ReportsUpdated = std::function<void (QMap<QString, qint64> const&, Context const&)>;
  struct StationStyle {
    QColor background, foreground;
    QString worked, criterion;
    bool workedBefore, workedOnBand;
  };
  using StyleProvider = std::function<StationStyle (QString const&, QString const&, Context const&)>;

  PskReporterDialog (QWidget *parent, ContextProvider context, SelectStation select,
                     QUrl endpoint = QUrl {"https://retrieve.pskreporter.info/query"})
    : QDialog {parent}, context_ {context}, select_ {select}, network_ {this}, endpoint_ {endpoint}
  {
    setWindowTitle (tr ("PSK Reporter — Who hears me?"));
    resize (1120, 560);
    auto layout = new QVBoxLayout {this};
    auto info = new QLabel {tr ("Receivers reporting your signal in the current mode on the current band over the last 30 minutes.\n"
                               "Reports are delayed. Call station enables TX using your current TX frequency and sequence.\n"
                               "Turquoise underlined callsigns in Band Activity are receivers from this list.")};
    info->setWordWrap (true);
    layout->addWidget (info);
    auto filters = new QHBoxLayout;
    scope_ = new QComboBox {this};
    scope_->setObjectName ("pskScope");
    scope_->addItems ({tr ("Current frequency and mode"), tr ("Current band and mode")});
    scope_->setCurrentIndex (1);
    filters->addWidget (scope_);
    search_ = new QLineEdit {this};
    search_->setObjectName ("pskSearch");
    search_->setPlaceholderText (tr ("Search callsign or grid (Ctrl+F)"));
    filter_ = new QComboBox {this};
    filter_->setObjectName ("pskFilter");
    filter_->addItems ({tr ("All stations"), tr ("New callsigns"), tr ("New on this band"), tr ("Favorites"), tr ("Called before")});
    age_ = new QComboBox {this};
    age_->setObjectName ("pskAge");
    age_->addItem (tr ("Heard within 30 min"), 1800);
    age_->addItem (tr ("Heard within 15 min"), 900);
    age_->addItem (tr ("Heard within 5 min"), 300);
    auto favorite = new QPushButton {tr ("Favorite"), this};
    favorite->setToolTip (tr ("Add or remove the selected station from your saved favorites."));
    visibleCount_ = new QLabel {this};
    filters->addWidget (search_, 1);
    filters->addWidget (filter_);
    filters->addWidget (age_);
    filters->addWidget (favorite);
    filters->addWidget (visibleCount_);
    layout->addLayout (filters);
    table_ = new QTableWidget {0, 10, this};
    table_->setHorizontalHeaderLabels ({tr ("Callsign"), tr ("Grid"), tr ("SNR (dB)"), tr ("Your signal (MHz)"), tr ("Heard (UTC)"), tr ("Mode"), tr ("QSO status"), tr ("Fav."), tr ("Calls"), tr ("Last call (UTC)")});
    table_->setSelectionBehavior (QAbstractItemView::SelectRows);
    table_->setSelectionMode (QAbstractItemView::SingleSelection);
    table_->setEditTriggers (QAbstractItemView::NoEditTriggers);
    table_->horizontalHeader ()->setSectionResizeMode (QHeaderView::Stretch);
    for (int column : {2, 5, 7, 8}) table_->horizontalHeader ()->setSectionResizeMode (column, QHeaderView::ResizeToContents);
    layout->addWidget (table_);
    status_ = new QLabel {this};
    status_->setWordWrap (true);
    layout->addWidget (status_);
    auto recall = new QHBoxLayout;
    recent_ = new QComboBox {this};
    recent_->setObjectName ("pskRecent");
    recent_->setMinimumContentsLength (24);
    recent_->setSizeAdjustPolicy (QComboBox::AdjustToMinimumContentsLengthWithIcon);
    auto callRecent = new QPushButton {tr ("Call recent"), this};
    auto repeat = new QPushButton {tr ("Repeat last (Ctrl+R)"), this};
    auto next = new QPushButton {tr ("Call next (Ctrl+N)"), this};
    halt_ = new QPushButton {tr ("Halt Tx"), this};
    halt_->setEnabled (false);
    recall->addWidget (recent_, 1);
    recall->addWidget (callRecent);
    recall->addWidget (repeat);
    recall->addWidget (next);
    recall->addWidget (halt_);
    layout->addLayout (recall);
    auto buttons = new QHBoxLayout;
    refresh_ = new QPushButton {tr ("Refresh"), this};
    auto automatic = new QCheckBox {tr ("Refresh every 5 minutes"), this};
    automatic->setChecked (true);
    auto map = new QPushButton {tr ("Open PSK Reporter map"), this};
    use_ = new QPushButton {tr ("Call station"), this};
    auto use = use_;
    use->setToolTip (tr ("Send the first standard call in the next configured TX period."));
    use->setEnabled (false);
    buttons->addWidget (refresh_);
    buttons->addWidget (automatic);
    buttons->addWidget (map);
    auto propagation = new QPushButton {tr ("Personal DX windows"), this};
    buttons->addWidget (propagation);
    connect (propagation, &QPushButton::clicked, this, [this] { showPropagation (); });
    buttons->addWidget (use);
    layout->addLayout (buttons);
    connect (refresh_, &QPushButton::clicked, this, [this] { refresh (); });
    connect (table_, &QTableWidget::itemSelectionChanged, this, [this, favorite] {
      bool selected = table_->currentRow () >= 0 && !table_->isRowHidden (table_->currentRow ());
      use_->setEnabled (selected);
      favorite->setEnabled (selected);
    });
    favorite->setEnabled (false);
    connect (use, &QPushButton::clicked, this, [this] { callSelected (); });
    connect (favorite, &QPushButton::clicked, this, [this] { toggleFavorite (); });
    connect (table_, &QTableWidget::cellDoubleClicked, this, [this] (int, int column) {
      if (column == 7) toggleFavorite (); else callSelected ();
    });
    connect (search_, &QLineEdit::textChanged, this, [this] { applyFilters (); });
    connect (scope_, QOverload<int>::of (&QComboBox::currentIndexChanged), this, [this] { applyFilters (); });
    connect (filter_, QOverload<int>::of (&QComboBox::currentIndexChanged), this, [this] { applyFilters (); });
    connect (age_, QOverload<int>::of (&QComboBox::currentIndexChanged), this, [this] { applyFilters (); });
    connect (repeat, &QPushButton::clicked, this, [this] { repeatLast (); });
    connect (callRecent, &QPushButton::clicked, this, [this] {
      int index = recent_->currentData ().toInt ();
      if (index < 0 || index >= history_.size ()) { status_->setText (tr ("Select a recent call first.")); return; }
      auto record = history_[index];
      startCall (record.call, record.grid, record.context);
    });
    connect (next, &QPushButton::clicked, this, [this] { callNext (); });
    connect (halt_, &QPushButton::clicked, this, [this] {
      if (haltTx_) haltTx_ ();
      status_->setText (tr ("Transmission stopped. Enable Tx is off."));
    });
    auto shortcut = [this] (QString const& key, std::function<void ()> action) {
      auto binding = new QShortcut {QKeySequence {key}, this};
      binding->setContext (Qt::WidgetWithChildrenShortcut);
      connect (binding, &QShortcut::activated, this, action);
    };
    shortcut ("Ctrl+F", [this] { search_->setFocus (); search_->selectAll (); });
    shortcut ("Ctrl+Return", [this] { callSelected (); });
    shortcut ("Ctrl+R", [this] { repeatLast (); });
    shortcut ("Ctrl+N", [this] { callNext (); });
    shortcut ("Ctrl+Shift+S", [this] { if (haltTx_) halt_->click (); });
    rebuildHistory ();
    connect (map, &QPushButton::clicked, this, [this] {
      QUrl url {"https://www.pskreporter.info/pskmap.html"};
      QUrlQuery query;
      query.addQueryItem ("callsign", context_ ().call);
      query.addQueryItem ("search", "Find");
      url.setQuery (query);
      QDesktopServices::openUrl (url);
    });
    auto timer = new QTimer {this};
    timer->setInterval (1000);
    connect (timer, &QTimer::timeout, this, [this, automatic] {
      auto remaining = queryWait ();
      bool ready = !busy_ && !remaining;
      refresh_->setEnabled (ready);
      refresh_->setText (ready ? tr ("Refresh") : busy_ ? tr ("Retrieving…")
        : tr ("Refresh in %1 s").arg ((remaining + 999) / 1000));
      if (isVisible () && table_->rowCount () && !sameContext (loaded_, context_ ())) {
        table_->setRowCount (0);
        auto current = context_ ();
        status_->setText (tr ("Operating context changed to %1 / %2 at %3 MHz. Waiting for the next allowed refresh.")
                          .arg (current.call, current.mode).arg (current.dial / 1.e6, 0, 'f', 6));
      }
      if (isVisible ()) { refreshStyles (); applyFilters (); }
      if (ready && automatic->isChecked () && isVisible ()) refresh ();
    });
    timer->start ();
  }

  void refresh ()
  {
    if (busy_ || queryWait ()) return;
    auto ctx = context_ ();
    if (ctx.call.isEmpty () || !ctx.dial) {
      status_->setText (tr ("Configure your callsign and select a working frequency first."));
      return;
    }
    QUrl url {endpoint_};
    QUrlQuery query;
    query.addQueryItem ("senderCallsign", ctx.call);
    query.addQueryItem ("mode", ctx.mode);
    query.addQueryItem ("flowStartSeconds", "-1800");
    auto range = bandRange_ ? bandRange_ (ctx.dial) : qMakePair (ctx.dial, ctx.dial + 5000);
    query.addQueryItem ("frange", QString::number (range.first) + "-" + QString::number (range.second));
    query.addQueryItem ("rptlimit", "1000");
    query.addQueryItem ("rronly", "1");
    query.addQueryItem ("noactive", "1");
    url.setQuery (query);
    if (!throttle_.reserve ()) {
      status_->setText (tr ("PSK Reporter query cooldown is active, including requests from another profile or instance. Next attempt in %1 s.")
                        .arg ((queryWait () + 999) / 1000));
      return;
    }
    busy_ = true;
    lastRequest_.start ();
    refresh_->setEnabled (false);
    status_->setText (tr ("Retrieving reception reports for %1 on the current band…").arg (ctx.call));
    QNetworkRequest request {url};
    request.setAttribute (QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    auto reply = network_.get (request);
    auto timeout = new QTimer {reply};
    timeout->setSingleShot (true);
    connect (timeout, &QTimer::timeout, reply, &QNetworkReply::abort);
    timeout->start (30000);
    connect (reply, &QNetworkReply::readyRead, this, [reply] {
      if (reply->bytesAvailable () > 5 * 1024 * 1024) reply->abort ();
    });
    auto setup = stationSetup_;
    auto setupSince = setupSince_;
    connect (reply, &QNetworkReply::finished, this, [this, reply, ctx, range, setup, setupSince] {
      busy_ = false;
      reply->deleteLater ();
      if (reply->error () != QNetworkReply::NoError) {
        int http = reply->attribute (QNetworkRequest::HttpStatusCodeAttribute).toInt ();
        throttle_.failed (http, reply->rawHeader ("Retry-After"));
        if (http == 429 || http == 503)
          status_->setText (tr ("PSK Reporter query service is limiting requests (HTTP %1). The browser map may still work. Automatic queries paused for %2 s; existing observations are retained.")
                            .arg (http).arg ((queryWait () + 999) / 1000));
        else
          status_->setText (tr ("PSK Reporter request failed (HTTP %1, network error %2). Next attempt in %3 s.")
                            .arg (http).arg (int (reply->error ())).arg ((queryWait () + 999) / 1000));
        return;
      }
      if (!sameContext (ctx, context_ ())) {
        status_->setText (tr ("Operating context changed during retrieval. Next request after the five-minute interval."));
        return;
      }
      struct Report { QString call, grid, snr, mode; quint64 frequency; qint64 time; };
      QMap<QString, Report> reports;
      QList<Report> observations;
      QXmlStreamReader xml {reply->readAll ()};
      bool rootSeen = false;
      while (!xml.atEnd ()) {
        xml.readNext ();
        if (!xml.isStartElement ()) continue;
        if (xml.name () == "receptionReports") rootSeen = true;
        if (xml.name () != "receptionReport") continue;
        auto a = xml.attributes ();
        Report r {a.value ("receiverCallsign").toString ().trimmed ().toUpper (),
                  a.value ("receiverLocator").toString (), a.value ("sNR").toString (),
                  a.value ("mode").toString (), a.value ("frequency").toULongLong (),
                  a.value ("flowStartSeconds").toLongLong ()};
        static QRegularExpression const callPattern {"^[A-Z0-9]+(?:/[A-Z0-9]+)*$"};
        if (!callPattern.match (r.call).hasMatch () || !r.call.contains (QRegularExpression {"[0-9]"})
            || !r.call.contains (QRegularExpression {"[A-Z]"}) || r.call.size () > 20
            || a.value ("senderCallsign").toString ().compare (ctx.call, Qt::CaseInsensitive)
            || (!r.mode.isEmpty () && r.mode != ctx.mode)
            || r.frequency < range.first || r.frequency > range.second
            || r.time < QDateTime::currentSecsSinceEpoch () - 1800
            || r.time > QDateTime::currentSecsSinceEpoch () + 300) continue;
        if (r.mode.isEmpty ()) r.mode = ctx.mode;
        observations.append (r);
        if (!reports.contains (r.call) || reports.value (r.call).time < r.time) reports.insert (r.call, r);
      }
      if (xml.hasError () || !rootSeen) {
        status_->setText (tr ("PSK Reporter returned an invalid response. Retry after five minutes."));
        return;
      }
      throttle_.succeeded ();
      loaded_ = ctx;
      if (country_ && bandName_) {
        bool changed = false;
        for (auto const& r : observations)
          if (r.time >= setupSince)
            changed = propagation_.add (ctx.call, setup, ctx.mode, country_ (r.call), bandName_ (r.frequency), r.call, r.time) || changed;
        if (!settingsFile_.isEmpty () && (changed || !statisticsError_.isEmpty ())) {
          QSettings saved {settingsFile_, settingsFormat_};
          saved.beginGroup (settingsGroup_);
          saved.setValue ("CustomPskReporter/PropagationV1", propagation_.save ());
          saved.sync ();
          if (saved.status () != QSettings::NoError)
            statisticsError_ = tr ("Statistics could not be saved to the configuration file.");
          else statisticsError_.clear ();
        }
      }
      table_->setSortingEnabled (false);
      table_->setRowCount (0);
      for (auto const& r : reports) {
        int row = table_->rowCount ();
        table_->insertRow (row);
        QStringList values {r.call, r.grid, r.snr, QString::number (r.frequency / 1.e6, 'f', 6),
                            QDateTime::fromSecsSinceEpoch (r.time, Qt::UTC).toString ("HH:mm:ss"), r.mode,
                            QString {}, QString {}, QString {}, QString {}};
        for (int column = 0; column < values.size (); ++column) {
          auto item = new ReportItem {values[column]};
          if (column == 2 && !r.snr.isEmpty ()) item->setData (Qt::UserRole, r.snr.toInt ());
          if (column == 3) item->setData (Qt::UserRole, QVariant::fromValue (r.frequency));
          if (column == 4) item->setData (Qt::UserRole, r.time);
          table_->setItem (row, column, item);
        }
      }
      table_->setSortingEnabled (true);
      table_->sortItems (4, Qt::DescendingOrder);
      refreshStyles ();
      refreshCallDetails ();
      if (reportsUpdated_) {
        QMap<QString, qint64> heard;
        for (auto const& report : reports)
          if (report.mode == ctx.mode && report.frequency >= ctx.dial && report.frequency <= ctx.dial + 5000)
            heard.insert (report.call, report.time);
        reportsUpdated_ (heard, ctx);
      }
      applyFilters ();
      status_->setText (tr ("%1 receivers for %2 on the band at %4 MHz (current mode: %3). Updated %5 UTC. Next request in five minutes.")
                        .arg (reports.size ()).arg (ctx.call, ctx.mode).arg (ctx.dial / 1.e6, 0, 'f', 6)
                        .arg (QDateTime::currentDateTimeUtc ().toString ("HH:mm:ss")));
    });
  }

  void setReportsUpdatedHandler (ReportsUpdated handler) { reportsUpdated_ = handler; }
  void setPropagationProviders (std::function<QString (QString const&)> country,
                                std::function<QString (quint64)> band)
  { country_ = country; bandName_ = band; }
  void setBandRangeProvider (BandRange provider) { bandRange_ = provider; }
  void setStyleProvider (StyleProvider provider) { style_ = provider; refreshStyles (); }
  void setHaltTxHandler (std::function<void ()> handler) { haltTx_ = handler; halt_->setEnabled (bool (haltTx_)); }
  void setPreferences (QSettings const& settings)
  {
    throttle_.configure (settings);
    if (queryWait ())
      status_->setText (tr ("Saved PSK Reporter query cooldown restored. Next attempt in %1 s.")
                        .arg ((queryWait () + 999) / 1000));
    settingsFile_ = settings.fileName ();
    settingsFormat_ = settings.format ();
    settingsGroup_ = settings.group ();
    QSettings saved {settingsFile_, settingsFormat_};
    saved.beginGroup (settingsGroup_);
    propagation_.load (saved.value ("CustomPskReporter/PropagationV1").toMap ());
    stationSetup_ = saved.value ("CustomPskReporter/StationSetup", "Default").toString ();
    setupSince_ = saved.value ("CustomPskReporter/StationSetupSince", 0).toLongLong ();
    auto favorites = saved.value ("CustomPskReporter/Favorites").toStringList ();
    favorites_.clear ();
    for (auto const& call : favorites) favorites_.insert (call.toUpper ());
    history_.clear ();
    for (auto const& value : saved.value ("CustomPskReporter/RecentCalls").toList ()) {
      auto item = value.toMap ();
      CallRecord record {item.value ("call").toString (), item.value ("grid").toString (),
                         {item.value ("myCall").toString (), item.value ("mode").toString (), item.value ("dial").toULongLong ()},
                         item.value ("time").toLongLong (), item.value ("count").toInt ()};
      if (!record.call.isEmpty () && record.context.dial && record.count > 0 && history_.size () < 20)
        history_.append (record);
    }
    rebuildHistory ();
    refreshCallDetails ();
  }
  void refreshStyles ()
  {
    if (!style_ || !table_->rowCount ()) return;
    auto sorting = table_->isSortingEnabled ();
    table_->setSortingEnabled (false);
    for (int row = 0; row < table_->rowCount (); ++row) {
      auto reportContext = loaded_;
      reportContext.mode = table_->item (row, 5)->text ();
      auto style = style_ (table_->item (row, 0)->text (), table_->item (row, 1)->text (), reportContext);
      auto status = table_->item (row, 6);
      table_->item (row, 0)->setData (Qt::UserRole + 1, style.workedBefore);
      table_->item (row, 0)->setData (Qt::UserRole + 2, style.workedOnBand);
      if (status->text () != style.worked) status->setText (style.worked);
      QBrush background = style.background.isValid () ? QBrush {style.background} : QBrush {};
      QBrush foreground = style.foreground.isValid () ? QBrush {style.foreground} : QBrush {};
      auto tooltip = tr ("WSJT-X highlighting: %1\nLogbook status: %2").arg (style.criterion, style.worked);
      for (int column = 0; column < table_->columnCount (); ++column) {
        auto item = table_->item (row, column);
        if (item->background () != background) item->setBackground (background);
        if (item->foreground () != foreground) item->setForeground (foreground);
        if (column <= 6 && item->toolTip () != tooltip) item->setToolTip (tooltip);
      }
    }
    table_->setSortingEnabled (sorting);
    applyFilters ();
  }

private:
  qint64 queryWait () const
  {
    return qMax (throttle_.remaining (), lastRequest_.isValid ()
                 ? qMax (qint64 {0}, 300000 - lastRequest_.elapsed ()) : qint64 {0});
  }
  void showPropagation ()
  {
    QDialog dialog {this};
    dialog.setWindowTitle (tr ("Personal DX windows — empirical reception history"));
    dialog.resize (1180, 640);
    auto layout = new QVBoxLayout {&dialog};
    auto info = new QLabel {tr ("Local observations of your transmitted signal, collected while Who hears me is open.\n"
      "Most observed: ranked by UTC reception days, then receiver-days. Next 24 hours: ordered by the next start.\n"
      "Upcoming times repeat historical hourly windows; they do not indicate current propagation or QSO probabilities.\n"
      "Missing reports do not prove a closed path. Activity, power and reporting coverage affect the results.\n"
      "Use a new station/antenna setup name after changing your antenna, location or power.")};
    info->setWordWrap (true); layout->addWidget (info);
    auto filters = new QHBoxLayout;
    filters->addWidget (new QLabel {tr ("Station / antenna setup:")});
    auto setup = new QLineEdit {stationSetup_};
    setup->setMaxLength (80); filters->addWidget (setup);
    auto month = new QComboBox;
    month->setObjectName ("propagationMonth");
    month->addItem (tr ("All months"), 0);
    for (int m = 1; m <= 12; ++m) month->addItem (QString::number (m), m);
    month->setCurrentIndex (QDateTime::currentDateTimeUtc ().date ().month ());
    filters->addWidget (month);
    auto search = new QLineEdit;
    search->setObjectName ("propagationSearch");
    search->setPlaceholderText (tr ("Filter country or band")); filters->addWidget (search);
    layout->addLayout (filters);
    auto options = new QHBoxLayout;
    auto view = new QComboBox;
    view->setObjectName ("propagationView");
    view->addItems ({tr ("Most observed windows"), tr ("Next 24 hours (UTC)")});
    options->addWidget (view);
    auto history = new QComboBox;
    history->setObjectName ("propagationHistory");
    history->addItem (tr ("All collected history"), 0);
    history->addItem (tr ("Last 90 UTC days"), 90);
    history->addItem (tr ("Last 365 UTC days"), 365);
    options->addWidget (history);
    options->addWidget (new QLabel {tr ("Minimum reception days:")});
    auto minimum = new QSpinBox;
    minimum->setObjectName ("propagationMinimumDays");
    minimum->setRange (1, 10000); minimum->setValue (1);
    options->addWidget (minimum);
    options->addStretch ();
    layout->addLayout (options);
    auto context = context_ ();
    layout->addWidget (new QLabel {tr ("Own call: %1 · Mode: %2 · Months combine calendar years. Small samples remain tentative.")
                                   .arg (context.call, context.mode)});
    auto table = new QTableWidget {0, 10};
    table->setHorizontalHeaderLabels ({tr ("DXCC country"), tr ("Band"), tr ("Month"), tr ("UTC window"),
                                       tr ("Reception days"), tr ("Receivers"), tr ("Receiver-days"),
                                       tr ("Next start (UTC)"), tr ("First observed"), tr ("Last observed")});
    table->setEditTriggers (QAbstractItemView::NoEditTriggers);
    table->horizontalHeader ()->setSectionResizeMode (QHeaderView::Stretch);
    layout->addWidget (table);
    auto status = new QLabel; status->setWordWrap (true); layout->addWidget (status);
    auto update = [this, table, month, search, status, context, view, history, minimum] {
      auto now = QDateTime::currentDateTimeUtc ();
      int days = history->currentData ().toInt ();
      auto since = days ? now.date ().addDays (1 - days) : QDate {};
      auto windows = propagation_.windows (context.call, stationSetup_, context.mode, month->currentData ().toInt (), since);
      if (view->currentIndex () == 1) {
        // Stable sorting preserves the reception-day ranking for simultaneous windows.
        std::stable_sort (windows.begin (), windows.end (), [&now] (PskPropagation::Window const& a, PskPropagation::Window const& b) {
          auto first = PskPropagation::nextWindow (a, now), second = PskPropagation::nextWindow (b, now);
          if (!first.isValid ()) return false;
          return !second.isValid () || first < second;
        });
      }
      table->setRowCount (0);
      for (auto const& w : windows) {
        auto next = PskPropagation::nextWindow (w, now);
        if (w.days.size () < minimum->value () || (view->currentIndex () == 1 && !next.isValid ())) continue;
        auto filter = search->text ().trimmed ();
        if (!filter.isEmpty () && !w.country.contains (filter, Qt::CaseInsensitive)
            && !w.band.contains (filter, Qt::CaseInsensitive)) continue;
        int row = table->rowCount (); table->insertRow (row);
        auto dates = w.days.values (); dates.sort ();
        QStringList values {w.country, w.band, QString::number (w.month),
          QString ("%1:00–%2:00").arg (w.hour, 2, 10, QChar {'0'}).arg ((w.hour + 1) % 24, 2, 10, QChar {'0'}),
          QString::number (w.days.size ()), QString::number (w.receivers.size ()), QString::number (w.receiverDays),
          !next.isValid () ? tr ("Outside next 24 h") : next == now ? tr ("Now") : next.toString ("MM-dd HH:mm"),
          dates.first (), dates.last ()};
        for (int c = 0; c < values.size (); ++c) table->setItem (row, c, new QTableWidgetItem {values[c]});
        table->item (row, 4)->setToolTip (w.days.size () == 1
          ? tr ("Only one observed day. Collect observations on more days before relying on this window.")
          : tr ("Observed UTC dates:\n%1").arg (dates.join ("\n")));
        table->item (row, 7)->setToolTip (tr ("Recurrence of a historical hourly window in its calendar month, not a live propagation forecast."));
      }
      status->setText (!statisticsError_.isEmpty () ? statisticsError_ : table->rowCount ()
        ? tr ("%1 observed windows · %2 UTC. Each receiver counts once per UTC date/hour. First/last dates refer to the selected history.")
          .arg (table->rowCount ()).arg (now.toString ("yyyy-MM-dd HH:mm"))
        : tr ("No observations match these filters. Try All months, All collected history or fewer minimum reception days. Collect reports during normal operation."));
    };
    connect (setup, &QLineEdit::editingFinished, &dialog, [this, setup, update] {
      auto name = setup->text ().trimmed (); name.remove (QChar {0x1f});
      if (name.isEmpty ()) name = "Default";
      if (stationSetup_ != name) setupSince_ = QDateTime::currentSecsSinceEpoch ();
      stationSetup_ = name; setup->setText (name);
      if (!settingsFile_.isEmpty ()) {
        QSettings saved {settingsFile_, settingsFormat_}; saved.beginGroup (settingsGroup_);
        saved.setValue ("CustomPskReporter/StationSetup", stationSetup_);
        saved.setValue ("CustomPskReporter/StationSetupSince", setupSince_);
      }
      update ();
    });
    connect (month, QOverload<int>::of (&QComboBox::currentIndexChanged), &dialog, [update] { update (); });
    connect (view, QOverload<int>::of (&QComboBox::currentIndexChanged), &dialog, [update, view, month] {
      if (view->currentIndex () == 1) month->setCurrentIndex (0);
      update ();
    });
    connect (history, QOverload<int>::of (&QComboBox::currentIndexChanged), &dialog, [update] { update (); });
    connect (minimum, QOverload<int>::of (&QSpinBox::valueChanged), &dialog, [update] { update (); });
    connect (search, &QLineEdit::textChanged, &dialog, [update] { update (); });
    auto timer = new QTimer {&dialog};
    connect (timer, &QTimer::timeout, &dialog, update); timer->start (5000);
    update (); dialog.exec ();
  }
  struct CallRecord { QString call, grid; Context context; qint64 time; int count; };
  static bool sameContext (Context const& a, Context const& b)
  { return a.call == b.call && a.mode == b.mode && a.dial == b.dial; }

  int recordFor (QString const& call, Context const& context) const
  {
    for (int i = 0; i < history_.size (); ++i)
      if (history_[i].call == call && sameContext (history_[i].context, context)) return i;
    return -1;
  }

  void startCall (QString const& call, QString const& grid, Context const& context)
  {
    if (!sameContext (context, context_ ())) {
      status_->setText (tr ("To recall %1, select %2 at %3 MHz with the same operator callsign first.")
                        .arg (call, context.mode).arg (context.dial / 1.e6, 0, 'f', 6));
      return;
    }
    auto error = select_ (call, grid, context);
    if (!error.isEmpty ()) { status_->setText (error); return; }
    int index = recordFor (call, context);
    int count = index >= 0 ? history_[index].count + 1 : 1;
    if (index >= 0) history_.removeAt (index);
    history_.prepend ({call, grid, context, QDateTime::currentSecsSinceEpoch (), count});
    while (history_.size () > 20) history_.removeLast ();
    savePreferences ();
    rebuildHistory ();
    refreshCallDetails ();
    status_->setText (tr ("Calling %1: Enable Tx is on. The first standard message will be sent in the next configured TX period. Call attempt %2.").arg (call).arg (count));
  }

  void callSelected ()
  {
    int row = table_->currentRow ();
    if (row < 0 || table_->isRowHidden (row)) { status_->setText (tr ("Select a visible station first.")); return; }
    if (!canCallRow (row)) return;
    startCall (table_->item (row, 0)->text (), table_->item (row, 1)->text (), loaded_);
  }

  bool canCallRow (int row)
  {
    auto frequency = table_->item (row, 3)->data (Qt::UserRole).toULongLong ();
    if (table_->item (row, 5)->text () != loaded_.mode
        || frequency < loaded_.dial || frequency > loaded_.dial + 5000) {
      status_->setText (tr ("This report belongs to another mode or working frequency. Select that mode/frequency and refresh before calling."));
      return false;
    }
    return true;
  }

  void repeatLast ()
  {
    if (history_.isEmpty ()) { status_->setText (tr ("No previous call yet.")); return; }
    auto record = history_.first ();
    startCall (record.call, record.grid, record.context);
  }

  void callNext ()
  {
    int rows = table_->rowCount ();
    int start = table_->currentRow ();
    int fallback = -1;
    for (int offset = 1; offset <= rows; ++offset) {
      int row = (start + offset + rows) % rows;
      if (table_->isRowHidden (row)) continue;
      auto frequency = table_->item (row, 3)->data (Qt::UserRole).toULongLong ();
      if (table_->item (row, 5)->text () != loaded_.mode || frequency < loaded_.dial || frequency > loaded_.dial + 5000) continue;
      if (fallback < 0) fallback = row;
      if (recordFor (table_->item (row, 0)->text (), loaded_) < 0) { fallback = row; break; }
    }
    if (fallback < 0) { status_->setText (tr ("No station matches the current filters.")); return; }
    table_->selectRow (fallback);
    callSelected ();
  }

  void toggleFavorite ()
  {
    int row = table_->currentRow ();
    if (row < 0 || table_->isRowHidden (row)) return;
    auto call = table_->item (row, 0)->text ();
    if (favorites_.contains (call)) favorites_.remove (call); else favorites_.insert (call);
    savePreferences ();
    refreshCallDetails ();
  }

  void savePreferences ()
  {
    if (settingsFile_.isEmpty ()) return;
    QSettings saved {settingsFile_, settingsFormat_};
    saved.beginGroup (settingsGroup_);
    QStringList favorites = favorites_.values ();
    favorites.sort ();
    saved.setValue ("CustomPskReporter/Favorites", favorites);
    QVariantList recent;
    for (auto const& record : history_)
      recent.append (QVariantMap {{"call", record.call}, {"grid", record.grid}, {"myCall", record.context.call},
                                 {"mode", record.context.mode}, {"dial", QVariant::fromValue (record.context.dial)},
                                 {"time", record.time}, {"count", record.count}});
    saved.setValue ("CustomPskReporter/RecentCalls", recent);
    saved.sync ();
  }

  void rebuildHistory ()
  {
    recent_->clear ();
    recent_->addItem (tr ("Recent calls…"), -1);
    for (int i = 0; i < history_.size (); ++i) {
      auto const& record = history_[i];
      recent_->addItem (tr ("%1 · %2 · %3 MHz (%4 calls)").arg (record.call, record.context.mode)
                        .arg (record.context.dial / 1.e6, 0, 'f', 6).arg (record.count), i);
    }
    if (!history_.isEmpty ()) recent_->setCurrentIndex (1);
  }

  void refreshCallDetails ()
  {
    bool sorting = table_->isSortingEnabled ();
    table_->setSortingEnabled (false);
    for (int row = 0; row < table_->rowCount (); ++row) {
      auto call = table_->item (row, 0)->text ();
      table_->item (row, 7)->setText (favorites_.contains (call) ? QStringLiteral ("*") : QString {});
      table_->item (row, 7)->setToolTip (tr ("Double-click to toggle a saved favorite."));
      int index = recordFor (call, loaded_);
      int count = index >= 0 ? history_[index].count : 0;
      auto time = index >= 0 ? history_[index].time : 0;
      table_->item (row, 8)->setText (QString::number (count));
      table_->item (row, 8)->setData (Qt::UserRole, count);
      table_->item (row, 8)->setToolTip (tr ("Call attempts started here. Successful QSOs are shown separately from your logbook."));
      table_->item (row, 9)->setText (time ? QDateTime::fromSecsSinceEpoch (time, Qt::UTC).toString ("HH:mm:ss") : QString {});
      table_->item (row, 9)->setData (Qt::UserRole, time);
    }
    table_->setSortingEnabled (sorting);
    applyFilters ();
  }

  void applyFilters ()
  {
    auto search = search_->text ().trimmed ().toUpper ();
    int visible = 0;
    auto oldest = QDateTime::currentSecsSinceEpoch () - age_->currentData ().toInt ();
    for (int row = 0; row < table_->rowCount (); ++row) {
      auto call = table_->item (row, 0)->text ();
      auto grid = table_->item (row, 1)->text ().toUpper ();
      bool worked = table_->item (row, 0)->data (Qt::UserRole + 1).toBool ();
      bool onBand = table_->item (row, 0)->data (Qt::UserRole + 2).toBool ();
      int filter = filter_->currentIndex ();
      auto frequency = table_->item (row, 3)->data (Qt::UserRole).toULongLong ();
      bool show = (search.isEmpty () || call.contains (search) || grid.contains (search))
                  && (scope_->currentIndex () == 1 || (table_->item (row, 5)->text () == loaded_.mode
                      && frequency >= loaded_.dial && frequency <= loaded_.dial + 5000))
                  && table_->item (row, 4)->data (Qt::UserRole).toLongLong () >= oldest
                  && (filter != 1 || !worked) && (filter != 2 || !onBand)
                  && (filter != 3 || favorites_.contains (call))
                  && (filter != 4 || recordFor (call, loaded_) >= 0);
      table_->setRowHidden (row, !show);
      if (show) ++visible;
    }
    if (table_->currentRow () >= 0 && table_->isRowHidden (table_->currentRow ())) {
      table_->clearSelection ();
      table_->setCurrentCell (-1, -1);
    }
    visibleCount_->setText (tr ("%1 / %2").arg (visible).arg (table_->rowCount ()));
    use_->setEnabled (table_->currentRow () >= 0 && !table_->isRowHidden (table_->currentRow ()));
  }

  class ReportItem : public QTableWidgetItem
  {
  public:
    explicit ReportItem (QString const& text) : QTableWidgetItem {text} {}
    bool operator< (QTableWidgetItem const& other) const override
    {
      if (data (Qt::UserRole).isValid () && other.data (Qt::UserRole).isValid ())
        return data (Qt::UserRole).toLongLong () < other.data (Qt::UserRole).toLongLong ();
      return QTableWidgetItem::operator< (other);
    }
  };
  ContextProvider context_;
  PskPropagation propagation_;
  QString stationSetup_ = "Default", statisticsError_;
  qint64 setupSince_ = 0;
  std::function<QString (QString const&)> country_;
  std::function<QString (quint64)> bandName_;
  BandRange bandRange_;
  SelectStation select_;
  ReportsUpdated reportsUpdated_;
  StyleProvider style_;
  std::function<void ()> haltTx_;
  QSet<QString> favorites_;
  QList<CallRecord> history_;
  QString settingsFile_, settingsGroup_;
  QSettings::Format settingsFormat_ = QSettings::IniFormat;
  QNetworkAccessManager network_;
  QUrl endpoint_;
  QElapsedTimer lastRequest_;
  PskQueryThrottle throttle_;
  Context loaded_;
  bool busy_ = false;
  QTableWidget *table_;
  QLabel *status_;
  QPushButton *refresh_;
  QPushButton *use_, *halt_;
  QLineEdit *search_;
  QComboBox *filter_, *age_, *recent_, *scope_;
  QLabel *visibleCount_;
};
#endif
