// REIST16-QSO/2: 77-bit standard messages, CRC14 + LDPC(174,91),
// 16-GFSK, 200 ms symbols, 64 symbols / 12.8 s in a 15-second UTC slot.
// Reuses this fork's GPLv3 WSJT-X LDPC and Gaussian waveform kernels.
#include "qso2.hpp"
#include "subtract.hpp"
#include "../ft8/native/ft8_ldpc.hpp"
#include "../ft8/native/ft8_wave.hpp"
#include <fftw3.h>
#include <algorithm>
#include <bitset>
#include "../ft8/native/ft8_tables.hpp"
#include <cmath>
#include "planner_lock.hpp"
#include <stdexcept>
namespace reist16 { namespace qso2 {
namespace {
constexpr double pi=3.14159265358979323846;
constexpr int sync_pos[20]={0,1,2,3,4,5,6,29,30,31,32,33,34,35,58,59,60,61,62,63};
constexpr int sync_tone[20]={0,15,3,12,7,1,10,13,2,11,5,14,8,4,6,9,0,12,3,15};
constexpr int fft_size=4800,bins=2401,step=600;
constexpr double bin_hz=2.5;
int sync_index(int symbol) { for(int i=0;i<20;++i) if(sync_pos[i]==symbol) return i; return -1; }
int gray(int word) { return word^(word>>1); }
double energy(float const* p,double frequency) {
  double c=2*std::cos(2*pi*frequency/sample_rate),a=0,b=0;
  for(int i=0;i<symbol_samples;++i){double v=p[i]+c*a-b;b=a;a=v;}
  return std::max(0.,a*a+b*b-c*a*b);
}
double sync_energy(float const* p,double f) {
  double sum=0;
  for(int k=0;k<20;++k) sum+=energy(p+sync_pos[k]*symbol_samples,f+sync_tone[k]*tone_spacing);
  return sum;
}
double log_i0(double x) {
  if(x<3.75) { double y=x/3.75;y*=y;
    return std::log(1+y*(3.5156229+y*(3.0899424+y*(1.2067492+y*(.2659732+y*(.0360768+y*.0045813)))))); }
  double y=3.75/x;
  return x-.5*std::log(x)+std::log(.39894228+y*(.01328592+y*(.00225319+y*(-.00157565+y*(.00916281+y*(-.02057706+y*(.02635537+y*(-.01647633+y*.00392377))))))));
}
double log_add(double a,double b) { double m=std::max(a,b);return m+std::log1p(std::exp(-std::abs(a-b))); }

// Ordered-statistics fallback: reliability-ordered information basis,
// order 0/1/2 re-encoding. Only the minimum-distance word is CRC-checked.
// The 14 CRC bits remain independent rejection bits, not forced payload.
bool osd(float const* llr,int order,int8_t* decoded,int& hard) {
  using Word=std::bitset<174>;
  std::array<Word,91> rows;
  for(int i=0;i<91;++i) {
    rows[i].set(i);
    for(int p=0;p<83;++p)
      if((ft8_native::tables::generator[p][i/64]>>(i%64))&1)rows[i].set(91+p);
  }
  std::array<int,174> indices;
  for(int i=0;i<174;++i)indices[i]=i;
  std::stable_sort(indices.begin(),indices.end(),[&](int a,int b){return std::abs(llr[a])>std::abs(llr[b]);});
  int pivots[91];int rank=0;
  for(int bit:indices) {
    int row=rank;while(row<91&&!rows[row][bit])++row;
    if(row==91)continue;
    std::swap(rows[rank],rows[row]);
    for(int i=0;i<91;++i)if(i!=rank&&rows[i][bit])rows[i]^=rows[rank];
    pivots[rank++]=bit;if(rank==91)break;
  }
  if(rank!=91)return false;
  Word base,received;
  for(int i=0;i<174;++i)received[i]=llr[i]>0;
  for(int i=0;i<91;++i)if(received[pivots[i]])base^=rows[i];
  Word best=base;double distance=1e100;
  auto consider=[&](Word const& word) {
    Word errors=word^received;double d=0;
    for(int i=0;i<174;++i)if(errors[i]){d+=std::abs(llr[i]);if(d>=distance)return;}
    if(d<distance){distance=d;best=word;}
  };
  consider(base);
  for(int i=0;i<91;++i) {
    Word once=base^rows[i];consider(once);
    if(order>=2)for(int j=0;j<i;++j)consider(once^rows[j]);
  }
  for(int i=0;i<174;++i)decoded[i]=best[i]?1:0;
  hard=int((best^received).count());
  return hard<=36&&best.any()&&ft8_native::validCrc(decoded);
}


std::complex<double> correlation(float const* p,double frequency) {
  int first=0,last=symbol_samples;
  auto oscillator=std::polar(1.,-2*pi*frequency*first/sample_rate);
  auto delta=std::polar(1.,-2*pi*frequency/sample_rate);
  std::complex<double> sum{};
  for(int i=first;i<last;++i){sum+=double(p[i])*oscillator;oscillator*=delta;}
  return sum;
}

bool cq_ap_decode(float const* llr,int depth,Bits& bits) {
  if(depth<2)return false;
  // Standard CQ token and its /R flag, exactly the pack77 CQ prior also
  // used by FT8. No sender, locator, report or expected payload is supplied.
  for(int type:{1,2}) {
    float assisted[174];std::copy(llr,llr+174,assisted);
    int8_t mask[174]={},word[174]={};
    for(int i=0;i<29;++i){mask[i]=1;assisted[i]=i==26?40.f:-40.f;}
    for(int i=74;i<77;++i){mask[i]=1;assisted[i]=(type&(1<<(76-i)))?40.f:-40.f;}
    int hard=-1,iteration=0,checks=83;
    ft8_native::decode(assisted,mask,80,bits.data(),word,hard,iteration,checks,false);
    bool valid=hard>=0&&checks==0;
    if(!valid&&depth>=3)valid=osd(assisted,1,word,hard);
    if(!valid||hard>36||!ft8_native::validCrc(word))continue;
    bool matches=true;
    for(int i=0;i<77;++i)if(mask[i]&&word[i]!=(assisted[i]>0))matches=false;
    if(!matches)continue;
    int8_t reencoded[174];ft8_native::encode(word,reencoded);
    if(!std::equal(word,word+174,reencoded))continue;
    std::copy(word,word+77,bits.begin());return true;
  }
  return false;
}

bool coherent(float const* data,int max_start,int& start,double& frequency,
              double noise,int depth,Bits& bits,bool cq_ap,bool matched_template=false,
              double minimum_coherence=0) {
  int original=start;double original_f=frequency,best=-1;
  struct Model {int start;double frequency,score;};
  std::vector<Model> models;
  models.reserve(matched_template?81*201:11*201);
  std::vector<std::complex<float>> reference;
  if(matched_template) {
    int tones[symbols]={};for(int k=0;k<20;++k)tones[sync_pos[k]]=sync_tone[k];
    reference.resize(symbols*symbol_samples);
    ft8_native::waveform(tones,symbols,symbol_samples,4,12000,float(original_f),
        reference.data(),nullptr,true,int(reference.size()));
  }
  int radius=matched_template?1200:300,increment=matched_template?30:60;
  for(int time=std::max(0,original-radius);time<=std::min(max_start,original+radius);time+=increment) {
    std::complex<double> c[20];
    for(int k=0;k<20;++k) {
      c[k]={};
      if(matched_template) {
        int first=(k==0||k==7||k==14)?symbol_samples/2:0;
        int last=(k==6||k==13||k==19)?symbol_samples/2:symbol_samples;
        for(int i=first;i<last;++i) {
          int j=sync_pos[k]*symbol_samples+i;
          c[k]+=double(data[time+j])*std::conj(std::complex<double>(reference[j]));
        }
      } else c[k]=correlation(data+time+sync_pos[k]*symbol_samples,original_f+5*sync_tone[k])
          *std::polar(1.,-2*pi*original_f*sync_pos[k]/5.);
    }
    for(int df=-100;df<=100;++df) {
      double offset=df*.01;std::complex<double> sum{};
      for(int k=0;k<20;++k)sum+=c[k]*std::polar(1.,-2*pi*offset*(sync_pos[k]+.5)/5.);
      double score=std::norm(sum);best=std::max(best,score);
      models.push_back({time,original_f+offset,score});
    }
  }
  if(matched_template&&best<minimum_coherence*17*noise)return false;
  std::stable_sort(models.begin(),models.end(),[](Model const& a,Model const& b){return a.score>b.score;});
  std::vector<Model> tried;
  std::vector<Model> soft_models;
  std::vector<std::array<float,174>> soft;
  for(auto const& model:models) {
  if(model.score<.85*best)break;
  bool repeated=false;for(auto const& old:tried)if(std::abs(model.frequency-old.frequency)<.08)repeated=true;
  if(repeated)continue;
  if(tried.size()>=size_t(depth>=3?3:1))break;
  tried.push_back(model);
  int trial_start=model.start;double trial_frequency=model.frequency;
  std::complex<double> sum{};
  if(matched_template) {
    int tones[symbols]={};for(int k=0;k<20;++k)tones[sync_pos[k]]=sync_tone[k];
    ft8_native::waveform(tones,symbols,symbol_samples,4,12000,float(trial_frequency),
        reference.data(),nullptr,true,int(reference.size()));
    for(int k=0;k<20;++k) {
      int first=(k==0||k==7||k==14)?symbol_samples/2:0;
      int last=(k==6||k==13||k==19)?symbol_samples/2:symbol_samples;
      for(int i=first;i<last;++i) {
        int j=sync_pos[k]*symbol_samples+i;
        sum+=double(data[trial_start+j])*std::conj(std::complex<double>(reference[j]));
      }
    }
  }
  else for(int k=0;k<20;++k)sum+=correlation(data+trial_start+sync_pos[k]*symbol_samples,trial_frequency+5*sync_tone[k])
       *std::polar(1.,-2*pi*trial_frequency*sync_pos[k]/5.);
  if(std::abs(sum)<1e-15)continue;
  auto phase=std::conj(sum/std::abs(sum));
  double amplitude=std::abs(sum)/(matched_template?17:20);
  float llr[174]={};int at=0;
  for(int s=0;s<symbols;++s)if(sync_index(s)<0) {
    double metric[16];
    auto rotation=phase*std::polar(1.,-2*pi*trial_frequency*s/5.);
    for(int w=0;w<16;++w)metric[w]=2*amplitude/(noise)
       *std::real(correlation(data+trial_start+s*symbol_samples,trial_frequency+5*gray(w))*rotation);
    for(int b=0;b<4;++b,++at)if(at<174) {
      double one=-1e100,zero=-1e100;
      for(int w=0;w<16;++w){double& v=(w&(1<<b))?one:zero;v=log_add(v,metric[w]);}
      llr[(43*at)%174]=float(std::max(-24.,std::min(24.,one-zero)));
    }
  }
  std::array<float,174> saved;std::copy(llr,llr+174,saved.begin());soft.push_back(saved);soft_models.push_back(model);
  int8_t cw[174]={};int hard=-1,checks=83;float reliable[174]={};
  bool valid=ft8_native::decodeReliability(llr,bits.data(),cw,reliable,hard);
  if(!valid&&depth>=2) valid=osd(llr,depth>=3?2:1,cw,hard)||osd(reliable,depth>=3?2:1,cw,hard);
  if(valid){std::copy(cw,cw+77,bits.begin());checks=0;}
  if(hard<0||hard>36||checks||!ft8_native::validCrc(cw)||std::none_of(cw,cw+174,[](int8_t b){return b!=0;}))continue;
  int8_t encoded[174];ft8_native::encode(bits.data(),encoded);
  if(std::equal(cw,cw+174,encoded)){start=trial_start;frequency=trial_frequency;return true;}
  }
  if(cq_ap)for(size_t i=0;i<soft.size();++i)if(cq_ap_decode(soft[i].data(),depth,bits)) {
    start=soft_models[i].start;frequency=soft_models[i].frequency;return true;
  }
  return false;
}


struct Spectrum {
  float* in; fftwf_complex* out; fftwf_plan plan;
  Spectrum():in(fftwf_alloc_real(fft_size)),out(fftwf_alloc_complex(bins)),plan(nullptr) {
    if(!in||!out){fftwf_free(in);fftwf_free(out);throw std::bad_alloc();}
    std::lock_guard<std::mutex> lock(planner_mutex());
    plan=fftwf_plan_dft_r2c_1d(fft_size,in,out,FFTW_ESTIMATE);
    if(!plan){fftwf_free(in);fftwf_free(out);throw std::bad_alloc();}
  }
  ~Spectrum(){std::lock_guard<std::mutex> lock(planner_mutex());fftwf_destroy_plan(plan);fftwf_free(in);fftwf_free(out);}
  Spectrum(Spectrum const&)=delete; Spectrum& operator=(Spectrum const&)=delete;
};
struct Candidate { int start;double frequency,score; };
}
std::vector<int> encode_tones(Bits const& bits) {
  for(auto bit:bits) if(bit!=0&&bit!=1) throw std::invalid_argument("REIST16-QSO/2 message bit");
  int8_t code[174];ft8_native::encode(bits.data(),code);
  std::vector<int> tones(symbols);int at=0;
  for(int s=0;s<symbols;++s) {
    int sync=sync_index(s);
    if(sync>=0){tones[s]=sync_tone[sync];continue;}
    int word=0;
    for(int b=0;b<4;++b,++at) if(at<174) word|=int(code[(43*at)%174])<<b;
    tones[s]=gray(word);
  }
  return tones;
}
std::vector<float> waveform(Bits const& bits,double frequency,int rate) {
  if((rate!=12000&&rate!=48000)||!std::isfinite(frequency)||frequency<100||frequency>4000)
    throw std::invalid_argument("REIST16-QSO/2 audio range");
  auto tones=encode_tones(bits);int n=rate/5;
  std::vector<float> wave(symbols*n);
  ft8_native::waveform(tones.data(),symbols,n,4.f,float(rate),float(frequency),nullptr,wave.data(),false,int(wave.size()));
  // Match the unit-peak FT8 generator. Output attenuation belongs to the
  // common audio path; a second mode-specific factor cost REIST16 1.94 dB.
  return wave;
}
static double payload_support(float const* data,int count,int& start,double frequency,Bits const& bits,double& uncertainty) {
  // Predict the data's coherent amplitude from sync symbols only. A CRC hit
  // among overlapping stations must also be supported by the actual payload.
  // Central samples avoid transition/ramp bias. The phase slope comes from
  // sync. Only models close to the best sync peak are admissible; clustered
  // sync can have ambiguous frequency/phase peaks at low SNR.
  auto tones=encode_tones(bits);
  std::vector<std::complex<float>> reference(symbols*symbol_samples);
  ft8_native::waveform(tones.data(),symbols,symbol_samples,4,12000,float(frequency),
      reference.data(),nullptr,true,int(reference.size()));
  auto evaluate=[&]() {
  std::complex<double> c[symbols]{};
  for(int s=0;s<symbols;++s)for(int i=300;i<symbol_samples-300;++i) {
    int j=s*symbol_samples+i;
    c[s]+=double(data[start+j])*std::conj(std::complex<double>(reference[j]));
  }
  struct Model {double offset,drift,power;std::complex<double> sync;};
  std::vector<Model> models;models.reserve(1701);
  double best=-1;
  for(int slope=-10;slope<=10;++slope)for(int df=-40;df<=40;++df) {
    double hz=df*.0125,rate=slope*.005;std::complex<double> sum{};
    for(int s:sync_pos) {
      double t=(s+.5)/5.;sum+=c[s]*std::polar(1.,-2*pi*(hz*t+.5*rate*t*t));
    }
    double power=std::norm(sum);best=std::max(best,power);
    models.push_back({hz,rate,power,sum});
  }
  double support=-1e100;
  for(auto const& model:models) {
  if(model.power<.85*best)continue;
  double offset=model.offset,drift=model.drift;auto sync=model.sync;
  std::complex<double> payload{};
  for(int s=0;s<symbols;++s)if(sync_index(s)<0) {
    double t=(s+.5)/5.;payload+=c[s]*std::polar(1.,-2*pi*(offset*t+.5*drift*t*t));
  }
  double amplitude=std::abs(sync)/20,variance=0;
  auto phase=std::conj(sync)/std::max(std::abs(sync),1e-20);
  for(int s:sync_pos) {
    double t=(s+.5)/5.;
    double v=std::real(c[s]*std::polar(1.,-2*pi*(offset*t+.5*drift*t*t))*phase)-amplitude;
    variance+=v*v;
  }
  double ratio=std::real(payload*std::conj(sync))/std::max(std::norm(sync),1e-20)*20./44;
  if(ratio>support) {
    support=ratio;
    uncertainty=std::sqrt(variance/17*(1./20+1./44))/std::max(amplitude,1e-20);
  }
  }
  return support;
  };
  double support=evaluate();
  if(support<.7&&support+3*uncertainty<.9) {
    // An energy-based start estimate can be good enough for noncoherent
    // decoding but not for phase comparison across different FSK tones.
    start=refine_start(data,count,reference,start,true);
    support=evaluate();
  }
  return support;
}

static std::vector<Decode> decode_pass(std::vector<float>& residual,double low,double high,int depth,
                                     std::vector<Decode> result,int budget,int& coherent_failures,
                                     int coherent_limit,bool cq_ap,bool& changed) {
  float const* data=residual.data();int count=int(residual.size());
  int first=int(std::floor(low/bin_hz)),last=int(std::ceil(high/bin_hz));
  int max_start=std::min(24000,count-symbols*symbol_samples);
  int rows=(count-symbol_samples)/step+1;
  Spectrum fft;std::vector<float> powers(size_t(rows)*bins);
  for(int row=0;row<rows;++row) {
    std::copy(data+row*step,data+row*step+symbol_samples,fft.in);
    std::fill(fft.in+symbol_samples,fft.in+fft_size,0);fftwf_execute(fft.plan);
    for(int k=0;k<bins;++k)powers[size_t(row)*bins+k]=fft.out[k][0]*fft.out[k][0]+fft.out[k][1]*fft.out[k][1];
  }
  std::vector<Candidate> candidates;
  for(int time=0;time<=max_start/step;++time)for(int bin=first;bin<=last;++bin) {
    double wanted=0,other=0;
    for(int k=0;k<20;++k) {
      auto p=powers.data()+size_t(time+4*sync_pos[k])*bins+bin;
      wanted+=p[2*sync_tone[k]];
      other+=.5*(p[2*((sync_tone[k]+5)%16)]+p[2*((sync_tone[k]+8)%16)]);
    }
    double score=wanted/std::max(other,1e-20);
    if(score>1.8&&wanted>1e-16)candidates.push_back({time*step,bin*bin_hz,score});
  }
  std::stable_sort(candidates.begin(),candidates.end(),[](Candidate const& a,Candidate const& b){return a.score>b.score;});
  std::vector<Candidate> tried;
  // A decoded station must not consume the budget for finding another one.
  // Bound failed hypotheses separately so empty/noisy slots stay inexpensive.
  int failures=0;
  for(auto const& candidate:candidates) {
    bool duplicate=false;
    for(auto const& old:tried)if(std::abs(old.frequency-candidate.frequency)<3.75&&std::abs(old.start-candidate.start)<symbol_samples)duplicate=true;
    if(duplicate)continue;
    for(auto const& old:result)if(std::abs(old.frequency-candidate.frequency)<3.75&&
        std::abs((old.dt+start_delay)*sample_rate-candidate.start)<symbol_samples)duplicate=true;
    if(duplicate)continue;
    if(failures>=budget||tried.size()>=256||result.size()>=max_decodes)break;
    tried.push_back(candidate);
    ++failures;
    int start=candidate.start;double frequency=candidate.frequency,best=-1;
    // Alternate time and frequency refinement instead of evaluating their
    // Cartesian product. The coarse FFT is already within half a 2.5-Hz bin.
    for(int time=std::max(0,start-step);time<=std::min(max_start,candidate.start+step);time+=120) {
      double score=sync_energy(data+time,frequency);
      if(score>best){best=score;start=time;}
    }
    for(int f=-3;f<=3;++f) {
      double hz=candidate.frequency+f*.625,score=sync_energy(data+start,hz);
      if(score>best){best=score;frequency=hz;}
    }
    int coarse=start;double coarse_f=frequency;
    for(int time=std::max(0,coarse-120);time<=std::min(max_start,coarse+120);time+=30) {
      double score=sync_energy(data+time,frequency);
      if(score>best){best=score;start=time;}
    }
    for(int f=-2;f<=2;++f) {
      double hz=coarse_f+f*.15625,score=sync_energy(data+start,hz);
      if(score>best){best=score;frequency=hz;}
    }
    if(frequency<low-1||frequency>high+1)continue;
    double noise=0;int sync_hits=0;
    for(int k=0;k<20;++k) {
      double powers[16];
      for(int tone=0;tone<16;++tone) {
        powers[tone]=energy(data+start+sync_pos[k]*symbol_samples,frequency+tone_spacing*tone);
        if(tone!=sync_tone[k])noise+=powers[tone];
      }
      if(std::max_element(powers,powers+16)-powers==sync_tone[k])++sync_hits;
    }
    // Weak sync is allowed only through the exact coherent template, with
    // additional coherent-energy and observed data-diversity checks below.
    bool weak_sync=sync_hits<7;
    if(weak_sync&&(depth<3||sync_hits<5))continue;
    noise=std::max(noise/300,1e-20);
    double signal=std::max(best/20-noise,.1*noise);
    float llr[174]={};int at=0;unsigned observed_tones=0;
    for(int s=0;s<symbols;++s)if(sync_index(s)<0) {
      double metric[16];
      for(int w=0;w<16;++w)metric[w]=log_i0(2*std::sqrt(signal*energy(data+start+s*symbol_samples,frequency+tone_spacing*gray(w)))/noise);
      observed_tones|=1u<<gray(int(std::max_element(metric,metric+16)-metric));
      for(int b=0;b<4;++b,++at)if(at<174) {
        double one=-1e100,zero=-1e100;
        for(int w=0;w<16;++w){double& v=(w&(1<<b))?one:zero;v=log_add(v,metric[w]);}
        llr[(43*at)%174]=float(std::max(-24.,std::min(24.,one-zero)));
      }
    }
    int diversity=0;for(unsigned mask=observed_tones;mask;mask>>=1)diversity+=mask&1u;
    if(weak_sync&&diversity<6)continue;
    int8_t mask[174]={},cw[174]={};int hard=-1,iter=0,checks=83,maximum=60;
    Decode found{};
    if(!weak_sync)for(float scale:{1.f,.65f,1.5f}) {
      float scaled[174];for(int i=0;i<174;++i)scaled[i]=llr[i]*scale;
      ft8_native::decode(scaled,mask,maximum,found.bits.data(),cw,hard,iter,checks,false);
      if(hard>=0)break;
    }
    float reliable[174]={};
    if(!weak_sync&&hard<0&&ft8_native::decodeReliability(llr,found.bits.data(),cw,reliable,hard))checks=0;
    if(!weak_sync&&hard<0&&depth>=2&&(osd(llr,depth>=3?2:1,cw,hard)||osd(reliable,depth>=3?2:1,cw,hard))){
      std::copy(cw,cw+77,found.bits.begin());checks=0;
    }
    bool valid=hard>=0&&hard<=36&&checks==0&&ft8_native::validCrc(cw)&&std::any_of(cw,cw+174,[](int8_t b){return b!=0;});
    if(valid){int8_t reencoded[174];ft8_native::encode(found.bits.data(),reencoded);valid=std::equal(cw,cw+174,reencoded);}
    bool used_coherent=false;
    // If no station has been found, no cancellation pass can follow. Spend
    // the remaining coherent budget now instead of stranding it unused.
    int available_coherent=result.empty()?12:coherent_limit;
    if(!valid&&coherent_failures<available_coherent) {
      used_coherent=true;++coherent_failures;
      if(weak_sync)valid=coherent(data,max_start,start,frequency,noise,depth,found.bits,cq_ap,true,16);
      else {
        valid=coherent(data,max_start,start,frequency,noise,depth,found.bits,cq_ap);
        if(!valid&&depth>=3)valid=coherent(data,max_start,start,frequency,noise,depth,found.bits,cq_ap,true);
      }
    }
    if(!valid&&!weak_sync&&cq_ap)valid=cq_ap_decode(llr,depth,found.bits);
    if(!valid)continue;
    int message_type=4*found.bits[74]+2*found.bits[75]+found.bits[76];
    if(message_type!=1&&message_type!=2)continue;
    double uncertainty=0,support=payload_support(data,count,start,frequency,found.bits,uncertainty);
    // Require a substantial deficit beyond the measured sync scatter before
    // rejecting: weak/noisy or fading frames need more amplitude tolerance.
    // This is a receiver acceptance guard, not a formal false-alarm bound.
    if(support<.7&&support+3*uncertainty<.9)continue;
    found.frequency=frequency;found.dt=double(start)/sample_rate-start_delay;
    found.snr=std::max(-50.,std::min(49.,10*std::log10(signal/noise*5./2500)));
    found.sync=candidate.score;found.sequence=0; // no sequence or header overhead in QSO/2
    bool repeated=false;
    for(auto const& old:result)if(old.bits==found.bits&&std::abs(old.frequency-frequency)<5)repeated=true;
    if(!repeated) {
      result.push_back(found);--failures;
      if(used_coherent)--coherent_failures;
      // Clean the samples immediately: later candidates in this very pass
      // can benefit, even before their synchronization spectrum is refreshed.
      if(depth>=2)changed=subtract(residual,found)||changed;
    }
  }
  std::sort(result.begin(),result.end(),[](Decode const& a,Decode const& b){return a.frequency<b.frequency;});
  return result;
}

std::vector<Decode> decode(float const* data,int count,double low,double high,int depth,bool cq_ap) {
  if(!data||count<symbols*symbol_samples||count>180000||!std::isfinite(low)||!std::isfinite(high))return {};
  for(int i=0;i<count;++i)if(!std::isfinite(data[i]))return {};
  low=std::max(100.,low);high=std::min(4000.,high);if(low>high)return {};
  std::vector<float> residual(data,data+count);
  std::vector<Decode> result;
  int passes=depth>=3?3:depth>=2?2:1,coherent_failures=0;
  for(int pass=0;pass<passes;++pass) {
    size_t previous=result.size();bool changed=false;
    // Keep the total failed-hypothesis work bounded even after cancellation.
    int budget=depth>=3?16:12;
    // Reserve coherent failure work for later, cleaner spectra too. Valid
    // coherent decodes, like valid noncoherent ones, do not exhaust the search.
    result=decode_pass(residual,low,high,depth,std::move(result),budget,coherent_failures,
                       (pass+1)*12/passes,cq_ap,changed);
    if(result.size()==previous||result.size()>=max_decodes||!changed)break;
  }
  return result;
}
} }
extern "C" int reist16_qso2_encode(int8_t const* bits,int /*sequence*/,float frequency,float* output,int capacity) {
  if(!bits||!output||capacity<reist16::qso2::tx_samples)return -1;
  try {reist16::Bits b;std::copy(bits,bits+77,b.begin());auto wave=reist16::qso2::waveform(b,frequency);
    std::copy(wave.begin(),wave.end(),output);return int(wave.size());}catch(...){return -1;}
}
