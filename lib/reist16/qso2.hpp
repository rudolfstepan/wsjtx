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
constexpr int default_tx_frequency = 1000;
// In split mode all 16 tone centres fit inside 1500..2000 Hz, even at
// the upper end of a tuning step. The RF position remains unchanged.
constexpr int split_offset(int requested_frequency) {
  return (requested_frequency / 250) * 250 - 1500;
}
// Live receiver FFT cadence is independent of the 2400-sample wire symbols.
// Receive-only monitoring can spend the reply budget on delayed WebSDR audio.
// Keep the earlier start in transceive mode for decoding and automatic replies.
constexpr int rx_block_samples = 3456;
constexpr int rx_half_symbols = 48;
constexpr int rx_capture_samples = rx_half_symbols * rx_block_samples;
constexpr int monitor_half_symbols = 50;
constexpr int monitor_capture_samples = monitor_half_symbols * rx_block_samples;
constexpr int capture_half_symbols(bool receive_only) {
  return receive_only ? monitor_half_symbols : rx_half_symbols;
}
std::vector<int> encode_tones(Bits const& bits);
std::vector<float> waveform(Bits const& bits, double frequency, int rate = 48000);
std::vector<Decode> decode(float const*, int count, double low, double high, int depth = 1,
                           bool cq_ap = true);
} }
extern "C" int reist16_qso2_encode(int8_t const*, int sequence, float frequency, float*, int capacity);
#endif
