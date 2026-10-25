#ifndef WSJTX_PLUTO_BRIDGE_HPP
#define WSJTX_PLUTO_BRIDGE_HPP
#include "Dsp.hpp"
#include <QObject>
#include <QByteArray>
#include <QWebSocketServer>
#include <QWebSocket>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

namespace pluto {
struct Options {
  QString uri="ip:192.168.2.1",library,readyFile,stopFile;
  quint16 port=40002;
  long long frequency=432500000;
  double rxGain=30,txGain=-40;
  bool simulate=false,hackrf=false;
  QString serial;
  int rxVga=20;
  float amplitude=.5f;
  bool txAmp=false;
  QString deviceName() const {return hackrf?"HackRF One":"PlutoSDR";}
  long long minimumFrequency() const {return hackrf?1000000:325000000;}
  long long maximumFrequency() const {return hackrf?6000000000LL:3800000000LL;}
};
struct AudioHeader {
  quint32 receiver=0,rate=audio_rate,format=3,codec=0,crc=0,length=0,type=0;
  quint32 reserved[9]={};
};
static_assert(sizeof(AudioHeader)==64,"TCI header ABI");
QByteArray audioPacket(quint32 type,std::vector<float> const& mono);
bool readAudioPacket(QByteArray const&,std::vector<float>& mono);

class RadioWorker final : public QObject {
  Q_OBJECT
  Options options;
  std::thread thread;
  std::atomic<bool> quit{false},requestedPtt{false};
  std::atomic<long long> rxFrequency,txFrequency;
  std::mutex mutex;
  std::condition_variable available;
  std::vector<float> pending;
  bool waiting=false;
  void run();
public:
  explicit RadioWorker(Options const&);
  ~RadioWorker() override;
  void start();
  void stop();
  void ptt(bool);
  void tune(long long rx,long long tx);
  bool submit(std::vector<float> const&);
Q_SIGNALS:
  void ready();
  void waitingForDevice(QString);
  void fault(QString);
  void txAborted(QString);
  void audio(QByteArray);
  void needAudio();
  void pttReady(bool);
  void frequencyReady(qint64,qint64);
};
class Bridge final : public QObject {
  Q_OBJECT
  Options options;
  QWebSocketServer server;
  QWebSocket* client=nullptr;
  RadioWorker radio;
  bool streaming=false,keyed=false,split=false,available=false;
  bool waitingDevice=false,greeted=false;
  long long rxFrequency,txFrequency;
  QString lastTxWarning,lastError;
  unsigned txAbortCount=0;
  void send(QString const&);
  void greet();
  void commands(QString const&);
  void fail(QString const&);
  void writeReady(bool,QString const& = {});
  void reportTxAbort(QString const& reason,QString const& code);
public:
  explicit Bridge(Options const&);
  ~Bridge() override;
  void start();
};
}
#endif
