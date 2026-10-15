#ifndef MODULATOR_HPP__
#define MODULATOR_HPP__

#include <QAudio>
#include <QPointer>
#include <QVector>
#include <atomic>

#include "Audio/AudioDevice.hpp"

class SoundOutput;

//
// Input device that generates PCM audio frames that encode a message
// and an optional CW ID.
//
// Output can be muted while underway, preserving waveform timing when
// transmission is resumed.
//
class Modulator
  : public AudioDevice
{
  Q_OBJECT;

public:
  enum ModulatorState {Synchronizing, Active, Idle};

  Modulator (unsigned frameRate, double periodLengthInSeconds, QObject * parent = nullptr);

  void close () override;

  bool isTuning () const {return m_tuning;}
  double frequency () const {return m_frequency;}
  bool isActive () const {return m_state != Idle;}
  void setSpread(double s) {m_fSpread=s;}
  void setTRPeriod(double p) {m_period=p;}
  void set_nsym(int n) {m_symbolsLength=n;}
  void set_ms0(qint64 ms) {m_ms0=ms;}

  Q_SLOT void start (QString mode, unsigned symbolsLength, double framesPerSymbol, double frequency,
                     double toneSpacing, SoundOutput *, Channel = Mono,
                     bool synchronize = true, bool fastMode = false,
                     double dBSNR = 99., double TRperiod=60.0);
  Q_SLOT void stop (bool quick = false);
  Q_SLOT void tune (bool newState = true);
  Q_SLOT void setFrequency (double newFrequency) {m_frequency = newFrequency;}
  // Delivered through the audio thread's event queue, never shared with GUI writers.
  Q_SLOT void setWaveform (QVector<float> const& waveform) {m_waveform = waveform;}
  Q_SIGNAL void startTiming (QString mode, int skippedMilliseconds) const;
  Q_SIGNAL void stateChanged (ModulatorState) const;

protected:
  virtual qint64 millisecondsSinceEpoch () const;
  qint64 readData (char * data, qint64 maxSize) override;
  qint64 writeData (char const * /* data */, qint64 /* maxSize */) override
  {
    return -1;			// we don't consume data
  }

private:
  qint16 postProcessSample (qint16 sample) const;

  QPointer<SoundOutput> m_stream;
  QVector<float> m_waveform;
  bool m_quickClose;

  std::atomic<unsigned> m_symbolsLength;

  static double constexpr m_twoPi = 2.0 * 3.141592653589793238462;
  unsigned m_nspd = 2048 + 512; // CW ID WPM factor = 22.5 WPM

  double m_phi;
  double m_dphi;
  double m_amp;
  double m_nsps;
  std::atomic<double> m_frequency;
  double m_frequency0;
  double m_snr;
  double m_fac;
  double m_toneSpacing;
  std::atomic<double> m_fSpread;
  double m_TRperiod;
  std::atomic<double> m_period;

  qint64 m_silentFrames;
  std::atomic<qint64> m_ms0;
  qint16 m_ramp;


  unsigned m_frameRate;
  std::atomic<ModulatorState> m_state;

  std::atomic<bool> m_tuning;
  bool m_addNoise;
  bool m_bFastMode;

  bool m_cwLevel;
  unsigned m_ic;
  unsigned m_icmin;
  unsigned m_icmax;
  unsigned m_isym0;
  int m_j0;
  double m_toneFrequency0;
};

#endif
