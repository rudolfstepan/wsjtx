#ifndef WSJTX_REIST16_QSO_ADDRESS_HPP
#define WSJTX_REIST16_QSO_ADDRESS_HPP
#include "Decoder/decodedtext.h"

namespace reist16 {
inline bool sameStation(QString left,QString right) {
  auto normalize=[](QString call) {
    return call.remove('<').remove('>').trimmed().toUpper();
  };
  return normalize(left)==normalize(right);
}
struct QsoAddress {
  bool fromUs;
  bool toUs;
};
inline QsoAddress qsoAddress(DecodedText const& message,QString const& stationCall) {
  QString sender,grid;
  message.deCallAndGrid(sender,grid);
  // Types 1/2 carry /P and /R explicitly. Preserve these suffixes when
  // identifying a station; a portable endpoint may share our base call.
  return {!stationCall.trimmed().isEmpty() && sameStation(sender,stationCall),
          !stationCall.trimmed().isEmpty() && sameStation(message.call(),stationCall)};
}
}
#endif
