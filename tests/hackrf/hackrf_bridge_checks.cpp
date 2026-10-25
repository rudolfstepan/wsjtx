#include "HackrfBridge/Hackrf.hpp"
#include "PlutoBridge/Profile.hpp"
#include "FakeState.hpp"
#include "Transceiver/TCITransceiver.hpp"
#include "widgets/itoneAndicw.h"
extern "C" { decltype(foxcom_) foxcom_; }
dec_data_t dec_data;
int volatile itone[MAX_NUM_SYMBOLS]={};
int volatile icw[NUM_CW_SYMBOLS]={};
float gran(){return 0;}
#include <QCoreApplication>
#include <QLibrary>
#include <QTemporaryDir>
#include <QProcess>
#include <QJsonDocument>
#include <QThread>
#include <iostream>
static int checks=0;
static void check(bool yes,char const* text){if(!yes)throw std::runtime_error(text);++checks;}
int main(int argc,char** argv){QCoreApplication app(argc,argv);
 try {
  qRegisterMetaTypeStreamOperators<TransceiverFactory::PTTMethod>("TransceiverFactory::PTTMethod");qRegisterMetaTypeStreamOperators<TransceiverFactory::SplitMode>("TransceiverFactory::SplitMode");
  QString dll=QDir(app.applicationDirPath()).filePath("libhackrf_fake.dll");QLibrary mock(dll);check(mock.load(),"load hardware mock");
  auto state=reinterpret_cast<FakeHackrfState*(*)()>(mock.resolve("fake_hackrf_state"))();
  auto sample=reinterpret_cast<int(*)(unsigned)>(mock.resolve("fake_hackrf_sample"));
  {pluto::HackrfApi api(dll);check(state->initialized==0&&state->opens==0,"runtime validation never initializes USB");}
  {
   pluto::HackrfRadio radio(dll,"",432500000,16,20,0);
   check(state->mode==1&&state->rate==3072000&&state->filter==1750000,"RX starts at fixed DSP rate");
   check(state->amp==0&&state->bias==0&&state->lna==16&&state->vga==20&&state->gain==0,"explicit gains and RF amp / bias OFF");
   auto pcm=radio.receive();check(pcm.size()==pluto::rf_block&&pcm[0]==pluto::IQ(.5f,-.25f),"signed IQ8 RX and complete block assembly");
   radio.tune(1000000,6000000000LL);check(state->frequency==1000000,"1 MHz RX / 6 GHz TX accepted without narrowing");
   radio.ptt(true);check(state->mode==0&&state->rxStops==1,"RX stops before pending TX");
   std::vector<int8_t> expected;
   for(int block=0;block<40;++block){float v=float(block%9+1)/20;std::vector<pluto::IQ> iq(pluto::rf_block,{v,-v});
    int8_t x=int8_t(std::lround(.5f*v*127));for(int i=0;i<pluto::rf_block;++i){expected.push_back(x);expected.push_back(-x);}radio.transmit(iq);
   }
   auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);while(state->txBytes<9*262144ULL&&std::chrono::steady_clock::now()<deadline)QThread::msleep(1);
   check(state->txStarts==1&&state->frequency==6000000000ULL&&state->txBytes>=9*262144ULL,"TX priming / synchronous callbacks / 64-bit tuning");
   for(unsigned i=0;i<9*262144U;i+=509)check(sample(i)==expected[i],"continuous exact IQ8 across callback boundaries");
   radio.ptt(false);check(state->mode==1&&state->txStops==1&&state->overlap==0,"half-duplex switch returns to RX without overlap");
   radio.ptt(true);std::vector<pluto::IQ> fresh(pluto::rf_block,{.1f,.2f});for(int i=0;i<18;++i)radio.transmit(fresh);
   check(sample(0)==6&&sample(1)==13,"new PTT never repeats the preceding frame tail");radio.ptt(false);
  }
  check(state->mode==0&&state->opens==state->closes&&state->initialized==0,"close stops streams and releases runtime");
  state->failTune=1;bool rejected=false;try{pluto::HackrfRadio radio(dll,"0123456789abcdef0123456789abcdef",432500000,16,20,0);}catch(...){rejected=true;}state->failTune=0;
  check(rejected&&state->opens==state->closes&&state->initialized==0,"configuration failure closes device");
  state->devices=2;int opened=state->opens;rejected=false;try{pluto::HackrfRadio radio(dll,"",432500000,16,20,0);}catch(...){rejected=true;}state->devices=1;
  check(rejected&&state->opens==opened&&state->initialized==0,"ambiguous automatic device selection rejected before opening");
  {
   pluto::HackrfRadio radio(dll,"",18100000,16,20,47,1.f,true);
   check(state->gain==47&&state->amp==0&&state->bias==0,"maximum TX gain configured; RX amp and bias remain OFF");
   radio.ptt(true);check(state->amp==1&&state->mode==0,"internal TX amplifier enabled after RX stops");
   std::vector<pluto::IQ> full(pluto::rf_block,{1.f,-1.f});for(int i=0;i<18;++i)radio.transmit(full);
   check(state->mode==2&&sample(0)==127&&sample(1)==-127,"full-scale IQ8 reaches TX with amplifier enabled");
   radio.ptt(false);check(state->amp==0&&state->mode==1,"return to RX disables TX amplifier");
   radio.ptt(true);
  }
  check(state->amp==0&&state->mode==0&&state->opens==state->closes,"cleanup disables amplifier even from pending TX");
  {
   state->rxDelayMs=1400;
   pluto::HackrfRadio radio(dll,"",18100000,16,20,47,1.f,true);
   auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(4);
   std::vector<pluto::IQ> iq;int waits=0;
   while(iq.empty()&&std::chrono::steady_clock::now()<deadline){iq=radio.receive();if(iq.empty())++waits;}
   check(waits>=10&&iq.size()==pluto::rf_block,"slow asynchronous USB startup waits beyond old one-second timeout without fake samples");
   state->rxDelayMs=0;state->pauseRx=1;
   int starts=state->rxStarts;
   deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
   while(state->rxStarts==starts&&std::chrono::steady_clock::now()<deadline)radio.receive();
   check(state->rxStarts>starts&&state->amp==0,"stalled RX stream restarted while TX amplifier stays off");
   state->pauseRx=0;iq.clear();deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
   while(iq.empty()&&std::chrono::steady_clock::now()<deadline)iq=radio.receive();
   check(iq.size()==pluto::rf_block,"RX resumes after transient USB stall");
  }
  QTemporaryDir temp;QString profile=temp.filePath("HackRF.ini"),source=temp.filePath("Pluto.ini");
  {QSettings ini(source,QSettings::IniFormat);ini.setValue("Configuration/MyCall","OE3SRO/P");ini.setValue("Configuration/MyGrid","JN88");ini.setValue("Configuration/Password","secret");ini.sync();}
  pluto::prepareProfile(profile,source,432500000,40003,true);QSettings settings(profile,QSettings::IniFormat);
  check(settings.value("Configuration/Rig")=="HackRF One (TCI bridge)"&&settings.value("Configuration/CATTCIPort")=="127.0.0.1:40003","separate named rig and endpoint");
  check(settings.value("Configuration/UDPServerPort")==2239&&settings.value("Configuration/MyCall")=="OE3SRO/P"&&!settings.contains("Configuration/Password"),"separate UDP / station identity only");
  check(!settings.value("Common/AutoTx").toBool()&&!settings.value("Common/TxFirst").toBool(),"new profile has auto TX OFF and odd slots");
  settings.setValue("Common/DialFreq",QVariant::fromValue(quint64(6000000000ULL)));settings.setValue("Configuration/CATTCIPort","127.0.0.1:40007");settings.setValue("Configuration/MyCall","DL1ABC");settings.sync();
  QProcess prepare;prepare.start(QDir(app.applicationDirPath()).filePath("wsjtx.exe"),{"--prepare-hackrf-profile",profile,"--hackrf-station-source",source});
  check(prepare.waitForFinished(10000)&&prepare.exitCode()==0,"real profile CLI remains headless");auto data=QJsonDocument::fromJson(prepare.readAllStandardOutput()).object();
  check(data.value("frequency").toDouble()==6000000000.&&data.value("port").toInt()==40007,"launcher receives preserved profile endpoint and dial");
  settings.sync();check(settings.value("Configuration/MyCall")=="DL1ABC","existing HackRF station identity preserved");
  auto ft8=temp.filePath("HackRF-FT8-17m.ini");
  QStringList ft8Args={"--prepare-hackrf-profile",ft8,"--hackrf-station-source",source,"--hackrf-frequency","18100000","--hackrf-port","40004","--hackrf-mode","FT8","--hackrf-udp-port","2243"};
  prepare.start(QDir(app.applicationDirPath()).filePath("wsjtx.exe"),ft8Args);
  check(prepare.waitForFinished(10000)&&prepare.exitCode()==0,"real FT8 17m profile CLI");
  QSettings ft8Settings(ft8,QSettings::IniFormat);
  check(ft8Settings.value("Common/Mode")=="FT8"&&ft8Settings.value("Common/DialFreq").toULongLong()==18100000,"new FT8 profile starts on 17m");
  check(ft8Settings.value("Configuration/UDPServerPort")==2243&&ft8Settings.value("Configuration/CATTCIPort")=="127.0.0.1:40004"&&ft8Settings.value("Configuration/TCIAudio").toBool(),"FT8 profile isolated ports and direct TCI audio");
  check(!ft8Settings.value("Common/AutoTx").toBool(),"FT8 profile does not enable TX");
  ft8Settings.setValue("Common/DialFreq",14074000);ft8Settings.setValue("Common/Mode","FT4");ft8Settings.sync();
  prepare.start(QDir(app.applicationDirPath()).filePath("wsjtx.exe"),ft8Args);
  check(prepare.waitForFinished(10000)&&prepare.exitCode()==0,"existing FT8 profile can restart");ft8Settings.sync();
  check(ft8Settings.value("Common/Mode")=="FT4"&&ft8Settings.value("Common/DialFreq").toULongLong()==14074000,"later mode and frequency choices preserved");
  std::cout<<"PASS HackRF: "<<checks<<" ABI, half-duplex, IQ8, priming, cleanup, profile and production CLI checks; mock only\n";
 }catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}
}
