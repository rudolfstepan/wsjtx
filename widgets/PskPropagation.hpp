#ifndef PSK_PROPAGATION_HPP
#define PSK_PROPAGATION_HPP

#include <QDateTime>
#include <QMap>
#include <QSet>
#include <QVariant>
#include <algorithm>

// One receiver per UTC date/hour, independent of polling frequency. Only
// positive observations are available: these counts are not QSO probabilities.
class PskPropagation
{
public:
  struct Window {
    QString country, band;
    int month = 0, hour = 0;
    QSet<QString> days, receivers;
    int receiverDays = 0;
  };
  bool add (QString const& own, QString const& setup, QString const& mode,
            QString const& country, QString const& band, QString const& receiver, qint64 seconds)
  {
    auto time = QDateTime::fromSecsSinceEpoch (seconds, Qt::UTC);
    QStringList fields {own.toUpper (), setup, mode, country, band,
                        time.date ().toString (Qt::ISODate), QString::number (time.time ().hour ())};
    if (!time.isValid () || receiver.trimmed ().isEmpty ()) return false;
    for (auto const& field : fields)
      if (field.isEmpty () || field.contains (QChar {0x1f})) return false;
    auto &receivers = observations_[fields.join (QChar {0x1f})];
    int size = receivers.size ();
    receivers.insert (receiver.toUpper ());
    return receivers.size () != size;
  }
  QList<Window> windows (QString const& own, QString const& setup, QString const& mode, int month,
                        QDate const& since = QDate {}) const
  {
    QMap<QString, Window> grouped;
    for (auto it = observations_.cbegin (); it != observations_.cend (); ++it) {
      auto f = it.key ().split (QChar {0x1f});
      if (f.size () != 7 || f[0] != own.toUpper () || f[1] != setup || f[2] != mode) continue;
      auto date = QDate::fromString (f[5], Qt::ISODate);
      if (!date.isValid () || (month && date.month () != month) || (since.isValid () && date < since)) continue;
      auto key = QStringList {f[3], f[4], QString::number (date.month ()), f[6]}.join (QChar {0x1f});
      auto &w = grouped[key];
      w.country = f[3]; w.band = f[4]; w.month = date.month (); w.hour = f[6].toInt ();
      w.days.insert (f[5]); w.receivers.unite (it.value ()); w.receiverDays += it.value ().size ();
    }
    auto result = grouped.values ();
    std::sort (result.begin (), result.end (), [] (Window const& a, Window const& b) {
      if (a.days.size () != b.days.size ()) return a.days.size () > b.days.size ();
      if (a.receiverDays != b.receiverDays) return a.receiverDays > b.receiverDays;
      if (a.country != b.country) return a.country < b.country;
      if (a.band != b.band) return a.band < b.band;
      if (a.month != b.month) return a.month < b.month;
      return a.hour < b.hour;
    });
    return result;
  }
  // Only schedule a recurrence within the next 24 hours in the matching
  // calendar month. A window already in progress starts at 'now'.
  static QDateTime nextWindow (Window const& window, QDateTime const& current)
  {
    auto now = current.toUTC ();
    if (!now.isValid () || window.hour < 0 || window.hour > 23 || window.month < 1 || window.month > 12)
      return {};
    auto start = QDateTime {now.date (), QTime {window.hour, 0}, Qt::UTC};
    if (window.hour == now.time ().hour ()) start = now;
    else if (start < now) start = start.addDays (1);
    return start.date ().month () == window.month ? start : QDateTime {};
  }
  QVariantMap save () const
  {
    QVariantMap result;
    for (auto it = observations_.cbegin (); it != observations_.cend (); ++it)
      result.insert (it.key (), QStringList {it.value ().values ()});
    return result;
  }
  void load (QVariantMap const& values)
  {
    observations_.clear ();
    for (auto it = values.cbegin (); it != values.cend (); ++it) {
      auto fields = it.key ().split (QChar {0x1f});
      if (fields.size () != 7) continue;
      bool validHour = false;
      int hour = fields[6].toInt (&validHour);
      auto date = QDate::fromString (fields[5], Qt::ISODate);
      if (!date.isValid () || !validHour || hour < 0 || hour > 23) continue;
      for (auto const& receiver : it.value ().toStringList ())
        add (fields[0], fields[1], fields[2], fields[3], fields[4], receiver,
             QDateTime {date, QTime {hour, 0}, Qt::UTC}.toSecsSinceEpoch ());
    }
  }
private:
  QMap<QString, QSet<QString>> observations_;
};
#endif
