#include "../../Network/WebControlServer.hpp"
#include <QApplication>
#include <QTcpSocket>
#include <QHostAddress>
#include <QEventLoop>
#include <QTimer>
#include <QPushButton>
#include <QLineEdit>
#include <QIntValidator>
#include <QSpinBox>
#include <QComboBox>
#include <QTextEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QJsonDocument>
#include <QJsonArray>
#include <iostream>
#include <stdexcept>

void check(bool condition, char const * message) { if (!condition) throw std::runtime_error(message); }

QByteArray request(WebControlServer& server, QByteArray method, QByteArray path, QByteArray body = {},
                   QByteArray key = {}, QByteArray extra = {}, bool split = false)
{
  QTcpSocket socket; QEventLoop loop; QTimer timeout; timeout.setSingleShot(true);
  QByteArray received;
  QObject::connect(&socket, &QTcpSocket::readyRead, [&] { received += socket.readAll(); });
  QObject::connect(&socket, &QTcpSocket::disconnected, &loop, &QEventLoop::quit);
  QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
  QObject::connect(&socket, &QTcpSocket::connected, [&] {
    QByteArray bytes = method + " " + path + " HTTP/1.1\r\nHost: localhost:" + QByteArray::number(server.serverPort()) + "\r\n";
    if (!key.isEmpty()) bytes += "Authorization: Bearer " + key + "\r\n";
    bytes += extra + "Content-Type: application/json\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body;
    if (split) {
      socket.write(bytes.left(bytes.size()-2));
      auto tail = bytes.right(2);
      QTimer::singleShot(20, &socket, [&socket, tail] { socket.write(tail); });
    } else socket.write(bytes);
  });
  timeout.start(2500); socket.connectToHost(QHostAddress::LocalHost, server.serverPort()); loop.exec();
  check(!received.isEmpty(), "HTTP response timeout"); return received;
}
QJsonObject json(QByteArray const& response) { return QJsonDocument::fromJson(response.mid(response.indexOf("\r\n\r\n")+4)).object(); }
bool status(QByteArray const& response, int code) { return response.startsWith("HTTP/1.1 " + QByteArray::number(code) + " "); }

int main(int argc, char ** argv)
{
  QApplication app(argc, argv);
  try {
    QWidget window, waterfall; QVBoxLayout layout(&window);
    QPushButton monitor("Monitor"), halt("Halt TX"), logButton("Log QSO");
    monitor.setObjectName("monitorButton"); monitor.setCheckable(true); halt.setObjectName("stopTxButton"); logButton.setObjectName("logQSOButton");
    layout.addWidget(&monitor); layout.addWidget(&halt); layout.addWidget(&logButton);
    QSpinBox frequency; frequency.setObjectName("RxFreqSpinBox"); frequency.setRange(200,5000); layout.addWidget(&frequency);
    QLineEdit call; call.setObjectName("dxCallEntry"); QIntValidator validator(1,999); call.setValidator(&validator); layout.addWidget(&call);
    QComboBox band; band.setObjectName("bandComboBox"); band.addItems({"14.074", "18.100"}); layout.addWidget(&band);
    QTextEdit decodes; decodes.setObjectName("decodedTextBrowser"); decodes.setPlainText("123000 -10 0.2 1234 ~ CQ TEST AA00"); layout.addWidget(&decodes);
    QDialog log; QVBoxLayout logLayout(&log); QLineEdit logCall; logCall.setObjectName("call"); logLayout.addWidget(&logCall);
    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel); buttons.setObjectName("buttonBox"); logLayout.addWidget(&buttons);
    QObject::connect(&buttons, &QDialogButtonBox::accepted, &log, &QDialog::accept);
    QObject::connect(&buttons, &QDialogButtonBox::rejected, &log, &QDialog::reject);
    QObject::connect(&logButton, &QPushButton::clicked, &log, &QDialog::show);
    int clicks=0, halts=0, selections=0, edits=0, decodesSelected=0;
    QObject::connect(&monitor, &QPushButton::clicked, [&] { ++clicks; });
    QObject::connect(&halt, &QPushButton::clicked, [&] { ++halts; });
    QObject::connect(&band, QOverload<int>::of(&QComboBox::activated), [&](int) { ++selections; });
    QObject::connect(&call, &QLineEdit::editingFinished, [&] { ++edits; });
    window.show(); waterfall.show(); app.processEvents();
    WebControlServer server(&window,&waterfall,&log);
    server.selectDecode = [&](QString const&,int) { ++decodesSelected; };
    check(server.start(0), "Listen failed"); auto key=server.token().toLatin1();
    if (app.arguments().contains("--serve")) {
      server.stationStatus = [] { return QJsonObject{{"callsign", "TEST"}, {"grid", "AA00"}, {"frequency", "14.074 000"}, {"mode", "FT8"}, {"utc", "12:30:00"}, {"monitoring", true}}; };
      std::cout << QJsonDocument(QJsonObject{{"port", server.serverPort()}, {"key", server.token()}}).toJson(QJsonDocument::Compact).constData() << std::endl;
      QTimer::singleShot(180000, &app, &QApplication::quit);
      return app.exec();
    }
    auto command = [&](QByteArray body) { return request(server,"POST","/api/command",body,key); };
    check(status(request(server,"GET","/"),200), "Embedded page missing");
    check(request(server,"GET","/app.js").contains("function render"), "Embedded JavaScript missing");
    check(status(request(server,"GET","/api/state"),401), "State accessible without key");
    check(status(request(server,"GET","/api/state",{},"wrong"),401), "Wrong key accepted");
    check(status(request(server,"POST","/api/command","{\"id\":\"monitorButton\",\"value\":true}"),401)&&clicks==0,"Unauthenticated mutation");
    auto state=json(request(server,"GET","/api/state",{},key));
    check(state["controls"].toObject().contains("monitorButton"), "Control state missing");
    check(state["decodedTextBrowser"].toArray().size()==1,"Decode missing");
    check(status(command("{\"id\":\"monitorButton\",\"value\":true}"),200)&&monitor.isChecked()&&clicks==1,"Monitor did not use desktop click");
    command("{\"id\":\"monitorButton\",\"value\":true}");check(clicks==1,"Desired state toggled on retry");
    check(status(command("{\"id\":\"monitorButton\",\"value\":1}"),409),"Invalid boolean accepted");
    monitor.setEnabled(false);check(status(command("{\"id\":\"monitorButton\",\"value\":false}"),409),"Disabled control accepted");monitor.setEnabled(true);
    check(status(command("{\"id\":\"RxFreqSpinBox\",\"value\":1500}"),200)&&frequency.value()==1500,"Frequency update failed");
    check(status(command("{\"id\":\"RxFreqSpinBox\",\"value\":99999}"),409)&&frequency.value()==1500,"Out of range frequency accepted");
    check(status(command("{\"id\":\"RxFreqSpinBox\",\"value\":1500.5}"),409),"Fractional frequency accepted");
    check(status(command("{\"id\":\"dxCallEntry\",\"value\":\"123\"}"),200)&&edits==1,"Editing signal missing");
    check(status(command("{\"id\":\"dxCallEntry\",\"value\":\"abc\"}"),409)&&call.text()=="123","Validator bypassed");
    check(status(command("{\"id\":\"bandComboBox\",\"value\":1}"),200)&&selections==1,"Band activation missing");
    check(status(command("{\"id\":\"notAllowed\"}"),409),"Unknown control accepted");
    check(status(command("{\"id\":\"decodedTextBrowser\",\"value\":0,\"text\":\"old\"}"),409)&&decodesSelected==0,"Stale decode accepted");
    QJsonObject select{{"id","decodedTextBrowser"},{"value",0},{"text",decodes.toPlainText()}};
    check(status(command(QJsonDocument(select).toJson()),200)&&decodesSelected==1,"Decode selection failed");
    check(status(request(server,"POST","/api/command","{}",key,"Origin: http://evil.example\r\n"),403),"Cross origin command accepted");
    check(status(request(server,"POST","/api/command","{",key),400),"Malformed JSON accepted");
    check(status(request(server,"POST","/api/command","{}",key,"Transfer-Encoding: chunked\r\n"),400),"Ambiguous framing accepted");
    check(status(request(server,"POST","/api/command","{}",key,"Content-Length: 2\r\n"),400),"Duplicate length accepted");
    check(status(request(server,"POST","/api/command","{\"id\":\"RxFreqSpinBox\",\"value\":1600}",key,{},true),200)&&frequency.value()==1600,"Fragmented request failed");
    check(status(request(server,"GET","/api/waterfall",{},key),200),"Waterfall missing");
    waterfall.hide();check(status(request(server,"GET","/api/waterfall",{},key),404),"Hidden waterfall captured");
    command("{\"id\":\"logQSOButton\"}");check(log.isVisible(),"Log dialog not opened");
    check(status(command("{\"id\":\"log.call\",\"value\":\"TEST\"}"),200)&&logCall.text()=="TEST","Log edit failed");
    check(status(command("{\"id\":\"monitorButton\",\"value\":false}"),409),"Station editable during logging");
    check(status(command("{\"id\":\"log.accept\"}"),200)&&!log.isVisible(),"Log accept failed");
    QDialog modal(&window);modal.setModal(true);modal.show();app.processEvents();
    check(json(request(server,"GET","/api/state",{},key))["blocked"].toBool(),"Modal state missing");
    check(status(command("{\"id\":\"monitorButton\",\"value\":false}"),409),"Modal bypassed");
    check(status(command("{\"id\":\"stopTxButton\"}"),200)&&halts==1,"Halt TX unavailable during modal");modal.hide();
    check(server.start(0)&&server.token().toLatin1()!=key,"Key not rotated");
    check(status(request(server,"GET","/api/state",{},key),401),"Old key accepted");server.stop();check(!server.isListening(),"Server not stopped");
    std::cout << "Web control: HTTP, authentication, controls, validation, decodes, logging, waterfall and lifecycle passed\n";
  } catch(std::exception const& e) { std::cerr << e.what() << '\n';return 1; }
}
