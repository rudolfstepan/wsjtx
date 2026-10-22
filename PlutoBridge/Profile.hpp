#ifndef WSJTX_PLUTO_PROFILE_HPP
#define WSJTX_PLUTO_PROFILE_HPP
#include "Transceiver/TransceiverFactory.hpp"
#include <QSettings>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QCommandLineParser>
#include <iostream>
#include <QJsonDocument>
#include <QJsonObject>
#include <stdexcept>

namespace pluto {
inline void prepareProfile(QString const& path,QString const& stationSource,quint64 frequency,unsigned port,bool hackrf=false) {
  auto rig=hackrf?"HackRF One (TCI bridge)":"PlutoSDR (TCI bridge)";
  quint64 minimum=hackrf?1000000:325000000,maximum=hackrf?6000000000ULL:3800000000ULL;
  if(QFileInfo::exists(path)) {
    // Repair only invalid startup values from older bridge profiles. Preserve
    // station settings, endpoint, valid dial frequency and all other choices.
    QSettings existing(path,QSettings::IniFormat);
    if(existing.value("Configuration/Rig")==rig) {
      // Old profile preparation registered only PTTMethod/SplitMode and could
      // replace other serialized Qt values with @Invalid(). Missing keys use
      // WSJT-X's normal defaults; keep every remaining valid setting intact.
      QStringList invalid;
      for(auto const& key:existing.allKeys())
        if(!existing.value(key).isValid()) invalid<<key;
      if(!invalid.isEmpty()) {
        auto backup=path+".before-invalid-repair";
        if(!QFileInfo::exists(backup) && !QFile::copy(path,backup))
          throw std::runtime_error("Cannot back up damaged SDR profile");
        for(auto const& key:invalid) existing.remove(key);
      }
      auto dial=existing.value("Common/DialFreq").toULongLong();
      if(dial<minimum || dial>maximum)
        existing.setValue("Common/DialFreq",QVariant::fromValue(frequency));
      if(existing.value("Configuration/TCIAudio").toBool())
        existing.setValue("Configuration/Polling",existing.value("Configuration/Polling",1).toInt() | tci__audio);
      existing.sync();
      if(existing.status()!=QSettings::NoError) throw std::runtime_error("Cannot repair SDR startup settings");
    }
    return;
  }
  if(!QDir().mkpath(QFileInfo(path).absolutePath())) throw std::runtime_error("Cannot create SDR profile directory");
  QSettings out(path,QSettings::IniFormat);
  out.setValue("Configuration/Rig",rig);
  out.setValue("Configuration/CATTCIPort",QString("127.0.0.1:%1").arg(port));
  out.setValue("Configuration/TCIAudio",true);
  out.setValue("Configuration/PTTMethod",QVariant::fromValue(TransceiverFactory::PTT_method_CAT));
  out.setValue("Configuration/SplitMode",QVariant::fromValue(TransceiverFactory::split_mode_none));
  out.setValue("Configuration/Polling",1 | tci__audio);
  out.setValue("Configuration/TxDelay",.02);
  out.setValue("Configuration/MonitorOFF",false);
  out.setValue("Configuration/PSKReporter",false);
  out.setValue("Configuration/UDPServer","127.0.0.1");
  out.setValue("Configuration/UDPServerPort",hackrf?2239:2238);
  out.setValue("Common/Mode","REIST16");
  out.setValue("Common/DialFreq",QVariant::fromValue(frequency));
  out.setValue("Common/TxFreq",1500);
  out.setValue("Common/RxFreq",1500);
  out.setValue("Common/TxFirst",!hackrf);
  if(hackrf)out.setValue("Common/AutoTx",false);
  out.setValue("Common/AutoSeq",true);
  out.setValue("Common/OutAttenuation",0);
  if(!stationSource.isEmpty() && QFileInfo::exists(stationSource)) {
    QSettings source(stationSource,QSettings::IniFormat);
    for(auto key:{"MyCall","MyGrid"}) {
      QString name=QString("Configuration/")+key;
      if(source.contains(name)) out.setValue(name,source.value(name));
    }
  }
  out.sync();
  if(out.status()!=QSettings::NoError) throw std::runtime_error("Cannot write SDR profile");
}
inline int prepareProfileCommand(int argc,char** argv,bool hackrf=false) {
  // This path intentionally initializes no GUI platform plugin, audio or CAT.
  QCoreApplication app(argc,argv);
  qRegisterMetaTypeStreamOperators<TransceiverFactory::PTTMethod>("TransceiverFactory::PTTMethod");
  qRegisterMetaTypeStreamOperators<TransceiverFactory::SplitMode>("TransceiverFactory::SplitMode");
  QCommandLineParser parser;
  QString prefix=hackrf?"hackrf":"pluto";
  parser.addOption({"prepare-"+prefix+"-profile","Create a separate SDR profile","FILE"});
  parser.addOption({prefix+"-station-source","Copy only callsign and grid","FILE"});
  parser.addOption({prefix+"-frequency","Initial SDR dial frequency","HZ","432500000"});
  parser.addOption({prefix+"-port","Local SDR bridge port","PORT",hackrf?"40003":"40002"});
  try {
    if(!parser.parse(app.arguments())) throw std::runtime_error(parser.errorText().toStdString());
    bool frequencyOk=false,portOk=false;
    auto frequency=parser.value(prefix+"-frequency").toULongLong(&frequencyOk);
    auto port=parser.value(prefix+"-port").toUInt(&portOk);
    if(!frequencyOk || !portOk || frequency<(hackrf?1000000ULL:325000000ULL) || frequency>(hackrf?6000000000ULL:3800000000ULL) || port<1024 || port>65535)
      throw std::runtime_error("Invalid SDR profile frequency or port");
    auto path=parser.value("prepare-"+prefix+"-profile");
    prepareProfile(path,parser.value(prefix+"-station-source"),frequency,port,hackrf);
    if(hackrf){QSettings profile(path,QSettings::IniFormat);
      if(profile.value("Configuration/Rig")!="HackRF One (TCI bridge)")throw std::runtime_error("Existing HackRF profile uses another rig; select HackRF One (TCI bridge) in Settings");
      auto endpoint=profile.value("Configuration/CATTCIPort").toString();
      if(!endpoint.startsWith("127.0.0.1:"))throw std::runtime_error("HackRF bridge profile requires a loopback endpoint");
      bool valid=false;auto effectivePort=endpoint.section(':',1).toUInt(&valid);
      if(!valid||effectivePort<1024||effectivePort>65535)throw std::runtime_error("Invalid existing HackRF endpoint");
      std::cout<<QJsonDocument(QJsonObject{{"profile",path},{"port",int(effectivePort)},{"frequency",double(profile.value("Common/DialFreq").toULongLong())}}).toJson(QJsonDocument::Compact).constData()<<'\n';
    }
    return 0;
  } catch(std::exception const& e) {std::cerr<<e.what()<<'\n';return 2;}
}
}
#endif
