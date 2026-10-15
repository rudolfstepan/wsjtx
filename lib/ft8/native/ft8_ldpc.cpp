// Native ports of encode174_91 and bpdecode174_91[ var ] (WSJT-X, GPLv3).
#include "ft8_ldpc.hpp"
#include "ft8_tables.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

extern "C" short crc14 (unsigned char const *, int);

namespace ft8_native
{
namespace
{
unsigned checksum (std::int8_t const *bits)
{
  unsigned char bytes[12] = {};
  for (int i = 0; i < 77; ++i) bytes[i / 8] |= unsigned (bits[i] & 1) << (7 - i % 8);
  return unsigned (crc14 (bytes, 12));
}
unsigned parity (std::uint64_t bits)
{
  bits ^= bits >> 32;
  bits ^= bits >> 16;
  bits ^= bits >> 8;
  bits ^= bits >> 4;
  return (0x6996u >> (bits & 15)) & 1u;
}
float plat (float x)
{
  float z = std::abs (x), sign = x < 0.f ? -1.f : 1.f;
  if (z <= .664f) return x / .83f;
  if (z <= .9217f) return sign * (z - .4064f) / .322f;
  if (z <= .9951f) return sign * (z - .8378f) / .0524f;
  if (z <= .9998f) return sign * (z - .9914f) / .0012f;
  return sign * 7.f;
}
struct Graph
{
  int const (*checks)[3];
  int const (*bits)[7];
  int const *degree;
};

void bp (Graph const& graph, float const *llr, std::int8_t const *mask, int maximum,
         std::int8_t *message, std::int8_t *cw, int& hard, int& iteration, int& ncheck)
{
  // Every invocation owns its messages: safe across SMP workers, no SAVE state.
  float toVariable[174][3] = {}, toCheck[83][7] = {}, hyperbolic[83][7] = {}, zn[174];
  int stalled = 0, lastChecks = 0;
  hard = -1;
  for (iteration = 0; iteration <= maximum; ++iteration) {
    for (int bit = 0; bit < 174; ++bit) {
      float sum = (toVariable[bit][0] + toVariable[bit][1]) + toVariable[bit][2];
      zn[bit] = mask[bit] == 1 ? llr[bit] : llr[bit] + sum;
      cw[bit] = zn[bit] > 0.f ? 1 : 0;
    }
    ncheck = 0;
    for (int check = 0; check < 83; ++check) {
      unsigned syndrome = 0;
      for (int edge = 0; edge < graph.degree[check]; ++edge) syndrome ^= cw[graph.bits[check][edge]];
      ncheck += syndrome;
    }
    if (ncheck == 0 && validCrc (cw)) {
      hard = 0;
      for (int bit = 0; bit < 174; ++bit) if ((2 * cw[bit] - 1) * llr[bit] < 0.f) ++hard;
      std::copy (cw, cw + 77, message);
      return;
    }
    if (iteration > 0) {
      stalled = ncheck < lastChecks ? 0 : stalled + 1;
      if (stalled >= 5 && iteration >= 10 && ncheck > 15) return;
    }
    lastChecks = ncheck;
    for (int check = 0; check < 83; ++check) {
      for (int edge = 0; edge < graph.degree[check]; ++edge) {
        int bit = graph.bits[check][edge];
        float value = zn[bit];
        for (int slot = 0; slot < 3; ++slot)
          if (graph.checks[bit][slot] == check) value -= toVariable[bit][slot];
        toCheck[check][edge] = value;
      }
      for (int edge = 0; edge < 7; ++edge) hyperbolic[check][edge] = std::tanh (-toCheck[check][edge] / 2.f);
    }
    for (int bit = 0; bit < 174; ++bit) {
      for (int slot = 0; slot < 3; ++slot) {
        int check = graph.checks[bit][slot];
        float product = 1.f;
        for (int edge = 0; edge < graph.degree[check]; ++edge)
          if (graph.bits[check][edge] != bit) product *= hyperbolic[check][edge];
        toVariable[bit][slot] = 2.f * plat (-product);
      }
    }
  }
}
}

bool validCrc (std::int8_t const *cw)
{
  unsigned received = 0;
  for (int i = 77; i < 91; ++i) received = (received << 1) | unsigned (cw[i] & 1);
  return received == checksum (cw);
}
void encode (std::int8_t const *bits, std::int8_t *cw)
{
  std::copy (bits, bits + 77, cw);
  unsigned crc = checksum (bits);
  for (int i = 0; i < 14; ++i) cw[77 + i] = (crc >> (13 - i)) & 1;
  std::uint64_t packed[2] = {};
  for (int i = 0; i < 91; ++i) packed[i / 64] |= std::uint64_t (cw[i] & 1) << (i % 64);
  for (int row = 0; row < 83; ++row) cw[91 + row] = parity (packed[0] & tables::generator[row][0])
    ^ parity (packed[1] & tables::generator[row][1]);
}
}

extern "C" void encode174_91_ (std::int8_t const *message, std::int8_t *cw)
{
  ft8_native::encode (message, cw);
}
extern "C" void encode174_91var_ (std::int8_t const *message, std::int8_t *cw)
{
  ft8_native::encode (message, cw);
}
extern "C" void bpdecode174_91_ (float const *llr, std::int8_t const *mask, int const *maximum,
  std::int8_t *message, std::int8_t *cw, int *hard, int *iteration, int *checks)
{
  ft8_native::bp ({ft8_native::tables::Mn_standard, ft8_native::tables::Nm_standard,
    ft8_native::tables::nrw_standard}, llr, mask, *maximum, message, cw, *hard, *iteration, *checks);
}
extern "C" void bpdecode174_91var_ (float const *llr, std::int8_t const *mask, int const *maximum,
  std::int8_t *message, std::int8_t *cw, int *hard, int *iteration)
{
  int checks = 0;
  ft8_native::bp ({ft8_native::tables::Mn_parallel, ft8_native::tables::Nm_parallel,
    ft8_native::tables::nrw_parallel}, llr, mask, *maximum, message, cw, *hard, *iteration, checks);
}
