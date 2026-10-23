#include "WasapiLoopback.hpp"
#include <stdexcept>
#include <QtGlobal>

#ifdef Q_OS_WIN
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <functiondiscoverykeys_devpkey.h>

namespace {
void check (HRESULT result, char const* operation)
{
  if (FAILED (result))
    throw std::runtime_error (QString ("WASAPI %1 failed (0x%2)")
      .arg (operation).arg (quint32 (result), 8, 16, QChar ('0')).toStdString ());
}
template<class T> struct ComPtr {
  T* p = nullptr;
  ~ComPtr () { if (p) p->Release (); }
  T* operator-> () const { return p; }
  T** address () { return &p; }
};
}

struct WasapiLoopback::Impl
{
  bool uninitialize = false;
  ComPtr<IMMDeviceEnumerator> enumerator;
  ComPtr<IMMDevice> device;
  ComPtr<IAudioClient> client;
  ComPtr<IAudioCaptureClient> capture;
  bool running = false;
  bool firstPacket = true;

  ~Impl ()
  {
    if (running && client.p) client->Stop ();
    // Release all COM interfaces before uninitializing this apartment.
    if (capture.p) { capture.p->Release (); capture.p = nullptr; }
    if (client.p) { client.p->Release (); client.p = nullptr; }
    if (device.p) { device.p->Release (); device.p = nullptr; }
    if (enumerator.p) { enumerator.p->Release (); enumerator.p = nullptr; }
    if (uninitialize) CoUninitialize ();
  }
};

WasapiLoopback::WasapiLoopback (QString const& outputName, unsigned sampleRate)
  : impl_ {new Impl}
{
  auto& s = *impl_;
  auto result = CoInitializeEx (nullptr, COINIT_MULTITHREADED);
  // A Qt audio thread may already own an STA apartment; it is still usable.
  if (result != RPC_E_CHANGED_MODE) { check (result, "COM initialization"); s.uninitialize = true; }
  check (CoCreateInstance (__uuidof (MMDeviceEnumerator), nullptr, CLSCTX_ALL,
    __uuidof (IMMDeviceEnumerator), reinterpret_cast<void**> (s.enumerator.address ())), "device enumeration");
  ComPtr<IMMDeviceCollection> devices;
  check (s.enumerator->EnumAudioEndpoints (eRender, DEVICE_STATE_ACTIVE, devices.address ()), "output enumeration");
  UINT count = 0;
  check (devices->GetCount (&count), "output count");
  unsigned matches = 0;
  for (UINT i = 0; i < count; ++i) {
    ComPtr<IMMDevice> candidate;
    check (devices->Item (i, candidate.address ()), "output lookup");
    ComPtr<IPropertyStore> properties;
    check (candidate->OpenPropertyStore (STGM_READ, properties.address ()), "output properties");
    PROPVARIANT name; PropVariantInit (&name);
    result = properties->GetValue (PKEY_Device_FriendlyName, &name);
    bool match = SUCCEEDED (result) && name.vt == VT_LPWSTR
      && QString::fromWCharArray (name.pwszVal) == outputName;
    PropVariantClear (&name);
    if (match) {
      ++matches;
      if (!s.device.p) { s.device.p = candidate.p; candidate.p = nullptr; }
    }
  }
  if (matches != 1)
    throw std::runtime_error (QString ("WASAPI output '%1': found %2 active matches; select a unique Windows playback device.")
      .arg (outputName).arg (matches).toStdString ());
  check (s.device->Activate (__uuidof (IAudioClient), CLSCTX_ALL, nullptr,
    reinterpret_cast<void**> (s.client.address ())), "output activation");
  WAVEFORMATEX format {};
  format.wFormatTag = WAVE_FORMAT_PCM;
  format.nChannels = 1;
  format.nSamplesPerSec = sampleRate;
  format.wBitsPerSample = 16;
  format.nBlockAlign = 2;
  format.nAvgBytesPerSec = sampleRate * 2;
  // Windows performs channel mixing and rate conversion to the detector's
  // exact PCM contract, including outputs running at 44.1 or 96 kHz.
  check (s.client->Initialize (AUDCLNT_SHAREMODE_SHARED,
    AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM
      | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
    1000000, 0, &format, nullptr), "loopback initialization");
  check (s.client->GetService (__uuidof (IAudioCaptureClient),
    reinterpret_cast<void**> (s.capture.address ())), "capture service");
  check (s.client->Start (), "capture start");
  s.running = true;
}

WasapiLoopback::~WasapiLoopback () = default;

QByteArray WasapiLoopback::read ()
{
  QByteArray pcm;
  auto& s = *impl_;
  if (!s.running) return pcm;
  UINT packet = 0;
  check (s.capture->GetNextPacketSize (&packet), "packet query");
  // Bound the work per timer tick. A backlog indicates an audio-thread stall;
  // report it instead of silently stretching or compressing captured audio.
  unsigned packets = 0;
  while (packet) {
    if (++packets > 100) throw std::runtime_error ("WASAPI capture backlog; restart Monitor.");
    BYTE* data = nullptr;
    UINT frames = 0;
    DWORD flags = 0;
    check (s.capture->GetBuffer (&data, &frames, &flags, nullptr, nullptr), "packet read");
    bool discontinuity = (flags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY) && !s.firstPacket;
    if (!discontinuity) {
      if (flags & AUDCLNT_BUFFERFLAGS_SILENT) pcm.append (QByteArray (int (frames * 2), '\0'));
      else pcm.append (reinterpret_cast<char const*> (data), int (frames * 2));
    }
    check (s.capture->ReleaseBuffer (frames), "packet release");
    s.firstPacket = false;
    if (discontinuity) throw std::runtime_error ("WASAPI audio discontinuity; restart Monitor to reset reception timing.");
    check (s.capture->GetNextPacketSize (&packet), "packet query");
  }
  return pcm;
}

void WasapiLoopback::suspend ()
{
  if (impl_->running) {
    check (impl_->client->Stop (), "capture stop");
    impl_->running = false;
  }
}

void WasapiLoopback::resume ()
{
  if (!impl_->running) {
    check (impl_->client->Reset (), "capture reset");
    check (impl_->client->Start (), "capture restart");
    impl_->firstPacket = true;
    impl_->running = true;
  }
}
#else
struct WasapiLoopback::Impl {};
WasapiLoopback::WasapiLoopback (QString const&, unsigned) { throw std::runtime_error ("WASAPI loopback requires Windows."); }
WasapiLoopback::~WasapiLoopback () = default;
QByteArray WasapiLoopback::read () { return {}; }
void WasapiLoopback::suspend () {}
void WasapiLoopback::resume () {}
#endif
