// DLL test double. No libusb, hardware or network access.
#include "FakeState.hpp"
#include <vector>
#include <mutex>
#include <thread>
#include <chrono>
#include <cstring>
#define API extern "C" __declspec(dllexport)
struct Transfer {void* device;uint8_t* buffer;int buffer_length,valid_length;void *rx_ctx,*tx_ctx;};
struct List {char** serial_numbers;int* board_ids;int* indices;int count;void** devices;int usbCount;};
using Callback=int(*)(Transfer*);
static FakeHackrfState state;static std::atomic<bool> active{false};static std::thread worker;
static std::mutex captureMutex;static std::vector<uint8_t> captured;
static char serial[]="0123456789abcdef0123456789abcdef";static char* serials[]={serial,serial};
static List list{serials,nullptr,nullptr,1,nullptr,1};
API FakeHackrfState* fake_hackrf_state(){return &state;}
API int fake_hackrf_sample(unsigned index){std::lock_guard<std::mutex> lock(captureMutex);return index<captured.size()?int(static_cast<int8_t>(captured[index])):9999;}
API char const* hackrf_library_version(){return "2026.01.3-fake";}
API char const* hackrf_error_name(int){return "fake error";}
API int hackrf_init(){++state.initialized;return 0;}
API int hackrf_exit(){--state.initialized;return 0;}
API List* hackrf_device_list(){list.count=state.devices;return &list;}
API void hackrf_device_list_free(List*){}
API int hackrf_open_by_serial(char const*,void** device){++state.opens;*device=&state;return 0;}
static int stop(bool tx){active=false;if(worker.joinable())worker.join();state.mode=0;if(tx)++state.txStops;else ++state.rxStops;return 0;}
API int hackrf_close(void*){if(worker.joinable())stop(state.mode==2);++state.closes;return 0;}
API int hackrf_set_sample_rate(void*,double v){state.rate=int(v);return 0;}
API int hackrf_set_baseband_filter_bandwidth(void*,uint32_t v){state.filter=int(v);return 0;}
API int hackrf_set_freq(void*,uint64_t v){if(state.failTune)return -2;state.frequency=v;return 0;}
API int hackrf_set_lna_gain(void*,uint32_t v){state.lna=int(v);return 0;}
API int hackrf_set_vga_gain(void*,uint32_t v){state.vga=int(v);return 0;}
API int hackrf_set_txvga_gain(void*,uint32_t v){state.gain=int(v);return 0;}
API int hackrf_set_amp_enable(void*,uint8_t v){state.amp=v;return 0;}
API int hackrf_set_antenna_enable(void*,uint8_t v){state.bias=v;return 0;}
static int start(bool tx,Callback callback,void* context){
 if(state.mode!=0){++state.overlap;return -6;}
 state.mode=tx?2:1;active=true;
 if(tx){++state.txStarts;{std::lock_guard<std::mutex> lock(captureMutex);captured.clear();}state.txBytes=0;}else ++state.rxStarts;
 auto transfer=[=](){std::vector<uint8_t> data(262144);if(!tx)for(size_t i=0;i<data.size();i+=2){data[i]=64;data[i+1]=uint8_t(int8_t(-32));}
  Transfer t{&state,data.data(),int(data.size()),int(data.size()),tx?nullptr:context,tx?context:nullptr};
  if(callback(&t)!=0)return false;
  if(tx){std::lock_guard<std::mutex> lock(captureMutex);captured.insert(captured.end(),data.begin(),data.begin()+t.valid_length);state.txBytes+=uint64_t(t.valid_length);}return true;
 };
 // Reproduce libhackrf's synchronous preparation of all four TX transfers.
 if(tx)for(int i=0;i<4;++i)if(!transfer()){active=false;state.mode=0;return -2;}
 auto rxDue=std::chrono::steady_clock::now()+std::chrono::milliseconds(state.rxDelayMs.load());
 worker=std::thread([=](){while(active){std::this_thread::sleep_for(std::chrono::milliseconds(42));if(!active)break;if(!tx&&(state.pauseRx||std::chrono::steady_clock::now()<rxDue))continue;if(!transfer()){active=false;break;}}});return 0;
}
API int hackrf_start_rx(void*,Callback cb,void* ctx){return start(false,cb,ctx);}
API int hackrf_start_tx(void*,Callback cb,void* ctx){return start(true,cb,ctx);}
API int hackrf_stop_rx(void*){return stop(false);}
API int hackrf_stop_tx(void*){return stop(true);}
API int hackrf_is_streaming(void*){return active?1:0;}
