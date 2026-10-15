#ifndef WSJT_NATIVE_FT8_LDPC_HPP
#define WSJT_NATIVE_FT8_LDPC_HPP
#include <cstdint>
namespace ft8_native {
void encode (std::int8_t const *message77, std::int8_t *codeword);
bool validCrc (std::int8_t const *codeword);
}
#endif
