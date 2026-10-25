#include "Bridge.hpp"
#include "Iio.hpp"
#include "HackrfBridge/Hackrf.hpp"
#include "qt_helpers.hpp"
#include <QCoreApplication>
#include <QFile>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QHostAddress>
#include <QTimer>
#include <QDebug>
#include <cstring>
#include <memory>

namespace pluto {
QByteArray audioPacket(quint32 type,std::vector<float> const& mono) {
  AudioHeader header;
  header.type=type;
  header.length=quint32(mono.size()*2);
  QByteArray packet(64+int(header.length)*4,0);
  std::memcpy(packet.data(),&header,64);
  for(size_t k=0;k<mono.size();++k) {
    float value=mono[k];
    std::memcpy(packet.data()+64+8*k,&value,4);
    std::memcpy(packet.data()+68+8*k,&value,4);
  }
  return packet;
}
bool readAudioPacket(QByteArray const& packet,std::vector<float>& mono) {
  if(packet.size()<64) return false;
  AudioHeader h;
  std::memcpy(&h,packet.constData(),64);
  if(h.receiver!=0 || h.rate!=audio_rate || h.format!=3 || h.codec!=0 || h.type!=2
     || h.length!=audio_block*2 || packet.size()<64+int(h.length)*4
     || packet.size()>64+int(h.length)*8) return false;
  mono.resize(audio_block);
  for(int k=0;k<audio_block;++k) {
    float a,b;
    std::memcpy(&a,packet.constData()+64+8*k,4);
    std::memcpy(&b,packet.constData()+68+8*k,4);
    if(!std::isfinite(a) || !std::isfinite(b) || std::abs(a)>1.01 || std::abs(b)>1.01) return false;
    mono[k]=std::max(-1.f,std::min(1.f,.5f*(a+b)));
  }
  return true;
}
RadioWorker::RadioWorker(Options const& o) : options(o),rxFrequency(o.frequency),txFrequency(o.frequency) {}
RadioWorker::~RadioWorker() { stop(); }
void RadioWorker::start() { thread=std::thread([this]{run();}); }
void RadioWorker::stop() { quit=true; requestedPtt=false; available.notify_all(); if(thread.joinable()) thread.join(); }
void RadioWorker::ptt(bool on) { requestedPtt=on; available.notify_all(); }
void RadioWorker::tune(long long rx,long long tx) {rxFrequency=rx;txFrequency=tx;}
bool RadioWorker::submit(std::vector<float> const& samples) {
  std::lock_guard<std::mutex> guard(mutex);
  if(!waiting || !pending.empty() || !requestedPtt) return false;
  pending=samples;
  available.notify_one();
  return true;
}
void RadioWorker::run() {
  try {
    std::unique_ptr<Radio> hardware;
    auto receiveDeadline=std::chrono::steady_clock::now()+std::chrono::seconds(30);
    auto openHardware=[&] {
      if(options.simulate)return;
      if(!options.hackrf) {hardware.reset(new IioRadio(options.library,options.uri,options.frequency,options.rxGain,options.txGain));return;}
      while(!quit) {
        try {hardware.reset(new HackrfRadio(options.library,options.serial,rxFrequency.load(),int(options.rxGain),options.rxVga,int(options.txGain),options.amplitude,options.txAmp));return;}
        catch(std::exception const& e) {
          if(std::chrono::steady_clock::now()>=receiveDeadline)throw;
          emit waitingForDevice(QString("Waiting for HackRF: %1").arg(QString::fromUtf8(e.what())));
          std::unique_lock<std::mutex> guard(mutex);
          available.wait_for(guard,std::chrono::seconds(1),[this]{return quit.load();});
        }
      }
    };
    openHardware();
    if(quit)return;
    UsbModulator modulator;
    UsbDemodulator demodulator;
    bool active=false;
    unsigned txBlocks=0;
    auto txStarted=std::chrono::steady_clock::now();
    long long rx=options.frequency,tx=options.frequency;
    bool receiving=!options.hackrf || options.simulate;
    if(receiving)emit ready();
    else emit waitingForDevice("Waiting for first HackRF USB RX samples (up to 30 seconds)");
    auto due=std::chrono::steady_clock::now();
    while(!quit) {
      bool on=requestedPtt;
      if(on && !receiving) {requestedPtt=false;on=false;}
      auto changePtt=[&](bool value) {
        if(hardware) hardware->ptt(value);
        active=value;
        if(value) {txBlocks=0;txStarted=std::chrono::steady_clock::now();}
        else receiveDeadline=std::chrono::steady_clock::now()+std::chrono::seconds(30);
        modulator=UsbModulator{};
        demodulator=UsbDemodulator{};
        { std::lock_guard<std::mutex> guard(mutex); pending.clear();waiting=false; }
        emit pttReady(active);
        due=std::chrono::steady_clock::now();
      };
      // Complete all requested tuning while muted, before acknowledging PTT.
      if(active && !on) changePtt(false);
      auto newRx=rxFrequency.load(),newTx=txFrequency.load();
      if(newRx!=rx || newTx!=tx) {
        if(active) throw std::runtime_error("Frequency changed during SDR TX; frame aborted");
        if(hardware) hardware->tune(newRx,newTx);
        rx=newRx; tx=newTx;
        emit frequencyReady(rx,tx);
      }
      if(on && !active) changePtt(true);
      std::vector<float> audio;
      if(active) {
        std::unique_lock<std::mutex> guard(mutex);
        waiting=true;
        emit needAudio();
        bool got=available.wait_for(guard,std::chrono::milliseconds(250),[this]{return !pending.empty()||quit||!requestedPtt;});
        waiting=false;
        if(quit || !requestedPtt) { pending.clear(); continue; }
        if(!got || pending.size()!=audio_block) {
          pending.clear();
          requestedPtt=false;
          guard.unlock();
          // Stop RF and discard the damaged frame. An audio-client stall is
          // recoverable; IIO failures during unkey still take the fatal path.
          auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-txStarted).count();
          changePtt(false);
          emit txAborted(QString("SDR TX audio watchdog: no complete block for 250 ms (block %1, %2 ms since PTT); TX aborted, receiving")
                         .arg(txBlocks).arg(qlonglong(elapsed)));
          continue;
        }
        audio.swap(pending);
        guard.unlock();
        auto iq=modulator.process(audio.data(),int(audio.size()));
        if(hardware) hardware->transmit(iq);
        else {
          auto echo=demodulator.process(iq.data(),int(iq.size()));
          emit this->audio(audioPacket(1,echo));
        }
        ++txBlocks;
      } else if(hardware) {
        std::vector<IQ> iq;
        try {iq=hardware->receive();}
        catch(std::exception const& e) {
          if(!options.hackrf || std::chrono::steady_clock::now()>=receiveDeadline)throw;
          requestedPtt=false;receiving=false;
          emit waitingForDevice(QString("Reconnecting HackRF RX: %1").arg(QString::fromUtf8(e.what())));
          hardware.reset();openHardware();if(quit)break;
          hardware->tune(rx,tx);demodulator=UsbDemodulator{};
          continue;
        }
        if(iq.empty()) {
          if(std::chrono::steady_clock::now()>=receiveDeadline)throw std::runtime_error("HackRF supplied no USB RX samples for 30 seconds; check USB connection, driver and device ownership");
          requestedPtt=false;
          if(receiving) {receiving=false;emit waitingForDevice("Waiting for HackRF USB RX data; transmission disabled");}
          demodulator=UsbDemodulator{};
          continue;
        }
        receiveDeadline=std::chrono::steady_clock::now()+std::chrono::seconds(30);
        if(!receiving){receiving=true;emit ready();}
        auto decoded=demodulator.process(iq.data(),int(iq.size()));
        emit this->audio(audioPacket(1,decoded));
      } else {
        emit this->audio(audioPacket(1,std::vector<float>(audio_block,0)));
      }
      if(options.simulate) {
        due+=std::chrono::milliseconds(10);
        std::this_thread::sleep_until(due);
      }
    }
    if(hardware) hardware->ptt(false);
    emit pttReady(false);
  } catch(std::exception const& e) {
    // IioRadio's destructor mutes TX before any failure reaches the client.
    requestedPtt=false;
    emit fault(QString::fromUtf8(e.what()));
  }
}
Bridge::Bridge(Options const& o) : options(o),server("WSJT-X "+o.deviceName(),QWebSocketServer::NonSecureMode,this),
  radio(o),rxFrequency(o.frequency),txFrequency(o.frequency) {
  connect(&radio,&RadioWorker::ready,this,[this]{
    available=true;waitingDevice=false;lastError.clear();
    writeReady(true);
    if(client) {if(!greeted)greet();else if(streaming)send("audio_samplerate:48000;audio_start:0;");}
    qInfo().noquote()<<options.deviceName()<<QString(" bridge ready: ws://127.0.0.1:%1 | %2 | %3 Hz | USB | RX %4 dB, TX %5 dB")
       .arg(options.port).arg(options.simulate?"SIMULATION":options.hackrf?options.serial:options.uri).arg(rxFrequency).arg(options.rxGain).arg(options.txGain);
  });
  connect(&radio,&RadioWorker::waitingForDevice,this,[this](QString reason){
    available=false;waitingDevice=true;lastError=reason;keyed=false;
    radio.ptt(false);
    send("trx:0,false;audio_stop:0;");
    writeReady(false,reason);
    qWarning().noquote()<<reason;
  });
  connect(&radio,&RadioWorker::fault,this,&Bridge::fail);
  connect(&radio,&RadioWorker::txAborted,this,[this](QString reason){
    reportTxAbort(reason,"audio_timeout");
  });
  connect(&radio,&RadioWorker::audio,this,[this](QByteArray packet){
    if(client && streaming) {
      if(client->bytesToWrite()>256*1024) {fail("TCI receiver is not consuming audio");return;}
      client->sendBinaryMessage(packet);
    }
  });
  connect(&radio,&RadioWorker::needAudio,this,[this]{
    if(client && streaming && keyed) {
      AudioHeader h; h.type=3; h.length=audio_block*2;
      client->sendBinaryMessage(QByteArray(reinterpret_cast<char const*>(&h),64));
    }
  });
  connect(&radio,&RadioWorker::pttReady,this,[this](bool on){keyed=on;writeReady(available);send(QString("trx:0,%1;").arg(on?"true":"false"));});
  connect(&radio,&RadioWorker::frequencyReady,this,[this](qint64 rx,qint64 tx){
    rxFrequency=rx;txFrequency=tx;
    writeReady(available);
    send(QString("vfo:0,0,%1;vfo:0,1,%2;").arg(rx).arg(tx));
  });
  connect(&server,&QWebSocketServer::newConnection,this,[this]{
    auto socket=server.nextPendingConnection();
    if(client) {socket->close(QWebSocketProtocol::CloseCodePolicyViolated,"SDR already has a WSJT-X client");socket->deleteLater();return;}
    client=socket;
    greeted=false;
    connect(socket,&QWebSocket::disconnected,this,[this,socket]{
      radio.ptt(false);keyed=false;streaming=false;split=false;
      radio.tune(rxFrequency,rxFrequency);
      txFrequency=rxFrequency;
      if(client==socket) client=nullptr;
      socket->deleteLater();
    });
    connect(socket,&QWebSocket::textMessageReceived,this,&Bridge::commands);
    connect(socket,&QWebSocket::binaryMessageReceived,this,[this](QByteArray const& packet){
      std::vector<float> audio;
      if(!readAudioPacket(packet,audio)) {fail("Invalid TCI transmit audio packet");return;}
      if(keyed) radio.submit(audio);
    });
    if(available)greet();
  });
}
void Bridge::greet() {
    greeted=true;
    send(QString("protocol:1.9;device:%3;version:WSJTX-SDRBridge,1.1;receive_only:false;trx_count:1;channels_count:2;"
                 "vfo_limits:%4,%5;if_limits:-24000,24000;modulations_list:usb;"
                 "modulation:0,usb;vfo:0,0,%1;vfo:0,1,%2;split_enable:0,false;trx:0,false;"
                 "audio_samplerate:48000;start;ready;").arg(rxFrequency).arg(txFrequency).arg(options.deviceName()).arg(options.minimumFrequency()).arg(options.maximumFrequency()));
}
void Bridge::start() {
  if(!server.listen(QHostAddress::LocalHost,options.port)) {fail(server.errorString());return;}
  waitingDevice=true;writeReady(false,"Waiting for SDR device");radio.start();
}
Bridge::~Bridge() {
  server.close();
  if(client) {client->disconnect(this);client->close();}
  radio.stop();
}
void Bridge::writeReady(bool ok,QString const& reason) {
  if(options.readyFile.isEmpty()) return;
  QSaveFile file(options.readyFile);
  if(file.open(QIODevice::WriteOnly|QIODevice::Truncate)) {
    file.write(QJsonDocument(QJsonObject{{"device",options.deviceName()},{"serial",options.serial},{"sampleRate",rf_rate},{"ok",ok},{"state",ok?"ready":waitingDevice?"waiting":"failed"},{"error",reason.isEmpty()?lastError:reason},{"simulate",options.simulate},{"port",options.port},{"ptt",keyed},
               {"rxFrequency",double(rxFrequency)},{"txFrequency",double(txFrequency)},
               {"pid",double(QCoreApplication::applicationPid())},
               {"txAborts",int(txAbortCount)},{"lastTxWarning",lastTxWarning}}).toJson());
    file.commit();
  }
}
void Bridge::reportTxAbort(QString const& reason,QString const& code) {
  lastTxWarning=reason;++txAbortCount;
  qWarning().noquote()<<reason;
  writeReady(available);
  send(QString("tx_abort:0,%1;").arg(code));
}
void Bridge::fail(QString const& reason) {
  lastError=reason;
  qCritical().noquote()<<reason;
  available=false;
  waitingDevice=false;
  keyed=false;
  radio.ptt(false);
  radio.stop();
  writeReady(false,reason);
  if(client) {send("trx:0,false;stop;");client->close(QWebSocketProtocol::CloseCodeBadOperation,reason.left(100));}
  QTimer::singleShot(0,qApp,[]{QCoreApplication::exit(1);});
}
void Bridge::send(QString const& message) {if(client) client->sendTextMessage(message);}
void Bridge::commands(QString const& messages) {
  if(messages.size()>4096) {fail("Oversized TCI command");return;}
  for(auto command:messages.split(';',SkipEmptyParts)) {
    int colon=command.indexOf(':');
    QString name=command.left(colon<0?command.size():colon).toLower();
    auto args=(colon<0?QString():command.mid(colon+1)).split(',');
    if(name=="trx" && args.size()>=2 && args[0]=="0") {
      bool on=args[1]=="true";
      if(on && !available) {
        radio.ptt(false);send("trx:0,false;");
        reportTxAbort("SDR TX rejected: waiting for radio USB data; enable TX again after recovery","device_not_ready");
        continue;
      }
      if(on && (!available || !streaming)) {
        // A CAT/PTT test or late audio_start must not destroy the RX service.
        // Refuse this TX request; a later audio_start never keys by itself.
        radio.ptt(false);
        send("trx:0,false;");
        reportTxAbort("SDR TX rejected: start TCI audio before PTT; receiving continues","audio_not_started");
        continue;
      }
      radio.ptt(on);
      if(!on && !keyed) send("trx:0,false;");
    } else if(name=="vfo" && args.size()>=3 && args[0]=="0") {
      bool ok=false;
      auto f=args[2].toLongLong(&ok);
      if(!ok || f<options.minimumFrequency() || f>options.maximumFrequency() || keyed) {fail("Invalid SDR frequency or retune during TX");return;}
      auto rx=args[1]=="0"?f:rxFrequency;
      auto tx=args[1]=="1"?f:(split?txFrequency:f);
      if(rx==rxFrequency && tx==txFrequency) send(QString("vfo:0,0,%1;vfo:0,1,%2;").arg(rx).arg(tx));
      else radio.tune(rx,tx);
    } else if(name=="modulation" && args.size()>=2) {
      if(args.last().toLower()!="usb" && args.last().toLower()!="digu") {fail("SDR bridge supports USB/DIGU");return;}
      send("modulation:0,usb;");
    } else if(name=="split_enable" && !args.isEmpty()) {
      split=args.last()=="true";
      if(!split && txFrequency!=rxFrequency) radio.tune(rxFrequency,rxFrequency);
      send(QString("split_enable:0,%1;").arg(split?"true":"false"));
    } else if(name=="audio_start") {streaming=true;if(available)send("audio_samplerate:48000;audio_start:0;");}
    else if(name=="audio_stop") {streaming=false;radio.ptt(false);send("audio_stop:0;");}
    else if(name=="start") {if(available)send("start;");}
    else if(name=="stop") {radio.ptt(false);send("stop;");}
    else if(name=="smeter") send("smeter:0,0,-100;");
    else if(name=="drive") send("drive:0;"); // Hardware attenuation is set explicitly at bridge startup.
    else if(name=="rx_enable") send("rx_enable:0,true;");
  }
}
}
