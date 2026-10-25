#ifndef WSJTX_HACKRF_RADIO_HPP
#define WSJTX_HACKRF_RADIO_HPP
#include "PlutoBridge/Radio.hpp"
#include <QString>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <cstdint>
#include <cstddef>
#include <chrono>
#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <QLibrary>
#endif
namespace pluto {
// Stable libhackrf C ABI, verified against Great Scott Gadgets v2026.01.3.
// https://github.com/greatscottgadgets/hackrf/blob/v2026.01.3/host/libhackrf/src/hackrf.h
struct HackrfTransfer {void* device;uint8_t* buffer;int buffer_length,valid_length;void *rx_ctx,*tx_ctx;};
struct HackrfDeviceList {char** serial_numbers;int* usb_board_ids;int* usb_device_index;int devicecount;void** usb_devices;int usb_devicecount;};
static_assert(sizeof(void*)!=8 || (sizeof(HackrfTransfer)==40 && offsetof(HackrfTransfer,tx_ctx)==32),"libhackrf x64 transfer ABI");
using HackrfCallback=int(*)(HackrfTransfer*);
class HackrfApi {
#ifdef Q_OS_WIN
  HMODULE library=nullptr;
#else
  QLibrary library;
#endif
  void* symbol(char const*);
public:
  explicit HackrfApi(QString const& path);
  ~HackrfApi();
  HackrfApi(HackrfApi const&)=delete;
  HackrfApi& operator=(HackrfApi const&)=delete;
#define RF_FN(type,name,args) type (*name) args=nullptr
  RF_FN(char const*,hackrf_library_version,());
  RF_FN(char const*,hackrf_error_name,(int));
  RF_FN(int,hackrf_init,());RF_FN(int,hackrf_exit,());
  RF_FN(HackrfDeviceList*,hackrf_device_list,());RF_FN(void,hackrf_device_list_free,(HackrfDeviceList*));
  RF_FN(int,hackrf_open_by_serial,(char const*,void**));RF_FN(int,hackrf_close,(void*));
  RF_FN(int,hackrf_set_sample_rate,(void*,double));
  RF_FN(int,hackrf_set_baseband_filter_bandwidth,(void*,uint32_t));
  RF_FN(int,hackrf_set_freq,(void*,uint64_t));
  RF_FN(int,hackrf_set_lna_gain,(void*,uint32_t));RF_FN(int,hackrf_set_vga_gain,(void*,uint32_t));
  RF_FN(int,hackrf_set_txvga_gain,(void*,uint32_t));
  RF_FN(int,hackrf_set_amp_enable,(void*,uint8_t));RF_FN(int,hackrf_set_antenna_enable,(void*,uint8_t));
  RF_FN(int,hackrf_start_rx,(void*,HackrfCallback,void*));RF_FN(int,hackrf_start_tx,(void*,HackrfCallback,void*));
  RF_FN(int,hackrf_stop_rx,(void*));RF_FN(int,hackrf_stop_tx,(void*));RF_FN(int,hackrf_is_streaming,(void*));
#undef RF_FN
  void check(int,char const*) const;
};
class HackrfRadio final:public Radio {
  // libhackrf fills four 262144-byte TX transfers synchronously in start_tx.
  // Prime all four before entering it; the audio producer runs on this thread.
  static constexpr size_t PrimeBytes=4*262144,MaximumBytes=PrimeBytes+2*rf_block;
  HackrfApi api;void* device=nullptr;bool initialized=false,keyed=false,rxActive=false,txActive=false;
  std::atomic<bool> stopping{true};
  std::mutex mutex;std::condition_variable event;
  std::deque<std::vector<uint8_t>> rxQueue,txQueue;
  size_t rxBytes=0,txBytes=0,rxOffset=0,txOffset=0;
  std::string failure;
  long long rxFrequency,txFrequency;
  float amplitude;
  bool txAmp;
  std::chrono::steady_clock::time_point lastRx;
  static int rxCallback(HackrfTransfer*);
  static int txCallback(HackrfTransfer*);
  void stopStream();void startReceive();void cleanup() noexcept;
  void checkFailure();
public:
  HackrfRadio(QString const& library,QString const& serial,long long frequency,int lna,int vga,int txGain,float amplitude=.5f,bool txAmp=false);
  ~HackrfRadio() override {cleanup();}
  void tune(long long,long long) override;void ptt(bool) override;
  std::vector<IQ> receive() override;void transmit(std::vector<IQ> const&) override;
};
}
#endif
