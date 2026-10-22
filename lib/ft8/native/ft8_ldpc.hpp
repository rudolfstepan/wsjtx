#ifndef WSJT_NATIVE_FT8_LDPC_HPP
#define WSJT_NATIVE_FT8_LDPC_HPP
#include <cstdint>
namespace ft8_native {
void encode (std::int8_t const *message77, std::int8_t *codeword);
bool validCrc (std::int8_t const *codeword);
void decode(float const*,std::int8_t const*,int,std::int8_t*,std::int8_t*,
            int&,int&,int&,bool parallel);
// QSO/2 uses the first BP iterations as additional reliability for OSD.
bool decodeReliability(float const*, std::int8_t*, std::int8_t*, float*, int&);
}
#endif
