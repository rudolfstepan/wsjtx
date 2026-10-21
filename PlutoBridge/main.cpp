#include "Bridge.hpp"
#include "Iio.hpp"
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QLockFile>
#include <QCryptographicHash>
#include <QTimer>
#include <iostream>

int main(int argc,char** argv) {
  QCoreApplication app(argc,argv);
  app.setApplicationName("wsjtx-pluto-bridge");
  app.setApplicationVersion("1.0 — build by OE3SRO");
  QCommandLineParser parser;
  parser.setApplicationDescription("PlutoSDR USB radio and TCI CAT/audio bridge for a second WSJT-X instance");
  parser.addHelpOption();parser.addVersionOption();
  parser.addOption({"uri","Pluto libiio URI","URI","ip:192.168.2.1"});
  parser.addOption({"port","Loopback TCI port","PORT","40002"});
  parser.addOption({"frequency","Initial dial frequency in Hz (325 MHz to 3.8 GHz)","HZ","432500000"});
  parser.addOption({"rx-gain","Manual RX gain in dB (0..70)","DB","30"});
  parser.addOption({"tx-gain","TX hardware gain in dB (-89.75..0)","DB","-40"});
  parser.addOption({"library","libiio 0.26 DLL path","FILE",QDir(app.applicationDirPath()).filePath("pluto/libiio.dll")});
  parser.addOption({"ready-file","Write startup status JSON","FILE"});
  parser.addOption({"stop-file","Exit gracefully when this file appears","FILE"});
  parser.addOption({"simulate","Run USB IQ loopback without opening any radio"});
  parser.addOption({"check-runtime","Validate libiio ABI without opening a device"});
  parser.process(app);
  bool ok1,ok2,ok3,ok4;
  pluto::Options options;
  options.uri=parser.value("uri");options.library=parser.value("library");options.readyFile=parser.value("ready-file");
  options.stopFile=parser.value("stop-file");
  auto port=parser.value("port").toUInt(&ok1);
  options.frequency=parser.value("frequency").toLongLong(&ok2);
  options.rxGain=parser.value("rx-gain").toDouble(&ok3);options.txGain=parser.value("tx-gain").toDouble(&ok4);
  options.simulate=parser.isSet("simulate");
  if(!ok1 || !ok2 || !ok3 || !ok4 || port<1024 || port>65535 || options.frequency<325000000 || options.frequency>3800000000LL
     || !std::isfinite(options.rxGain) || !std::isfinite(options.txGain) || options.rxGain<0 || options.rxGain>70 || options.txGain< -89.75 || options.txGain>0) {
    std::cerr<<"Invalid Pluto bridge port, frequency or gain\n";return 2;
  }
  options.port=quint16(port);
  if(parser.isSet("check-runtime")) {
    try {pluto::IioApi api(options.library);std::cout<<"Pluto libiio 0.26 runtime verified; no device opened\n";return 0;}
    catch(std::exception const& e) {std::cerr<<e.what()<<'\n';return 1;}
  }
  auto key=QCryptographicHash::hash((options.uri+(options.simulate?"-simulate":"")).toUtf8(),QCryptographicHash::Sha256).toHex();
  QLockFile deviceLock(QDir::temp().filePath("wsjtx-pluto-"+QString::fromLatin1(key)+".lock"));
  deviceLock.setStaleLockTime(0);
  if(!deviceLock.tryLock()) {std::cerr<<"Another WSJT-X Pluto bridge owns this URI\n";return 3;}
  QTimer shutdownTimer;
  if(!options.stopFile.isEmpty()) {
    QObject::connect(&shutdownTimer,&QTimer::timeout,&app,[&]{if(QFileInfo::exists(options.stopFile)) app.quit();});
    shutdownTimer.start(100);
  }
  pluto::Bridge bridge(options);
  bridge.start();
  return app.exec();
}
