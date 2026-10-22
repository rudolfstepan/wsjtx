#ifndef WSJTX_REIST16_QSO2_HPP
#define WSJTX_REIST16_QSO2_HPP
#include "reist16.hpp"
namespace reist16 { namespace qso2 {
constexpr int symbol_samples = 2400;
constexpr int symbols = 64;
constexpr int tx_samples = 614400;
constexpr double frame_seconds = 12.8;
constexpr double tone_spacing = 5.;
constexpr double tone_span = 75.;
std::vector<int> encode_tones(Bits const& bits);
std::vector<float> waveform(Bits const& bits, double frequency, int rate = 48000);
std::vector<Decode> decode(float const*, int count, double low, double high, int depth = 1);
} }
extern "C" int reist16_qso2_encode(int8_t const*, int sequence, float frequency, float*, int capacity);
#endif
