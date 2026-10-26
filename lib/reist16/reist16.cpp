// REIST16 by Rudolf Stepan / OE3SRO. The CFP2 framing, K7 (171,133)
// convolutional code and interleaver follow SDR-suite's cfpm_packet.cpp /
// cfpm_fec.h. This receiver operates directly on WSJT-X's 12-kHz slot audio.
#include "reist16.hpp"
#include "qso2.hpp"
#include <fftw3.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <future>
#include <limits>
#include "planner_lock.hpp"
#include <stdexcept>

namespace reist16 {
namespace {
constexpr double pi = 3.14159265358979323846;
constexpr int training[16] = {0,15,1,14,2,13,3,12,4,11,5,10,6,9,7,8};
constexpr int header_symbols = 43, body_symbols = 59;
constexpr int fft_size = 1920, bins = fft_size / 2 + 1, step = 240;
constexpr double bin_hz = double(sample_rate) / fft_size;
int parity(unsigned v) { v ^= v >> 4; v ^= v >> 2; v ^= v >> 1; return v & 1; }
uint32_t crc(uint8_t const* data, size_t count) {
  uint32_t value = ~0u;
  for (size_t i = 0; i < count; ++i) {
    value ^= data[i];
    for (int b = 0; b < 8; ++b) value = (value >> 1) ^ (0xedb88320u & (0u - (value & 1u)));
  }
  return ~value;
}
void put32(std::vector<uint8_t>& bytes, uint32_t v) {
  for (int i = 0; i < 4; ++i) bytes.push_back(uint8_t(v >> (8 * i)));
}
uint32_t get32(uint8_t const* bytes) {
  uint32_t v = 0;
  for (int i = 0; i < 4; ++i) v |= uint32_t(bytes[i]) << (8 * i);
  return v;
}
std::vector<int> channel_encode(std::vector<uint8_t> const& bytes) {
  std::vector<int> raw, interleaved;
  unsigned state = 0;
  for (int i = 0; i < int(bytes.size()) * 8 + 6; ++i) {
    int bit = i < int(bytes.size()) * 8 ? (bytes[i / 8] >> (i % 8)) & 1 : 0;
    unsigned reg = (state << 1) | unsigned(bit);
    raw.push_back(parity(reg & 0171)); raw.push_back(parity(reg & 0133));
    state = reg & 63;
  }
  for (size_t column = 0; column < 16; ++column)
    for (size_t i = column; i < raw.size(); i += 16) interleaved.push_back(raw[i]);
  return interleaved;
}
std::vector<uint8_t> channel_decode(std::vector<double> const& input, int bytes) {
  int steps = bytes * 8 + 6;
  if (int(input.size()) != 2 * steps) throw std::invalid_argument("REIST16 FEC size");
  std::vector<double> soft(input.size());
  size_t at = 0;
  for (size_t column = 0; column < 16; ++column)
    for (size_t i = column; i < soft.size(); i += 16) soft[i] = input[at++];
  std::array<double,64> score, next;
  score.fill(-1e100); score[0] = 0;
  std::vector<std::array<uint8_t,64>> trace(steps);
  for (int i = 0; i < steps; ++i) {
    next.fill(-1e100);
    for (unsigned state = 0; state < 64; ++state) for (unsigned bit = 0; bit < 2; ++bit) {
      unsigned reg = (state << 1) | bit, dest = reg & 63;
      double value = score[state] + (parity(reg & 0171) ? soft[2*i] : -soft[2*i])
                                  + (parity(reg & 0133) ? soft[2*i+1] : -soft[2*i+1]);
      if (value > next[dest]) { next[dest] = value; trace[i][dest] = uint8_t(state); }
    }
    score = next;
  }
  std::vector<uint8_t> result(bytes, 0);
  unsigned state = 0; // terminated trellis
  for (int i = steps - 1; i >= 0; --i) {
    if (i < bytes * 8) result[i/8] |= uint8_t((state & 1) << (i%8));
    state = trace[i][state];
  }
  return result;
}
double energy(float const* data, double frequency) {
  double coeff = 2 * std::cos(2 * pi * frequency / sample_rate), a = 0, b = 0;
  for (int i = 0; i < symbol_samples; ++i) {
    double v = data[i] + coeff * a - b; b = a; a = v;
  }
  return std::max(0., a*a + b*b - coeff*a*b);
}
double training_energy(float const* data, int start, double frequency) {
  double sum = 0;
  for (int t = 0; t < 16; ++t)
    sum += energy(data + start + t * symbol_samples, frequency + tone_spacing * training[t]);
  return sum;
}
std::vector<uint8_t> decode_block(float const* data, int bytes, double frequency, double reference) {
  int coded = 2 * (8 * bytes + 6), words = (coded + 3) / 4;
  std::vector<double> soft(coded);
  for (int i = 0; i < words; ++i) {
    double scores[16];
    for (int t = 0; t < 16; ++t)
      scores[t] = std::sqrt(std::min(16., energy(data + i * symbol_samples,
                                                frequency + tone_spacing * t) / reference));
    for (int b = 0; b < 4 && 4*i+b < coded; ++b) {
      double zero = -1e100, one = -1e100;
      for (int t = 0; t < 16; ++t) {
        double& best = (t & (1 << b)) ? one : zero;
        best = std::max(best, scores[t]);
      }
      soft[4*i+b] = one-zero;
    }
  }
  return channel_decode(soft, bytes);
}
// Only plan creation/destruction is serialized. Plans and data are call-local.

struct Spectrum {
  float* input;
  fftwf_complex* output;
  fftwf_plan plan;
  Spectrum() : input(fftwf_alloc_real(fft_size)), output(fftwf_alloc_complex(bins)), plan(nullptr) {
    if (!input || !output) { fftwf_free(input); fftwf_free(output); throw std::bad_alloc(); }
    std::lock_guard<std::mutex> lock(planner_mutex());
    plan = fftwf_plan_dft_r2c_1d(fft_size, input, output, FFTW_ESTIMATE);
    if (!plan) { fftwf_free(input); fftwf_free(output); throw std::bad_alloc(); }
  }
  ~Spectrum() {
    std::lock_guard<std::mutex> lock(planner_mutex());
    fftwf_destroy_plan(plan); fftwf_free(input); fftwf_free(output);
  }
  Spectrum(Spectrum const&) = delete;
  Spectrum& operator=(Spectrum const&) = delete;
};
struct Candidate { int start; double frequency, score; };
}

std::vector<int> encode_tones(Bits const& bits, uint16_t sequence) {
  std::vector<uint8_t> payload(10, 0);
  for (int i = 0; i < 77; ++i) {
    if (bits[i] != 0 && bits[i] != 1) throw std::invalid_argument("REIST16 message bit");
    payload[i/8] |= uint8_t(bits[i] << (i%8));
  }
  std::vector<uint8_t> header = {'C','F',2,10,uint8_t(sequence),uint8_t(sequence>>8)};
  put32(header, profile_tag);
  auto checked = header; checked.insert(checked.end(), payload.begin(), payload.end());
  put32(payload, crc(checked.data(), checked.size()));
  std::vector<int> tones(training, training+16);
  for (auto const& block : {header, payload}) {
    auto code = channel_encode(block);
    for (size_t i = 0; i < code.size(); i += 4) {
      int tone = 0;
      for (size_t b = 0; b < 4 && i+b < code.size(); ++b) tone |= code[i+b] << b;
      tones.push_back(tone);
    }
  }
  tones.push_back(-1); tones.push_back(-1);
  return tones;
}
std::vector<float> waveform(Bits const& bits, uint16_t sequence, double frequency, int rate) {
  if ((rate != 12000 && rate != 48000) || !std::isfinite(frequency)
      || frequency < 100 || frequency > 4000) throw std::invalid_argument("REIST16 audio range");
  auto tones = encode_tones(bits, sequence);
  int n = rate * 80 / 1000;
  std::vector<float> result(tones.size() * n, 0);
  double phase = 0;
  for (size_t symbol = 0; symbol < tones.size(); ++symbol) {
    if (tones[symbol] < 0) continue;
    double delta = 2 * pi * (frequency + tone_spacing * tones[symbol]) / rate;
    for (int k = 0; k < n; ++k) {
      result[symbol * n + k] = float(.8 * std::sin(phase));
      phase += delta;
      if (phase >= pi) phase -= 2 * pi;
    }
  }
  // Smooth keying at the ends; frequency transitions remain continuous phase.
  int ramp = rate / 200;
  int active = int(tones.size()-2)*n;
  for (int k=0;k<ramp;++k) {
    float gain=float(.5-.5*std::cos(pi*k/(ramp-1)));
    result[k] *= gain;
    result[active-1-k] *= gain;
  }
  return result;
}

std::vector<Decode> decode(float const* data, int count, double low, double high, int depth) {
  if (!data || count < symbols * symbol_samples || count > 180000
      || !std::isfinite(low) || !std::isfinite(high)) return {};
  for (int i = 0; i < count; ++i) if (!std::isfinite(data[i])) return {};
  low = std::max(100., low); high = std::min(4000., high);
  if (low > high) return {};
  int first_bin = int(std::ceil(low / bin_hz)), last_bin = int(std::floor(high / bin_hz));
  int max_start = std::min(2 * sample_rate, count - symbols * symbol_samples);
  // Acquisition requires only the first 16 known symbols, not a full-slot STFT.
  int rows = max_start / step + 16 * (symbol_samples / step);
  std::vector<float> power(size_t(rows) * bins);
  Spectrum fft;
  for (int row = 0; row < rows; ++row) {
    std::copy(data + row*step, data + row*step + symbol_samples, fft.input);
    std::fill(fft.input + symbol_samples, fft.input + fft_size, 0);
    fftwf_execute(fft.plan);
    for (int k = 0; k < bins; ++k)
      power[size_t(row)*bins+k] = fft.output[k][0]*fft.output[k][0]+fft.output[k][1]*fft.output[k][1];
  }
  std::vector<Candidate> candidates;
  for (int start = 0; start <= max_start / step; ++start) {
    for (int bin = first_bin; bin <= last_bin; ++bin) {
      double wanted = 0, other = 0;
      for (int t = 0; t < 16; ++t) {
        float const* p = power.data() + size_t(start + 4*t) * bins + bin;
        wanted += p[4*training[t]];
        other += .5 * (p[4*((training[t]+5)%16)] + p[4*((training[t]+8)%16)]);
      }
      double score = wanted / std::max(other, 1e-20);
      if (score > 2.0 && wanted > 1e-16) candidates.push_back({start*step, bin*bin_hz, score});
    }
  }
  std::stable_sort(candidates.begin(), candidates.end(), [](Candidate const& a, Candidate const& b) {
    return a.score > b.score;
  });
  std::vector<Candidate> tried;
  std::vector<Decode> result;
  int budget = depth >= 3 ? 96 : depth >= 2 ? 48 : 24;
  for (auto candidate : candidates) {
    bool duplicate = false;
    for (auto const& old : tried)
      if (std::abs(old.frequency-candidate.frequency) < 12.5 && std::abs(old.start-candidate.start) < 960)
        duplicate = true;
    if (duplicate) continue;
    if (int(tried.size()) >= budget || result.size() >= max_decodes) break;
    tried.push_back(candidate);
    double best = -1;
    int start = candidate.start;
    double frequency = candidate.frequency;
    for (int time = std::max(0, candidate.start-step); time <= std::min(max_start, candidate.start+step); time += 60) {
      for (int f = -4; f <= 4; ++f) {
        double hz = candidate.frequency + f * 1.5625;
        double score = training_energy(data, time, hz);
        if (score > best) { best = score; start = time; frequency = hz; }
      }
    }
    int coarse_start = start; double coarse_frequency = frequency;
    for (int time = std::max(0, coarse_start-60); time <= std::min(max_start, coarse_start+60); time += 12) {
      for (int f = -2; f <= 2; ++f) {
        double hz = coarse_frequency + f * .390625;
        double score = training_energy(data, time, hz);
        if (score > best) { best = score; start = time; frequency = hz; }
      }
    }
    if (frequency < low-1 || frequency > high+1) continue;
    double reference = std::max(best / 16, 1e-20);
    auto header = decode_block(data + start + 16*symbol_samples, 10, frequency, reference);
    if (header[0] != 'C' || header[1] != 'F' || header[2] != 2 || header[3] != 10
        || get32(header.data()+6) != profile_tag) continue;
    auto body = decode_block(data + start + (16+header_symbols)*symbol_samples, 14, frequency, reference);
    auto checked = header; checked.insert(checked.end(), body.begin(), body.begin()+10);
    if (get32(body.data()+10) != crc(checked.data(), checked.size()) || (body[9] & 0xe0)) continue;
    Decode found{};
    for (int i = 0; i < 77; ++i) found.bits[i] = (body[i/8] >> (i%8)) & 1;
    found.sequence = uint16_t(header[4] | (unsigned(header[5]) << 8));
    found.frequency = frequency; found.dt = double(start)/sample_rate-start_delay;
    found.sync = candidate.score;
    double noise = 0;
    for (int t = 0; t < 16; ++t) for (int tone = 0; tone < 16; ++tone)
      if (tone != training[t]) noise += energy(data+start+t*symbol_samples, frequency+tone_spacing*tone);
    noise = std::max(noise/(16*15), 1e-20);
    found.snr = 10 * std::log10(std::max(1e-5, (reference/noise-1)*12.5/2500));
    found.snr = std::max(-50., std::min(49., found.snr));
    bool repeated = false;
    for (auto const& old : result)
      if (old.bits == found.bits && std::abs(old.frequency-found.frequency) < 25) repeated = true;
    if (!repeated) result.push_back(found);
  }
  std::sort(result.begin(), result.end(), [](Decode const& a, Decode const& b) { return a.frequency < b.frequency; });
  return result;
}
}

extern "C" int reist16_encode(int8_t const* bits, int sequence, float frequency, float* output, int capacity) {
  if (!bits || !output || capacity < reist16::tx_samples) return -1;
  try {
    reist16::Bits input; std::copy(bits, bits+77, input.begin());
    auto pcm = reist16::waveform(input, uint16_t(sequence), frequency);
    std::copy(pcm.begin(), pcm.end(), output); return int(pcm.size());
  } catch (...) { return -1; }
}
extern "C" int reist16_decode(int16_t const* samples, int count, int low, int high, int depth,
                               int8_t* bits, float* frequencies, float* times, float* snrs, int capacity) {
  return reist16_decode_ex(samples,count,low,high,depth,bits,frequencies,times,snrs,capacity,1);
}
extern "C" int reist16_decode_ex(int16_t const* samples, int count, int low, int high, int depth,
                               int8_t* bits, float* frequencies, float* times, float* snrs, int capacity,int cq_ap) {
  if (!samples || !bits || !frequencies || !times || !snrs || count < 0 || count > 180000 || capacity < 1) return -1;
  try {
    std::vector<float> data(count);
    for (int i = 0; i < count; ++i) data[i] = samples[i] / 32768.f;
    // The legacy receiver has independent state. Preserve compatibility
    // without adding its complete search time to the QSO/2 reply budget.
    std::future<std::vector<reist16::Decode>> legacy_task;
    try {
      legacy_task=std::async(std::launch::async,[&] {
        return reist16::decode(data.data(),count,low,high,depth);
      });
    } catch(std::system_error const&) { /* Fall back if a worker cannot start. */ }
    auto found = reist16::qso2::decode(data.data(), count, low, high, depth,cq_ap!=0);
    auto legacy = legacy_task.valid()?legacy_task.get():reist16::decode(data.data(),count,low,high,depth);
    found.insert(found.end(),legacy.begin(),legacy.end());
    std::sort(found.begin(),found.end(),[](reist16::Decode const& a,reist16::Decode const& b){return a.frequency<b.frequency;});
    int n = std::min(int(found.size()), capacity);
    for (int i = 0; i < n; ++i) {
      std::copy(found[i].bits.begin(), found[i].bits.end(), bits+77*i);
      frequencies[i] = float(found[i].frequency); times[i] = float(found[i].dt); snrs[i] = float(found[i].snr);
    }
    return n;
  } catch (...) { return -1; }
}
