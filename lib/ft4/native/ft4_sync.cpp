// Native sync4d and its independent search grid (WSJT-X, GPLv3).
#include "ft4_sync.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>
#ifdef _OPENMP
#include <omp.h>
#endif

namespace ft4_native
{
namespace
{
using Complex = std::complex<float>;
using References = std::array<std::array<Complex, syncSamples>, 4>;
References const& references ()
{
  static References const value = [] {
    References result {};
    int const tones[4][4] = {{0,1,3,2}, {1,0,2,3}, {2,3,1,0}, {3,2,0,1}};
    float const twoPi = 8.f * std::atan (1.f);
    for (int group = 0; group < 4; ++group) {
      float phase = 0.f;
      for (int symbol = 0; symbol < 4; ++symbol) {
        float step = 2.f * twoPi * tones[group][symbol] / 32.f;
        for (int j = 0; j < 16; ++j) {
          result[group][symbol * 16 + j] = {std::cos (phase), std::sin (phase)};
          phase = std::fmod (phase + step, twoPi);
        }
      }
    }
    return result;
  } ();
  return value;
}
References tweaked (Complex const *tweak, bool apply)
{
  References result = references ();
  if (apply) for (auto& group : result)
    for (int i = 0; i < syncSamples; ++i) group[i] = tweak[i] * group[i];
  return result;
}
float power (Complex const *data, int start, References const& reference)
{
  float total = 0.f;
  for (int group = 0; group < 4; ++group) {
    int offset = start + group * 33 * 32;
    int first = 0, last = syncSamples;
    if (offset < 0 && group == 0) {
      int points = (offset + 127) / 2;
      if (points <= 16) continue;
      first = 63 - points;
      offset = -2 * first;
    } else if (offset >= 0 && offset + 127 < downsampledSamples) {
      // Complete Costas array.
    } else if (group == 3 && offset + 127 >= downsampledSamples) {
      int points = (downsampledSamples - offset) / 2;
      if (points <= 16) continue;
      last = points;
    } else continue;
    Complex sum {};
    for (int i = first; i < last; ++i) sum += data[offset + 2 * i] * std::conj (reference[group][i]);
    float real = sum.real () * (1.f / 64.f), imag = sum.imag () * (1.f / 64.f);
    total += std::sqrt (real * real + imag * imag);
  }
  return total;
}
}

float sync (Complex const *data, int start, Complex const *tweak, bool applyTweak)
{
  return power (data, start, tweaked (tweak, applyTweak));
}
void search (Complex const *data, Complex const *tweaks,
             int frequencyMin, int frequencyMax, int frequencyStep,
             int startMin, int startMax, int startStep,
             int& bestStart, int& bestFrequency, float& bestPower)
{
  bestStart = -1;
  bestFrequency = 0;
  bestPower = -99.f;
  if (frequencyStep <= 0 || startStep <= 0 || frequencyMin < -16 || frequencyMax > 16
      || frequencyMin > frequencyMax || startMin > startMax) return;
  struct Best { int start; float value; };
  int const count = (frequencyMax - frequencyMin) / frequencyStep + 1;
  std::vector<Best> rows (count);
  int workers = 1;
#ifdef _OPENMP
  if (!omp_in_parallel () && omp_get_max_active_levels () > 0
      && count * ((startMax - startMin) / startStep + 1) >= 256)
    workers = std::max (1, std::min ({8, count, omp_get_max_threads (),
      omp_get_thread_limit (), std::max (1, omp_get_num_procs () - 2)}));
#pragma omp parallel for schedule(static) num_threads(workers) if(workers > 1)
#endif
  for (int row = 0; row < count; ++row) {
    int frequency = frequencyMin + row * frequencyStep;
    auto const reference = tweaked (tweaks + (frequency + 16) * syncSamples, true);
    Best best {-1, -99.f};
    for (int start = startMin; start <= startMax; start += startStep) {
      float value = power (data, start, reference);
      if (value > best.value) best = {start, value};
    }
    rows[row] = best;
  }
  (void) workers;
  // Reduce in original frequency/time order, preserving the first equal peak.
  for (int row = 0; row < count; ++row) if (rows[row].value > bestPower) {
    bestPower = rows[row].value;
    bestStart = rows[row].start;
    bestFrequency = frequencyMin + row * frequencyStep;
  }
}
}

extern "C" void sync4d_ (std::complex<float> const *data, int const *start,
  std::complex<float> const *tweak, int const *apply, float *power)
{
  *power = ft4_native::sync (data, *start, tweak, *apply == 1);
}
extern "C" void sync4d_search_ (std::complex<float> const *data, std::complex<float> const *tweaks,
  int const *fmin, int const *fmax, int const *fstep, int const *tmin, int const *tmax, int const *tstep,
  int *bestStart, int *bestFrequency, float *power)
{
  ft4_native::search (data, tweaks, *fmin, *fmax, *fstep, *tmin, *tmax, *tstep, *bestStart, *bestFrequency, *power);
}
