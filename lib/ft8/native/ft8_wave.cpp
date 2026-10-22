// FT8/FT4/FT2 waveform ports of gen_ft8wave/gen_ft4wave (WSJT-X, GPLv3).
// Centered phase accumulation applies the bounded-state correction principle
// discussed in Rudolf Stepan's REIST-Division, sections 3 and 5. The paper's
// integer benchmark results are not claims about this floating-point kernel.
#include "ft8_wave.hpp"
#include <algorithm>
#include <cmath>
#include <memory>
#include <mutex>
#include <vector>

namespace ft8_native
{
namespace
{
double const pi = std::acos (-1.);
double const twoPi = 2. * pi;
int const tableSize = 65536;
std::vector<std::complex<float>> const& sineTable ()
{
  static auto const table = [] {
    std::vector<std::complex<float>> values (tableSize + 1);
    for (int i = 0; i <= tableSize; ++i) {
      double angle = twoPi * i / tableSize;
      values[i] = {float (std::cos (angle)), float (std::sin (angle))};
    }
    return values;
  } ();
  return table;
}
struct Pulse
{
  int nsps = 0;
  float bt = 0.f;
  std::vector<double> values;
  void prepare (int count, float bandwidth)
  {
    if (nsps == count && bt == bandwidth) return;
    nsps = count;
    bt = bandwidth;
    values.resize (3 * nsps);
    double c = pi * std::sqrt (2. / std::log (2.));
    for (int i = 0; i < 3 * nsps; ++i) {
      double t = (i + 1. - 1.5 * nsps) / nsps;
      values[i] = 0.5 * (std::erf (c * bt * (t + .5)) - std::erf (c * bt * (t - .5)));
    }
  }
};

std::shared_ptr<Pulse const> pulseFor (int nsps, float bt)
{
  // Cache only immutable pulses. The lock is held once per waveform, never in
  // the sample loop. Ownership remains valid even when an entry is evicted.
  static std::mutex mutex;
  static std::vector<std::shared_ptr<Pulse const>> cache;
  std::lock_guard<std::mutex> lock {mutex};
  for (auto const& pulse : cache)
    if (pulse->nsps == nsps && pulse->bt == bt) return pulse;
  auto pulse = std::make_shared<Pulse> ();
  pulse->prepare (nsps, bt);
  if (cache.size () == 8) cache.erase (cache.begin ());
  cache.push_back (pulse);
  return pulse;
}
}

void waveform (int const *tones, int symbols, int nsps, float bt, float fs,
               float frequency, std::complex<float> *complexWave,
               float *realWave, bool complexOutput, int samples, FrameShape shape)
{
  bool const ft4 = shape == FrameShape::Ft4;
  if (symbols < 1 || nsps < 1 || samples < 1 || fs <= 0.f) return;
  int const frame = (symbols + (ft4 ? 2 : 0)) * nsps;
  if (!ft4 && samples > frame) return;
  int const count = std::min (samples, frame);
  // FT2 may reuse a buffer sized for FT4; never leave an old waveform tail.
  if (complexOutput) std::fill (complexWave + count, complexWave + samples, std::complex<float> {});
  else std::fill (realWave + count, realWave + samples, 0.f);
  auto const pulse = pulseFor (nsps, bt);
  std::vector<double> increments ((symbols + 2) * nsps, 0.);
  double peak = twoPi / nsps;
  for (int symbol = 0; symbol < symbols; ++symbol) {
    double scale = peak * tones[symbol];
    auto dest = increments.data () + symbol * nsps;
    for (int i = 0; i < 3 * nsps; ++i) dest[i] += scale * pulse->values[i];
  }
  for (int i = 0; !ft4 && i < 2 * nsps; ++i) {
    increments[i] += peak * tones[0] * pulse->values[nsps + i];
    increments[symbols * nsps + i] += peak * tones[symbols - 1] * pulse->values[i];
  }
  double base = twoPi * frequency / fs;
  double phase = 0.;
  auto const& table = sineTable ();
  double const tableScale = tableSize / twoPi;
  for (int k = 0; k < count; ++k) {
    double position = (phase < 0. ? phase + twoPi : phase) * tableScale;
    int index = int (position);
    float fraction = float (position - index);
    if (index >= tableSize) { index = 0; fraction = 0.f; }
    // Linear interpolation improves on the old nearest-table complex path.
    auto value = table[index] + fraction * (table[index + 1] - table[index]);
    if (complexOutput) complexWave[k] = value;
    else realWave[k] = value.imag ();
    double step = increments[(ft4 ? 0 : nsps) + k] + base;
    // Handle arbitrary caller frequencies before entering the bounded update.
    if (step > pi || step < -pi) step = std::remainder (step, twoPi);
    phase += step;
    if (phase >= pi) phase -= twoPi;
    else if (phase < -pi) phase += twoPi;
  }
  int ramp = ft4 ? nsps : int (std::round (nsps / 8.));
  for (int i = 0; i < ramp; ++i) {
    float rise = float ((1. - std::cos (pi * i / ramp)) * .5);
    int tail = frame - ramp + i;
    float fall = float ((1. + std::cos (pi * i / ramp)) * .5);
    if (complexOutput) {
      if (i < samples) complexWave[i] *= rise;
      if (tail < samples) complexWave[tail] *= fall;
    } else {
      if (i < samples) realWave[i] *= rise;
      if (tail < samples) realWave[tail] *= fall;
    }
  }
}
}
