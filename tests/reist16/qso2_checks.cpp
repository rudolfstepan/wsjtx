#include "lib/reist16/qso2.hpp"
#include "lib/ft8/native/ft8_wave.hpp"
#include <algorithm>
#include <limits>
#include "lib/ft8/native/ft8_ldpc.hpp"
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
extern "C" void genft8_(char*,int*,int*,char*,char*,int*,size_t,size_t);
static void check(bool ok,char const* text){if(!ok)throw std::runtime_error(text);}
static void wav(std::string const& name,std::vector<float> const& audio) {
 std::ofstream f(name,std::ios::binary);
 auto word=[&](unsigned x,int n){for(int i=0;i<n;++i)f.put(char(x>>(8*i)));};
 f.write("RIFF",4);word(36+2*audio.size(),4);f.write("WAVEfmt ",8);word(16,4);word(1,2);word(1,2);
 word(12000,4);word(24000,4);word(2,2);word(16,2);f.write("data",4);word(2*audio.size(),4);
 for(float x:audio){check(std::abs(x)<1,"no clipping");word(unsigned(int16_t(std::lround(x*32768))),2);}
}

static void unit_tests() {
 std::mt19937 rng(2026102208);
 for(int test=0;test<80;++test) {
  reist16::Bits bits{};
  if(test<77)bits[test]=1;else for(auto& b:bits)b=rng()&1;
  int8_t code[174];ft8_native::encode(bits.data(),code);
  auto tones=reist16::qso2::encode_tones(bits);
  check(tones.size()==64,"64 symbols");
  int at=0;
  for(int s=0;s<64;++s)if((s>=7&&s<29)||(s>=36&&s<58)) {
   int word=0;for(int g=tones[s];g;g>>=1)word^=g;
   for(int b=0;b<4;++b,++at)
    check(((word>>b)&1)==(at<174?code[(43*at)%174]:0),"independent wire mapping");
  }
 }
 reist16::Bits bits{};for(auto& b:bits)b=rng()&1;
 bits[74]=0;bits[75]=0;bits[76]=1;
 for(double frequency:{100.,1500.375,4000.}) {
  auto wave=reist16::qso2::waveform(bits,frequency,12000);
  auto full=reist16::qso2::waveform(bits,frequency);
  check(full.size()==614400&&wave.size()==153600,"exact TX budget");
  for(int variant:{0,1}) for(int start:{0,12000}) {
   std::vector<float> audio(180000,0);
   for(size_t i=0;i<wave.size();++i)audio[start+i]=variant?full[4*i]:wave[i];
   auto found=reist16::qso2::decode(audio.data(),180000,100,4000,2);
   check(found.size()==1&&found[0].bits==bits,"clean edge/start round trip");
   check(std::abs(found[0].frequency-frequency)<.5,"clean frequency");
   check(std::abs(found[0].dt-(start/12000.-.5))<.015,"clean timing");
  }
 }
 auto bad=bits;bad[17]=2;bool rejected=false;
 try{reist16::qso2::encode_tones(bad);}catch(std::invalid_argument const&){rejected=true;}
 check(rejected,"bad bits");
 std::vector<float> output(614401,9);
 check(reist16_qso2_encode(bits.data(),73,1500,output.data(),614399)==-1&&output[0]==9,"short ABI output");
 check(reist16_qso2_encode(bits.data(),73,1500,output.data(),614400)==614400&&output.back()==9,"exact ABI capacity");
 check(reist16_qso2_encode(bits.data(),73,99,output.data(),614400)==-1,"audio bounds");
 std::vector<float> audio(180000,0);
 audio[0]=std::numeric_limits<float>::infinity();
 check(reist16::qso2::decode(audio.data(),180000,100,4000,3).empty(),"nonfinite input");
 check(reist16::qso2::decode(audio.data(),180001,100,4000,3).empty(),"bounded input");
 // Independent noisy waveform: preserve the final synchronization symbols
 // when browser/network delay moves a complete frame past the old cutoff.
 std::string message="CQ TEST OE3SRO JN88";message.resize(37,' ');
 char sent[37],packed[77];int packedTones[79],i3=0,n3=0;
 genft8_(&message[0],&i3,&n3,sent,packed,packedTones,37,37);
 reist16::Bits delayedBits;for(int i=0;i<77;++i)delayedBits[i]=packed[i];
 auto tones=reist16::qso2::encode_tones(delayedBits);
 std::vector<std::complex<float>> independent(153600);
 ft8_native::waveform(tones.data(),64,2400,4,12000,1500.375f,independent.data(),nullptr,true,int(independent.size()));
 int delayedExact=0,oldExact=0;
 for(int start:{13200,15600,18000})for(int repeat=0;repeat<2;++repeat) {
   std::mt19937 noiseRng(2026102340+repeat);std::normal_distribution<float> normal;
   double phase=6.283185307179586*(noiseRng()/double(noiseRng.max()));
   std::vector<float> signal(independent.size());double power=0;
   for(size_t i=0;i<signal.size();++i){signal[i]=.8*std::imag(std::complex<double>(independent[i])*std::polar(1.,phase));power+=signal[i]*signal[i];}
   power/=signal.size();
   double sigma=std::sqrt(power*.0001*6000/2500/std::pow(10.,-18./10));
   std::vector<float> rx(180000);for(auto& v:rx)v=sigma*normal(noiseRng);
   for(size_t i=0;i<signal.size();++i)rx[start+i]+=.01f*signal[i];
   auto old=reist16::qso2::decode(rx.data(),48*3456,100,4000,3);
   auto extended=reist16::qso2::decode(rx.data(),reist16::qso2::monitor_capture_samples,100,4000,3);
   for(auto const& d:old)if(d.bits==delayedBits)++oldExact;
   check(extended.size()==1&&extended[0].bits==delayedBits,"delayed noisy frame fits extended capture");
   ++delayedExact;
 }
 check(delayedExact>oldExact,"longer capture recovers delayed noisy frames");
 std::cout<<"PASS delayed noisy frames: old "<<oldExact<<"/6, extended "<<delayedExact<<"/6"<<std::endl;
 std::cout<<"PASS QSO/2 wire mapping, 12 clean edge/start round trips, 12/48 kHz, ABI and bounds"<<std::endl;
}

int main(int argc,char**argv) {
 try {
 if(argc>1&&std::string(argv[1])=="--unit"){unit_tests();return 0;}
 int trials=argc>1?std::stoi(argv[1]):4;unsigned seed=argc>2?unsigned(std::stoul(argv[2])):2026102204;
 check(trials>=1&&trials<=100,"trial budget");
 std::mt19937 rng(seed);std::normal_distribution<float> normal;
 std::ofstream report("results.jsonl");
 int false_count=0;
 for(int snr:{-18,-20,-22,-24}) {
 int old_ok=0,new_ok=0;double old_time=0,new_time=0;
 for(int trial=0;trial<trials;++trial) {
  std::string message="CQ DL"+std::to_string(1+rng()%9);
  for(int k=0;k<3;++k)message+=char('A'+rng()%26);
  message+=" JO31";message.resize(37,' ');
  char sent[37],packed[77];int tones[79],i3=0,n3=0;
  genft8_(&message[0],&i3,&n3,sent,packed,tones,37,37);
  check(i3==1,"standard random CQ");
  reist16::Bits bits;for(int i=0;i<77;++i)bits[i]=packed[i];
  double frequency=400.+rng()%2800+double(rng()%1000)/1000;
  int start=4200+int(rng()%5401);
  auto old=reist16::waveform(bits,77,frequency,12000);
  auto next=reist16::qso2::waveform(bits,frequency,12000);
  std::vector<float> ft8(79*1920);
  ft8_native::waveform(tones,79,1920,2,12000,float(frequency),nullptr,ft8.data(),false,int(ft8.size()));
  for(auto& v:ft8)v*=.8f;
  check(next.size()==153600,"12.8-second frame");
  double gain=.01,sigma=std::sqrt(.32*gain*gain*6000/2500/std::pow(10.,snr/10.));
  std::vector<float> noise(180000);for(auto& v:noise)v=float(sigma)*normal(rng);
  std::vector<std::vector<float>> inputs;
  for(auto const& wave:{old,next,ft8}) {
   auto audio=noise;for(size_t i=0;i<wave.size();++i)audio[start+i]+=float(gain)*wave[i];
   // Decode the same quantized PCM as the independent jt9 comparison.
   for(auto& v:audio){check(std::abs(v)<1,"AWGN headroom");v=float(std::lround(v*32767))/32768;}
   inputs.push_back(std::move(audio));
  }
  auto t=std::chrono::steady_clock::now();
  auto a=reist16::decode(inputs[0].data(),180000,100,4000,3);
  old_time+=std::chrono::duration<double>(std::chrono::steady_clock::now()-t).count();
  t=std::chrono::steady_clock::now();
  auto b=reist16::qso2::decode(inputs[1].data(),180000,100,4000,3);
  new_time+=std::chrono::duration<double>(std::chrono::steady_clock::now()-t).count();
  bool oa=false,nb=false;
  for(auto const& d:a){if(d.bits==bits)oa=true;else ++false_count;}
  for(auto const& d:b){if(d.bits==bits)nb=true;else ++false_count;}
  old_ok+=oa;new_ok+=nb;
  std::string id=std::to_string(-snr)+"_"+std::to_string(trial)+"_000000.wav";
  wav("ft8_"+id,inputs[2]);wav("qso2_"+id,inputs[1]);
  auto trimmed=message.substr(0,message.find_last_not_of(' ')+1);
  report<<"{\"snr2500\":"<<snr<<",\"trial\":"<<trial<<",\"seed\":"<<seed
   <<",\"message\":\""<<trimmed<<"\",\"frequency\":"<<frequency<<",\"start_sample\":"<<start
   <<",\"qso1\":"<<oa<<",\"qso2\":"<<nb<<",\"ft8_wave\":\"ft8_"<<id
   <<"\",\"qso2_wave\":\"qso2_"<<id<<"\"}\n";
  report.flush();
 }
 std::cout<<"SNR "<<snr<<" QSO1 "<<old_ok<<"/"<<trials<<" QSO2 "<<new_ok<<"/"<<trials
  <<" wall_seconds "<<old_time<<" "<<new_time<<std::endl;
 }
 for(int trial=0;trial<16;++trial) {
  std::vector<float> audio(180000,0);
  if(trial)for(auto& v:audio)v=.03f*normal(rng);
  if(trial>7)for(int i=0;i<180000;++i)audio[i]+=.05f*std::sin(2*3.141592653589793*(500+trial*137)*i/12000.);
  auto result=reist16::qso2::decode(audio.data(),180000,100,4000,3);
  false_count+=int(result.size());
 }
 report<<"{\"negative_slots\":16,\"false_decodes\":"<<false_count<<"}\n";
 check(false_count==0,"false decodes");
 std::cout<<"PASS negatives / no false payloads"<<std::endl;
 }catch(std::exception const& e){std::cerr<<e.what()<<std::endl;return 1;}
}
