#ifndef WSJTX_WASAPI_LOOPBACK_HPP
#define WSJTX_WASAPI_LOOPBACK_HPP

#include <QByteArray>
#include <QString>
#include <memory>

// Open, poll and close on the same audio thread. No recording device or driver
// is installed: the Windows shared audio engine supplies PCM from an output.
class WasapiLoopback
{
public:
  WasapiLoopback (QString const& outputName, unsigned sampleRate);
  ~WasapiLoopback ();
  WasapiLoopback (WasapiLoopback const&) = delete;
  WasapiLoopback& operator= (WasapiLoopback const&) = delete;
  QByteArray read (); // signed PCM16, mono, requested sample rate
  void suspend ();
  void resume ();
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

#endif
