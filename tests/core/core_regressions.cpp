#include <QtTest>
#include <QUdpSocket>
#include <QHostInfo>
#include "Network/MessageClient.hpp"
#include "Network/NetworkMessage.hpp"
#include "logbook/ADIFRecordReader.hpp"

class CoreRegressions final : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void adifBufferedRecordsAtEof_data ()
  {
    QTest::addColumn<QString> ("contents");
    QTest::addColumn<int> ("expected");
    QTest::newRow ("one-line") << QStringLiteral ("<CALL:5>K1ABC <EOR>") << 1;
    QTest::newRow ("two-on-last-line") << QStringLiteral ("<CALL:5>K1ABC <eor><CALL:5>W9XYZ <EOR>") << 2;
    QTest::newRow ("multiline") << QStringLiteral ("<CALL:5>K1ABC\n<EOR>\n<CALL:5>W9XYZ <EOR>") << 2;
    QTest::newRow ("several-on-final-line") << QStringLiteral ("<CALL:5>K1ABC <EOR>\n<CALL:5>N0ABC <EOR><CALL:5>W9XYZ <EOR>") << 3;
    QTest::newRow ("trailing-newline") << QStringLiteral ("<CALL:5>K1ABC <EOR><CALL:5>W9XYZ <EOR>\n") << 2;
    QTest::newRow ("truncated-tail") << QStringLiteral ("<CALL:5>K1ABC <EOR><CALL:5>W9XYZ") << 1;
    QTest::newRow ("no-terminator") << QStringLiteral ("<CALL:5>K1ABC") << 0;
  }

  void adifBufferedRecordsAtEof ()
  {
    QFETCH (QString, contents);
    QFETCH (int, expected);
    QTextStream in {&contents, QIODevice::ReadOnly};
    // WorkedBefore pre-reads at least one line to find the optional header.
    QString buffer = in.readLine () + '\n';
    QStringList records;
    for (auto record = next_adif_record (in, buffer); !record.isEmpty (); record = next_adif_record (in, buffer))
      records.append (record);
    QCOMPARE (records.size (), expected);
    if (expected > 0) QVERIFY (records.first ().contains ("K1ABC"));
    if (expected > 1) QVERIFY (records.last ().contains ("W9XYZ"));
  }

  void udpCommandsMustMatchInstance ()
  {
    QUdpSocket server;
    QVERIFY (server.bind (QHostAddress {QHostAddress::LocalHost}, 0));
    // This is only a protocol client, with no radio or MainWindow attached.
    auto client = new MessageClient {"WSJT-X - Test", "test", "test", "127.0.0.1", server.localPort (), {}, 1, this};
    client->enable (true);
    QSignalSpy halts {client, &MessageClient::halt_tx};
    QTRY_VERIFY (server.hasPendingDatagrams ());
    QHostAddress address;
    quint16 port = 0;
    QByteArray heartbeat;
    heartbeat.resize (server.pendingDatagramSize ());
    QVERIFY (server.readDatagram (heartbeat.data (), heartbeat.size (), &address, &port) > 0);
    auto send = [&] (QString const& id) {
      QByteArray bytes;
      NetworkMessage::Builder out {&bytes, NetworkMessage::HaltTx, id, 2};
      out << false;
      QCOMPARE (server.writeDatagram (bytes, address, port), qint64 (bytes.size ()));
    };
    send ("WSJT-X - Other");
    send (QString {});
    send ("WSJT-X - Test");
    QTRY_VERIFY (halts.count () >= 1);
    QCOMPARE (halts.count (), 1);
    client->enable (false);
    send ("WSJT-X - Test");
    QTest::qWait (50);
    QCOMPARE (halts.count (), 1);
    delete client;
  }

  void staleDnsMustNotRestorePreviousServer_data ()
  {
    QTest::addColumn<QString> ("replacement");
    QTest::newRow ("numeric-address") << QStringLiteral ("127.0.0.2");
    QTest::newRow ("disabled-server") << QString {};
  }

  void staleDnsMustNotRestorePreviousServer ()
  {
    QFETCH (QString, replacement);
    auto client = new MessageClient {"test", "test", "test", "", 0, {}, 1, this};
    client->set_server ("localhost", {});
    client->set_server (replacement, {});
    // Wait for a real, local lookup to finish and queued callbacks to run.
    bool lookupFinished = false;
    QHostInfo::lookupHost ("localhost", this, [&] (QHostInfo) { lookupFinished = true; });
    QTRY_VERIFY (lookupFinished);
    QTest::qWait (100);
    QCOMPARE (client->server_address (), QHostAddress {replacement});
    delete client;
  }
};

QTEST_GUILESS_MAIN (CoreRegressions)
#include "core_regressions.moc"
