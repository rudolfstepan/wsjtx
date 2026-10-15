#ifndef ADIF_RECORD_READER_HPP
#define ADIF_RECORD_READER_HPP

#include <QString>
#include <QTextStream>

// buffer may already contain records read while locating the optional header.
inline QString next_adif_record (QTextStream& in, QString& buffer)
{
  int end_position = buffer.indexOf ("<EOR>", 0, Qt::CaseInsensitive);
  while (end_position < 0 && !in.atEnd ())
    {
      buffer += in.readLine () + '\n';
      end_position = buffer.indexOf ("<EOR>", 0, Qt::CaseInsensitive);
    }
  // EOF applies to the stream, not to complete records still in the buffer.
  if (end_position < 0) return {};
  auto record = buffer.left (end_position + 5).trimmed ();
  buffer.remove (0, end_position + 5);
  return record.mid (record.indexOf (QChar {'<'}));
}

#endif
