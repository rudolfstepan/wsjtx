// Production settings dialog regression. Temporary profiles and loopback only.
#include "Configuration.hpp"
#include "PlutoBridge/Bridge.hpp"
#include <QTcpServer>
#include <QElapsedTimer>
#include <QThread>
#include "PlutoBridge/Profile.hpp"
#include "Logger.hpp"
#include "Transceiver/TCITransceiver.hpp"
#include "widgets/itoneAndicw.h"
#include <QApplication>
#include <QAudioDeviceInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTemporaryDir>
#include <QDialog>
#include <QCheckBox>
#include <QComboBox>
#include <QTimer>
#include <iostream>

BOOST_LOG_GLOBAL_LOGGER_CTOR_ARGS(sys,
 boost::log::sources::severity_channel_logger_mt<boost::log::trivial::severity_level>,
 (boost::log::keywords::channel="SDR configuration checks"));
extern "C" { decltype(foxcom_) foxcom_; }
dec_data_t dec_data;
int volatile itone[MAX_NUM_SYMBOLS]={};
int volatile icw[NUM_CW_SYMBOLS]={};
float gran() {return 0;}
static void check(bool yes,char const* why) {if(!yes)throw std::runtime_error(why);}
class LocalNetwork final : public QNetworkAccessManager {
 QNetworkReply* createRequest(Operation op,QNetworkRequest const&,QIODevice*) override {
   // Any optional startup downloads are redirected to a local empty resource.
   return QNetworkAccessManager::createRequest(op,QNetworkRequest(QUrl("qrc:/configuration-check-empty")),nullptr);
 }
};
int main(int argc,char**argv) {
 qputenv("QT_QPA_PLATFORM","offscreen");
 auto work=QFileInfo(QString::fromLocal8Bit(argv[0])).absoluteDir().filePath("sdr-configuration-checks");
 QDir().mkpath(work);
 qputenv("LOCALAPPDATA",work.toUtf8());qputenv("APPDATA",work.toUtf8());
 QApplication app(argc,argv);app.setOrganizationName("OE3SRO Tests");app.setApplicationName("SDR settings");
 try {
  qRegisterMetaTypeStreamOperators<TransceiverFactory::PTTMethod>("TransceiverFactory::PTTMethod");
  qRegisterMetaTypeStreamOperators<TransceiverFactory::SplitMode>("TransceiverFactory::SplitMode");
  QTemporaryDir profiles(work+"/profiles-XXXXXX");check(profiles.isValid(),"temporary profiles");
  LocalNetwork network;
  // A render-loopback RX must not replace CAT/PTT or the separate TX device.
  {
   auto path=profiles.filePath("Yaesu-WebSDR.ini");
   QSettings settings(path,QSettings::IniFormat);
   auto loopback=QString("Realtek Digital Output (Realtek USB Audio)");
   QString output="TX output test device";
   bool haveOutput=false;
   for(auto const& device:QAudioDeviceInfo::availableDevices(QAudio::AudioOutput))
     if(device.deviceName()!=loopback){output=device.deviceName();haveOutput=true;break;}
   settings.setValue("Configuration/Rig","Yaesu FT-991");
   settings.setValue("Configuration/CATSerialPort","COM987"); // must never open
   settings.setValue("Configuration/SoundInName","Original Yaesu microphone");
   settings.setValue("Configuration/AudioInputChannel","Left");
   settings.setValue("Configuration/SoundOutName",output);
   settings.setValue("Configuration/AudioOutputChannel","Mono");
   settings.setValue("Configuration/MyCall","OE3SRO");
   settings.setValue("Configuration/MyGrid","JN87");
   settings.sync();
   app.setProperty("wasapiLoopbackDevice",loopback);
   app.setProperty("receiveOnly",false);
   {
    Configuration configuration(&network,QDir(profiles.path()),&settings,nullptr);
    check(configuration.rig_name()=="Yaesu FT-991","hybrid must retain Yaesu CAT rig");
    check(!configuration.is_tci(),"hybrid uses normal Yaesu transmit path");
    check(configuration.audio_input_channel()==AudioDevice::Mono,"loopback detector must use mono");
    if(haveOutput)check(configuration.audio_output_device().deviceName()==output,"loopback RX must not replace TX output");
    check(!app.property("receiveOnly").toBool(),"hybrid must permit normal transmit sequencing");
   }
   check(settings.value("Configuration/SoundInName")=="Original Yaesu microphone","runtime loopback must preserve normal RX choice");
   check(settings.value("Configuration/SoundOutName")==output,"TX output must persist unchanged");
   check(settings.value("Configuration/CATSerialPort")=="COM987","CAT port must persist unchanged");
   app.setProperty("wasapiLoopbackDevice",QVariant());
   std::cout<<"PASS hybrid configuration: WASAPI mono RX, Yaesu rig/CAT, independent TX output, original microphone choice preserved; no radio opened.\n";
  }
  for(bool hackrf:{true,false}) {
   auto path=profiles.filePath(hackrf?"HackRF.ini":"Pluto.ini");
   QTcpServer port;check(port.listen(QHostAddress::LocalHost,0),"allocate loopback test port");
   auto number=port.serverPort();port.close();
   pluto::Options options;options.hackrf=hackrf;options.simulate=true;options.port=number;
   pluto::Bridge bridge(options);bridge.start();
   QElapsedTimer ready;ready.start();while(ready.elapsed()<100){app.processEvents();QThread::msleep(1);}
   pluto::prepareProfile(path,{},432500000,number,hackrf);
   QSettings settings(path,QSettings::IniFormat);
   settings.setValue("Configuration/MyCall","OE3SRO/P");settings.setValue("Configuration/MyGrid","JN88");settings.sync();
   Configuration configuration(&network,QDir(profiles.path()),&settings,nullptr);
   check(configuration.is_tci(),"SDR alias must be recognized as TCI when loading the profile");
   check(configuration.tci_audio(),"SDR profile keeps TCI audio enabled");
   QString failure;int frames=0;
   QObject::connect(&configuration,&Configuration::transceiver_failure,[&](QString why){failure=why;});
   QObject::connect(&configuration,&Configuration::transceiver_TCIframesWritten,[&](qint64){++frames;});
   QTimer::singleShot(0,[&]{
     auto dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());
     if(!dialog) {failure="settings dialog missing";app.exit(1);return;}
     auto rig=dialog->findChild<QComboBox*>("rig_combo_box");
     auto audio=dialog->findChild<QCheckBox*>("tci_audio_check_box");
     if(!rig||!audio)failure="settings controls missing";
     else {
       if(audio->isHidden()||!audio->isChecked())failure="SDR TCI audio checkbox must be offered and checked";
       rig->setCurrentText("None");
       if(!audio->isHidden())failure="non-TCI rig must hide TCI audio";
       rig->setCurrentText(hackrf?"HackRF One (TCI bridge)":"PlutoSDR (TCI bridge)");
       if(audio->isHidden()||!audio->isChecked()||!configuration.is_tci())failure="switching back to SDR alias must restore TCI controls";
     }
     if(failure.isEmpty()) {
       if(!QMetaObject::invokeMethod(dialog,"on_test_CAT_push_button_clicked",Qt::DirectConnection))failure="CAT test slot missing";
       QElapsedTimer deadline;deadline.start();
       while(failure.isEmpty()&&frames==0&&deadline.elapsed()<7000){app.processEvents();QThread::msleep(1);}
       if(failure.isEmpty()&&frames==0)failure="CAT test must start TCI audio through the production settings dialog";
     }
     if(failure.isEmpty())dialog->accept();else dialog->reject();
   });
   configuration.exec();
   check(configuration.is_tci()&&configuration.tci_audio(),"accepting settings preserves SDR TCI mode and audio");
   if(!failure.isEmpty())throw std::runtime_error(failure.toStdString());
  }
  std::cout<<"PASS production SDR settings: HackRF/Pluto profile load, TCI audio, rig switch, CAT test receives audio, settings acceptance; simulated radio only.\n";
 }catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}
}
