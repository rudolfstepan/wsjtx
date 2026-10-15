#ifndef WSJT_NATIVE_FT8_WAVE_HPP
#define WSJT_NATIVE_FT8_WAVE_HPP

#include <complex>

namespace ft8_native
{
enum class FrameShape { Ft8, Ft4 };
// Gaussian continuous-phase FSK. The phase is a persistent centered state:
// one bounded +/-2pi correction replaces a general remainder per sample.
void waveform (int const *tones, int symbols, int samplesPerSymbol, float bt,
               float sampleRate, float frequency, std::complex<float> *complexWave,
               float *realWave, bool complexOutput, int samples,
               FrameShape shape = FrameShape::Ft8);
}
#endif
