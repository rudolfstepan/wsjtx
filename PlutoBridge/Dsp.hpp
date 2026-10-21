#ifndef WSJTX_PLUTO_DSP_HPP
#define WSJTX_PLUTO_DSP_HPP
#include <array>
#include <complex>
#include <vector>
#include <cmath>

// Streaming USB conversion. All filters retain their state across IIO blocks.
namespace pluto {
using IQ=std::complex<float>;
constexpr int audio_rate=48000, ratio=64, rf_rate=audio_rate*ratio;
constexpr int audio_block=480, rf_block=audio_block*ratio;
constexpr double pi=3.14159265358979323846;
class HalfBand {
  std::array<IQ,33> history{};
  std::array<float,33> taps{};
  unsigned at=0;
public:
  HalfBand() {
    double sum=0;
    for(int k=0;k<33;++k) {
      int n=k-16;
      double sinc=n==0?.5:(n%2==0?0:std::sin(pi*n/2)/(pi*n));
      taps[k]=float(sinc*(.42-.5*std::cos(2*pi*k/32)+.08*std::cos(4*pi*k/32)));
      sum+=taps[k];
    }
    for(auto& t:taps) t=float(t/sum);
  }
  void put(IQ v) { history[at]=v; at=(at+1)%33; }
  IQ value() const {
    IQ v{};
    for(int k=0;k<33;++k) if(taps[k]!=0) v+=history[(at+32-k)%33]*taps[k];
    return v;
  }
};
class Hilbert {
  std::array<float,513> history{};
  std::array<float,513> taps{};
  unsigned at=0;
public:
  Hilbert() {
    for(int k=0;k<513;++k) {
      int n=k-256;
      taps[k]=n%2==0?0:float(2/(pi*n)*(.42-.5*std::cos(2*pi*k/512)+.08*std::cos(4*pi*k/512)));
    }
  }
  IQ analytic(float sample) {
    history[at]=sample;
    float delayed=history[(at+513-256)%513], quad=0;
    for(int k=1;k<513;k+=2) quad+=taps[k]*history[(at+513-k)%513];
    at=(at+1)%513;
    return {delayed,quad};
  }
};
class UsbModulator {
  Hilbert hilbert;
  std::array<HalfBand,6> filters;
public:
  std::vector<IQ> process(float const* audio,int count) {
    std::vector<IQ> data;
    data.reserve(count);
    for(int k=0;k<count;++k) data.push_back(hilbert.analytic(audio[k]));
    for(auto& filter:filters) {
      std::vector<IQ> next;
      next.reserve(data.size()*2);
      for(auto v:data) {
        filter.put(v); next.push_back(2.f*filter.value());
        filter.put({}); next.push_back(2.f*filter.value());
      }
      data.swap(next);
    }
    return data;
  }
};
class UsbDemodulator {
  std::array<HalfBand,6> filters;
  std::array<unsigned,6> phases{};
  Hilbert realHilbert, imagHilbert;
public:
  std::vector<float> process(IQ const* samples,int count) {
    std::vector<IQ> data(samples,samples+count);
    for(unsigned stage=0;stage<6;++stage) {
      std::vector<IQ> next;
      next.reserve((data.size()+1)/2);
      for(auto v:data) {
        filters[stage].put(v);
        if(++phases[stage]%2==0) next.push_back(filters[stage].value());
      }
      data.swap(next);
    }
    std::vector<float> audio;
    audio.reserve(data.size());
    for(auto v:data) {
      auto re=realHilbert.analytic(v.real());
      auto im=imagHilbert.analytic(v.imag());
      audio.push_back(.5f*(re.real()-im.imag()));
    }
    return audio;
  }
};
// libiio scan metadata, not an assumption about RX/TX bit alignment.
struct SampleFormat {
  unsigned bits,shift;
  float decode(unsigned short raw) const {
    int value=(raw>>shift)&((1u<<bits)-1);
    if(value&(1u<<(bits-1))) value-=1<<bits;
    return float(value)/float(1u<<(bits-1));
  }
  unsigned short encode(float v) const {
    float bounded=std::max(-1.f,std::min(1.f,v));
    int limit=(1<<(bits-1))-1;
    int value=int(std::round(bounded*limit));
    return static_cast<unsigned short>(static_cast<unsigned>(value)<<shift);
  }
};
}
#endif
