#ifndef WSJTX_REIST16_TX_MESSAGE_HPP
#define WSJTX_REIST16_TX_MESSAGE_HPP
#include "wsjtx_config.h"
#include <QByteArray>
#include <QLineEdit>
#include <QVariant>
#include <QString>
extern "C" void genft8_(char*,int*,int*,char*,char*,int*,fortran_charlen_t,fortran_charlen_t);

namespace reist16 {
constexpr int max_message_characters = 37;

// Same pack77 round-trip contract as gen_reist16; no waveform, CAT or PTT.
inline bool validTxMessage (QString const& text)
{
  if (text.isEmpty () || text.size () > max_message_characters) return false;
  auto input = text.toUpper ().toLatin1 ().leftJustified (37, ' ');
  QByteArray sent (37, ' '), bits (77, '0');
  int tones[79] {}, i3 = -1, n3 = -1;
  genft8_ (input.data (), &i3, &n3, sent.data (), bits.data (), tones, 37, 37);
  return (i3 == 1 || i3 == 2) && sent == input;
}

inline void setMessageLimit (QLineEdit* field, bool enabled)
{
  char const* key = "reist16OriginalMaxLength";
  if (!field->property (key).isValid ()) field->setProperty (key, field->maxLength ());
  field->setMaxLength (enabled ? max_message_characters : field->property (key).toInt ());
}

// A queued CAT release can outlive the software frame. Never show RX while
// PTT is requested, its acknowledgement is pending, or the rig reports TX.
inline bool txIndicatorActive (bool frameActive, bool pttRequested, bool waitingForPtt, bool rigPtt)
{
  return frameActive || pttRequested || waitingForPtt || rigPtt;
}
}
#endif
