#ifndef WSJT_NATIVE_FT4_SYNC_HPP
#define WSJT_NATIVE_FT4_SYNC_HPP
#include <complex>

namespace ft4_native
{
constexpr int syncSamples = 64;
constexpr int downsampledSamples = 23 * 3456 / 18;
float sync (std::complex<float> const *data, int start,
            std::complex<float> const *tweak, bool applyTweak);
// Tweak columns for -16..16 Hz, 64 complex samples per column (Fortran layout).
void search (std::complex<float> const *data, std::complex<float> const *tweaks,
             int frequencyMin, int frequencyMax, int frequencyStep,
             int startMin, int startMax, int startStep,
             int& bestStart, int& bestFrequency, float& bestPower);
}
#endif
