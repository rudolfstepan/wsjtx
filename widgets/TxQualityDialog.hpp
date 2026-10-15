#ifndef TX_QUALITY_DIALOG_HPP
#define TX_QUALITY_DIALOG_HPP

#include <QDialog>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QVBoxLayout>
#include <QTimer>
#include <functional>

class TxQualityDialog final : public QDialog
{
public:
  struct Context {
    QString mode, device;
    int audioFrequency, logicalCpus, requestedThreads;
    bool split, parallel, transmitting;
    float alc;
    double attenuation;
  };
  TxQualityDialog (QWidget *parent, std::function<Context ()> context,
                   std::function<void (bool)> parallel, std::function<void (int)> threads)
    : QDialog {parent}, context_ {context}
  {
    setWindowTitle (tr ("TX quality — FT8 / FT4 / FT2"));
    resize (650, 390);
    auto layout = new QVBoxLayout {this};
    auto description = new QLabel {tr ("Checks for complete audio, late starts and transmitter drive.\n"
      "These readings do not measure your radiated signal or guarantee reception."), this};
    description->setWordWrap (true);
    layout->addWidget (description);
    operating_ = new QLabel {this};
    operating_->setObjectName ("txQualityContext");
    operating_->setWordWrap (true);
    layout->addWidget (operating_);
    timing_ = new QLabel {tr ("Start timing: no transmission measured yet."), this};
    timing_->setObjectName ("txQualityTiming");
    timing_->setWordWrap (true);
    layout->addWidget (timing_);
    audio_ = new QLabel {tr ("Audio output: no stream started yet."), this};
    audio_->setObjectName ("txQualityAudio");
    audio_->setWordWrap (true);
    layout->addWidget (audio_);
    alc_ = new QLabel {tr ("ALC: unavailable."), this};
    alc_->setObjectName ("txQualityAlc");
    alc_->setWordWrap (true);
    layout->addWidget (alc_);
    parallel_ = new QCheckBox {tr ("Use multithreaded FT8 decoder"), this};
    parallel_->setObjectName ("txQualityParallel");
    layout->addWidget (parallel_);
    threads_ = new QComboBox {this};
    threads_->setObjectName ("txQualityThreads");
    threads_->addItem (tr ("Decoder threads: Auto (leave CPU time for audio)"), 0);
    for (int n = 1; n <= 12; ++n) threads_->addItem (tr ("Decoder threads: %1").arg (n), n);
    layout->addWidget (threads_);
    sync_ = new QLabel {this};
    sync_->setObjectName ("txQualitySync");
    sync_->setWordWrap (true);
    layout->addWidget (sync_);
    connect (parallel_, &QCheckBox::clicked, this, parallel);
    connect (threads_, QOverload<int>::of (&QComboBox::activated), this, threads);
    auto hint = new QLabel {tr ("Keep the FT-991A in DATA-USB with the correct USB audio source. "
      "Use Rig split or Fake It to keep transmit audio near 1500–2000 Hz. "
      "Adjust drive using the radio's ALC indication and an external signal check; "
      "ALC alone is not a distortion measurement."), this};
    hint->setWordWrap (true);
    layout->addWidget (hint);
    auto timer = new QTimer {this};
    connect (timer, &QTimer::timeout, this, [this] { if (isVisible ()) refresh (); });
    timer->start (1000);
  }

  void refresh ()
  {
    auto c = context_ ();
    operating_->setText (tr ("%1 · %2\nTX audio: %3 Hz · Split: %4 · Audio attenuation: %5 dB\n"
                            "%6 logical CPUs · Requested FT8 decoder threads: %7")
      .arg (c.mode, c.device).arg (c.audioFrequency).arg (c.split ? tr ("on") : tr ("off"))
      .arg (c.attenuation, 0, 'f', 1).arg (c.logicalCpus)
      .arg (c.requestedThreads ? QString::number (c.requestedThreads) : tr ("Auto")));
    parallel_->setChecked (c.parallel);
    parallel_->setEnabled (c.mode == "FT8");
    threads_->setCurrentIndex (qBound (0, c.requestedThreads, 12));
    threads_->setEnabled (c.mode == "FT8" && c.parallel);
    bool const ft4Family = c.mode == "FT4" || c.mode == "FT2";
    sync_->setVisible (ft4Family);
    sync_->setText (tr ("FT4 / FT2: automatic parallel synchronization search, up to 8 workers. "
      "CPU and OpenMP limits apply; fine searches run serially."));
    if (c.transmitting) {
      alc_->setText (c.alc >= 0.f
        ? tr ("ALC (CAT, relative scale): %1. Compare with the radio's meter.").arg (c.alc, 0, 'f', 3)
        : tr ("ALC: not reported by this connection. Check the radio's meter."));
    } else alc_->setText (tr ("ALC: shown during transmission when supported by the CAT connection."));
  }

  void startTiming (QString const& mode, int skippedMilliseconds)
  {
    timing_->setText (skippedMilliseconds
      ? tr ("%1: late audio start; %2 ms of the message skipped. Enable TX earlier for a complete frame.")
        .arg (mode).arg (skippedMilliseconds)
      : tr ("%1: no message samples skipped at audio preparation.").arg (mode));
    timing_->setToolTip (tr ("Software timing only. USB, driver and radio latency are not measured here."));
  }
  void audioStarted (QString const& device, int sampleRate, int bufferMilliseconds)
  {
    audio_->setText (tr ("Audio: %1 Hz · Buffer capacity: %2 ms · %3")
      .arg (sampleRate).arg (bufferMilliseconds).arg (device));
  }
  void audioError (QString const& error) { audio_->setText (tr ("Audio error: %1").arg (error)); }

private:
  std::function<Context ()> context_;
  QLabel *operating_, *timing_, *audio_, *alc_, *sync_;
  QCheckBox *parallel_;
  QComboBox *threads_;
};
#endif
