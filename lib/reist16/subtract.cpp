#include "subtract.hpp"
#include "../ft8/native/ft8_wave.hpp"
#include <algorithm>
#include <cmath>
#include <complex>

namespace reist16 { namespace qso2 {
namespace {
using Complex = std::complex<double>;

// Local complex fits tolerate the remaining frequency error and slow fading.
// Timing, unlike carrier phase, changes the GFSK transitions. Fit timing over
// all data symbols as well as sync; do not estimate it from sync tones alone.
double timing_score(float const* data, std::vector<std::complex<float>> const& reference,
                    int start,bool sync_only=false) {
  double score=0;
  if(sync_only) {
    int const first[]={0,29,58},last[]={7,36,64};
    for(int block=0;block<3;++block) {
      Complex sum{};double weight=0;
      // Exclude the unknown data transitions at each end of a sync block.
      for(int i=first[block]*symbol_samples+symbol_samples/2;
          i<last[block]*symbol_samples-symbol_samples/2;i+=8) {
        Complex r=reference[i];sum+=double(data[start+i])*std::conj(r);weight+=std::norm(r);
      }
      score+=std::norm(sum)/std::max(weight,1e-20);
    }
    return score;
  }
  for(int block=0;block<symbols/2;++block) {
    Complex sum{};double weight=0;
    for(int i=block*2*symbol_samples;i<(block+1)*2*symbol_samples;i+=8) {
      Complex r=reference[i];sum+=double(data[start+i])*std::conj(r);weight+=std::norm(r);
    }
    score+=std::norm(sum)/std::max(weight,1e-20);
  }
  return score;
}

template<class T> std::vector<T> smooth(std::vector<T> const& input) {
  // Two 200-ms box filters form a triangular low-pass. Prefix sums give
  // linear work rather than one convolution per output sample.
  int count=int(input.size()),half=symbol_samples/2;
  std::vector<T> prefix(count+1),output(count);
  for(int i=0;i<count;++i)prefix[i+1]=prefix[i]+input[i];
  for(int i=0;i<count;++i) {
    int lo=std::max(0,i-half),hi=std::min(count,i+half);
    output[i]=(prefix[hi]-prefix[lo])/double(hi-lo);
  }
  return output;
}
}

int refine_start(float const* data,int count,
                 std::vector<std::complex<float>> const& reference,int initial,bool sync_only) {
  int frame=int(reference.size());
  int start=initial;double best=timing_score(data,reference,start,sync_only);
  for(int stride:{60,12,3,1}) {
    int center=start,radius=stride==60?(sync_only?1200:300):stride==12?60:stride==3?12:3;
    for(int offset=-radius;offset<=radius;offset+=stride) {
      int trial=center+offset;
      if(trial<0||trial+frame>count)continue;
      double score=timing_score(data,reference,trial,sync_only);
      if(score>best){best=score;start=trial;}
    }
  }
  return start;
}

bool subtract(std::vector<float>& residual, Decode const& decoded) {
  int frame=symbols*symbol_samples,count=int(residual.size());
  int initial=int(std::lround((decoded.dt+start_delay)*sample_rate));
  if(initial<0||initial+frame>count)return false;
  auto tones=encode_tones(decoded.bits);
  std::vector<std::complex<float>> reference(frame);
  ft8_native::waveform(tones.data(),symbols,symbol_samples,4,12000,
      float(decoded.frequency),reference.data(),nullptr,true,frame);
  int start=refine_start(residual.data(),count,reference,initial);
  std::vector<Complex> mixed(frame);
  std::vector<double> weight(frame);
  for(int i=0;i<frame;++i) {
    Complex r=reference[i];mixed[i]=double(residual[start+i])*std::conj(r);weight[i]=std::norm(r);
  }
  mixed=smooth(smooth(mixed));weight=smooth(smooth(weight));
  std::vector<float> cleaned(frame);
  double before=0,after=0;
  for(int i=0;i<frame;++i) {
    double estimate=2*std::real(Complex(reference[i])*mixed[i])/std::max(weight[i],1e-12);
    double original=residual[start+i],value=original-estimate;
    cleaned[i]=float(value);before+=original*original;after+=value*value;
  }
  // An invalid or non-improving reconstruction must leave the buffer intact.
  if(!std::isfinite(after)||after>=before)return false;
  std::copy(cleaned.begin(),cleaned.end(),residual.begin()+start);
  return true;
}
} }
