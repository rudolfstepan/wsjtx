#include <QtTest>
#include <QApplication>
#include <QBuffer>
#include <QFontDatabase>
#include <QThread>
#include <limits>
#include "commons.h"
#include "Modulator/Modulator.hpp"
#include "Audio/soundout.h"
#include "Transceiver/Transceiver.hpp"
#include "widgets/TxQualityDialog.hpp"

extern "C" { decltype(foxcom_) foxcom_; }
int volatile itone[250] = {};
int volatile icw[250] = {};
float gran () { return 0.f; }

// No audio device is opened. The production modulator is drained into memory.
class MemoryOutput final : public SoundOutput
{
public:
  void restart (QIODevice *) override {}
  void stop () override {}
  void reset () override {}
};
class TimedModulator final : public Modulator
{
public:
  TimedModulator () : Modulator {48000, 15.0} {}
  qint64 now = 500;
protected:
  qint64 millisecondsSinceEpoch () const override {return now;}
};

class TxAudioChecks final : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void waveformSnapshotAndExactEnd_data ()
  {
    QTest::addColumn<QString> ("mode");
    QTest::addColumn<int> ("symbols");
    QTest::addColumn<int> ("nsps");
    QTest::addColumn<double> ("spacing");
    QTest::addColumn<double> ("period");
    QTest::addColumn<qint64> ("now");
    QTest::newRow ("FT8") << QString ("FT8") << 79 << 1920 << -3. << 15. << qint64 (500);
    QTest::newRow ("FT4") << QString ("FT4") << 105 << 576 << -2. << 7.5 << qint64 (300);
    QTest::newRow ("FT2") << QString ("FT2") << 105 << 288 << -2. << 3.75 << qint64 (150);
    QTest::newRow ("REIST16") << QString ("REIST16") << 120 << 960 << -4. << 15. << qint64 (500);
  }
  void waveformSnapshotAndExactEnd ()
  {
    QFETCH (QString, mode);
    QFETCH (int, symbols);
    QFETCH (int, nsps);
    QFETCH (double, spacing);
    QFETCH (double, period);
    QFETCH (qint64, now);
    MemoryOutput output;
    TimedModulator modulator;
    modulator.now = now;
    int const frames = symbols * 4 * nsps;
    QVector<float> wave (frames, 0.25f);
    modulator.setWaveform (wave);
    wave.fill (-0.75f); // GUI-side changes must detach, not alter active audio.
    foxcom_.wave[0] = -0.9f;
    foxcom_.wave[frames] = 0.9f; // stale tail sample from a previous, longer mode
    modulator.start (mode, symbols, nsps, 1500., spacing, &output, AudioDevice::Both, true, false, 99., period);
    QByteArray audio;
    while (audio.size () <= frames * 4 + 8192) {
      auto block = modulator.read (4096);
      if (block.isEmpty ()) break;
      audio += block;
    }
    QCOMPARE (audio.size (), frames * 4);
    auto samples = reinterpret_cast<qint16 const *> (audio.constData ());
    for (int i = 0; i < frames * 2; ++i) QCOMPARE (samples[i], qint16 (8192));
    QVERIFY (!modulator.isActive ());
  }
  void lateStartIsReported_data () { waveformSnapshotAndExactEnd_data (); }
  void lateStartIsReported ()
  {
    QFETCH (QString, mode);
    QFETCH (int, symbols);
    QFETCH (int, nsps);
    QFETCH (double, spacing);
    QFETCH (double, period);
    QFETCH (qint64, now);
    MemoryOutput output;
    TimedModulator modulator;
    modulator.now = now + 600;
    modulator.setWaveform (QVector<float> (symbols * nsps * 4, 0.1f));
    QSignalSpy timing {&modulator, &Modulator::startTiming};
    modulator.start (mode, symbols, nsps, 1500., spacing, &output, AudioDevice::Mono, true, false, 99., period);
    QCOMPARE (timing.count (), 1);
    QCOMPARE (timing.first ().at (1).toInt (), 600);
  }
  void legacyWaveformsClearSnapshot_data ()
  {
    QTest::addColumn<QString> ("mode");
    QTest::addColumn<int> ("symbols");
    QTest::addColumn<int> ("nsps");
    QTest::addColumn<double> ("spacing");
    QTest::addColumn<double> ("period");
    QTest::addColumn<qint64> ("now");
    QTest::newRow ("FT8 Fox") << QString ("FT8") << 79 << 1920 << -1. << 15. << qint64 (500);
  }
  void legacyWaveformsClearSnapshot ()
  {
    QFETCH (QString, mode);
    QFETCH (int, symbols);
    QFETCH (int, nsps);
    QFETCH (double, spacing);
    QFETCH (double, period);
    QFETCH (qint64, now);
    MemoryOutput output;
    TimedModulator modulator;
    modulator.now = now;
    int frames = symbols * nsps * 4;
    std::fill (foxcom_.wave, foxcom_.wave + frames + 1, .5f);
    itone[0] = 0;
    icw[0] = 0;
    modulator.setWaveform (QVector<float> (79 * 7680, -.25f));
    modulator.start (mode, symbols, nsps, 1500., spacing, &output, AudioDevice::Mono, true, false, 99., period);
    QByteArray audio;
    while (audio.size () <= frames * 2 + 4096) {
      auto block = modulator.read (4096);
      if (block.isEmpty ()) break;
      audio += block;
    }
    QCOMPARE (audio.size (), frames * 2);
    auto samples = reinterpret_cast<qint16 const *> (audio.constData ());
    for (int i = 0; i < frames; ++i) QCOMPARE (samples[i], qint16 (16384));
    QVERIFY (!modulator.isActive ());
  }
  void queuedWaveformAcrossThreads ()
  {
    qRegisterMetaType<QVector<float>> ("QVector<float>");
    QThread thread;
    auto output = new MemoryOutput;
    auto modulator = new TimedModulator;
    output->moveToThread (&thread);
    modulator->moveToThread (&thread);
    connect (&thread, &QThread::finished, output, &QObject::deleteLater);
    connect (&thread, &QThread::finished, modulator, &QObject::deleteLater);
    QVector<float> waveform (79 * 7680, 0.25f);
    QVERIFY (QMetaObject::invokeMethod (modulator, "setWaveform", Qt::QueuedConnection,
      Q_ARG (QVector<float>, waveform)));
    waveform.fill (-0.5f);
    thread.start ();
    QByteArray audio;
    bool invoked = QMetaObject::invokeMethod (modulator, [&] {
      modulator->start ("FT8", 79, 1920., 1500., -3., output, AudioDevice::Mono, true, false, 99., 15.);
      audio = modulator->read (1024);
      modulator->stop ();
    }, Qt::BlockingQueuedConnection);
    thread.quit ();
    QVERIFY (thread.wait (5000));
    QVERIFY (invoked);
    QCOMPARE (audio.size (), 1024);
    QCOMPARE (*reinterpret_cast<qint16 const *> (audio.constData ()), qint16 (8192));
  }
  void unavailableAudioDoesNotReadSource ()
  {
    SoundOutput output;
    QSignalSpy errors {&output, &SoundOutput::error};
    QBuffer source;
    source.setData (QByteArray (4096, 'x'));
    QVERIFY (source.open (QIODevice::ReadOnly));
    output.setFormat (QAudioDeviceInfo {}, 1);
    output.restart (&source);
    QCOMPARE (errors.count (), 1);
    QCOMPARE (source.pos (), qint64 (0));
  }
  void alcStateAndUnavailableValues ()
  {
    Transceiver::TransceiverState original, changed;
    QCOMPARE (original.alc (), -1.f);
    changed.alc (0.25f);
    QVERIFY (changed != original);
    QCOMPARE (changed.alc (), 0.25f);
    changed.alc (std::numeric_limits<float>::quiet_NaN ());
    QVERIFY (changed == original);
    changed.alc (std::numeric_limits<float>::infinity ());
    QVERIFY (changed == original);
  }
  void qualityDialogAndSmpControls ()
  {
    QFontDatabase::addApplicationFont ("C:/Windows/Fonts/segoeui.ttf");
    QApplication::setFont (QFont {"Segoe UI", 10});
    TxQualityDialog::Context context {"FT8", "Test USB CODEC", 1855, 16, 0, true, true, true, 0.25f, 3.0};
    bool parallel = true;
    int threads = 0;
    TxQualityDialog dialog {nullptr, [&] {return context;}, [&] (bool b) {parallel = b;}, [&] (int n) {threads = n;}};
    dialog.refresh ();
    QVERIFY (dialog.findChild<QLabel *> ("txQualityContext")->text ().contains ("1855"));
    QVERIFY (dialog.findChild<QLabel *> ("txQualityAlc")->text ().contains ("0.250"));
    dialog.startTiming ("FT8", 600);
    QVERIFY (dialog.findChild<QLabel *> ("txQualityTiming")->text ().contains ("600"));
    dialog.findChild<QCheckBox *> ("txQualityParallel")->click ();
    QVERIFY (!parallel);
    auto combo = dialog.findChild<QComboBox *> ("txQualityThreads");
    QVERIFY (QMetaObject::invokeMethod (combo, "activated", Q_ARG (int, 4)));
    QCOMPARE (threads, 4);
    for (auto const& mode : {QString ("FT4"), QString ("FT2")}) {
      context.mode = mode;
      dialog.refresh ();
      QVERIFY (!dialog.findChild<QCheckBox *> ("txQualityParallel")->isEnabled ());
      QVERIFY (!combo->isEnabled ());
      QVERIFY (!dialog.findChild<QLabel *> ("txQualitySync")->isHidden ());
    }
    dialog.resize (650, 430);
    dialog.show ();
    QTest::qWait (20);
    QDir {}.mkpath (".local");
    QVERIFY (dialog.grab ().save (".local/tx-quality-preview.png"));
  }
};
QTEST_MAIN (TxAudioChecks)
#include "txaudio_checks.moc"
