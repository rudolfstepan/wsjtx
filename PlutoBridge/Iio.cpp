#include "Iio.hpp"
#include <QFileInfo>
#include <QDir>
#include <cstring>

namespace pluto {
void* IioApi::symbol(char const* name) {
#ifdef Q_OS_WIN
  auto address=reinterpret_cast<void*>(GetProcAddress(library,name));
#else
  auto address=reinterpret_cast<void*>(library.resolve(name));
#endif
  if(!address) throw std::runtime_error(std::string("libiio function missing: ")+name);
  return address;
}
IioApi::IioApi(QString const& path) {
#ifdef Q_OS_WIN
  auto nativePath=QDir::toNativeSeparators(QFileInfo(path).absoluteFilePath());
  library=LoadLibraryExW(reinterpret_cast<wchar_t const*>(nativePath.utf16()),nullptr,
                         LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
  if(!library) throw std::runtime_error("Cannot load Pluto libiio 0.26 runtime and its DLL dependencies (Windows "+std::to_string(GetLastError())+")");
#else
  library.setFileName(path);
  if(!library.load()) throw std::runtime_error(library.errorString().toStdString());
#endif
  try {
#define LOAD(name) name=reinterpret_cast<decltype(name)>(symbol(#name))
    LOAD(iio_library_get_version); LOAD(iio_create_context_from_uri); LOAD(iio_context_destroy);
    LOAD(iio_context_set_timeout); LOAD(iio_context_find_device); LOAD(iio_device_find_channel);
    LOAD(iio_device_get_channels_count); LOAD(iio_device_get_channel); LOAD(iio_channel_enable);
    LOAD(iio_channel_disable); LOAD(iio_channel_get_data_format); LOAD(iio_channel_attr_write_longlong);
    LOAD(iio_channel_attr_read_longlong); LOAD(iio_channel_attr_write_double); LOAD(iio_channel_attr_write);
    LOAD(iio_device_attr_write); LOAD(iio_device_attr_read);
    LOAD(iio_device_set_kernel_buffers_count); LOAD(iio_device_create_buffer); LOAD(iio_buffer_destroy);
    LOAD(iio_buffer_refill); LOAD(iio_buffer_push); LOAD(iio_buffer_first); LOAD(iio_buffer_end); LOAD(iio_buffer_step);
#undef LOAD
    unsigned major=0,minor=0; char tag[16]={};
    iio_library_get_version(&major,&minor,tag);
    if(major!=0 || minor!=26) throw std::runtime_error("Pluto bridge requires the libiio 0.26 ABI");
  } catch(...) {
#ifdef Q_OS_WIN
    FreeLibrary(library); library=nullptr;
#endif
    throw;
  }
}
IioApi::~IioApi() {
#ifdef Q_OS_WIN
  if(library) FreeLibrary(library);
#endif
}
void IioApi::check(ptrdiff_t result,char const* operation) {
  if(result<0) throw std::runtime_error(std::string("Pluto ")+operation+" failed (IIO "+std::to_string(result)+")");
}
void* IioRadio::required(void* p,char const* what) {
  if(!p) throw std::runtime_error(std::string("Pluto missing ")+what);
  return p;
}
SampleFormat IioRadio::format(void* channel) {
  auto f=api.iio_channel_get_data_format(channel);
  if(!f || f->length!=16 || f->bits<8 || f->bits>16 || f->bits+f->shift>16
     || !f->is_signed || f->is_be || f->with_scale || f->repeat!=1)
    throw std::runtime_error("Unsupported Pluto IIO scan format");
  return {f->bits,f->shift};
}
void IioRadio::state(char const* mode) {
  api.check(api.iio_device_attr_write(phy,"ensm_mode",mode),"AD936x RF state");
  char actual[128]={};
  auto count=api.iio_device_attr_read(phy,"ensm_mode",actual,sizeof(actual)-1);
  api.check(count,"AD936x RF state readback");
  // libiio backends may include the terminating NUL in the returned count.
  if(count>=ptrdiff_t(sizeof(actual)) || QByteArray(actual).trimmed()!=mode)
    throw std::runtime_error(std::string("Pluto AD936x RF state readback mismatch (expected ")+mode+", got "+actual+")");
}
IioRadio::IioRadio(QString const& library,QString const& uri,long long frequency,double rxGain,double txGain)
  : api(library) {
  try {
    context=required(api.iio_create_context_from_uri(uri.toUtf8().constData()),"IIO context (check URI and exclusive device access)");
    api.check(api.iio_context_set_timeout(context,500),"timeout");
    phy=required(api.iio_context_find_device(context,"ad9361-phy"),"ad9361-phy");
    rxLo=required(api.iio_device_find_channel(phy,"altvoltage0",true),"RX LO");
    txLo=required(api.iio_device_find_channel(phy,"altvoltage1",true),"TX LO");
    // The AD936x driver rejects ALL LO writes in sleep with -EINVAL.
    // ALERT wakes the control path without entering the active TX/RX state.
    state("alert");
    api.check(api.iio_channel_attr_write_longlong(txLo,"powerdown",1),"mute TX LO");
    for(bool tx:{false,true}) {
      auto channel=required(api.iio_device_find_channel(phy,"voltage0",tx),"RF channel");
      api.check(api.iio_channel_attr_write(channel,"rf_port_select",tx?"A":"A_BALANCED"),"RF port");
      api.check(api.iio_channel_attr_write_longlong(channel,"sampling_frequency",rf_rate),"sample rate");
      long long actual=0;
      api.check(api.iio_channel_attr_read_longlong(channel,"sampling_frequency",&actual),"sample rate readback");
      if(std::llabs(actual-rf_rate)>1) throw std::runtime_error("Pluto sample rate readback mismatch");
      api.check(api.iio_channel_attr_write_longlong(channel,"rf_bandwidth",200000),"RF bandwidth");
      if(!tx) api.check(api.iio_channel_attr_write(channel,"gain_control_mode","manual"),"manual RX gain");
      api.check(api.iio_channel_attr_write_double(channel,"hardwaregain",tx?txGain:rxGain),"gain");
    }
    tune(frequency,frequency);
    rxDevice=required(api.iio_context_find_device(context,"cf-ad9361-lpc"),"RX streaming device");
    txDevice=required(api.iio_context_find_device(context,"cf-ad9361-dds-core-lpc"),"TX streaming device");
    for(auto device:{rxDevice,txDevice}) {
      for(unsigned i=0;i<api.iio_device_get_channels_count(device);++i)
        api.iio_channel_disable(api.iio_device_get_channel(device,i));
      api.check(api.iio_device_set_kernel_buffers_count(device,device==txDevice?4:2),"kernel buffer count");
    }
    rxI=required(api.iio_device_find_channel(rxDevice,"voltage0",false),"RX I");
    rxQ=required(api.iio_device_find_channel(rxDevice,"voltage1",false),"RX Q");
    txI=required(api.iio_device_find_channel(txDevice,"voltage0",true),"TX I");
    txQ=required(api.iio_device_find_channel(txDevice,"voltage1",true),"TX Q");
    for(auto channel:{rxI,rxQ,txI,txQ}) api.iio_channel_enable(channel);
    rxFormatI=format(rxI); rxFormatQ=format(rxQ); txFormatI=format(txI); txFormatQ=format(txQ);
    api.check(api.iio_channel_attr_write_longlong(rxLo,"powerdown",0),"enable RX LO");
    state("fdd");
    rxBuffer=required(api.iio_device_create_buffer(rxDevice,rf_block,false),"RX buffer");
  } catch(...) { cleanup(); throw; }
}
void IioRadio::cleanup() noexcept {
  if(txLo) api.iio_channel_attr_write_longlong(txLo,"powerdown",1);
  if(txBuffer) api.iio_buffer_destroy(txBuffer);
  if(rxBuffer) api.iio_buffer_destroy(rxBuffer);
  if(rxLo) api.iio_channel_attr_write_longlong(rxLo,"powerdown",1);
  if(phy) api.iio_device_attr_write(phy,"ensm_mode","alert");
  if(context) api.iio_context_destroy(context);
  context=phy=nullptr; rxBuffer=txBuffer=rxLo=txLo=nullptr;
}
void IioRadio::tune(long long rx,long long tx) {
  if(keyed) throw std::runtime_error("Cannot retune Pluto during a transmit frame");
  if(rx<325000000 || rx>3800000000LL || tx<325000000 || tx>3800000000LL)
    throw std::runtime_error("Pluto bridge frequency must be within 325 MHz to 3.8 GHz");
  api.check(api.iio_channel_attr_write_longlong(rxLo,"frequency",rx),"RX frequency");
  api.check(api.iio_channel_attr_write_longlong(txLo,"frequency",tx),"TX frequency");
  long long actualRx=0,actualTx=0;
  api.check(api.iio_channel_attr_read_longlong(rxLo,"frequency",&actualRx),"RX frequency readback");
  api.check(api.iio_channel_attr_read_longlong(txLo,"frequency",&actualTx),"TX frequency readback");
  if(std::llabs(actualRx-rx)>1 || std::llabs(actualTx-tx)>1)
    throw std::runtime_error("Pluto LO frequency readback mismatch");
}
void IioRadio::ptt(bool on) {
  if(on==keyed) return;
  if(on) {
    txPending.clear();
    txPending.reserve(txSamples);
    txBuffer=required(api.iio_device_create_buffer(txDevice,txSamples,false),"TX buffer");
    // First transfer zeros with the LO muted: no stale cyclic/DDS payload.
    std::vector<IQ> zeros(txSamples);
    pushTransmit(zeros);
    api.check(api.iio_channel_attr_write_longlong(txLo,"powerdown",0),"PTT on");
    keyed=true;
  } else {
    api.check(api.iio_channel_attr_write_longlong(txLo,"powerdown",1),"PTT off");
    keyed=false;
    txPending.clear();
    if(txBuffer) { api.iio_buffer_destroy(txBuffer); txBuffer=nullptr; }
    // Drop RX samples captured while transmitting before restarting receive.
    if(rxBuffer) { api.iio_buffer_destroy(rxBuffer); rxBuffer=nullptr; }
    rxBuffer=required(api.iio_device_create_buffer(rxDevice,rf_block,false),"RX restart buffer");
  }
}
std::vector<IQ> IioRadio::receive() {
  auto bytes=api.iio_buffer_refill(rxBuffer);
  api.check(bytes,"RX refill");
  auto step=api.iio_buffer_step(rxBuffer);
  auto i=static_cast<char*>(api.iio_buffer_first(rxBuffer,rxI));
  auto q=static_cast<char*>(api.iio_buffer_first(rxBuffer,rxQ));
  auto end=static_cast<char*>(api.iio_buffer_end(rxBuffer));
  if(step<4 || !i || !q || i>=end || q>=end) throw std::runtime_error("Invalid RX scan stride");
  int count=int(std::min(ptrdiff_t(rf_block),std::min(bytes/step,std::min((end-i)/step,(end-q-2)/step+1))));
  if(count!=rf_block) throw std::runtime_error("Short Pluto RX block");
  std::vector<IQ> out(count);
  for(int k=0;k<count;++k) {
    unsigned short a,b;
    std::memcpy(&a,i+k*step,2); std::memcpy(&b,q+k*step,2);
    out[k]={rxFormatI.decode(a),rxFormatQ.decode(b)};
  }
  return out;
}
void IioRadio::transmit(std::vector<IQ> const& samples) {
  if(!keyed || !txBuffer || samples.size()!=rf_block) throw std::runtime_error("Invalid Pluto TX block");
  // Preserve every sample, including filter state across the 10-ms chunks.
  // The blocking push paces the worker once the four 40-ms buffers fill.
  txPending.insert(txPending.end(),samples.begin(),samples.end());
  if(txPending.size()==txSamples) {
    pushTransmit(txPending);
    txPending.clear();
  }
}
void IioRadio::pushTransmit(std::vector<IQ> const& samples) {
  if(!txBuffer || samples.size()!=txSamples) throw std::runtime_error("Invalid Pluto TX transfer");
  auto step=api.iio_buffer_step(txBuffer);
  auto i=static_cast<char*>(api.iio_buffer_first(txBuffer,txI));
  auto q=static_cast<char*>(api.iio_buffer_first(txBuffer,txQ));
  auto end=static_cast<char*>(api.iio_buffer_end(txBuffer));
  if(step<4 || !i || !q || i+(txSamples-1)*step+2>end || q+(txSamples-1)*step+2>end)
    throw std::runtime_error("Invalid TX scan stride");
  for(int k=0;k<txSamples;++k) {
    auto a=txFormatI.encode(samples[k].real()*.65f),b=txFormatQ.encode(samples[k].imag()*.65f);
    std::memcpy(i+k*step,&a,2); std::memcpy(q+k*step,&b,2);
  }
  auto bytes=api.iio_buffer_push(txBuffer);
  api.check(bytes,"TX push");
  if(bytes!=ptrdiff_t(txSamples)*step) throw std::runtime_error("Short Pluto TX transfer; frame aborted");
}
}
