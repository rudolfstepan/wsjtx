#include "lib/reist16/qso2.hpp"
#include "lib/ft8/native/ft8_wave.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
extern "C" void genft8_(char*,int*,int*,char*,char*,int*,size_t,size_t);
namespace {
void require(bool value,char const* why){if(!value)throw std::runtime_error(why);}
void wav(std::string const& path,std::vector<float>& data) {
  std::ofstream f(path,std::ios::binary);
  auto word=[&](unsigned x,int n){for(int i=0;i<n;++i)f.put(char(x>>(8*i)));};
  f.write("RIFF",4);word(36+2*data.size(),4);f.write("WAVEfmt ",8);word(16,4);
  word(1,2);word(1,2);word(12000,4);word(24000,4);word(2,2);word(16,2);
  f.write("data",4);word(2*data.size(),4);
  for(float& value:data) {
    require(std::abs(value)<1,"clipping");int16_t pcm=int16_t(std::lround(value*32767));
    word(unsigned(pcm),2);value=pcm/32768.f;
  }
}
std::string call(std::mt19937& rng,char const* prefix) {
  std::string text=prefix;text+=char('1'+rng()%9);
  for(int i=0;i<3;++i)text+=char('A'+rng()%26);
  return text;
}
void sync_reference_check() {
  int const positions[]={0,1,2,3,4,5,6,29,30,31,32,33,34,35,58,59,60,61,62,63};
  std::mt19937 rng(2026102641);
  for(int trial=0;trial<4;++trial) {
    reist16::Bits bits;for(auto& b:bits)b=rng()&1;
    auto tones=reist16::qso2::encode_tones(bits);int known[64]={};
    for(int s:positions)known[s]=tones[s];
    std::vector<std::complex<float>> actual(153600),reference(153600);
    ft8_native::waveform(tones.data(),64,2400,4,12000,1500.375f,actual.data(),nullptr,true,153600);
    ft8_native::waveform(known,64,2400,4,12000,1500.375f,reference.data(),nullptr,true,153600);
    for(int k=0;k<20;++k) {
      int first=(k==0||k==7||k==14)?1200:0;
      int last=(k==6||k==13||k==19)?1200:2400;
      for(int i=first;i<last;++i)
        require(std::abs(actual[positions[k]*2400+i]-reference[positions[k]*2400+i])<1e-4,
                "sync template must be independent of unknown data");
    }
  }
  std::cout<<"PASS exact Gaussian sync reference is independent of unknown payload symbols\n";
}
void ap_message_guards() {
  std::mt19937 rng(2026102643);std::normal_distribution<float> normal;
  for(int snr:{-18,-24})for(std::string message:{"CQ TEST DL1ABC JO31","CQ RST DL2ABC JN88",
      "DL3ABC OE1XYZ R-12","CQ DL4ABC/P JO31"}) {
    message.resize(37,' ');char sent[37],packed[77];int ft8_tones[79],i3=0,n3=0;
    genft8_(&message[0],&i3,&n3,sent,packed,ft8_tones,37,37);
    require(i3==1||i3==2,"AP guard standard message");
    reist16::Bits bits;std::copy(packed,packed+77,bits.begin());auto tones=reist16::qso2::encode_tones(bits);
    std::vector<std::complex<float>> wave(153600);
    ft8_native::waveform(tones.data(),64,2400,4,12000,1500.375f,wave.data(),nullptr,true,153600);
    double sigma=std::sqrt(.32*.0001*6000/2500/std::pow(10.,snr/10.));
    std::vector<float> audio(180000);for(auto& x:audio)x=sigma*normal(rng);
    for(size_t i=0;i<wave.size();++i)audio[6000+i]+=.008*std::imag(std::complex<double>(wave[i])*std::polar(1.,.731));
    auto found=reist16::qso2::decode(audio.data(),180000,100,4000,3,true);
    for(auto const& d:found)require(d.bits==bits,"CQ prior must not rewrite CQ modifiers, /P or QSO payloads");
    if(snr==-18)require(found.size()==1,"AP guard strong frame preserved");
  }
  std::cout<<"PASS CQ TEST/RST, portable CQ and QSO guard frames, no forced-CQ substitutions\n";
}
}
int main(int argc,char** argv) {
 try {
  sync_reference_check();
  if(argc>1&&std::string(argv[1])=="--unit"){ap_message_guards();return 0;}
  unsigned seed=argc>1?unsigned(std::stoul(argv[1])):2026102627u;
  int trials=argc>2?std::stoi(argv[2]):16;require(trials>=2&&trials<=100,"trials");
  bool generate_only=argc>3&&std::string(argv[3])=="--generate-only";
  std::mt19937 rng(seed);std::normal_distribution<float> normal;
  std::ofstream report("paired.jsonl");int false_count=0;
  for(int snr:{-20,-22,-24,-26}) {
    int off=0,on=0;
    for(int trial=0;trial<trials;++trial) {
      bool cq=trial%2==0;std::string sender=call(rng,"DL"),recipient=call(rng,"OE");
      if(trial%7==6)sender+="/P";
      char const* endings[]={"JO31","-12","R-10","RR73","73"};
      std::string message=cq?"CQ "+sender+" JO31":recipient+" "+sender+" "+endings[(trial/2)%5];
      std::string padded=message;padded.resize(37,' ');
      char sent[37],packed[77];int ft8_tones[79],i3=0,n3=0;
      genft8_(&padded[0],&i3,&n3,sent,packed,ft8_tones,37,37);
      require((i3==1||i3==2)&&std::equal(sent,sent+37,padded.begin()),"standard CQ/QSO pack77");
      reist16::Bits bits;std::copy(packed,packed+77,bits.begin());
      auto tones=reist16::qso2::encode_tones(bits);
      double frequency=400.+rng()%2800+double(rng()%1000)/1000;
      double phase=6.283185307179586*rng()/double(rng.max());
      double drift=trial%4==3?.02:0.;int start=4200+int(rng()%5401);
      double gain=.01,sigma=std::sqrt(.32*gain*gain*6000/2500/std::pow(10.,snr/10.));
      std::vector<float> noise(180000);for(auto& v:noise)v=sigma*normal(rng);
      auto reist=noise,ft8=noise;
      for(int mode=0;mode<2;++mode) {
        int symbols=mode?79:64,nsps=mode?1920:2400;
        std::vector<std::complex<float>> wave(symbols*nsps);
        ft8_native::waveform(mode?ft8_tones:tones.data(),symbols,nsps,mode?2:4,12000,
            float(frequency),wave.data(),nullptr,true,int(wave.size()));
        auto& audio=mode?ft8:reist;
        for(size_t i=0;i<wave.size();++i) {
          double t=i/12000.;auto rotation=std::polar(1.,phase+3.141592653589793*drift*t*t);
          audio[start+i]+=.8*gain*std::imag(std::complex<double>(wave[i])*rotation);
        }
      }
      std::string id=std::to_string(-snr)+"_"+std::to_string(trial)+"_000000.wav";
      wav("reist_"+id,reist);wav("ft8_"+id,ft8);
      bool exact[2]={};double elapsed[2]={};
      if(!generate_only)for(int ap=0;ap<2;++ap) {
        auto before=std::chrono::steady_clock::now();
        auto found=reist16::qso2::decode(reist.data(),180000,100,4000,3,ap!=0);
        elapsed[ap]=std::chrono::duration<double>(std::chrono::steady_clock::now()-before).count();
        for(auto const& d:found){if(d.bits==bits)exact[ap]=true;else ++false_count;}
      }
      off+=exact[0];on+=exact[1];
      report<<"{\"seed\":"<<seed<<",\"trial\":"<<trial<<",\"snr2500\":"<<snr
        <<",\"native_evaluated\":"<<(!generate_only)<<",\"kind\":\""<<(cq?"CQ":"QSO")<<"\",\"message\":\""<<message
        <<"\",\"frequency\":"<<frequency<<",\"start_sample\":"<<start<<",\"phase\":"<<phase
        <<",\"drift_hz_s\":"<<drift<<",\"ap_off\":"<<exact[0]<<",\"ap_on\":"<<exact[1]
        <<",\"seconds_off\":"<<elapsed[0]<<",\"seconds_on\":"<<elapsed[1]
        <<",\"reist_wave\":\"reist_"<<id<<"\",\"ft8_wave\":\"ft8_"<<id<<"\"}\n";
      report.flush();
    }
    if(generate_only)std::cout<<"Generated SNR "<<snr<<": "<<trials<<" paired inputs"<<std::endl;
    else std::cout<<"SNR "<<snr<<" AP off "<<off<<'/'<<trials<<" AP on "<<on<<'/'<<trials<<std::endl;
  }
  require(false_count==0,"foreign weak-signal payload");
  std::cout<<"PASS mixed CQ/QSO payloads, independent random phase/time/frequency, mild drift, no foreign bits\n";
 }catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}
}
