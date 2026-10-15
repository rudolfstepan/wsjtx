#include "lib/ft8/native/ft8_ldpc.hpp"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <random>
#include <array>

extern "C" {
void encode174_91_reference_ (std::int8_t const *, std::int8_t *);
void encode174_91var_reference_ (std::int8_t const *, std::int8_t *);
void encode174_91var_ (std::int8_t const *, std::int8_t *);
void bpdecode174_91_reference_ (float const *, std::int8_t const *, int const *, std::int8_t *, std::int8_t *, int *, int *, int *);
void bpdecode174_91_ (float const *, std::int8_t const *, int const *, std::int8_t *, std::int8_t *, int *, int *, int *);
void bpdecode174_91var_reference_ (float const *, std::int8_t const *, int const *, std::int8_t *, std::int8_t *, int *, int *);
void bpdecode174_91var_ (float const *, std::int8_t const *, int const *, std::int8_t *, std::int8_t *, int *, int *);
}

int checkLdpc ()
{
  std::mt19937 random (17491);
  std::normal_distribution<float> noise (0.f, 1.f);
  std::array<std::int8_t, 77> message;
  std::array<std::int8_t, 174> reference, native, mask;
  std::array<float, 174> llr;
  double oldBp = 0., newBp = 0.;
  int successful = 0;
  for (int trial = 0; trial < 1000; ++trial) {
    for (auto& bit : message) bit = random () & 1;
    encode174_91_reference_ (message.data (), reference.data ());
    ft8_native::encode (message.data (), native.data ());
    if (native != reference || !ft8_native::validCrc (native.data ())) {
      std::cerr << "Encoder mismatch at " << trial << '\n'; return 10;
    }
    encode174_91var_reference_ (message.data (), reference.data ());
    encode174_91var_ (message.data (), native.data ());
    if (native != reference) return 14;
    native[80] ^= 1;
    if (ft8_native::validCrc (native.data ())) return 11;
    float strength = .5f + (trial % 8) * .5f;
    for (int i = 0; i < 174; ++i) {
      mask[i] = trial % 3 == 0 && i < 20 ? 1 : 0;
      llr[i] = (2 * reference[i] - 1) * strength + noise (random) * 2.f;
      if (mask[i]) llr[i] = (2 * reference[i] - 1) * 20.f;
    }
    int maximum = 30;
    for (bool parallel : {false, true}) {
      std::array<std::int8_t, 77> aMessage = {}, bMessage = {};
      std::array<std::int8_t, 174> aWord = {}, bWord = {};
      int aHard = -1, bHard = -1, aIteration = -1, bIteration = -1, aChecks = -1, bChecks = -1;
      auto before = std::chrono::steady_clock::now ();
      if (parallel) bpdecode174_91var_reference_ (llr.data (), mask.data (), &maximum, aMessage.data (), aWord.data (), &aHard, &aIteration);
      else bpdecode174_91_reference_ (llr.data (), mask.data (), &maximum, aMessage.data (), aWord.data (), &aHard, &aIteration, &aChecks);
      auto middle = std::chrono::steady_clock::now ();
      if (parallel) bpdecode174_91var_ (llr.data (), mask.data (), &maximum, bMessage.data (), bWord.data (), &bHard, &bIteration);
      else bpdecode174_91_ (llr.data (), mask.data (), &maximum, bMessage.data (), bWord.data (), &bHard, &bIteration, &bChecks);
      auto after = std::chrono::steady_clock::now ();
      oldBp += std::chrono::duration<double, std::micro> (middle - before).count ();
      newBp += std::chrono::duration<double, std::micro> (after - middle).count ();
      if (aHard != bHard || aIteration != bIteration || (!parallel && aChecks != bChecks)
          || aWord != bWord || (aHard >= 0 && aMessage != bMessage)) {
        std::cerr << "BP mismatch trial=" << trial << " parallel=" << parallel
          << " errors=" << aHard << '/' << bHard << " iterations=" << aIteration << '/' << bIteration << '\n';
        return 12;
      }
      if (aHard >= 0) {
        if (aMessage != message) return 13;
        ++successful;
      }
    }
  }
  auto before = std::chrono::steady_clock::now ();
  for (int i = 0; i < 10000; ++i) encode174_91_reference_ (message.data (), reference.data ());
  auto middle = std::chrono::steady_clock::now ();
  for (int i = 0; i < 10000; ++i) ft8_native::encode (message.data (), native.data ());
  auto after = std::chrono::steady_clock::now ();
  std::cout << "LDPC: 1000 exact encoder comparisons; 2000 BP comparisons (" << successful << " successful); Fortran/C++ BP ratio "
    << oldBp / newBp << "x\nEncoder: Fortran " << std::chrono::duration<double, std::micro> (middle - before).count () / 10000
    << " us; C++ " << std::chrono::duration<double, std::micro> (after - middle).count () / 10000 << " us\n";
  return 0;
}
