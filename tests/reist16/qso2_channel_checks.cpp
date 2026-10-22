#include "lib/reist16/qso2.hpp"
#include "lib/ft8/native/ft8_wave.hpp"
#include <algorithm>
#include <cmath>
#include <complex>
#include <fstream>
#include <iostream>
#include <random>
#include <stdexcept>
int main(){
 try{
 std::mt19937 rng(2026102210);std::normal_distribution<float> normal;
 std::ofstream report("channel.jsonl");
 int channel_exact=0;
 for(int trial=0;trial<8;++trial){
  reist16::Bits bits;for(auto& b:bits)b=rng()&1;
  bits[74]=0;bits[75]=0;bits[76]=1;
  double f=600.375+trial*350,drift=trial<2?0:trial%2?.02:-.02;
  auto tones=reist16::qso2::encode_tones(bits);
  std::vector<std::complex<float>> wave(153600);
  ft8_native::waveform(tones.data(),64,2400,4,12000,float(f),wave.data(),nullptr,true,int(wave.size()));
  double phase=2*3.141592653589793*(rng()/double(rng.max()));
  double gain=.01,snr=-16,sigma=std::sqrt(.32*gain*gain*6000/2500/std::pow(10.,snr/10));
  std::vector<float> audio(180000);for(auto& v:audio)v=float(sigma)*normal(rng);
  int start=3600+int(rng()%8401);
  for(size_t i=0;i<wave.size();++i){
   double t=i/12000.,fade=trial>=4?.65+.35*std::cos(2*3.141592653589793*t/3.):1.;
   auto shift=std::polar(1.,phase+3.141592653589793*drift*t*t);
   audio[start+i]+=float(.8*gain*fade*std::imag(std::complex<double>(wave[i])*shift));
  }
  auto found=reist16::qso2::decode(audio.data(),180000,100,4000,3);
  bool exact=false;for(auto const& d:found)if(d.bits==bits)exact=true;else throw std::runtime_error("wrong channel payload");
  report<<"{\"trial\":"<<trial<<",\"snr2500_before_fading\":-16,\"phase\":"<<phase<<",\"drift_hz_s\":"<<drift<<",\"fading\":"<<(trial>=4)<<",\"exact\":"<<exact<<"}\n";report.flush();
  channel_exact+=exact;
  if(!exact&&trial<4)throw std::runtime_error("random phase / mild drift round trip");
 }
 for(int trial=0;trial<128;++trial){
  std::vector<float> audio(180000);for(auto& v:audio)v=.03f*normal(rng);
  int carriers=trial/32;double base=150.+rng()%3200;
  for(int c=0;c<carriers;++c)for(int i=0;i<180000;++i){
   double t=i/12000.,amplitude=.04*(c+1);
   if(trial%2)amplitude*=std::fmod(t,1.)<.5?1.:0.;
   audio[i]+=float(amplitude*std::sin(2*3.141592653589793*(base+137*c)*t));
  }
  auto found=reist16::qso2::decode(audio.data(),180000,100,4000,3);
  report<<"{\"negative_slot\":"<<trial<<",\"carriers\":"<<carriers<<",\"false_decodes\":"<<found.size()<<"}\n";report.flush();
  if(!found.empty())throw std::runtime_error("false negative-control decode");
  if(trial%32==31)std::cout<<"Negative slots "<<trial+1<<"/128, no false decodes"<<std::endl;
 }
 // Two separated new-profile stations and an old-profile station share one slot.
 std::vector<float> mixture(180000);
 for(auto& v:mixture)v=.003f*normal(rng);
 reist16::Bits first{},second{},old{};
 for(auto& v:first)v=rng()&1;
 for(auto& v:second)v=rng()&1;
 for(auto& v:old)v=rng()&1;
 first[74]=second[74]=old[74]=0;first[75]=second[75]=old[75]=0;first[76]=second[76]=old[76]=1;
 auto a=reist16::qso2::waveform(first,650.375,12000),b=reist16::qso2::waveform(second,1650.625,12000),c=reist16::waveform(old,571,2650.125,12000);
 for(auto const& wave:{a,b,c})for(size_t i=0;i<wave.size();++i)mixture[6000+i]+=.05f*wave[i];
 std::vector<int16_t> pcm(180000);for(int i=0;i<180000;++i)pcm[i]=int16_t(std::lround(mixture[i]*32767));
 int8_t bits[77*32];float f[32],t[32],snr[32];
 int n=reist16_decode(pcm.data(),180000,100,4000,3,bits,f,t,snr,32);
 if(n!=3||!std::equal(first.begin(),first.end(),bits)||!std::equal(second.begin(),second.end(),bits+77)||!std::equal(old.begin(),old.end(),bits+154))
  throw std::runtime_error("mixed QSO/1+QSO/2 production ABI");
 report<<"{\"mixed_profile_stations\":3,\"exact\":true}\n";
 std::cout<<"Channel measurement: "<<channel_exact<<"/8 phase/drift/fading frames exact. PASS 128 noise/interference slots, 3 mixed-profile stations"<<std::endl;
 }catch(std::exception const& e){std::cerr<<e.what()<<std::endl;return 1;}
}
