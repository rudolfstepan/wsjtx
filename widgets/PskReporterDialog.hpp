#ifndef PSK_REPORTER_DIALOG_HPP
#define PSK_REPORTER_DIALOG_HPP

#include <QDialog>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QSettings>
#include <QShortcut>
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
      bool ready = !busy_ && (!lastRequest_.isValid () || lastRequest_.elapsed () >= 300000);
      refresh_->setEnabled (ready);
      refresh_->setText (ready ? tr ("Refresh") : busy_ ? tr ("Retrieving…")
        : tr ("Refresh in %1 s").arg ((300000 - lastRequest_.elapsed () + 999) / 1000));
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
    if (busy_ || (lastRequest_.isValid () && lastRequest_.elapsed () < 300000)) return;
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
    connect (reply, &QNetworkReply::finished, this, [this, reply, ctx, range] {
      busy_ = false;
      reply->deleteLater ();
      if (reply->error () != QNetworkReply::NoError) {
        status_->setText (tr ("PSK Reporter request failed: %1. Retry after five minutes.").arg (reply->errorString ()));
        return;
      }
      if (!sameContext (ctx, context_ ())) {
        status_->setText (tr ("Operating context changed during retrieval. Next request after the five-minute interval."));
        return;
      }
      struct Report { QString call, grid, snr, mode; quint64 frequency; qint64 time; };
      QMap<QString, Report> reports;
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
        if (!reports.contains (r.call) || reports.value (r.call).time < r.time) reports.insert (r.call, r);
      }
      if (xml.hasError () || !rootSeen) {
        status_->setText (tr ("PSK Reporter returned an invalid response. Retry after five minutes."));
        return;
      }
      loaded_ = ctx;
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
  void setBandRangeProvider (BandRange provider) { bandRange_ = provider; }
  void setStyleProvider (StyleProvider provider) { style_ = provider; refreshStyles (); }
  void setHaltTxHandler (std::function<void ()> handler) { haltTx_ = handler; halt_->setEnabled (bool (haltTx_)); }
  void setPreferences (QSettings const& settings)
  {
    settingsFile_ = settings.fileName ();
    settingsFormat_ = settings.format ();
    settingsGroup_ = settings.group ();
    QSettings saved {settingsFile_, settingsFormat_};
    saved.beginGroup (settingsGroup_);
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
