#include "lib/reist16/qso2.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
extern "C" void genft8_(char*,int*,int*,char*,char*,int*,size_t,size_t);
namespace {
constexpr int rate=12000,count=180000,offset=6000;
constexpr double baseGain=.01;
double quietGain=std::pow(10.,-89./20.);
struct Frame {std::string message;reist16::Bits bits;std::vector<float> wave,noise;double power;};
void require(bool ok,char const* why){if(!ok)throw std::runtime_error(why);}
void wav(std::string const& file,std::vector<float> const& data,bool floating){
 std::ofstream f(file,std::ios::binary);require(bool(f),"WAV open failed");
 auto word=[&](unsigned x,int n){for(int i=0;i<n;i++)f.put(char(x>>(8*i)));};
 unsigned bytes=floating?4:2;
 f.write("RIFF",4);word(36+bytes*data.size(),4);f.write("WAVEfmt ",8);word(16,4);word(floating?3:1,2);word(1,2);
 word(rate,4);word(rate*bytes,4);word(bytes,2);word(bytes*8,2);f.write("data",4);word(bytes*data.size(),4);
 for(float x:data){require(std::isfinite(x)&&std::abs(x)<1,"WAV clipping/nonfinite");if(floating)f.write(reinterpret_cast<char*>(&x),4);else word(unsigned(int16_t(std::lround(x*32768))),2);}
 require(bool(f),"WAV write failed");
}
std::vector<float> mix(Frame const& f,double snr,bool clean=false){
 double sigma=clean?0:std::sqrt(f.power*baseGain*baseGain*6000/2500/std::pow(10.,snr/10.));
 std::vector<float> out(count);for(int i=0;i<count;i++)out[i]=float(sigma*f.noise[i]);
 for(size_t i=0;i<f.wave.size();i++)out[offset+i]+=float(baseGain*f.wave[i]);
 // All exported variants share this exact PCM16 realization and sample clock.
 for(auto& x:out){require(std::abs(x)<.99,"PCM headroom");x=float(std::lround(x*32768))/32768;}
 return out;
}
bool decode(std::vector<float> const& pcm,Frame const& frame,int& wrong){
 auto found=reist16::qso2::decode(pcm.data(),int(pcm.size()),100,4000,3);bool exact=false;
 for(auto const& d:found){if(d.bits==frame.bits)exact=true;else wrong++;}return exact;
}
std::vector<float> attenuate(std::vector<float> pcm){for(auto& x:pcm)x=float(x*quietGain/baseGain);return pcm;}
}
int main(){try{
 std::vector<Frame> frames;
 std::mt19937 rng(2026102301u);std::normal_distribution<float> gaussian;
 for(std::string text:{"CQ OE3SRO JN88","OE3SRO DL1ABC JO31","DL1ABC OE3SRO -12","OE3SRO DL1ABC R-10","DL1ABC OE3SRO RR73","OE3SRO DL1ABC 73"}){
  Frame f;f.message=text;text.resize(37,' ');char sent[37],packed[77];int tones[79],i3=0,n3=0;
  genft8_(&text[0],&i3,&n3,sent,packed,tones,37,37);require(i3==1,"QSO pack failed");
  for(int i=0;i<77;i++)f.bits[i]=packed[i];f.wave=reist16::qso2::waveform(f.bits,1500.375,rate);
  f.power=0;for(float x:f.wave)f.power+=double(x)*x;f.power/=f.wave.size();
  f.noise.resize(count);double sum=0;for(auto& x:f.noise){x=gaussian(rng);sum+=double(x)*x;}double rms=std::sqrt(sum/count);for(auto& x:f.noise)x=float(x/rms);
  frames.push_back(std::move(f));
 }
 std::ofstream report("sweep.jsonl");int falseDecodes=0;double threshold=0;bool foundThreshold=false;int lastGood=-1;
 std::vector<int> scores;
 for(int step=0;step<=10;step++){
  double snr=-18.-step;int exact=0;for(auto const& frame:frames){auto pcm=mix(frame,snr);bool a=decode(pcm,frame,falseDecodes);bool b=decode(attenuate(pcm),frame,falseDecodes);if(a&&b)exact++;}
  scores.push_back(exact);if(exact==6){lastGood=step;threshold=snr;foundThreshold=true;}
  report<<"{\"snr2500_db\":"<<snr<<",\"exact_both_variants\":"<<exact<<",\"slots\":6}\n";report.flush();
  std::cout<<"SNR2500 "<<snr<<" dB: "<<exact<<"/6 in both PCM16 and attenuated Float32"<<std::endl;
  if(lastGood>=0&&step>=lastGood+2&&exact<6)break;
 }
 require(foundThreshold,"No complete decodable QSO found");require(lastGood+1<int(scores.size())&&scores[lastGood+1]<6,"No threshold transition found");
 std::ofstream manifest("slots.jsonl");
 for(std::string name:{"clean","reference","threshold","below"}){
  double snr=name=="reference"?-12:name=="below"?threshold-2:threshold;
  std::vector<float> sequence;std::vector<float> sequenceQuiet;int total=0,totalQuiet=0;
  for(size_t slot=0;slot<frames.size();slot++){
   auto pcm=mix(frames[slot],snr,name=="clean");auto quiet=attenuate(pcm);
   bool a=decode(pcm,frames[slot],falseDecodes),b=decode(quiet,frames[slot],falseDecodes);total+=a;totalQuiet+=b;
   std::string stem=name+"_slot"+std::to_string(slot+1);
   wav(stem+"_000000.wav",pcm,false);wav(stem+"_minus89dB_f32.wav",quiet,true);
   sequence.insert(sequence.end(),pcm.begin(),pcm.end());sequenceQuiet.insert(sequenceQuiet.end(),quiet.begin(),quiet.end());
   manifest<<"{\"variant\":\""<<name<<"\",\"slot\":"<<slot+1<<",\"message\":\""<<frames[slot].message<<"\",\"snr2500_db\":";
   if(name=="clean")manifest<<"null";else manifest<<snr;
   manifest<<",\"pcm16_exact\":"<<a<<",\"minus89_f32_exact\":"<<b<<",\"pcm16\":\""<<stem<<"_000000.wav\",\"float32\":\""<<stem<<"_minus89dB_f32.wav\"}\n";
  }
  wav(name+"_sequence_90s_PCM16.wav",sequence,false);wav(name+"_sequence_90s_minus89dB_Float32.wav",sequenceQuiet,true);
  std::cout<<"EXPORT "<<name<<" "<<total<<"/6 PCM16 "<<totalQuiet<<"/6 Float32"<<std::endl;
  if(name!="below")require(total==6&&totalQuiet==6,"Positive export decode failed");else require(total<6&&totalQuiet<6,"Below-threshold negative not demonstrated");
 }
 std::ofstream summary("generation.json");summary<<"{\"seed\":2026102301,\"rate\":12000,\"frequency_hz\":1500.375,\"start_seconds\":0.5,\"slot_seconds\":15,\"threshold_snr2500_db\":"<<threshold<<",\"attenuation_db\":89,\"pcm16_gain\":0.01,\"float32_gain\":"<<quietGain<<",\"false_decodes\":"<<falseDecodes<<"}\n";
 require(falseDecodes==0,"Unexpected payload in test corpus");
 }catch(std::exception const& e){std::cerr<<e.what()<<std::endl;return 1;}return 0;}
