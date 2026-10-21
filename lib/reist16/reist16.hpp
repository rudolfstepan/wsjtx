#ifndef WSJTX_REIST16_HPP
#define WSJTX_REIST16_HPP
#include <array>
#include <cstdint>
#include <vector>

// REIST16-QSO/1: the REIST Weak16/CFP2 channel code with a fixed 77-bit
// standard-QSO payload. Independent of FT8's LDPC and modulation.
namespace reist16 {
constexpr int sample_rate = 12000;
constexpr int symbol_samples = 960;
constexpr int symbols = 120;
constexpr int tx_samples = 460800; // 48 kHz, including two silent symbols
constexpr int mode_id = 116;
constexpr uint32_t profile_tag = 0x31515352; // little-endian "RSQ1"
constexpr double period = 15.0;
constexpr double start_delay = 0.5;
constexpr double frame_seconds = 9.6;
constexpr double tone_spacing = 25.0;
constexpr double tone_span = 375.0;
using Bits = std::array<int8_t, 77>;
struct Decode {
  Bits bits;
  double frequency;
  double dt;
  double snr;
  double sync;
  uint16_t sequence;
};
std::vector<int> encode_tones(Bits const&, uint16_t sequence);
std::vector<float> waveform(Bits const&, uint16_t sequence, double frequency, int rate = 48000);
std::vector<Decode> decode(float const* samples, int count, double low, double high, int depth = 1);
}

extern "C" {
int reist16_encode(int8_t const* bits, int sequence, float frequency, float* output, int capacity);
int reist16_decode(int16_t const* samples, int count, int low, int high, int depth,
                   int8_t* bits, float* frequencies, float* times, float* snrs, int capacity);
}
#endif
