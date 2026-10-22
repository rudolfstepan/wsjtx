#ifndef WSJTX_SDR_RADIO_HPP
#define WSJTX_SDR_RADIO_HPP
#include "Dsp.hpp"
namespace pluto {
class Radio {
public:
  virtual ~Radio() = default;
  virtual void tune(long long rx,long long tx)=0;
  virtual void ptt(bool)=0;
  virtual std::vector<IQ> receive()=0;
  virtual void transmit(std::vector<IQ> const&)=0;
};
}
#endif
