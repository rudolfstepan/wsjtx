#include "lib/reist16/reist16.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>

void check(bool yes, char const* message) { if (!yes) throw std::runtime_error(message); }
int main() {
  try {
    std::mt19937 rng(20261022);
    std::normal_distribution<float> normal;
    auto begin = std::chrono::steady_clock::now();
    for (int test = 0; test < 8; ++test) {
      reist16::Bits bits;
      for (auto& bit : bits) bit = rng() & 1;
      double frequency = 300. + test*400. + .375;
      int start = 4200 + 600*test;
      auto wave = reist16::waveform(bits, uint16_t(100+test), frequency, 12000);
      auto tones = reist16::encode_tones(bits, uint16_t(100+test));
      check(wave.size() == 115200 && tones.size() == 120, "frame length");
      check(tones[0]==0 && tones[1]==15 && tones[117]>=0 && tones[118]==-1 && tones[119]==-1, "training and silence");
      check(std::all_of(wave.end()-1920,wave.end(),[](float x){return x==0;}), "zero tail");
      // At 12 kHz, real AWGN occupies 6 kHz. SNR refers to 2500 Hz.
      double gain=.02, snr=-12, sigma=std::sqrt(.32*gain*gain*6000/2500/std::pow(10.,snr/10));
      std::vector<float> audio(180000);
      for (auto& x : audio) x = float(sigma)*normal(rng);
      for (size_t i=0;i<wave.size();++i) audio[start+i] += float(gain)*wave[i];
      auto found = reist16::decode(audio.data(), int(audio.size()), 100, 4000, 2);
      check(found.size()==1, "single weak signal count");
      check(found[0].bits==bits && found[0].sequence==100+test, "exact weak payload/sequence");
      check(std::abs(found[0].frequency-frequency)<1.5, "frequency estimate");
      check(std::abs(found[0].dt-(start/12000.-.5))<.025, "time estimate");
      check(std::abs(found[0].snr-snr)<2.5, "SNR2500 estimate");
      check(reist16::decode(audio.data(),int(audio.size()),frequency+150,frequency+200).empty(), "RX bounds");
    }
    std::vector<float> mixture(180000);
    for (auto& x : mixture) x=.025f*normal(rng);
    std::vector<reist16::Bits> messages(3);
    for (int station=0;station<3;++station) {
      for (auto& bit : messages[station]) bit=rng()&1;
      auto wave=reist16::waveform(messages[station],73+station,650.5+station*850.,12000);
      for (size_t k=0;k<wave.size();++k) mixture[6000+k] += .05f*wave[k];
    }
    auto found=reist16::decode(mixture.data(),int(mixture.size()),200,3500,3);
    check(found.size()==3,"three simultaneous stations");
    for(int station=0;station<3;++station) check(found[station].bits==messages[station],"multi-station exact payload");
    std::vector<float> negative(180000,0);
    // Both FEC blocks are valid, but the body belongs to another sequence.
    // This must fail the CRC over header AND payload.
    auto frameA=reist16::waveform(messages[0],1,1500.,12000);
    auto frameB=reist16::waveform(messages[0],2,1500.,12000);
    std::copy(frameB.begin()+59*960,frameB.end(),frameA.begin()+59*960);
    std::copy(frameA.begin(),frameA.end(),negative.begin()+6000);
    check(reist16::decode(negative.data(),180000,100,4000,3).empty(),"CRC rejects mixed valid FEC blocks");
    std::fill(negative.begin(),negative.end(),0);
    check(reist16::decode(negative.data(),180000,100,4000,3).empty(),"silence");
    for(auto& x:negative)x=.03f*normal(rng);
    check(reist16::decode(negative.data(),180000,100,4000,3).empty(),"noise");
    for(int k=0;k<180000;++k) negative[k] += .1f*std::sin(2*3.141592653589793*1500*k/12000.);
    check(reist16::decode(negative.data(),180000,100,4000,3).empty(),"carrier plus noise");
    negative[100]=std::numeric_limits<float>::quiet_NaN();
    check(reist16::decode(negative.data(),180000,100,4000).empty(),"invalid samples");
    check(reist16::decode(negative.data(),180001,100,4000).empty(),"bounded input");
    std::cout<<"PASS REIST16: 8 weak-signal frames, 3 simultaneous stations, frequency/DT/SNR, bounds and negative controls; "
             <<std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count()<<" s\n";
  } catch(std::exception const& error) { std::cerr<<error.what()<<'\n'; return 1; }
}
