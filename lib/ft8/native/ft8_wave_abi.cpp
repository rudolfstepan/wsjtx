#include "ft8_wave.hpp"
// Existing Fortran and C++ callers share this ABI; no Fortran runs here.
extern "C" void gen_ft8wave_ (int const *tones, int const *symbols, int const *nsps,
  float const *bt, float const *fs, float const *f0, std::complex<float> *complexWave,
  float *realWave, int const *complexOutput, int const *samples)
{
  ft8_native::waveform (tones, *symbols, *nsps, *bt, *fs, *f0, complexWave,
    realWave, *complexOutput != 0, *samples);
}

// The parallel decoder uses the same pulse definition. Share the kernel, not
// its mutable state (the previous SAVE cache was common to all worker calls).
extern "C" void gen_ft8wavevar_ (int const *tones, int const *symbols, int const *nsps,
  float const *bt, float const *fs, float const *f0, std::complex<float> *complexWave,
  float *realWave, int const *complexOutput, int const *samples)
{
  ft8_native::waveform (tones, *symbols, *nsps, *bt, *fs, *f0, complexWave,
    realWave, *complexOutput != 0, *samples);
}

extern "C" void gen_ft4wave_ (int const *tones, int const *symbols, int const *nsps,
  float const *fs, float const *f0, std::complex<float> *complexWave,
  float *realWave, int const *complexOutput, int const *samples)
{
  ft8_native::waveform (tones, *symbols, *nsps, 1.f, *fs, *f0, complexWave,
    realWave, *complexOutput != 0, *samples, ft8_native::FrameShape::Ft4);
}
