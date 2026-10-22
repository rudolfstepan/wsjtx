// Fortran ABI in a separate archive member: reference decoders may link
// their original Fortran entry points alongside the shared native core.
#include "ft8_ldpc.hpp"
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
  ft8_native::decode(llr,mask,*maximum,message,cw,*hard,*iteration,*checks,false);
}
extern "C" void bpdecode174_91var_ (float const *llr, std::int8_t const *mask, int const *maximum,
  std::int8_t *message, std::int8_t *cw, int *hard, int *iteration)
{
  int checks = 0;
  ft8_native::decode(llr,mask,*maximum,message,cw,*hard,*iteration,checks,true);
}
