#ifndef PSK_QUERY_THROTTLE_HPP
#define PSK_QUERY_THROTTLE_HPP

#include <QDateTime>
#include <QLockFile>
#include <QSettings>
#include <functional>
#include <limits>

// Shared by profiles/instances using the same configuration file. Reserve
// before sending, so restarting the program cannot bypass the query interval.
class PskQueryThrottle
{
public:
  void configure (QSettings const& settings)
  {
    file_ = settings.fileName (); format_ = settings.format ();
    QSettings saved {file_, format_}; read (saved);
  }
  qint64 remaining () const { return qMax (qint64 {0}, next_ - QDateTime::currentMSecsSinceEpoch ()); }
  bool reserve ()
  {
    bool accepted = false;
    bool stored = change ([&] {
      if (remaining ()) return;
      next_ = QDateTime::currentMSecsSinceEpoch () + 300000;
      accepted = true;
    });
    return stored && accepted;
  }
  static qint64 retryAfter (QByteArray const& header, QDateTime const& now)
  {
    bool ok = false;
    auto seconds = header.trimmed ().toLongLong (&ok);
    if (ok && seconds >= 0 && seconds <= std::numeric_limits<qint64>::max () / 1000)
      return seconds * 1000;
    auto date = QDateTime::fromString (QString::fromLatin1 (header), Qt::RFC2822Date);
    return date.isValid () ? qMax (qint64 {0}, now.msecsTo (date)) : 0;
  }
  void failed (int status, QByteArray const& retry)
  {
    bool applied = false;
    auto action = [&] {
      applied = true;
      if (status == 429 || status == 503) failures_ = qMin (failures_ + 1, 4);
      qint64 fallback = (status == 429 || status == 503) ? qMin (600000LL << (qMax (1, failures_) - 1), 3600000LL) : 300000LL;
      auto now = QDateTime::currentDateTimeUtc ();
      // Leave one second beyond the advertised boundary; guard addition overflow.
      qint64 delay = qMax (fallback, retryAfter (retry, now));
      delay = qMin (delay, std::numeric_limits<qint64>::max () - now.toMSecsSinceEpoch () - 1000);
      next_ = qMax (next_, now.toMSecsSinceEpoch () + delay + 1000);
    };
    if (!change (action) && !applied) action ();
  }
  void succeeded () { change ([&] { failures_ = 0; }); }
private:
  void read (QSettings& saved)
  {
    next_ = qMax (next_, saved.value ("CustomPskReporterService/NextQueryUtcMs", 0).toLongLong ());
    failures_ = qBound (0, saved.value ("CustomPskReporterService/RateLimitFailures", failures_).toInt (), 4);
  }
  bool change (std::function<void ()> const& action)
  {
    if (file_.isEmpty ()) { action (); return true; }
    QLockFile lock {file_ + ".psk-query.lock"};
    if (!lock.tryLock (0)) {
      next_ = qMax (next_, QDateTime::currentMSecsSinceEpoch () + 5000);
      return false;
    }
    QSettings saved {file_, format_}; saved.sync (); read (saved);
    action ();
    saved.setValue ("CustomPskReporterService/NextQueryUtcMs", next_);
    saved.setValue ("CustomPskReporterService/RateLimitFailures", failures_);
    saved.sync ();
    return saved.status () == QSettings::NoError;
  }
  QString file_;
  QSettings::Format format_ = QSettings::IniFormat;
  qint64 next_ = 0;
  int failures_ = 0;
};
#endif
