#include "lib/ft8/native/ft8_wave.hpp"
#include "lib/ft4/native/ft4_sync.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <complex>
#include <future>
#include <iostream>
#include <random>
#include <vector>

extern "C" {
void gen_ft4wave_reference_ (int const *, int const *, int const *, float const *, float const *,
  std::complex<float> *, float *, int const *, int const *);
void sync4d_reference_ (std::complex<float> const *, int const *, std::complex<float> const *, int const *, float *);
void sync4d_search_reference_ (std::complex<float> const *, std::complex<float> const *,
  int const *, int const *, int const *, int const *, int const *, int const *, int *, int *, float *);
}

int checkFt4 ()
{
  using Complex = std::complex<float>;
  int tones[103], symbols = 103;
  for (int i = 0; i < symbols; ++i) tones[i] = (i * 3 + i / 7) % 4;
  for (int nsps : {288, 576, 1152, 2304}) {
    float fs = nsps <= 576 ? 12000.f : 48000.f;
    int frame = 105 * nsps, samples = 105 * 2304;
    std::vector<float> a (samples), b (samples);
    std::vector<Complex> ca (samples), cb (samples);
    for (float frequency : {50.f, 500.f, 1855.f, 2900.f}) {
      for (int complexOutput : {0, 1}) {
        gen_ft4wave_reference_ (tones, &symbols, &nsps, &fs, &frequency, ca.data (), a.data (), &complexOutput, &samples);
        ft8_native::waveform (tones, symbols, nsps, 1.f, fs, frequency, cb.data (), b.data (), complexOutput, samples, ft8_native::FrameShape::Ft4);
        double error = 0.;
        for (int i = 0; i < samples; ++i) {
          if (complexOutput ? !std::isfinite (cb[i].real ()) || !std::isfinite (cb[i].imag ()) || std::abs (cb[i]) > 1.000001f
              : !std::isfinite (b[i]) || std::abs (b[i]) > 1.000001f) return 26;
          double delta = complexOutput ? std::abs (ca[i] - cb[i]) : std::abs (a[i] - b[i]);
          error = std::max (error, delta);
          if (i >= frame && (complexOutput ? cb[i] != Complex {} : b[i] != 0.f)) return 20;
        }
        if (error > .03) { std::cerr << "FT4 wave mismatch " << nsps << ' ' << frequency << ' ' << error << '\n'; return 21; }
      }
    }
    int zeroTones[103] = {};
    ft8_native::waveform (zeroTones, symbols, nsps, 1.f, fs, 1855.f, nullptr, b.data (), false, samples, ft8_native::FrameShape::Ft4);
    for (int i = nsps; i < frame - nsps; ++i)
      if (std::abs (b[i] - std::sin (2. * std::acos (-1.) * 1855. * i / fs)) > 2.e-6) return 27;
    int realOutput = 0;
    float frequency = 1855.f;
    auto before = std::chrono::steady_clock::now ();
    for (int i = 0; i < 30; ++i) gen_ft4wave_reference_ (tones, &symbols, &nsps, &fs, &frequency, ca.data (), a.data (), &realOutput, &samples);
    auto middle = std::chrono::steady_clock::now ();
    for (int i = 0; i < 30; ++i) ft8_native::waveform (tones, symbols, nsps, 1.f, fs, frequency, cb.data (), b.data (), false, samples, ft8_native::FrameShape::Ft4);
    auto after = std::chrono::steady_clock::now ();
    std::cout << "FT4/FT2 waveform " << nsps << " samples/symbol: original "
      << std::chrono::duration<double, std::milli> (middle - before).count () / 30
      << " ms; C++ " << std::chrono::duration<double, std::milli> (after - middle).count () / 30 << " ms\n";
  }
  // Mixed FT8/FT4/FT2 calls must not share mutable pulse or phase state.
  std::vector<std::future<bool>> futures;
  for (int worker = 0; worker < 8; ++worker) futures.push_back (std::async (std::launch::async, [&, worker] {
    int nsps = worker % 2 ? 1152 : 2304, count = 105 * nsps;
    std::vector<float> expected (count), actual (count);
    ft8_native::waveform (tones, symbols, nsps, 1.f, 48000.f, 1855.f, nullptr, expected.data (), false, count, ft8_native::FrameShape::Ft4);
    for (int repeat = 0; repeat < 8; ++repeat) {
      ft8_native::waveform (tones, 79, 32, 2.f, 200.f, 0.f, nullptr, actual.data (), false, 79 * 32);
      ft8_native::waveform (tones, symbols, nsps, 1.f, 48000.f, 1855.f, nullptr, actual.data (), false, count, ft8_native::FrameShape::Ft4);
      if (expected != actual) return false;
    }
    return true;
  }));
  for (auto& future : futures) if (!future.get ()) return 22;

  std::mt19937 random (4422);
  std::normal_distribution<float> normal (0.f, 1.f);
  std::vector<Complex> data (ft4_native::downsampledSamples), tweaks (33 * 64);
  for (int frequency = -16; frequency <= 16; ++frequency)
    for (int i = 0; i < 64; ++i) {
      double phase = 2. * std::acos (-1.) * frequency * i / (12000. / 18. / 2.);
      tweaks[(frequency + 16) * 64 + i] = {float (std::cos (phase)), float (std::sin (phase))};
    }
  int apply = 1, fmin = -12, fmax = 12, fstep = 3, tmin = -344, tmax = 1012, tstep = 4;
  double oldTime = 0., newTime = 0.;
  for (int trial = 0; trial < 30; ++trial) {
    for (auto& sample : data) sample = {normal (random), normal (random)};
    for (int start = -344; start <= 1400; start += 7) {
      float a = 0.f;
      sync4d_reference_ (data.data (), &start, tweaks.data () + 16 * 64, &apply, &a);
      float b = ft4_native::sync (data.data (), start, tweaks.data () + 16 * 64, true);
      if (std::abs (a - b) > 2.e-5f * std::max (1.f, a)) return 23;
    }
    int aStart, bStart, aFrequency, bFrequency;
    float aPower, bPower;
    auto before = std::chrono::steady_clock::now ();
    sync4d_search_reference_ (data.data (), tweaks.data (), &fmin, &fmax, &fstep, &tmin, &tmax, &tstep, &aStart, &aFrequency, &aPower);
    auto middle = std::chrono::steady_clock::now ();
    ft4_native::search (data.data (), tweaks.data (), fmin, fmax, fstep, tmin, tmax, tstep, bStart, bFrequency, bPower);
    auto after = std::chrono::steady_clock::now ();
    oldTime += std::chrono::duration<double, std::milli> (middle - before).count ();
    newTime += std::chrono::duration<double, std::milli> (after - middle).count ();
    if (aStart != bStart || aFrequency != bFrequency || std::abs (aPower - bPower) > 2.e-5f * std::max (1.f, aPower)) return 24;
    int f1 = aFrequency - 4, f2 = aFrequency + 4, fs = 1;
    int t1 = aStart - 5, t2 = aStart + 5, ts = 1;
    sync4d_search_reference_ (data.data (), tweaks.data (), &f1, &f2, &fs, &t1, &t2, &ts, &aStart, &aFrequency, &aPower);
    ft4_native::search (data.data (), tweaks.data (), f1, f2, fs, t1, t2, ts, bStart, bFrequency, bPower);
    if (aStart != bStart || aFrequency != bFrequency || std::abs (aPower - bPower) > 2.e-5f * std::max (1.f, aPower)) return 29;
  }
  std::vector<std::future<bool>> searches;
  for (int worker = 0; worker < 4; ++worker) searches.push_back (std::async (std::launch::async, [&, worker] {
    auto input = data;
    for (auto& sample : input) sample *= worker + 1.f;
    int aStart, bStart, aFrequency, bFrequency;
    float aPower, bPower;
    // The original SAVE-based reference was compared serially above.
    ft4_native::search (input.data (), tweaks.data (), fmin, fmax, fstep, tmin, tmax, tstep, aStart, aFrequency, aPower);
    for (int repeat = 0; repeat < 8; ++repeat) {
      ft4_native::search (input.data (), tweaks.data (), fmin, fmax, fstep, tmin, tmax, tstep, bStart, bFrequency, bPower);
      if (aStart != bStart || aFrequency != bFrequency || aPower != bPower) return false;
    }
    return true;
  }));
  for (auto& future : searches) if (!future.get ()) return 28;
  // Equal peaks must retain the first grid point, independent of thread count.
  std::fill (data.begin (), data.end (), Complex {});
  int bestStart, bestFrequency;
  float bestPower;
  ft4_native::search (data.data (), tweaks.data (), fmin, fmax, fstep, tmin, tmax, tstep, bestStart, bestFrequency, bestPower);
  if (bestStart != tmin || bestFrequency != fmin || bestPower != 0.f) return 25;
  std::cout << "FT4/FT2 sync reference comparisons passed; grid original " << oldTime / 30
    << " ms; C++/SMP " << newTime / 30 << " ms\n";
  return 0;
}
