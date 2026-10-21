// A test double for libiio 0.26. It never connects to USB, IP or RF hardware.
#include <cstdint>
#include <cstring>
#include <vector>
#include <string>
#include <algorithm>
#define API extern "C" __declspec(dllexport)
struct Format {unsigned length=16,bits=12,shift=0;bool sign=true,defined=true,be=false,scaleFlag=false;double scale=1;unsigned repeat=1;};
struct Channel {int id;bool output;Format format;};
struct Device {int id;Channel channels[2];};
struct Buffer {std::vector<int16_t> samples;Device* device;};
struct State {int txPowerdown=1,rxPowerdown=1,contexts=0,buffers=0,pushes=0,failPush=0;long long rxFrequency=0,txFrequency=0,rate=3072000;int ensm=0,failStateReadback=0;
 unsigned rxKernel=0,txKernel=0;int txSamples=0,shortPush=0,underflows=0;long long nowUs=0,queuedUntilUs=0;};
static State state;
static std::vector<int16_t> lastTx;
static Device phy{0,{{0,false,{}},{1,false,{}}}},rx{1,{{0,false,{}},{1,false,{}}}},tx{2,{{0,true,{}},{1,true,{}}}};
static Channel rxLo{10,true,{}},txLo{11,true,{}},rfRx{20,false,{}},rfTx{21,true,{}};
API State* fake_iio_state() {return &state;}
API int fake_iio_tx_sample(unsigned index) {return index<lastTx.size()?lastTx[index]:99999;}
API void iio_library_get_version(unsigned* major,unsigned* minor,char* tag) {*major=0;*minor=26;std::strcpy(tag,"test");}
API void* iio_create_context_from_uri(char const*) {++state.contexts;return &state;}
API void iio_context_destroy(void*) {--state.contexts;}
API int iio_context_set_timeout(void*,unsigned) {return 0;}
API void* iio_context_find_device(void*,char const* name) {
 if(!std::strcmp(name,"ad9361-phy")) return &phy;
 if(!std::strcmp(name,"cf-ad9361-lpc")) return &rx;
 if(!std::strcmp(name,"cf-ad9361-dds-core-lpc")) return &tx;
 return nullptr;
}
API void* iio_device_find_channel(void* device,char const* name,bool output) {
 auto d=static_cast<Device*>(device);
 if(d==&phy) {
  if(!std::strcmp(name,"altvoltage0")) return &rxLo;
  if(!std::strcmp(name,"altvoltage1")) return &txLo;
  return output?&rfTx:&rfRx;
 }
 return &d->channels[!std::strcmp(name,"voltage1")?1:0];
}
API unsigned iio_device_get_channels_count(void*) {return 2;}
API void* iio_device_get_channel(void* d,unsigned n) {return &static_cast<Device*>(d)->channels[n%2];}
API void iio_channel_enable(void*) {}
API void iio_channel_disable(void*) {}
API Format const* iio_channel_get_data_format(void* c) {return &static_cast<Channel*>(c)->format;}
API int iio_channel_attr_write_longlong(void* channel,char const* name,long long value) {
 auto c=static_cast<Channel*>(channel);
 if((c==&rxLo || c==&txLo) && state.ensm==0) return -22;
 if(!std::strcmp(name,"powerdown")) {(c==&txLo?state.txPowerdown:state.rxPowerdown)=int(value);}
 if(!std::strcmp(name,"frequency")) {(c==&txLo?state.txFrequency:state.rxFrequency)=value;}
 if(!std::strcmp(name,"sampling_frequency")) state.rate=value;
 return 0;
}
API int iio_channel_attr_read_longlong(void* channel,char const* name,long long* value) {
 *value=!std::strcmp(name,"frequency")?(channel==&txLo?state.txFrequency:state.rxFrequency):state.rate;
 return 0;
}
API int iio_channel_attr_write_double(void*,char const*,double) {return 0;}
API ptrdiff_t iio_channel_attr_write(void*,char const*,char const* text) {return std::strlen(text);}
API ptrdiff_t iio_device_attr_write(void*,char const*,char const* text) {
 state.ensm=!std::strcmp(text,"fdd")?2:!std::strcmp(text,"alert")?1:0;
 return std::strlen(text);
}
API ptrdiff_t iio_device_attr_read(void*,char const*,char* text,size_t size) {
 auto mode=state.failStateReadback?"sleep":state.ensm==2?"fdd":state.ensm==1?"alert":"sleep";
 auto n=std::strlen(mode)+1;if(size<n) return -22;std::memcpy(text,mode,n);return n;
}
API int iio_device_set_kernel_buffers_count(void* d,unsigned n) {(d==&tx?state.txKernel:state.rxKernel)=n;return 0;}
API void* iio_device_create_buffer(void* d,size_t count,bool cyclic) {
 if(cyclic) return nullptr;
 auto b=new Buffer;b->samples.resize(count*2);b->device=static_cast<Device*>(d);++state.buffers;
 if(d==&tx) {state.txSamples=int(count);state.queuedUntilUs=state.nowUs;lastTx.clear();}
 return b;
}
API void iio_buffer_destroy(void* buffer) {delete static_cast<Buffer*>(buffer);--state.buffers;}
API ptrdiff_t iio_buffer_refill(void* b) {return ptrdiff_t(static_cast<Buffer*>(b)->samples.size()*2);}
API ptrdiff_t iio_buffer_push(void* b) {
 ++state.pushes;if(state.failPush) {state.failPush=0;return -5;}
 auto buffer=static_cast<Buffer*>(b);
 // Virtual DAC clock and blocking backpressure: no sleeps or real device.
 auto duration=static_cast<long long>(buffer->samples.size()/2)*1000000/state.rate;
 if(state.queuedUntilUs<state.nowUs) ++state.underflows;
 state.nowUs=std::max(state.nowUs,state.queuedUntilUs-(state.txKernel-1)*duration);
 state.queuedUntilUs=std::max(state.queuedUntilUs,state.nowUs)+duration;
 lastTx=buffer->samples;
 auto bytes=ptrdiff_t(buffer->samples.size()*2);
 if(state.shortPush) {state.shortPush=0;return bytes-4;}
 return bytes;
}
API void* iio_buffer_first(void* b,void* c) {return static_cast<Buffer*>(b)->samples.data()+static_cast<Channel*>(c)->id;}
API void* iio_buffer_end(void* b) {auto& v=static_cast<Buffer*>(b)->samples;return v.data()+v.size();}
API ptrdiff_t iio_buffer_step(void*) {return 4;}
