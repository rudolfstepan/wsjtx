#ifndef WSJTX_WEB_CONTROL_SERVER_HPP
#define WSJTX_WEB_CONTROL_SERVER_HPP

#include <QTcpServer>
#include <QPointer>
#include <QJsonObject>
#include <functional>

class QWidget;
class QDialog;

// Runs on the GUI thread: the desktop remains the single source of truth.
class WebControlServer final : public QTcpServer
{
public:
  explicit WebControlServer (QWidget * window, QWidget * waterfall, QDialog * log);
  bool start (quint16 port, QString const& accessKey = {});
  void stop ();
  QString token () const { return token_; }
  void showSettings ();
  std::function<void(QString const&, int)> selectDecode;
  std::function<QJsonObject()> stationStatus;

private:
  void acceptConnections ();
  void readRequest (QTcpSocket *);
  void respond (QTcpSocket *, int, QByteArray const&, QByteArray const& type = "application/json");
  QJsonObject state () const;
  QString command (QJsonObject const&);
  QPointer<QWidget> window_, waterfall_;
  QPointer<QDialog> log_;
  QString token_;
};
#endif
