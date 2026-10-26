#include "lib/reist16/subtract.hpp"
#include "lib/ft8/native/ft8_wave.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <future>
#include <random>
#include <stdexcept>
#include <string>
extern "C" void genft8_(char*,int*,int*,char*,char*,int*,size_t,size_t);

namespace {
void check(bool value,char const* message) {if(!value)throw std::runtime_error(message);}
reist16::Bits payload(std::mt19937& rng) {
  reist16::Bits bits;for(auto& b:bits)b=rng()&1;
  bits[74]=0;bits[75]=0;bits[76]=1;return bits;
}
std::vector<float> signal(reist16::Bits const& bits,double frequency,double phase) {
  auto tones=reist16::qso2::encode_tones(bits);
  std::vector<std::complex<float>> wave(153600);
  ft8_native::waveform(tones.data(),64,2400,4,12000,float(frequency),wave.data(),nullptr,true,int(wave.size()));
  std::vector<float> out(wave.size());
  for(size_t i=0;i<wave.size();++i)out[i]=.8*std::imag(std::complex<double>(wave[i])*std::polar(1.,phase));
  return out;
}
void reconstruction() {
  std::mt19937 rng(2026102617);std::normal_distribution<float> normal;
  for(double frequency:{100.375,1500.625,4000.})for(int start:{0,6000,26400}) {
    auto bits=payload(rng);
    auto wave=signal(bits,frequency,.731);
    std::vector<float> audio(180000),noise(180000);
    for(auto& x:noise)x=.003*normal(rng);
    audio=noise;
    for(size_t i=0;i<wave.size();++i)audio[start+i]+=.1*wave[i];
    double before=0;for(size_t i=0;i<audio.size();++i)before+=std::pow(audio[i]-noise[i],2);
    reist16::Decode known{};known.bits=bits;known.frequency=frequency+.15625;
    int timingError=start==0?30:start==26400?-30:15;
    known.dt=(start+timingError)/12000.-.5;
    check(reist16::qso2::subtract(audio,known),"accepted reconstruction");
    double after=0;for(size_t i=0;i<audio.size();++i)after+=std::pow(audio[i]-noise[i],2);
    check(after<before*.03,"at least 15 dB signal suppression despite frequency/timing error");
  }
  std::vector<float> silence(180000);auto original=silence;
  reist16::Decode known{};known.bits=payload(rng);known.frequency=1500;known.dt=0;
  check(!reist16::qso2::subtract(silence,known)&&silence==original,"zero-energy reconstruction leaves data intact");
  known.dt=3;
  check(!reist16::qso2::subtract(silence,known)&&silence==original,"out-of-bounds reconstruction leaves data intact");
  std::cout<<"PASS nine arbitrary-phase reconstructions, frequency/timing errors, >=15 dB suppression and bounds\n";
}
void capacity() {
  std::mt19937 rng(2026102618);std::normal_distribution<float> normal;
  std::vector<reist16::Bits> messages;
  std::vector<float> audio(reist16::qso2::rx_capture_samples);
  for(auto& x:audio)x=.005*normal(rng);
  for(int station=0;station<40;++station) {
    // The same payload at a different frequency is another station, not a dupe.
    auto bits=station==1?messages[0]:payload(rng);messages.push_back(bits);
    auto wave=signal(bits,250.375+80*station,.17*station);
    for(size_t i=0;i<wave.size();++i)audio[6000+i]+=.01*wave[i];
  }
  std::vector<int16_t> pcm(audio.size());
  for(size_t i=0;i<audio.size();++i)pcm[i]=int16_t(std::lround(audio[i]*32767));
  for(int capacity:{1,32,reist16::max_decodes}) {
    std::vector<int8_t> bits(77*capacity+1,-7);
    std::vector<float> f(capacity+1,-777),t=f,snr=f;
    int n=reist16_decode(pcm.data(),int(pcm.size()),100,4000,1,bits.data(),f.data(),t.data(),snr.data(),capacity);
    check(n==std::min(40,capacity),"40 simultaneous stations, caller capacity respected");
    check(bits.back()==-7&&f.back()==-777&&t.back()==-777&&snr.back()==-777,"no ABI output overrun");
    for(int station=0;station<n;++station) {
      check(std::equal(messages[station].begin(),messages[station].end(),bits.data()+77*station),"exact payload at every frequency");
      check(std::abs(f[station]-(250.375+80*station))<.5,"sorted accurate frequencies");
    }
  }
  std::vector<std::future<bool>> workers;
  for(int station=0;station<4;++station)workers.push_back(std::async(std::launch::async,[&,station] {
    int8_t bits[77];float f,t,snr;
    int low=250+80*station;
    return reist16_decode(pcm.data(),int(pcm.size()),low,low+2,1,bits,&f,&t,&snr,1)==1&&
      std::equal(messages[station].begin(),messages[station].end(),bits);
  }));
  for(auto& worker:workers)check(worker.get(),"concurrent independent profile searches");
  std::cout<<"PASS 40 stations at depth 1, equal payload on two frequencies, legacy/small ABI capacities and canaries\n";
  std::cout<<"PASS four concurrent C-ABI receivers with independent frequency ranges\n";
}
void dense_regression() {
  // Independent seed that lost seven stations with a global coherent-attempt
  // cap. Successful coherent decodes must leave search capacity for neighbors.
  for(unsigned seed:{2026102601u,2026102602u}) {
  std::mt19937 rng(seed);std::normal_distribution<float> normal;
  double gain=.008,sigma=std::sqrt(.32*gain*gain*6000/2500/std::pow(10.,-18./10));
  std::vector<float> audio(180000);for(auto& v:audio)v=sigma*normal(rng);
  std::vector<reist16::Bits> messages;
  for(int station=0;station<48;++station) {
    std::string message="CQ DL"+std::to_string(1+station%9);
    message+=char('A'+station/26);message+=char('A'+station%26);message+="A JO31";
    message.resize(37,' ');
    char sent[37],packed[77];int tones[79],i3=0,n3=0;
    genft8_(&message[0],&i3,&n3,sent,packed,tones,37,37);
    check(i3==1,"dense regression standard message");
    reist16::Bits bits;std::copy(packed,packed+77,bits.begin());messages.push_back(bits);
    double phase=6.283185307179586*rng()/double(rng.max());
    int start=4800+int(rng()%2401);
    auto wave=signal(bits,200.375+50*station,phase);
    for(size_t i=0;i<wave.size();++i)audio[start+i]+=gain*wave[i];
  }
  std::vector<int16_t> pcm(audio.size());
  for(size_t i=0;i<audio.size();++i)pcm[i]=int16_t(std::lround(audio[i]*32767));
  for(int count:{reist16::qso2::rx_capture_samples,180000}) {
    int8_t bits[77*reist16::max_decodes];float f[reist16::max_decodes],t[reist16::max_decodes],snr[reist16::max_decodes];
    int n=reist16_decode(pcm.data(),count,100,4000,3,bits,f,t,snr,reist16::max_decodes);
    check(n==48,"48 overlapping stations at -18 dB, independent seed, both capture windows");
    for(int i=0;i<n;++i)check(std::equal(messages[i].begin(),messages[i].end(),bits+77*i),"dense regression exact payloads");
  }
  }
  std::cout<<"PASS 48 overlapping standard CQs at -18 dB, 50 Hz spacing, two seeds, live and full capture, no foreign payloads\n";
}
}
int main() {
  try {reconstruction();capacity();dense_regression();}
  catch(std::exception const& error){std::cerr<<error.what()<<'\n';return 1;}
}
