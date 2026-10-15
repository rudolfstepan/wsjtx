#include "lib/ft8/native/ft8_wave.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <complex>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <future>
#include <string>

extern "C" void gen_ft8wave_reference_ (int const *, int const *, int const *,
  float const *, float const *, float const *, std::complex<float> *, float *, int const *, int const *);
extern "C" void gen_ft8wavevar_reference_ (int const *, int const *, int const *,
  float const *, float const *, float const *, std::complex<float> *, float *, int const *, int const *);
int checkLdpc ();
int checkFt4 ();

int main (int argc, char **argv)
{
  std::cout.setf (std::ios::unitbuf);
  if (argc == 2 && std::string (argv[1]) == "--ft4-only") return checkFt4 ();
  int tones[79];
  for (int i = 0; i < 79; ++i) tones[i] = (i * 3 + i / 7) % 8;
  int symbols = 79, realOutput = 0;
  float bt = 2.f;
  for (int nsps : {1920, 7680}) {
    float fs = nsps == 1920 ? 12000.f : 48000.f;
    int samples = symbols * nsps;
    std::vector<float> reference (samples), native (samples);
    std::vector<std::complex<float>> complexWave (samples);
    for (float f0 : {50.f, 500.f, 1855.f, 2900.f}) {
      gen_ft8wave_reference_ (tones, &symbols, &nsps, &bt, &fs, &f0,
        complexWave.data (), reference.data (), &realOutput, &samples);
      ft8_native::waveform (tones, symbols, nsps, bt, fs, f0, complexWave.data (), native.data (), false, samples);
      double maxError = 0., energy = 0.;
      for (int i = 0; i < samples; ++i) {
        maxError = std::max (maxError, std::abs (double (native[i]) - reference[i]));
        energy += double (native[i]) * native[i];
        if (!std::isfinite (native[i]) || std::abs (native[i]) > 1.000001f) return 1;
      }
      // The reference accumulates phase in single precision; the native path
      // uses double precision. Compare waveforms and independently decode both.
      std::cout << "Waveform " << fs << " Hz / " << f0 << " Hz: max delta " << maxError << '\n';
      if (maxError > .05 || energy < samples * .45 || energy > samples * .51) return 2;
      if (std::abs (native.front ()) > 1.e-6 || std::abs (native.back ()) > 1.e-4) return 3;
    }
  }
  int nsps = 7680, samples = symbols * nsps;
  float fs = 48000.f, f0 = 1855.f;
  std::vector<float> output (samples);
  std::vector<std::complex<float>> unused (samples);
  // Independent analytic oracle: a zero-tone frame is a pure carrier with
  // shaped edges. This checks precision, not just agreement with old rounding.
  int zeroTones[79] = {};
  ft8_native::waveform (zeroTones, symbols, nsps, bt, fs, f0, unused.data (), output.data (), false, samples);
  double twoPi = 2. * std::acos (-1.);
  for (int i = nsps; i < samples - nsps; ++i)
    if (std::abs (output[i] - std::sin (twoPi * f0 * i / fs)) > 2.e-6) return 4;
  std::cout << "Analytic carrier passed\n";
  for (int testNsps : {32, 1920}) {
    int count = symbols * testNsps, complexOutput = 1;
    float testFs = testNsps == 32 ? 200.f : 12000.f, base = 0.f;
    std::vector<std::complex<float>> a (count), b (count);
    gen_ft8wave_reference_ (tones, &symbols, &testNsps, &bt, &testFs, &base,
      a.data (), output.data (), &complexOutput, &count);
    ft8_native::waveform (tones, symbols, testNsps, bt, testFs, base, b.data (), output.data (), true, count);
    for (int i = 0; i < count; ++i) if (std::abs (a[i] - b[i]) > .002f) return 5;
    if (testNsps == 1920) {
      gen_ft8wavevar_reference_ (tones, &symbols, &testNsps, &bt, &testFs, &base,
        a.data (), output.data (), &complexOutput, &count);
      for (int i = 0; i < count; ++i) if (std::abs (a[i] - b[i]) > .002f) return 6;
    }
  }
  std::cout << "Complex reference passed\n";
  // Concurrent calls deliberately use different pulse sizes and bandwidths.
  // Each must match its serial result exactly, including cache re-use.
  std::vector<std::future<bool>> workers;
  for (int worker = 0; worker < 12; ++worker) workers.emplace_back (std::async (std::launch::async, [&, worker] {
    int countPerSymbol = worker % 2 ? 1920 : 32;
    float rate = worker % 2 ? 12000.f : 200.f;
    std::vector<std::complex<float>> serial (79 * countPerSymbol), concurrent (serial.size ());
    ft8_native::waveform (tones, 79, countPerSymbol, 2.f, rate, 0.f, serial.data (), nullptr, true, int (serial.size ()));
    for (int repeat = 0; repeat < 8; ++repeat) {
      ft8_native::waveform (tones, 79, 32, 1.f + worker * .05f, 200.f, 0.f, concurrent.data (), nullptr, true, 79 * 32);
      ft8_native::waveform (tones, 79, countPerSymbol, 2.f, rate, 0.f, concurrent.data (), nullptr, true, int (concurrent.size ()));
      if (serial != concurrent) return false;
    }
    return true;
  }));
  for (auto& worker : workers) if (!worker.get ()) return 7;
  std::cout << "Concurrent waveforms passed\n";
  // Isolate the paper-inspired bounded correction from lookup-table changes.
  // This is a floating-point phase microbenchmark, not the paper's integer test.
  auto phaseLoop = [twoPi] (bool centered) {
    double phase = 0.;
    auto start = std::chrono::steady_clock::now ();
    for (int i = 0; i < 10000000; ++i) {
      phase += .24 + .001 * (i & 7);
      if (centered) { if (phase >= twoPi / 2.) phase -= twoPi; }
      else phase = std::remainder (phase, twoPi);
    }
    return std::make_pair (std::chrono::duration<double, std::milli> (std::chrono::steady_clock::now () - start).count (), phase);
  };
  auto modulo = phaseLoop (false), centered = phaseLoop (true);
  if (std::abs (std::remainder (modulo.second - centered.second, twoPi)) > 1.e-7) return 8;
  std::cout << "Phase-only microbenchmark: remainder " << modulo.first << " ms; bounded correction "
    << centered.first << " ms (" << modulo.first / centered.first << "x)\n";
  int const repeats = 30;
  auto before = std::chrono::steady_clock::now ();
  for (int i = 0; i < repeats; ++i) gen_ft8wave_reference_ (tones, &symbols, &nsps, &bt, &fs,
    &f0, unused.data (), output.data (), &realOutput, &samples);
  auto middle = std::chrono::steady_clock::now ();
  for (int i = 0; i < repeats; ++i) ft8_native::waveform (tones, symbols, nsps, bt, fs,
    f0, unused.data (), output.data (), false, samples);
  auto after = std::chrono::steady_clock::now ();
  double oldMs = std::chrono::duration<double, std::milli> (middle - before).count () / repeats;
  double newMs = std::chrono::duration<double, std::milli> (after - middle).count () / repeats;
  std::cout << "FT8 TX waveform: Fortran reference " << oldMs << " ms; C++ centered phase "
    << newMs << " ms; ratio " << oldMs / newMs << "x\n";
  int result = checkLdpc ();
  return result ? result : checkFt4 ();
}
