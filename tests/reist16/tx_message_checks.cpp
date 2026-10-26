#include "lib/reist16/tx_message.hpp"
#include "lib/reist16/qso2.hpp"
#include <QApplication>
#include <QComboBox>
#include <iostream>
#include <stdexcept>
void check (bool ok, char const* message) { if (!ok) throw std::runtime_error (message); }
int main (int argc, char** argv)
{
  qputenv ("QT_QPA_PLATFORM", "offscreen");
  QApplication app (argc, argv);
  try {
    for(int frequency=0;frequency<=5000;++frequency) {
      int offset=reist16::qso2::split_offset(frequency);
      int audio=frequency-offset;
      check(audio>=1500 && audio+reist16::qso2::tone_span<=2000,"All REIST16 split tones inside filter centre");
      check(14000000+offset+audio==14000000+frequency,"Split must preserve RF frequency");
    }
    for (QString message : {"CQ OE3SRO JN88", "CQ TEST OE3SRO JN88", "CQ DX OE3SRO JN88",
         "OE3SRO DL1ABC JO31", "DL1ABC OE3SRO -12", "OE3SRO DL1ABC R-10",
         "DL1ABC OE3SRO RR73", "OE3SRO DL1ABC 73"})
      check (reist16::validTxMessage (message), "Valid CQ/QSO rejected before PTT");
    for (QString message : QStringList {"", "CQ ABCDE OE3SRO JN88", "HELLO THIS IS A MUCH LONGER MESSAGE", QString (38, 'A')})
      check (!reist16::validTxMessage (message), "Unsupported/oversized message accepted before PTT");
    QLineEdit field;
    field.setMaxLength (80);
    reist16::setMessageLimit (&field, true);
    field.insert (QString (100, 'A'));
    check (field.text ().size () == 37 && field.maxLength () == 37, "Typed/pasted message exceeds serializer capacity");
    reist16::setMessageLimit (&field, true);
    reist16::setMessageLimit (&field, false);
    check (field.maxLength () == 80, "Mode switch failed to restore previous input limit");
    QComboBox combo; combo.setEditable (true);
    reist16::setMessageLimit (combo.lineEdit (), true);
    combo.lineEdit ()->setText (QString (100, 'B'));
    check (combo.currentText ().size () == 37, "Editable Tx5 exceeds serializer capacity");
    check (!reist16::txIndicatorActive (false,false,false,false), "Idle must show RX");
    check (reist16::txIndicatorActive (true,false,false,false), "Active frame must show TX");
    check (reist16::txIndicatorActive (false,true,false,false), "Asserted PTT must not show RX");
    check (reist16::txIndicatorActive (false,false,true,false), "Pending PTT must not show RX");
    check (reist16::txIndicatorActive (false,false,false,true), "Delayed physical PTT release must not show RX");
    std::cout << "PASS REIST16 CQ TEST/QSO preflight, unsupported text rejection, 37-character entry limits, mode restoration and PTT-aware TX indication. No radio/audio opened.\n";
    return 0;
  } catch (std::exception const& e) { std::cerr << e.what () << '\n'; return 1; }
}
