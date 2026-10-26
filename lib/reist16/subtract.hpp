#ifndef WSJTX_REIST16_SUBTRACT_HPP
#define WSJTX_REIST16_SUBTRACT_HPP
#include "qso2.hpp"
#include <complex>
namespace reist16 { namespace qso2 {
int refine_start(float const* data,int count,
                 std::vector<std::complex<float>> const& reference,int initial,bool sync_only=false);
// Internal receiver operation. Only use independently validated payloads.
bool subtract(std::vector<float>& residual, Decode const& decoded);
} }
#endif
