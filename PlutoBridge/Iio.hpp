#ifndef WSJTX_PLUTO_IIO_HPP
#define WSJTX_PLUTO_IIO_HPP
#include "Dsp.hpp"
#include <QString>
#include <stdexcept>
#include <cstddef>
#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <QLibrary>
#endif

namespace pluto {
// The deliberately pinned libiio 0.x ABI; libiio 1.x buffers are incompatible.
struct DataFormat {
  unsigned length,bits,shift;
  bool is_signed,is_fully_defined,is_be,with_scale;
  double scale;
  unsigned repeat;
};
class IioApi {
#ifdef Q_OS_WIN
  HMODULE library=nullptr;
#else
  QLibrary library;
#endif
  void* symbol(char const* name);
public:
  explicit IioApi(QString const& path);
  ~IioApi();
  IioApi(IioApi const&)=delete;
  IioApi& operator=(IioApi const&)=delete;
#define IIO_FN(type,name,args) type (*name) args = nullptr
  IIO_FN(void,iio_library_get_version,(unsigned*,unsigned*,char*));
  IIO_FN(void*,iio_create_context_from_uri,(char const*));
  IIO_FN(void,iio_context_destroy,(void*));
  IIO_FN(int,iio_context_set_timeout,(void*,unsigned));
  IIO_FN(void*,iio_context_find_device,(void*,char const*));
  IIO_FN(void*,iio_device_find_channel,(void*,char const*,bool));
  IIO_FN(unsigned,iio_device_get_channels_count,(void*));
  IIO_FN(void*,iio_device_get_channel,(void*,unsigned));
  IIO_FN(void,iio_channel_enable,(void*));
  IIO_FN(void,iio_channel_disable,(void*));
  IIO_FN(DataFormat const*,iio_channel_get_data_format,(void*));
  IIO_FN(int,iio_channel_attr_write_longlong,(void*,char const*,long long));
  IIO_FN(int,iio_channel_attr_read_longlong,(void*,char const*,long long*));
  IIO_FN(int,iio_channel_attr_write_double,(void*,char const*,double));
  IIO_FN(ptrdiff_t,iio_channel_attr_write,(void*,char const*,char const*));
  IIO_FN(ptrdiff_t,iio_device_attr_write,(void*,char const*,char const*));
  IIO_FN(ptrdiff_t,iio_device_attr_read,(void*,char const*,char*,size_t));
  IIO_FN(int,iio_device_set_kernel_buffers_count,(void*,unsigned));
  IIO_FN(void*,iio_device_create_buffer,(void*,size_t,bool));
  IIO_FN(void,iio_buffer_destroy,(void*));
  IIO_FN(ptrdiff_t,iio_buffer_refill,(void*));
  IIO_FN(ptrdiff_t,iio_buffer_push,(void*));
  IIO_FN(void*,iio_buffer_first,(void*,void*));
  IIO_FN(void*,iio_buffer_end,(void*));
  IIO_FN(ptrdiff_t,iio_buffer_step,(void*));
#undef IIO_FN
  static void check(ptrdiff_t result,char const* operation);
};
class IioRadio {
  // Keep RX latency at 10 ms. TX uses fewer network/DMA submissions and
  // enough queued samples to absorb ordinary Windows/USB scheduling jitter.
  static constexpr int txBlocks=4;
  static constexpr int txSamples=rf_block*txBlocks;
  IioApi api;
  void* context=nullptr;
  void* phy=nullptr;
  void *rxDevice=nullptr,*txDevice=nullptr,*rxI=nullptr,*rxQ=nullptr,*txI=nullptr,*txQ=nullptr;
  void *rxLo=nullptr,*txLo=nullptr,*rxBuffer=nullptr,*txBuffer=nullptr;
  SampleFormat rxFormatI{},rxFormatQ{},txFormatI{},txFormatQ{};
  bool keyed=false;
  std::vector<IQ> txPending;
  void pushTransmit(std::vector<IQ> const&);
  void cleanup() noexcept;
  void state(char const* mode);
  SampleFormat format(void* channel);
  void* required(void*,char const*);
public:
  IioRadio(QString const& library,QString const& uri,long long frequency,double rxGain,double txGain);
  ~IioRadio() { cleanup(); }
  void tune(long long rx,long long tx);
  void ptt(bool on);
  std::vector<IQ> receive();
  void transmit(std::vector<IQ> const&);
};
}
#endif
