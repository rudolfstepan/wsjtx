#include "Hackrf.hpp"
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <algorithm>
#include <cstring>
#include <chrono>
#include <stdexcept>
namespace pluto {
void* HackrfApi::symbol(char const* name) {
#ifdef Q_OS_WIN
  auto p=reinterpret_cast<void*>(GetProcAddress(library,name));
#else
  auto p=reinterpret_cast<void*>(library.resolve(name));
#endif
  if(!p)throw std::runtime_error(std::string("libhackrf function missing: ")+name);
  return p;
}
HackrfApi::HackrfApi(QString const& path) {
#ifdef Q_OS_WIN
  auto native=QDir::toNativeSeparators(QFileInfo(path).absoluteFilePath());
  library=LoadLibraryExW(reinterpret_cast<wchar_t const*>(native.utf16()),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
  if(!library)throw std::runtime_error("Cannot load HackRF runtime and its dependencies (Windows "+std::to_string(GetLastError())+")");
#else
  library.setFileName(path);if(!library.load())throw std::runtime_error(library.errorString().toStdString());
#endif
  try {
#define LOAD(name) name=reinterpret_cast<decltype(name)>(symbol(#name))
    LOAD(hackrf_library_version);LOAD(hackrf_error_name);LOAD(hackrf_init);LOAD(hackrf_exit);
    LOAD(hackrf_device_list);LOAD(hackrf_device_list_free);LOAD(hackrf_open_by_serial);LOAD(hackrf_close);
    LOAD(hackrf_set_sample_rate);LOAD(hackrf_set_baseband_filter_bandwidth);LOAD(hackrf_set_freq);
    LOAD(hackrf_set_lna_gain);LOAD(hackrf_set_vga_gain);LOAD(hackrf_set_txvga_gain);
    LOAD(hackrf_set_amp_enable);LOAD(hackrf_set_antenna_enable);
    LOAD(hackrf_start_rx);LOAD(hackrf_start_tx);LOAD(hackrf_stop_rx);LOAD(hackrf_stop_tx);LOAD(hackrf_is_streaming);
#undef LOAD
  }catch(...){
#ifdef Q_OS_WIN
    FreeLibrary(library);library=nullptr;
#endif
    throw;
  }
}
HackrfApi::~HackrfApi(){
#ifdef Q_OS_WIN
  if(library)FreeLibrary(library);
#endif
}
void HackrfApi::check(int result,char const* action) const {if(result!=0)throw std::runtime_error(std::string("HackRF ")+action+": "+hackrf_error_name(result)+" ("+std::to_string(result)+")");}
HackrfRadio::HackrfRadio(QString const& library,QString const& serial,long long frequency,int lna,int vga,int txGain,float level,bool amplifier)
 :api(library),rxFrequency(frequency),txFrequency(frequency),amplitude(level),txAmp(amplifier) {
  if(frequency<1000000||frequency>6000000000LL||lna<0||lna>40||lna%8||vga<0||vga>62||vga%2||txGain<0||txGain>47||!std::isfinite(level)||level<=0||level>1)throw std::runtime_error("Invalid HackRF frequency or gain");
  try {
    api.check(api.hackrf_init(),"initialize");initialized=true;
    auto selected=serial;
    if(selected.isEmpty()){
      auto list=api.hackrf_device_list();if(!list)throw std::runtime_error("HackRF device enumeration failed");
      int count=list->devicecount;if(count==1&&list->serial_numbers&&list->serial_numbers[0])selected=QString::fromLatin1(list->serial_numbers[0]);
      api.hackrf_device_list_free(list);
      if(count!=1||selected.isEmpty())throw std::runtime_error("Select one HackRF with -Serial / --serial (automatic selection requires exactly one device)");
    }
    api.check(api.hackrf_open_by_serial(selected.toLatin1().constData(),&device),"open selected serial");
    api.check(api.hackrf_set_sample_rate(device,rf_rate),"sample rate");
    api.check(api.hackrf_set_baseband_filter_bandwidth(device,1750000),"baseband filter");
    api.check(api.hackrf_set_amp_enable(device,0),"RF amplifier OFF");
    api.check(api.hackrf_set_antenna_enable(device,0),"bias tee OFF");
    api.check(api.hackrf_set_lna_gain(device,unsigned(lna)),"RX LNA gain");
    api.check(api.hackrf_set_vga_gain(device,unsigned(vga)),"RX VGA gain");
    api.check(api.hackrf_set_txvga_gain(device,unsigned(txGain)),"TX VGA gain");
    api.check(api.hackrf_set_freq(device,uint64_t(frequency)),"initial frequency");
    startReceive();
  }catch(...){cleanup();throw;}
}
void HackrfRadio::checkFailure(){if(!failure.empty())throw std::runtime_error(failure);}
void HackrfRadio::stopStream(){
  stopping=true;event.notify_all();
  int result=0;if(txActive)result=api.hackrf_stop_tx(device);else if(rxActive)result=api.hackrf_stop_rx(device);
  rxActive=txActive=false;
  {std::lock_guard<std::mutex> lock(mutex);rxQueue.clear();txQueue.clear();rxBytes=txBytes=rxOffset=txOffset=0;}
  if(result!=0&&result!=-1003)api.check(result,"stop stream");
}
void HackrfRadio::startReceive(){api.check(api.hackrf_set_amp_enable(device,0),"RX RF amplifier OFF");stopping=false;api.check(api.hackrf_set_freq(device,uint64_t(rxFrequency)),"RX frequency");rxActive=true;api.check(api.hackrf_start_rx(device,rxCallback,this),"start RX");lastRx=std::chrono::steady_clock::now();}
void HackrfRadio::cleanup() noexcept {
  if(device){try{stopStream();}catch(...){}api.hackrf_set_amp_enable(device,0);api.hackrf_set_antenna_enable(device,0);api.hackrf_close(device);device=nullptr;}
  if(initialized){api.hackrf_exit();initialized=false;}
}
void HackrfRadio::tune(long long rx,long long tx){
  if(keyed||rx<1000000||rx>6000000000LL||tx<1000000||tx>6000000000LL)throw std::runtime_error("Invalid HackRF frequency or retune during TX");
  api.check(api.hackrf_set_freq(device,uint64_t(rx)),"tune RX");rxFrequency=rx;txFrequency=tx;
}
void HackrfRadio::ptt(bool on){
  if(on==keyed)return;
  stopStream();keyed=on;
  {std::lock_guard<std::mutex> lock(mutex);checkFailure();}
  if(on){api.check(api.hackrf_set_freq(device,uint64_t(txFrequency)),"TX frequency");api.check(api.hackrf_set_amp_enable(device,txAmp?1:0),"TX RF amplifier");stopping=false;}
  else startReceive();
}
int HackrfRadio::rxCallback(HackrfTransfer* transfer){
  auto self=static_cast<HackrfRadio*>(transfer->rx_ctx);if(self->stopping)return -1;
  try {std::lock_guard<std::mutex> lock(self->mutex);
    if(transfer->valid_length<=0||transfer->valid_length>transfer->buffer_length||transfer->valid_length%2){self->failure="Invalid HackRF RX transfer";self->event.notify_all();return -1;}
    if(self->rxBytes+size_t(transfer->valid_length)>4*MaximumBytes){self->failure="HackRF RX queue overflow; no samples silently discarded";self->event.notify_all();return -1;}
    self->rxQueue.emplace_back(transfer->buffer,transfer->buffer+transfer->valid_length);self->rxBytes+=size_t(transfer->valid_length);self->event.notify_all();return 0;
  }catch(...){return -1;}
}
int HackrfRadio::txCallback(HackrfTransfer* transfer){
  auto self=static_cast<HackrfRadio*>(transfer->tx_ctx);if(self->stopping)return -1;
  if(transfer->buffer_length<=0||transfer->buffer_length%2)return -1;
  std::unique_lock<std::mutex> lock(self->mutex);size_t need=size_t(transfer->buffer_length);
  if(!self->event.wait_for(lock,std::chrono::milliseconds(250),[&]{return self->stopping||!self->failure.empty()||self->txBytes>=need;}))self->failure="HackRF TX underflow; frame stopped without inserting samples";
  if(self->stopping||!self->failure.empty()){self->event.notify_all();return -1;}
  size_t at=0;while(at<need){auto& block=self->txQueue.front();size_t count=std::min(need-at,block.size()-self->txOffset);
    std::memcpy(transfer->buffer+at,block.data()+self->txOffset,count);at+=count;self->txOffset+=count;self->txBytes-=count;
    if(self->txOffset==block.size()){self->txQueue.pop_front();self->txOffset=0;}
  }
  transfer->valid_length=int(need);self->event.notify_all();return 0;
}
std::vector<IQ> HackrfRadio::receive(){
  std::unique_lock<std::mutex> lock(mutex);size_t need=2*rf_block;
  if(!event.wait_for(lock,std::chrono::milliseconds(100),[&]{return stopping||!failure.empty()||rxBytes>=need;})) {
    // An asynchronous USB start or a temporary stall is not a fatal CAT error.
    // No artificial samples: the worker waits and resets the audio path.
    if(std::chrono::steady_clock::now()-lastRx>=std::chrono::seconds(3)) {
      lock.unlock();
      qWarning()<<"HackRF waiting for USB RX data; restarting receive stream";
      stopStream();startReceive();
    }
    return {};
  }
  checkFailure();if(stopping)throw std::runtime_error("HackRF RX stopped");
  lastRx=std::chrono::steady_clock::now();
  std::vector<IQ> result(rf_block);for(int n=0;n<2*rf_block;++n){auto& block=rxQueue.front();float value=float(static_cast<int8_t>(block[rxOffset++]))/128.f;
    if(n%2)result[n/2].imag(value);else result[n/2].real(value);--rxBytes;
    if(rxOffset==block.size()){rxQueue.pop_front();rxOffset=0;}
  }
  return result;
}
void HackrfRadio::transmit(std::vector<IQ> const& samples){
  if(!keyed||samples.size()!=rf_block)throw std::runtime_error("HackRF TX requires a complete IQ block after PTT");
  std::vector<uint8_t> bytes(2*samples.size());for(size_t i=0;i<samples.size();++i){
    if(!std::isfinite(samples[i].real())||!std::isfinite(samples[i].imag()))throw std::runtime_error("Nonfinite HackRF TX IQ");
    bytes[2*i]=uint8_t(int8_t(std::lround(std::max(-1.f,std::min(1.f,amplitude*samples[i].real()))*127)));
    bytes[2*i+1]=uint8_t(int8_t(std::lround(std::max(-1.f,std::min(1.f,amplitude*samples[i].imag()))*127)));
  }
  bool start=false;
  {std::unique_lock<std::mutex> lock(mutex);
    if(!event.wait_for(lock,std::chrono::milliseconds(250),[&]{return stopping||!failure.empty()||txBytes+bytes.size()<=MaximumBytes;}))throw std::runtime_error("HackRF TX queue stalled");
    checkFailure();if(stopping)throw std::runtime_error("HackRF TX stopped");
    txBytes+=bytes.size();txQueue.push_back(std::move(bytes));start=!txActive&&txBytes>=PrimeBytes;
  }
  event.notify_all();if(start){txActive=true;api.check(api.hackrf_start_tx(device,txCallback,this),"start TX");}
  if(txActive&&api.hackrf_is_streaming(device)!=1)throw std::runtime_error("HackRF TX stream stopped");
}
}
