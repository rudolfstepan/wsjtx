// Offline capacity probe; links the existing production libraries, no RF access.
#include "lib/reist16/qso2.hpp"
#include "lib/ft8/native/ft8_wave.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <random>
#include <stdexcept>
extern "C" void genft8_(char*,int*,int*,char*,char*,int*,size_t,size_t);
static void wav(std::string const& name,std::vector<float> const& audio) {
  std::ofstream f(name,std::ios::binary);
  auto word=[&](unsigned x,int n){for(int i=0;i<n;++i)f.put(char(x>>(8*i)));};
  f.write("RIFF",4);word(36+2*audio.size(),4);f.write("WAVEfmt ",8);
  word(16,4);word(1,2);word(1,2);word(12000,4);word(24000,4);word(2,2);word(16,2);
  f.write("data",4);word(2*audio.size(),4);
  for(float x:audio){if(std::abs(x)>=1)throw std::runtime_error("clipping");word(unsigned(int16_t(std::lround(x*32767))),2);}
}
int main(int argc,char** argv) {
  unsigned seed=argc>1?unsigned(std::stoul(argv[1])):2026102601;
  struct Case {int n,spacing,nearfar;};
  Case cases[]={{3,100,0},{12,100,0},{24,100,0},{32,100,0},{36,100,0},
                {40,60,0},{2,50,0},{2,80,0},{2,100,24},
                {48,50,0},{60,60,0},{2,25,24},{2,50,24},{2,80,24}};
  std::ofstream expected("expected.tsv"),report("native.tsv");
  bool unexpected=false;
  report<<"case\tstations\tspacing_hz\tnearfar_db\tdepth\texact\tfalse\tseconds\n";
  for(auto c:cases) {
    std::string id="n"+std::to_string(c.n)+"s"+std::to_string(c.spacing)+"p"+std::to_string(c.nearfar);
    std::mt19937 rng(seed);std::normal_distribution<float> normal;
    double gain=.008,sigma=std::sqrt(.32*gain*gain*6000/2500/std::pow(10.,-18./10));
    std::vector<float> noise(180000);for(auto& v:noise)v=sigma*normal(rng);
    auto reist=noise,ft8=noise;std::vector<reist16::Bits> wanted;
    for(int station=0;station<c.n;++station) {
      std::string msg="CQ DL"+std::to_string(1+station%9);
      msg+=char('A'+station/26);msg+=char('A'+station%26);msg+="A JO31";
      expected<<id<<'\t'<<msg<<'\n';msg.resize(37,' ');
      char sent[37],packed[77];int tones[79],i3=0,n3=0;
      genft8_(&msg[0],&i3,&n3,sent,packed,tones,37,37);
      if(i3!=1)throw std::runtime_error("pack77");
      reist16::Bits bits;std::copy(packed,packed+77,bits.begin());wanted.push_back(bits);
      auto rt=reist16::qso2::encode_tones(bits);
      double frequency=200.375+station*c.spacing,phase=6.283185307179586*rng()/double(rng.max());
      int start=4800+int(rng()%2401); // +0.4 .. +0.6 s, complete in live capture
      double amplitude=gain*std::pow(10.,(station==0?c.nearfar:0)/20.);
      for(int mode=0;mode<2;++mode) {
        int n=mode?79:64,nsps=mode?1920:2400;
        std::vector<std::complex<float>> wave(n*nsps);
        ft8_native::waveform(mode?tones:rt.data(),n,nsps,mode?2:4,12000,float(frequency),wave.data(),nullptr,true,int(wave.size()));
        auto& audio=mode?ft8:reist;
        for(size_t j=0;j<wave.size();++j)audio[start+j]+=float(.8*amplitude*std::imag(std::complex<double>(wave[j])*std::polar(1.,phase)));
      }
    }
    wav("reist_"+id+"_000000.wav",reist);wav("ft8_"+id+"_000000.wav",ft8);
    std::vector<int16_t> pcm(180000);
    for(int j=0;j<180000;++j){if(std::abs(reist[j])>=1)throw std::runtime_error("clipping");pcm[j]=int16_t(std::lround(reist[j]*32767));}
    for(int depth:{1,3}) {
      int8_t bits[77*reist16::max_decodes];float f[reist16::max_decodes],t[reist16::max_decodes],snr[reist16::max_decodes];
      auto before=std::chrono::steady_clock::now();
      int n=reist16_decode(pcm.data(),reist16::qso2::rx_capture_samples,100,4000,depth,bits,f,t,snr,reist16::max_decodes);
      if(n<0)throw std::runtime_error("decode ABI");
      double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-before).count();
      int exact=0,bad=0;
      for(int j=0;j<n;++j) {
        bool match=false;for(auto const& w:wanted)if(std::equal(w.begin(),w.end(),bits+77*j))match=true;
        if(match)++exact;else ++bad;
      }
      report<<id<<'\t'<<c.n<<'\t'<<c.spacing<<'\t'<<c.nearfar<<'\t'<<depth<<'\t'<<exact<<'\t'<<bad<<'\t'<<seconds<<std::endl;
      unexpected=unexpected||bad!=0;
      std::cout<<id<<" depth "<<depth<<": "<<exact<<'/'<<c.n<<", false "<<bad<<", "<<seconds<<" s"<<std::endl;
    }
  }
  return unexpected?1:0;
}
