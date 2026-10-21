#include "PlutoBridge/Bridge.hpp"
#include "PlutoBridge/Profile.hpp"
#include "PlutoBridge/Iio.hpp"
#include "Transceiver/TCITransceiver.hpp"
#include "lib/reist16/reist16.hpp"
#include "Logger.hpp"
#include "qt_helpers.hpp"
#include "widgets/itoneAndicw.h"
#include <cstring>
#include <limits>
#include <QCoreApplication>
#include <QTcpServer>
#include <QElapsedTimer>
#include <QThread>
#include <QTemporaryDir>
#include <QLibrary>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <iostream>
#include <chrono>
#include <functional>

BOOST_LOG_GLOBAL_LOGGER_CTOR_ARGS(sys,
 boost::log::sources::severity_channel_logger_mt<boost::log::trivial::severity_level>,
 (boost::log::keywords::channel="Pluto checks"));
extern "C" { decltype(foxcom_) foxcom_; }
dec_data_t dec_data;
int volatile itone[MAX_NUM_SYMBOLS]={};
int volatile icw[NUM_CW_SYMBOLS]={};
float gran() {return 0;}
void check(bool yes,char const* why) {if(!yes) throw std::runtime_error(why);}
void waitUntil(std::function<bool()> done,int timeout=5000) {
 QElapsedTimer timer;timer.start();
 while(!done() && timer.elapsed()<timeout) {QCoreApplication::processEvents();QThread::msleep(1);}
 check(done(),"event deadline");
}
struct PlutoTciChecks {
 static void abort(TCITransceiver& rig) {
   int aborted=0;
   auto connection=QObject::connect(&rig,&Transceiver::transmitAbort,[&](QString reason){if(reason.contains("timeout")) ++aborted;});
   rig.m_state=TCITransceiver::Active;
   rig.onMessageReceived("tx_abort:0,audio_timeout;");
   check(aborted==1 && rig.m_state==TCITransceiver::Idle && rig.tci_Ready && rig._power_,"TX timeout stops modulator without losing online radio");
   QObject::disconnect(connection);
 }
 static void handshake(TCITransceiver& rig) {
   rig.busy_mode_=true;
   rig.requested_mode_="usb";
   rig._power_=false;
   rig.onMessageReceived("modulation:0,usb;start;ready;");
   check(rig._power_,"TCI mode reply must not discard following START/READY in handshake");
   rig.busy_mode_=false;
 }
 static void pcm(TCITransceiver& rig,QVector<float> wave) {
   rig.setWaveform(wave);
   rig.do_modulator_start("REIST16",120,960.,1500.,-4.,false,false,99.,15.);
   rig.setWaveform(QVector<float>(wave.size(),-.9f));
   std::vector<float> packet(960);
   for(int block=0;block<960;++block) {
     check(rig.readAudioData(packet.data(),int(packet.size()),0)==packet.size(),"TCI complete audio packet");
     for(int n=0;n<480;++n) {
       check(packet[2*n]==wave[block*480+n] && packet[2*n+1]==packet[2*n],"immutable TCI waveform / exact sample");
     }
   }
   check(rig.m_state==TCITransceiver::Idle,"TCI frame exact end");
   std::fill(packet.begin(),packet.end(),1);
   rig.readAudioData(packet.data(),int(packet.size()),0);
   check(std::all_of(packet.begin(),packet.end(),[](float v){return v==0;}),"no stale TCI tail");
 }
};
int main(int argc,char**argv) {
 QCoreApplication app(argc,argv);
 qRegisterMetaType<Transceiver::TransceiverState>("Transceiver::TransceiverState");
 try {
   qRegisterMetaTypeStreamOperators<TransceiverFactory::PTTMethod>("TransceiverFactory::PTTMethod");
   qRegisterMetaTypeStreamOperators<TransceiverFactory::SplitMode>("TransceiverFactory::SplitMode");
   qRegisterMetaTypeStreamOperators<TransceiverFactory::DataBits>("TransceiverFactory::DataBits");
   qRegisterMetaTypeStreamOperators<TransceiverFactory::StopBits>("TransceiverFactory::StopBits");
   qRegisterMetaTypeStreamOperators<TransceiverFactory::Handshake>("TransceiverFactory::Handshake");
   qRegisterMetaTypeStreamOperators<TransceiverFactory::TXAudioSource>("TransceiverFactory::TXAudioSource");
   QTemporaryDir profiles;
   auto path=profiles.filePath("Pluto.ini");
   auto sourcePath=profiles.filePath("Source.ini");
   {QSettings source(sourcePath,QSettings::IniFormat);source.setValue("Configuration/MyCall","OE3SRO");
    source.setValue("Configuration/Rig","Yaesu FT-991");source.setValue("Configuration/EQSLPasswd","private");}
   pluto::prepareProfile(path,sourcePath,432500000,40002);
   QSettings profile(path,QSettings::IniFormat);
   check(profile.value("Configuration/MyCall")=="OE3SRO" && !profile.contains("Configuration/EQSLPasswd"),"station-only profile copy");
   check(profile.value("Configuration/PTTMethod").value<TransceiverFactory::PTTMethod>()==TransceiverFactory::PTT_method_CAT,"typed CAT PTT profile");
   check(profile.value("Configuration/SplitMode").value<TransceiverFactory::SplitMode>()==TransceiverFactory::split_mode_none,"typed simplex profile");
   check((profile.value("Configuration/Polling").toInt() & tci__audio)!=0,"TCI audio enabled in persisted rig parameters");
   profile.setValue("Common/TxFreq",1234);profile.sync();
   pluto::prepareProfile(path,sourcePath,432500000,40002);
   check(profile.value("Common/TxFreq")==1234,"existing Pluto profile preserved");
   profile.setValue("Common/DialFreq",0);profile.setValue("Configuration/Polling",1000);profile.sync();
   pluto::prepareProfile(path,sourcePath,432500000,40003);profile.sync();
   check(profile.value("Common/DialFreq").toULongLong()==432500000,"repair zero Pluto startup frequency");
   check(profile.value("Common/TxFreq")==1234 && profile.value("Configuration/CATTCIPort")=="127.0.0.1:40002","repair preserves audio offset and endpoint");
   check((profile.value("Configuration/Polling").toInt() & tci__audio)!=0,"repair legacy audio flag");
   profile.setValue("Common/DialFreq",432510000);profile.sync();
   pluto::prepareProfile(path,sourcePath,432500000,40003);profile.sync();
   check(profile.value("Common/DialFreq").toULongLong()==432510000,"preserve valid user dial frequency");
   // Run the real headless CLI in a fresh process. Registering only the two
   // Pluto enums used to destroy all other custom QVariant values on sync.
   auto fixture=profiles.filePath("registration.ini");
   {
     QSettings f(fixture,QSettings::IniFormat);
     f.setValue("Configuration/Rig","PlutoSDR (TCI bridge)");
     f.setValue("Common/DialFreq",0);
     f.setValue("Configuration/CATDataBits",QVariant::fromValue(TransceiverFactory::default_data_bits));
     f.setValue("Configuration/CATStopBits",QVariant::fromValue(TransceiverFactory::default_stop_bits));
     f.setValue("Configuration/CATHandshake",QVariant::fromValue(TransceiverFactory::handshake_hardware));
     f.setValue("Configuration/TXAudioSource",QVariant::fromValue(TransceiverFactory::TX_audio_source_rear));
     f.setValue("Configuration/BrokenValue",QVariant());
     f.setValue("Common/TestBandMemory",QVariantHash{{"70cm",42},{"2m",19}});
     f.sync();
   }
   QProcess prepare;
   auto env=QProcessEnvironment::systemEnvironment();env.insert("QT_QPA_PLATFORM","deliberately-unavailable");
   prepare.setProcessEnvironment(env);
   prepare.start(QDir(app.applicationDirPath()).filePath("wsjtx.exe"),{"--prepare-pluto-profile",fixture});
   check(prepare.waitForFinished(10000) && prepare.exitCode()==0,"production profile CLI is headless");
   auto fixtureReadback=profiles.filePath("registration-readback.ini");
   check(QFile::copy(fixture,fixtureReadback),"fresh profile readback");
   QSettings f(fixtureReadback,QSettings::IniFormat);
   check(f.value("Configuration/CATDataBits").value<TransceiverFactory::DataBits>()==TransceiverFactory::default_data_bits,"CLI preserves CAT data bits");
   check(f.value("Configuration/CATStopBits").value<TransceiverFactory::StopBits>()==TransceiverFactory::default_stop_bits,"CLI preserves CAT stop bits");
   check(f.value("Configuration/CATHandshake").value<TransceiverFactory::Handshake>()==TransceiverFactory::handshake_hardware,"CLI preserves CAT handshake");
   check(f.value("Configuration/TXAudioSource").value<TransceiverFactory::TXAudioSource>()==TransceiverFactory::TX_audio_source_rear,"CLI preserves TX audio source");
   check(f.value("Common/TestBandMemory").toHash()==QVariantHash{{"70cm",42},{"2m",19}},"CLI preserves band memory");
   check(!f.contains("Configuration/BrokenValue") && QFileInfo::exists(fixture+".before-invalid-repair"),"invalid profile values backed up and removed");
#ifdef Q_OS_WIN
   struct FakeState {int txPowerdown,rxPowerdown,contexts,buffers,pushes,failPush;long long rxFrequency,txFrequency,rate;int ensm,failStateReadback;
     unsigned rxKernel,txKernel;int txSamples,shortPush,underflows;long long nowUs,queuedUntilUs;};
   auto fakePath=QDir(app.applicationDirPath()).filePath("libpluto_fake_iio.dll");
   QLibrary fake(fakePath);
   check(fake.load(),"load IIO test double");
   auto fakeState=reinterpret_cast<FakeState*(*)()>(fake.resolve("fake_iio_state"));
   check(fakeState!=nullptr,"fake IIO state accessor");
   auto status=fakeState();
   auto txSample=reinterpret_cast<int(*)(unsigned)>(fake.resolve("fake_iio_tx_sample"));
   check(txSample!=nullptr,"fake IIO transmitted sample accessor");
   check(status->ensm==0,"regression starts with Pluto asleep");
   {
     pluto::IioRadio radio(fakePath,"fake:",432500000,30,-40);
     check(status->ensm==2 && status->txPowerdown==1 && status->contexts==1 && status->buffers==1,"RX startup wakes sleeping Pluto and keeps TX LO muted");
     radio.ptt(true);
     check(status->txPowerdown==0 && status->pushes==1 && status->buffers==2,"zero buffer primed before PTT");
     check(status->rxKernel==2 && status->txKernel==4 && status->txSamples==4*pluto::rf_block,"RX 10-ms buffers / TX four 40-ms buffers");
     auto encode=[](float value) {return int(int16_t(pluto::SampleFormat{12,0}.encode(value*.65f)));};
     for(int block=0;block<4;++block) {
       radio.transmit(std::vector<pluto::IQ>(pluto::rf_block,{.1f*(block+1),-.1f*(block+1)}));
       check(status->pushes==(block==3?2:1),"four consecutive audio chunks per IIO transfer");
     }
     for(int block=0;block<4;++block) for(int n:{0,pluto::rf_block-1}) {
       unsigned at=2*(block*pluto::rf_block+n);
       check(txSample(at)==encode(.1f*(block+1)) && txSample(at+1)==encode(-.1f*(block+1)),"TX sample order and I/Q across batch boundaries");
     }
     // A complete frame with occasional 25-ms producer stalls. A pair of
     // 10-ms buffers cannot cover these pauses; the new DMA queue must.
     auto before=status->pushes;
     auto end=status->queuedUntilUs;
     for(int block=0;block<1200;++block) {
       status->nowUs+=2000+(block%67==0?25000:0);
       radio.transmit(std::vector<pluto::IQ>(pluto::rf_block));
     }
     check(status->underflows==0 && status->queuedUntilUs-end==12000000 && status->pushes-before==300,"continuous DAC timeline under simulated 25-ms client/USB stalls");
     radio.transmit(std::vector<pluto::IQ>(pluto::rf_block,{.7f,0}));
     radio.ptt(false);
     check(status->txPowerdown==1 && status->buffers==1,"PTT off mutes and destroys TX buffer");
     radio.ptt(true);
     for(int block=0;block<4;++block) radio.transmit(std::vector<pluto::IQ>(pluto::rf_block));
     check(txSample(0)==0 && txSample(2*(4*pluto::rf_block-1))==0,"partial TX batch discarded on unkey, no stale samples on next PTT");
     radio.ptt(false);
   }
   check(status->ensm==1 && status->txPowerdown==1 && status->contexts==0 && status->buffers==0,"IIO clean shutdown leaves muted ALERT state");
   status->failStateReadback=1;
   bool stateFault=false;
   try {pluto::IioRadio radio(fakePath,"fake:",432500000,30,-40);}
   catch(std::exception const&) {stateFault=true;}
   check(stateFault && status->txPowerdown==1 && status->contexts==0 && status->buffers==0,"RF state readback failure aborts startup");
   status->failStateReadback=0;
   bool faulted=false;
   try {
     pluto::IioRadio radio(fakePath,"fake:",432500000,30,-40);
     radio.ptt(true);status->failPush=1;
     for(int block=0;block<4;++block) radio.transmit(std::vector<pluto::IQ>(pluto::rf_block));
   } catch(std::exception const&) {faulted=true;}
   check(faulted && status->txPowerdown==1 && status->contexts==0 && status->buffers==0,"IIO push failure mutes and releases resources");
   faulted=false;
   try {
     pluto::IioRadio radio(fakePath,"fake:",432500000,30,-40);
     radio.ptt(true);status->shortPush=1;
     for(int block=0;block<4;++block) radio.transmit(std::vector<pluto::IQ>(pluto::rf_block));
   } catch(std::exception const&) {faulted=true;}
   check(faulted && status->txPowerdown==1 && status->contexts==0 && status->buffers==0,"short IIO transfer aborts and mutes instead of silently corrupting timing");
#endif
   for(unsigned bits:{8u,12u,16u}) for(unsigned shift:{0u,16u-bits}) {
     pluto::SampleFormat format{bits,shift};
     for(float v:{-1.f,-.4f,0.f,.4f,1.f})
       check(std::abs(format.decode(format.encode(v))-v)<2.f/(1<<(bits-1)),"signed IIO format alignment");
   }
   auto started=std::chrono::steady_clock::now();
   reist16::Bits bits{};
   for(int k=0;k<77;++k) bits[k]=(k*19+7)%3==0;
   auto wave=reist16::waveform(bits,392,1500.375);
   wave.resize(wave.size()+4800,0);
   pluto::UsbModulator tx;
   pluto::UsbDemodulator rx;
   std::vector<float> decoded(6000,0);
   for(size_t at=0;at<wave.size();at+=480) {
     auto iq=tx.process(wave.data()+at,480);
     auto audio=rx.process(iq.data(),int(iq.size()));
     check(audio.size()==480,"resampler frame count");
     for(size_t k=0;k<audio.size();k+=4) decoded.push_back(audio[k]);
   }
   decoded.resize(180000,0);
   auto found=reist16::decode(decoded.data(),int(decoded.size()),100,4000,3);
   check(found.size()==1 && found[0].bits==bits && found[0].sequence==392,"REIST16 through USB IQ / 3.072 MHz / audio");
   check(std::abs(found[0].dt)<.02,"USB filter delay within 20 ms");
   std::cout<<"USB IQ modem round trip: "<<std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count()<<" s for 9.7 s audio\n";
   // Opposite sideband rejection after filter settling.
   for(int frequency:{300,1500,4000}) for(int sign:{-1,1}) {
     pluto::UsbDemodulator usb;
     double sum=0;
     for(int block=0;block<12;++block) {
       std::vector<pluto::IQ> iq(pluto::rf_block);
       for(int k=0;k<pluto::rf_block;++k) {
         double phase=2*pluto::pi*frequency*(block*pluto::rf_block+k)/pluto::rf_rate;
         iq[k]={float(std::cos(phase)),float(sign*std::sin(phase))};
       }
       auto audio=usb.process(iq.data(),int(iq.size()));
       if(block>=6) for(auto v:audio) sum+=v*v;
     }
     check(sign>0?sum>1000:sum<.02,"USB selected and lower sideband suppressed");
   }
   auto packet=pluto::audioPacket(2,std::vector<float>(480,.1f));
   std::vector<float> parsed;
   check(pluto::readAudioPacket(packet,parsed) && parsed.size()==480,"TCI audio wire format");
   check(!pluto::readAudioPacket(packet.left(65),parsed),"short TCI audio rejected");
   float nan=std::numeric_limits<float>::quiet_NaN();
   std::memcpy(packet.data()+64,&nan,4);
   check(!pluto::readAudioPacket(packet,parsed),"invalid TCI sample rejected");

   Transceiver::logger_type logger(boost::log::keywords::channel="Pluto TCI test");
   {TCITransceiver isolated(&logger,{},"0","127.0.0.1:40002",true,tci__audio);
    PlutoTciChecks::pcm(isolated,QVector<float>(wave.data(),wave.data()+460800));}
   QTcpServer portProbe;
   check(portProbe.listen(QHostAddress::LocalHost,0),"allocate test port");
   auto port=portProbe.serverPort();portProbe.close();
   pluto::Options options;options.simulate=true;options.port=port;
   pluto::Bridge bridge(options);bridge.start();
   // Allow the worker's queued ready event to bind the loopback listener.
   QElapsedTimer pause;pause.start();waitUntil([&]{return pause.elapsed()>30;});
   TCITransceiver rig(&logger,{},"0",QString("127.0.0.1:%1").arg(port),true,tci__audio);
   QString failure;
   QObject::connect(&rig,&Transceiver::failure,[&](QString e){failure=e;});
   rig.start(0);
   check(failure.isEmpty() && rig.state().online(),"actual WSJT-X TCI handshake with Pluto bridge");
   PlutoTciChecks::handshake(rig);
   auto state=rig.state();
   state.audio(true);state.period(15.);state.blocksize(3456);state.volume(0);state.txvolume(0);
   rig.set(state,1);
   state.frequency(432510000);state.mode(Transceiver::USB);
   rig.set(state,2);
   check(failure.isEmpty() && rig.state().frequency()==432510000,"Pluto CAT retune readback");
   int frames=0;
   QObject::connect(&rig,&Transceiver::tciframeswritten,[&](qint64){++frames;});
   waitUntil([&]{return frames>0;});
   state.ptt(true);rig.set(state,3);
   check(failure.isEmpty() && rig.state().ptt(),"Pluto CAT PTT on");
   state.ptt(false);rig.set(state,4);
   check(failure.isEmpty() && !rig.state().ptt(),"Pluto CAT Halt TX");
   PlutoTciChecks::abort(rig);
   rig.stop();
   // Exercise the production topology: separate bridge process and CAT thread.
   check(portProbe.listen(QHostAddress::LocalHost,0),"allocate external bridge port");
   auto externalPort=portProbe.serverPort();portProbe.close();
   QProcess service;
   auto readyPath=profiles.filePath("bridge-ready.json"),stopPath=profiles.filePath("bridge-stop");
   bool hardwareRx=app.arguments().contains("--pluto-hardware-rx");
   QStringList serviceArgs={"--port",QString::number(externalPort),"--ready-file",readyPath,"--stop-file",stopPath};
   if(hardwareRx) serviceArgs<<"--library"<<QDir::current().filePath(".tools/pluto/runtime/libiio.dll");
   else serviceArgs<<"--simulate"<<"--uri"<<"thread-test:";
   service.start(QDir(app.applicationDirPath()).filePath("wsjtx_pluto_bridge.exe"),serviceArgs);
   check(service.waitForStarted(),"separate bridge process starts");
   waitUntil([&]{QFile f(readyPath);return f.open(QIODevice::ReadOnly) && QJsonDocument::fromJson(f.readAll()).object().value("ok").toBool();});
   QThread catThread;
   auto threaded=new TCITransceiver(&logger,{},"0",QString("127.0.0.1:%1").arg(externalPort),true,1048675);
   threaded->moveToThread(&catThread);
   QObject::connect(&catThread,&QThread::finished,threaded,&QObject::deleteLater);
   QString threadFailure;bool online=false;int threadFrames=0;
   Transceiver::TransceiverState threadState;
   QObject::connect(threaded,&Transceiver::failure,&app,[&](QString e){threadFailure=e;});
   QObject::connect(threaded,&Transceiver::update,&app,[&](Transceiver::TransceiverState s,unsigned){online=s.online();threadState=s;});
   QObject::connect(threaded,&Transceiver::tciframeswritten,&app,[&](qint64){++threadFrames;});
   catThread.start();
   QMetaObject::invokeMethod(threaded,[=]{threaded->start(0);},Qt::QueuedConnection);
   waitUntil([&]{return online || !threadFailure.isEmpty();},7000);
   if(online) {
     threadState.audio(true);threadState.period(15.);threadState.blocksize(3456);
     QMetaObject::invokeMethod(threaded,[=]{threaded->set(threadState,1);},Qt::QueuedConnection);
     waitUntil([&]{return threadFrames>0 || !threadFailure.isEmpty();});
   }
   QMetaObject::invokeMethod(threaded,[=]{threaded->stop();},Qt::BlockingQueuedConnection);
   catThread.quit();catThread.wait();
   {QFile f(stopPath);check(f.open(QIODevice::WriteOnly),"write bridge stop");f.write("stop");}
   check(service.waitForFinished(5000) && service.exitCode()==0,"separate bridge stops cleanly");
   if(!threadFailure.isEmpty()) throw std::runtime_error(threadFailure.toStdString());
   check(online,"separate process / CAT thread handshake");
   check(threadFrames>0,"separate process / CAT thread receives audio");
   if(hardwareRx) std::cout<<"PASS real Pluto CAT thread: startup, frequency readback and RX audio; no hardware PTT requested.\n";
   std::cout<<"PASS Pluto: IIO formats, USB sideband rejection, modem round trip, TCI PCM snapshot, real driver CAT/PTT/RX against simulated bridge.\n";
 } catch(std::exception const& e) {std::cerr<<e.what()<<'\n';return 1;}
}
