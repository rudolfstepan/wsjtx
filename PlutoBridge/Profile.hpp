#ifndef WSJTX_PLUTO_PROFILE_HPP
#define WSJTX_PLUTO_PROFILE_HPP
#include "Transceiver/TransceiverFactory.hpp"
#include <QSettings>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QCommandLineParser>
#include <iostream>
#include <stdexcept>

namespace pluto {
inline void prepareProfile(QString const& path,QString const& stationSource,quint64 frequency,unsigned port) {
  if(QFileInfo::exists(path)) {
    // Repair only invalid startup values from older bridge profiles. Preserve
    // station settings, endpoint, valid dial frequency and all other choices.
    QSettings existing(path,QSettings::IniFormat);
    if(existing.value("Configuration/Rig")=="PlutoSDR (TCI bridge)") {
      // Old profile preparation registered only PTTMethod/SplitMode and could
      // replace other serialized Qt values with @Invalid(). Missing keys use
      // WSJT-X's normal defaults; keep every remaining valid setting intact.
      QStringList invalid;
      for(auto const& key:existing.allKeys())
        if(!existing.value(key).isValid()) invalid<<key;
      if(!invalid.isEmpty()) {
        auto backup=path+".before-invalid-repair";
        if(!QFileInfo::exists(backup) && !QFile::copy(path,backup))
          throw std::runtime_error("Cannot back up damaged Pluto profile");
        for(auto const& key:invalid) existing.remove(key);
      }
      auto dial=existing.value("Common/DialFreq").toULongLong();
      if(dial<325000000 || dial>3800000000ULL)
        existing.setValue("Common/DialFreq",QVariant::fromValue(frequency));
      if(existing.value("Configuration/TCIAudio").toBool())
        existing.setValue("Configuration/Polling",existing.value("Configuration/Polling",1).toInt() | tci__audio);
      existing.sync();
      if(existing.status()!=QSettings::NoError) throw std::runtime_error("Cannot repair Pluto startup settings");
    }
    return;
  }
  if(!QDir().mkpath(QFileInfo(path).absolutePath())) throw std::runtime_error("Cannot create Pluto profile directory");
  QSettings out(path,QSettings::IniFormat);
  out.setValue("Configuration/Rig","PlutoSDR (TCI bridge)");
  out.setValue("Configuration/CATTCIPort",QString("127.0.0.1:%1").arg(port));
  out.setValue("Configuration/TCIAudio",true);
  out.setValue("Configuration/PTTMethod",QVariant::fromValue(TransceiverFactory::PTT_method_CAT));
  out.setValue("Configuration/SplitMode",QVariant::fromValue(TransceiverFactory::split_mode_none));
  out.setValue("Configuration/Polling",1 | tci__audio);
  out.setValue("Configuration/TxDelay",.02);
  out.setValue("Configuration/MonitorOFF",false);
  out.setValue("Configuration/PSKReporter",false);
  out.setValue("Configuration/UDPServer","127.0.0.1");
  out.setValue("Configuration/UDPServerPort",2238);
  out.setValue("Common/Mode","REIST16");
  out.setValue("Common/DialFreq",QVariant::fromValue(frequency));
  out.setValue("Common/TxFreq",1500);
  out.setValue("Common/RxFreq",1500);
  out.setValue("Common/TxFirst",true);
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
  if(out.status()!=QSettings::NoError) throw std::runtime_error("Cannot write Pluto profile");
}
inline int prepareProfileCommand(int argc,char** argv) {
  // This path intentionally initializes no GUI platform plugin, audio or CAT.
  QCoreApplication app(argc,argv);
  qRegisterMetaTypeStreamOperators<TransceiverFactory::PTTMethod>("TransceiverFactory::PTTMethod");
  qRegisterMetaTypeStreamOperators<TransceiverFactory::SplitMode>("TransceiverFactory::SplitMode");
  QCommandLineParser parser;
  parser.addOption({"prepare-pluto-profile","Create a new Pluto profile","FILE"});
  parser.addOption({"pluto-station-source","Copy only callsign and grid","FILE"});
  parser.addOption({"pluto-frequency","Initial Pluto dial frequency","HZ","432500000"});
  parser.addOption({"pluto-port","Local Pluto bridge port","PORT","40002"});
  try {
    if(!parser.parse(app.arguments())) throw std::runtime_error(parser.errorText().toStdString());
    bool frequencyOk=false,portOk=false;
    auto frequency=parser.value("pluto-frequency").toULongLong(&frequencyOk);
    auto port=parser.value("pluto-port").toUInt(&portOk);
    if(!frequencyOk || !portOk || frequency<325000000 || frequency>3800000000ULL || port<1024 || port>65535)
      throw std::runtime_error("Invalid Pluto profile frequency or port");
    prepareProfile(parser.value("prepare-pluto-profile"),parser.value("pluto-station-source"),frequency,port);
    return 0;
  } catch(std::exception const& e) {std::cerr<<e.what()<<'\n';return 2;}
}
}
#endif
