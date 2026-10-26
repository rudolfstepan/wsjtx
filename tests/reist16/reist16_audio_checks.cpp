#include "lib/reist16/qso2.hpp"
#include "lib/reist16/qso_address.hpp"
#include "lib/ft8/native/ft8_wave.hpp"
#include "Modulator/Modulator.hpp"
#include "Audio/soundout.h"
#include "Decoder/decodedtext.h"
#include "models/Modes.hpp"
#include "helper_functions.h"
#include "wsjtx_config.h"
#include "commons.h"
#include "Logger.hpp"
#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QElapsedTimer>
#include <QProcess>
#include <QSharedMemory>
#include <QStringList>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>

BOOST_LOG_GLOBAL_LOGGER_CTOR_ARGS(sys,
  boost::log::sources::severity_channel_logger_mt<boost::log::trivial::severity_level>,
  (boost::log::keywords::channel = "REIST16 test"));
extern "C" {
  decltype(foxcom_) foxcom_;
  void gen_reist16_(char*,char*,float*,int*,float*,int*,fortran_charlen_t,fortran_charlen_t);
  void genft8_(char*,int*,int*,char*,char*,int*,fortran_charlen_t,fortran_charlen_t);
}
int volatile itone[250]={};
int volatile icw[250]={};
float gran() { return 0.f; } // Synthetic-noise output is disabled in this test.
class MemoryOutput final : public SoundOutput {
public:
  void restart(QIODevice*) override {}
  void stop() override {}
  void reset() override {}
};
class TestModulator final : public Modulator {
public:
  TestModulator() : Modulator(48000,15.) {}
  qint64 now=0;
protected:
  qint64 millisecondsSinceEpoch() const override { return now; }
};
void check(bool ok,char const* message) {if(!ok) throw std::runtime_error(message);}

int main(int argc,char**argv) {
  QCoreApplication app(argc,argv);
  try {
    check(Modes::FT2==13 && Modes::FT8==8 && Modes::Q65==12,"persisted mode IDs");
    check(Modes::value("REIST16")==Modes::REIST16,"mode lookup");
    check(QString(Modes::name(Modes::REIST16))=="REIST16","mode name");
    for(unsigned start=0;start<86400;start+=15)
      check(received_in_second_period(start,15.)==bool((start/15)%2),"UTC alternating slots");
    check(tx_duration("REIST16",15,6912,false)>=14.1 && tx_duration("REIST16",15,6912,false)<15,"complete late frame fits TX window");

    // Real packer -> encoder -> production audio thread interface -> decoder.
    // No SoundOutput device is opened and no MainWindow/CAT is instantiated.
    QByteArray message=QByteArray("CQ OE3SRO JN87").leftJustified(37,' '), sent(37,' ');
    QVector<float> wave(reist16::qso2::tx_samples);
    float frequency=1500.375f;
    int sequence=731,ok=0;
    gen_reist16_(message.data(),sent.data(),&frequency,&sequence,wave.data(),&ok,37,37);
    check(ok==1 && sent==message,"Fortran QSO encoder");
    // Compare actual PCM after the common modulator, including lower audio
    // frequencies. This catches both hidden gain factors and int16 overflow.
    char packed[77]; int ft8Tones[79],i3=0,n3=0;
    genft8_(message.data(),&i3,&n3,sent.data(),packed,ft8Tones,37,37);
    for(float base : {500.f,1000.f,1500.375f,2000.f}) {
      double rms[2]={};
      for(int mode=0;mode<2;++mode) {
        int length=mode?79*7680:reist16::qso2::tx_samples;
        QVector<float> frame(length);
        if(mode) ft8_native::waveform(ft8Tones,79,7680,2,48000,base,nullptr,frame.data(),false,length);
        else gen_reist16_(message.data(),sent.data(),&base,&sequence,frame.data(),&ok,37,37);
        check(mode||ok==1,"lower-tone production encoder");
        double peak=0;
        for(float v:frame) {check(std::isfinite(v)&&std::abs(v)<=1,"bounded TX waveform");peak=std::max(peak,double(std::abs(v)));}
        check(peak>.999,"TX uses full digital peak range");
        MemoryOutput output; TestModulator modulator; modulator.now=500;
        modulator.setWaveform(frame);
        modulator.start(mode?"FT8":"REIST16",mode?79:64,mode?1920.:2400.,base,
                        mode?-3.:-4.,&output,AudioDevice::Mono,true,false,99.,15.);
        QByteArray pcm;while(true){auto block=modulator.read(4096);if(block.isEmpty())break;pcm+=block;}
        check(pcm.size()==2*length,"TX PCM exact length");
        auto samples=reinterpret_cast<int16_t const*>(pcm.constData());
        double power=0;
        // Ignore the different endpoint ramps; compare the active envelope.
        for(int i=4800;i<length-4800;++i) {
          int expected=int(std::lround(32767.*frame[i]));
          check(std::abs(int(samples[i])-expected)<=1,"TX PCM preserves waveform without wrap or clipping");
          power+=double(samples[i])*samples[i];
        }
        rms[mode]=std::sqrt(power/(length-9600))/32767.;
      }
      double difference=20*std::log10(rms[0]/rms[1]);
      check(std::abs(difference)<.01,"REIST16/FT8 active PCM RMS parity");
      std::cout<<"TX "<<base<<" Hz REIST16/FT8 RMS "<<rms[0]<<" / "<<rms[1]
               <<" delta_dB "<<difference<<std::endl;
    }
    for(qint64 now : {0LL,300LL,500LL,800LL,1000LL,15000LL,30500LL}) {
      MemoryOutput output;
      TestModulator modulator;
      modulator.now=now;
      modulator.setWaveform(wave);
      icw[0]=12; // stale CW ID must never extend the immutable QSO frame
      modulator.start("REIST16",64,2400.,frequency,-4.,&output,AudioDevice::Mono,true,false,99.,15.);
      QByteArray pcm;
      while(pcm.size()<1500000) {
        auto block=modulator.read(4096);
        if(block.isEmpty()) break;
        pcm+=block;
      }
      int intoSlot=int(now%15000);
      int silence=std::max(0,500-intoSlot)*48;
      check(pcm.size()==2*(silence+reist16::qso2::tx_samples),"complete PCM frame and sync delay");
      auto samples=reinterpret_cast<int16_t const*>(pcm.constData());
      std::vector<float> rx(180000,0);
      for(int i=0;i<pcm.size()/2;i+=4) rx[intoSlot*12+i/4]=samples[i]/32768.f;
      auto result=reist16::qso2::decode(rx.data(),int(rx.size()),100,4000,2);
      check(result.size()==1 && result[0].sequence==0,"production audio round trip");
      check(std::abs(result[0].dt-(std::max(500,intoSlot)/1000.-.5))<.02,"audio UTC alignment");
      check(!modulator.isActive(),"audio exact stop");
    }

    // The reported CQ TEST case and non-CQ messages have the exact same
    // immutable frame length and must pass the real modulator/decoder path.
    for (QByteArray text : {QByteArray("CQ TEST OE3SRO JN88"), QByteArray("CQ DX OE3SRO JN88"),
         QByteArray("OE3SRO DL1ABC -10"), QByteArray("DL1ABC OE3SRO RR73")}) {
      auto padded=text.leftJustified(37,' '); QByteArray rendered(37,' ');
      QVector<float> frame(reist16::qso2::tx_samples); int encoded=0;
      gen_reist16_(padded.data(),rendered.data(),&frequency,&sequence,frame.data(),&encoded,37,37);
      check(encoded==1&&rendered==padded,"CQ TEST/non-CQ message preserved");
      MemoryOutput output; TestModulator modulator; modulator.now=500;
      modulator.setWaveform(frame);
      modulator.start("REIST16",64,2400.,frequency,-4.,&output,AudioDevice::Mono,true,false,99.,15.);
      QByteArray pcm; while(true){auto block=modulator.read(4096);if(block.isEmpty())break;pcm+=block;}
      check(pcm.size()==2*reist16::qso2::tx_samples,"Message text must not change frame duration");
      std::vector<float> rx(180000,0);auto samples=reinterpret_cast<int16_t const*>(pcm.constData());
      for(int i=0;i<pcm.size()/2;i+=4)rx[6000+i/4]=samples[i]/32768.f;
      auto decoded=reist16::qso2::decode(rx.data(),rx.size(),100,4000,2);
      check(decoded.size()==1&&!modulator.isActive(),"CQ TEST/non-CQ production PCM must decode and finish");
    }

    // Read actual jt9 stdout from the script, including six standard QSO stages.
    check(argc==2,"provide jt9 decoder output file");
    QFile file(QString::fromLocal8Bit(argv[1]));
    check(file.open(QIODevice::ReadOnly),"read decoder output");
    int count=0;
    for(auto line : QString::fromUtf8(file.readAll()).split('\n')) {
      if(!line.contains(" % ")) continue;
      DecodedText decoded(line.trimmed());
      check(decoded.isStandardMessage(),"standard decoded message");
      check(decoded.frequencyOffset()==300+500*count,"display frequency column");
      check(decoded.timeInSeconds()==0,"display UTC column");
      check(decoded.snr()>0,"display SNR column");
      check(decoded.messageWords().size()>=4,"QSO token parsing");
      if(count==0) check(decoded.CQersCall()=="OE3SRO","CQ call parsing");
      ++count;
    }
    check(count==6,"all six decoded QSO stages");
    // Same base call, different explicitly encoded station identities.
    // This used to make Pluto OE3SRO/P silently reject a clicked OE3SRO CQ.
    for(QString suffix:{"/P","/R"}) {
      QString portable="OE3SRO"+suffix;
      check(!reist16::sameStation("OE3SRO",portable),"full partner comparison replaces saved portable self DX call with base sender");
      check(!reist16::sameStation(portable,"OE3SRO"),"full partner comparison replaces saved base self DX call with portable sender");
      check(reist16::sameStation("<"+portable.toLower()+">",portable),"partner comparison ignores brackets and case, preserves suffix");
      auto parse=[](QString body) {return DecodedText("000000 -10  0.0 1500 %  "+body);};
      auto cq=parse("CQ OE3SRO JN88");
      check(cq.isStandardMessage(),"portable QSO CQ is standard");
      auto selected=reist16::qsoAddress(cq,portable);
      check(!selected.fromUs && !selected.toUs,"base-call CQ belongs to other endpoint, not an addressed reply");
      auto own=parse("CQ "+portable+" JN88");
      check(own.isStandardMessage() && reist16::qsoAddress(own,portable).fromUs,"own complete callsign remains rejected");
      for(QString payload:{"JN88","-10","R-08","RR73","73"}) {
        auto reply=parse(portable+" OE3SRO "+payload);
        check(reply.isStandardMessage(),"portable QSO response is standard");
        auto address=reist16::qsoAddress(reply,portable);
        check(address.toUs && !address.fromUs,"portable endpoint receives every QSO response");
        check(!reist16::qsoAddress(reply,"OE3SRO").toUs,"base endpoint cannot consume portable-addressed response");
        auto inverse=parse("OE3SRO "+portable+" "+payload);
        check(inverse.isStandardMessage(),"inverse portable QSO response is standard");
        address=reist16::qsoAddress(inverse,"OE3SRO");
        check(address.toUs && !address.fromUs,"base endpoint receives every portable response");
      }
    }

    // Exercise the exact shared-memory worker path used by MainWindow, with
    // the live capture length. Include frames delayed past the old 13.824-s
    // cutoff, an empty slot, and a new nominal frame after those delayed ones.
    QSharedMemory memory(QString("reist16-check-%1").arg(app.applicationPid()));
    check(memory.create(sizeof(dec_data_t)),"create decoder shared memory");
    check(memory.lock(),"lock decoder shared memory");
    auto shared=static_cast<dec_data_t*>(memory.data());
    std::memset(shared,0,sizeof(*shared));
    shared->params.nmode=116;
    shared->params.ntxmode=116;
    shared->params.ntrperiod=15;
    shared->params.nfa=100;
    shared->params.nfb=4000;
    shared->params.ndepth=3;
    shared->params.kin=reist16::qso2::rx_capture_samples;
    shared->params.nzhsym=reist16::qso2::rx_half_symbols;
    shared->params.newdat=true;
    memory.unlock();
    QProcess worker;
    worker.start(QDir(app.applicationDirPath()).filePath("jt9"),
                 {"-s",memory.key(),"-a",QDir::currentPath(),"-t",QDir::currentPath()});
    check(worker.waitForStarted(5000),"start shared-memory jt9");
    int const starts[] = {6000,-1,13200,15600,18000,6000};
    for(int slot=0;slot<6;++slot) {
      check(memory.lock(),"lock next slot");
      bool monitorOnly=slot>=2&&slot<=4;
      int captureHalfSymbols=reist16::qso2::capture_half_symbols(monitorOnly);
      int captureSamples=captureHalfSymbols*reist16::qso2::rx_block_samples;
      shared->params.kin=captureSamples;
      shared->params.lft8apon=slot%2==0; // Exercise both REIST16 AP states through IPC.
      shared->params.nzhsym=captureHalfSymbols;
      std::fill(shared->d2,shared->d2+180000,0);
      if(starts[slot]>=0) {
        check(starts[slot]+wave.size()/4<=captureSamples,"delayed frame fits live capture");
        for(int i=0;i<wave.size()/4;++i) shared->d2[starts[slot]+i]=int16_t(wave[4*i]*6000);
      }
      shared->params.nutc=slot*15;
      shared->ipc[0]=captureHalfSymbols;
      shared->ipc[1]=1;
      memory.unlock();
      QElapsedTimer timer;
      timer.start();
      QByteArray answer;
      while(!answer.contains("<DecodeFinished>") && timer.elapsed()<10000) {
        worker.waitForReadyRead(100);
        answer+=worker.readAllStandardOutput();
      }
      check(answer.contains("<DecodeFinished>"),"shared-memory decode completed");
      check(answer.contains("CQ OE3SRO JN87")==bool(slot!=1),"live capture / empty slot / repeated capture");
      std::cout<<"Live IPC slot "<<slot<<", start "<<starts[slot]/12000.
               <<" s, capture "<<captureSamples/12000.<<" s, depth 3, worker "<<timer.elapsed()<<" ms"<<std::endl;
      check(memory.lock(),"acknowledge decode");
      shared->ipc[2]=1;
      memory.unlock();
    }
    check(memory.lock(),"terminate test worker");
    shared->ipc[1]=999;
    memory.unlock();
    check(worker.waitForFinished(5000) && worker.exitCode()==0,"worker shutdown");
    std::cout<<"PASS REIST16: mode IDs, UTC parity, 7 production PCM round trips, QSO parser, live IPC worker lifecycle.\n";
  } catch(std::exception const& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
