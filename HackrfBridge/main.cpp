#include "PlutoBridge/Bridge.hpp"
#include "Hackrf.hpp"
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QLockFile>
#include <QRegularExpression>
#include <QTimer>
#include <iostream>
int main(int argc,char** argv){
  QCoreApplication app(argc,argv);app.setApplicationName("wsjtx-hackrf-bridge");app.setApplicationVersion("1.0 — build by OE3SRO");
  QCommandLineParser parser;parser.setApplicationDescription("HackRF One half-duplex USB-IQ / TCI CAT and audio bridge");parser.addHelpOption();parser.addVersionOption();
  parser.addOption({"serial","HackRF serial; empty requires exactly one attached radio","SERIAL"});
  parser.addOption({"port","Loopback TCI port","PORT","40003"});
  parser.addOption({"frequency","Initial dial frequency, 1 MHz to 6 GHz","HZ","432500000"});
  parser.addOption({"rx-lna","RX LNA gain, 0..40 dB, steps of 8","DB","16"});
  parser.addOption({"rx-vga","RX VGA gain, 0..62 dB, steps of 2","DB","20"});
  parser.addOption({"tx-gain","TX VGA gain, 0..47 dB; RF amplifier remains OFF","DB","0"});
  parser.addOption({"amplitude","Digital IQ amplitude scale, greater than 0 and at most 1","SCALE","0.5"});
  parser.addOption({"library","HackRF DLL path","FILE",QDir(app.applicationDirPath()).filePath("hackrf/hackrf-0.dll")});
  parser.addOption({"ready-file","Write startup and PTT status JSON","FILE"});parser.addOption({"stop-file","Exit gracefully when this file appears","FILE"});
  parser.addOption({"simulate","USB IQ loopback without loading or opening hardware"});
  parser.addOption({"check-runtime","Resolve libhackrf ABI without initializing USB or opening a device"});parser.process(app);
  pluto::Options o;o.hackrf=true;o.simulate=parser.isSet("simulate");o.serial=parser.value("serial");o.library=parser.value("library");o.readyFile=parser.value("ready-file");o.stopFile=parser.value("stop-file");
  bool a,b,c,d,e,f;auto port=parser.value("port").toUInt(&a);o.frequency=parser.value("frequency").toLongLong(&b);
  int lna=parser.value("rx-lna").toInt(&c);o.rxVga=parser.value("rx-vga").toInt(&d);int gain=parser.value("tx-gain").toInt(&e);o.amplitude=parser.value("amplitude").toFloat(&f);
  if(!a||!b||!c||!d||!e||!f||port<1024||port>65535||o.frequency<o.minimumFrequency()||o.frequency>o.maximumFrequency()||lna<0||lna>40||lna%8||o.rxVga<0||o.rxVga>62||o.rxVga%2||gain<0||gain>47||!std::isfinite(o.amplitude)||o.amplitude<=0||o.amplitude>1||(!o.simulate&&!o.serial.isEmpty()&&!QRegularExpression("^[0-9a-fA-F]{1,32}$").match(o.serial).hasMatch())){
    std::cerr<<"Invalid HackRF serial, port, frequency or gain\n";return 2;
  }
  o.port=quint16(port);o.rxGain=lna;o.txGain=gain;
  if(parser.isSet("check-runtime")){try{pluto::HackrfApi api(o.library);std::cout<<"HackRF libhackrf "<<api.hackrf_library_version()<<" runtime verified; no USB initialization or device opened\n";return 0;}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
  auto key=QCryptographicHash::hash((o.serial+(o.simulate?"-simulate":"")).toUtf8(),QCryptographicHash::Sha256).toHex();
  QLockFile lock(QDir::temp().filePath("wsjtx-hackrf-"+QString::fromLatin1(key)+".lock"));lock.setStaleLockTime(0);if(!lock.tryLock()){std::cerr<<"Another WSJT-X HackRF bridge owns this serial\n";return 3;}
  QTimer timer;if(!o.stopFile.isEmpty()){QObject::connect(&timer,&QTimer::timeout,&app,[&]{if(QFileInfo::exists(o.stopFile))app.quit();});timer.start(100);}
  pluto::Bridge bridge(o);bridge.start();return app.exec();
}
