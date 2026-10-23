#include "Audio/WasapiLoopback.hpp"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QThread>
#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>

int main (int argc, char** argv)
{
  QCoreApplication app (argc, argv);
  try {
    bool rejected = false;
    try { WasapiLoopback missing ("WSJTX nonexistent output 4bdde510", 48000); }
    catch (std::exception const&) { rejected = true; }
    if (!rejected) throw std::runtime_error ("Missing endpoint silently accepted");
    if (argc < 2) { std::cout << "Missing endpoint rejected; no audio opened.\n"; return 0; }
    for (unsigned rate : {48000u, 12000u}) {
      WasapiLoopback capture (QString::fromLocal8Bit (argv[1]), rate);
      for (int pass = 0; pass < 2; ++pass) {
        qint64 samples = 0;
        double sum = 0, peak = 0;
        QElapsedTimer timer; timer.start ();
        while (timer.elapsed () < 3000) {
          auto pcm = capture.read ();
          if (pcm.size () % 2) throw std::runtime_error ("Torn PCM frame");
          for (int i = 0; i < pcm.size (); i += 2) {
            qint16 sample; std::memcpy (&sample, pcm.constData () + i, 2);
            double value = sample / 32768.; sum += value * value;
            peak = std::max (peak, std::abs (value)); ++samples;
          }
          QThread::msleep (10);
        }
        double rms = std::sqrt (sum / std::max<qint64> (1, samples));
        std::cout << "rate=" << rate << " pass=" << pass << " samples=" << samples
          << " rms=" << rms << " peak=" << peak << std::endl;
        if (samples < rate * 2 || samples > rate * 4 || rms < .001)
          throw std::runtime_error ("Expected active playback with audible PCM and correct capture rate");
        capture.suspend ();
        if (!capture.read ().isEmpty ()) throw std::runtime_error ("Suspended capture produced audio");
        QThread::msleep (100);
        capture.resume ();
      }
    }
    std::cout << "Live loopback PCM/rate/suspend/resume checks passed.\n";
    return 0;
  } catch (std::exception const& e) { std::cerr << e.what () << '\n'; return 1; }
}
