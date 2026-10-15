#include "soundout.h"

#include <QDateTime>
#include <QAudioDeviceInfo>
#include <QAudioOutput>
#include <QSysInfo>
#include <qmath.h>
#include <QDebug>

#include "Logger.hpp"
#include "Audio/AudioDevice.hpp"

#include "moc_soundout.cpp"

bool SoundOutput::checkStream () const
{
  bool result {false};

  Q_ASSERT_X (m_stream, "SoundOutput", "programming error");
  if (m_stream) {
    switch (m_stream->error ())
      {
      case QAudio::OpenError:
        Q_EMIT error (tr ("An error opening the audio output device has occurred."));
        break;

      case QAudio::IOError:
        Q_EMIT error (tr ("An error occurred during write to the audio output device."));
        break;

      case QAudio::UnderrunError:
        // Qt also enters Idle/Underrun after a finite source has ended.
        // A completed message is not a dropped-audio fault.
        if (m_sourceFinished) { result = true; break; }
        Q_EMIT error (tr ("Audio data not being fed to the audio output device fast enough."));
        break;

      case QAudio::FatalError:
        Q_EMIT error (tr ("Non-recoverable error, audio output device not usable at this time."));
        break;

      case QAudio::NoError:
        result = true;
        break;
      }
  }
  return result;
}

void SoundOutput::setFormat (QAudioDeviceInfo const& device, unsigned channels, int frames_buffered)
{
  Q_ASSERT (0 < channels && channels < 3);
  if (device != m_device || channels != m_channels) m_stream.reset ();
  m_device = device;
  m_channels = channels;
  m_framesBuffered = frames_buffered;
}

void SoundOutput::restart (QIODevice * source)
{
  if (!m_device.isNull ())
    {
      QAudioFormat format (m_device.preferredFormat ());
      //  qDebug () << "Preferred audio output format:" << format;
      format.setChannelCount (m_channels);
      format.setCodec ("audio/pcm");
      format.setSampleRate (48000);
      format.setSampleType (QAudioFormat::SignedInt);
      format.setSampleSize (16);
      format.setByteOrder (QAudioFormat::Endian (QSysInfo::ByteOrder));
      if (!format.isValid ())
        {
          m_stream.reset ();
          Q_EMIT error (tr ("Requested output audio format is not valid."));
          return;
        }
      else if (!m_device.isFormatSupported (format))
        {
          m_stream.reset ();
          Q_EMIT error (tr ("Requested output audio format is not supported on device."));
          return;
        }
      else
        {
          // qDebug () << "Selected audio output format:" << format;
          if (!m_stream) {
            m_stream.reset (new QAudioOutput (m_device, format));
            m_stream->setNotifyInterval (1000);
            connect (m_stream.data(), &QAudioOutput::stateChanged, this, &SoundOutput::handleStateChanged);
            connect (m_stream.data(), &QAudioOutput::notify, this, [this] () {checkStream ();});
          }
          m_stream->setVolume (m_volume);
          error_ = false;

          //      qDebug() << "A" << m_volume << m_stream->notifyInterval();
        }
    }
  else m_stream.reset ();
  if (!m_stream)
    {
      if (!error_)
        {
          error_ = true;        // only signal error once
          Q_EMIT error (tr ("No audio output device configured."));
        }
      return;
    }
  else
    {
      error_ = false;
    }

  // we have to set this before every start on the stream because the
  // Windows implementation seems to forget the buffer size after a
  // stop.
  //qDebug () << "SoundOut default buffer size (bytes):" << m_stream->bufferSize () << "period size:" << m_stream->periodSize ();
  if (m_framesBuffered > 0)
    {
      m_stream->setBufferSize (m_stream->format().bytesForFrames (m_framesBuffered));
    }
  m_stream->setCategory ("production");
  m_sourceFinished = false;
  m_stream->start (source);
  if (checkStream ()) Q_EMIT streamStarted (m_device.deviceName (), 48000,
    m_stream->format ().durationForBytes (m_stream->bufferSize ()) / 1000);
//  LOG_DEBUG ("Selected buffer size (bytes): " << m_stream->bufferSize () << " period size: " << m_stream->periodSize ());
}

void SoundOutput::suspend ()
{
  if (m_stream && QAudio::ActiveState == m_stream->state ())
    {
      m_stream->suspend ();
      checkStream ();
    }
}

void SoundOutput::resume ()
{
  if (m_stream && QAudio::SuspendedState == m_stream->state ())
    {
      m_stream->resume ();
      checkStream ();
    }
}

void SoundOutput::reset ()
{
  m_sourceFinished = true;
  if (m_stream)
    {
      m_stream->reset ();
      checkStream ();
    }
}

void SoundOutput::stop ()
{
  m_sourceFinished = true;
  if (m_stream)
    {
      m_stream->reset ();
      m_stream->stop ();
    }
#ifdef __APPLE__
  // this code is here to help certain rigs not drop audio, however on Sequoia this causes audio to drop, so we don't do it!
  if( QOperatingSystemVersion::current() < QOperatingSystemVersion(QOperatingSystemVersion::MacOS, 15) )
    m_stream.reset ();
#endif
}

qreal SoundOutput::attenuation () const
{
  return -(20. * qLn (m_volume) / qLn (10.));
}

void SoundOutput::setAttenuation (qreal a)
{
  Q_ASSERT (0. <= a && a <= 999.);
  m_volume = qPow(10.0, -a/20.0);
  //  qDebug () << "SoundOut: attn = " << a << ", vol = " << m_volume;
  if (m_stream)
    {
      m_stream->setVolume (m_volume);
    }
}

void SoundOutput::resetAttenuation ()
{
  m_volume = 1.;
  if (m_stream)
    {
      m_stream->setVolume (m_volume);
    }
}

void SoundOutput::handleStateChanged (QAudio::State newState)
{
  switch (newState)
    {
    case QAudio::IdleState:
      if (m_stream && m_stream->error () == QAudio::UnderrunError) checkStream ();
      Q_EMIT status (tr ("Idle"));
      break;

    case QAudio::ActiveState:
      Q_EMIT status (tr ("Sending"));
      break;

    case QAudio::SuspendedState:
      Q_EMIT status (tr ("Suspended"));
      break;

#if QT_VERSION >= QT_VERSION_CHECK (5, 10, 0)
    case QAudio::InterruptedState:
      Q_EMIT status (tr ("Interrupted"));
      break;
#endif

    case QAudio::StoppedState:
      if (!checkStream ())
        {
          Q_EMIT status (tr ("Error"));
        }
      else
        {
          Q_EMIT status (tr ("Stopped"));
        }
      break;
    }
}
