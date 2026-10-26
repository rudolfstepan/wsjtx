"""Black-box loopback tests of the packaged bridge; always --simulate, no RF."""
import base64
import json
import os
from pathlib import Path
import socket
import struct
import subprocess
import sys
import time
import uuid

root=Path(__file__).resolve().parents[1]
executable=Path(sys.argv[1]) if len(sys.argv)>1 else root/'build-custom-make/wsjtx_pluto_bridge.exe'
hackrf='hackrf' in executable.name.lower()
work=root/('build-custom-make/hackrf-checks/protocol' if hackrf else '.local/pluto-protocol-checks')
work.mkdir(parents=True,exist_ok=True)

def wait_until(test,seconds=5):
    deadline=time.monotonic()+seconds
    while time.monotonic()<deadline:
        value=test()
        if value:
            return value
        time.sleep(.01)
    raise AssertionError('Protocol test deadline')

class Client:
    def __init__(self,port):
        self.sock=socket.create_connection(('127.0.0.1',port),timeout=2)
        key=base64.b64encode(os.urandom(16)).decode()
        self.sock.sendall((f'GET / HTTP/1.1\r\nHost: 127.0.0.1:{port}\r\nUpgrade: websocket\r\n'
                          f'Connection: Upgrade\r\nSec-WebSocket-Key: {key}\r\nSec-WebSocket-Version: 13\r\n\r\n').encode())
        data=b''
        while b'\r\n\r\n' not in data:
            data+=self.sock.recv(8192)
        header,self.buffer=data.split(b'\r\n\r\n',1)
        assert b'101 Switching Protocols' in header
    def take(self,n):
        while len(self.buffer)<n:
            more=self.sock.recv(65536)
            if not more:
                raise EOFError('WebSocket closed')
            self.buffer+=more
        out,self.buffer=self.buffer[:n],self.buffer[n:]
        return out
    def frame(self):
        first,length=self.take(2)
        assert not length&128
        length&=127
        if length==126:
            length=struct.unpack('!H',self.take(2))[0]
        elif length==127:
            length=struct.unpack('!Q',self.take(8))[0]
        return first&15,self.take(length)
    def send(self,text):
        self.send_payload(1,text.encode())
    def send_payload(self,opcode,payload):
        mask=os.urandom(4)
        length=len(payload)
        header=bytes((0x80|opcode,0x80|length)) if length<126 else bytes((0x80|opcode,0xfe))+struct.pack('!H',length)
        self.sock.sendall(header+mask+bytes(v^mask[k%4] for k,v in enumerate(payload)))
    def close(self):
        self.sock.close()

def run_test(graceful=False,invalid=False):
    unique=uuid.uuid4().hex
    ready=work/f'{unique}-ready.json'
    stop=work/f'{unique}-stop'
    with socket.socket() as probe:
        probe.bind(('127.0.0.1',0))
        port=probe.getsockname()[1]
    log=open(work/f'{unique}.log','wb')
    process=subprocess.Popen([str(executable),'--simulate',('--serial' if hackrf else '--uri'),'test:'+unique,'--port',str(port),
                              '--ready-file',str(ready),'--stop-file',str(stop)],stdout=log,stderr=log,
                             creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
    def state():
        return json.loads(ready.read_text()) if ready.exists() else {}
    client=None
    try:
        def started():
            status=state()
            if status.get('state')=='failed' or process.poll() is not None:
                raise AssertionError(f'Bridge startup failed: {status}')
            return status.get('ok')
        wait_until(started,seconds=35)
        if graceful:
            stop.write_text('stop')
            assert process.wait(5)==0
            return
        client=Client(port)
        opcode,greeting=client.frame()
        assert opcode==1
        if hackrf:
            assert b'device:HackRF One;' in greeting and b'vfo_limits:1000000,6000000000;' in greeting
        if invalid:
            client.send_payload(2,bytes(64))
            assert process.wait(5)==1
            assert not state()['ok'] and not state()['ptt']
            assert 'Invalid TCI' in state()['error']
            return
        # Reproduce the reported crash: PTT with no TCI audio_start. The
        # bridge must reject TX, stay online and accept a later normal stream.
        client.send('trx:0,true;')
        wait_until(lambda:state().get('txAborts')==1)
        assert state()['ok'] and not state()['ptt'] and process.poll() is None
        assert 'start TCI audio' in state()['lastTxWarning']
        rejected=False
        while not rejected:
            opcode,payload=client.frame()
            rejected=opcode==1 and b'tx_abort:0,audio_not_started;' in payload
        client.send('audio_start:0;')
        receiving=False
        while not receiving:
            opcode,payload=client.frame()
            receiving=opcode==2 and struct.unpack_from('<I',payload,24)[0]==1
        assert not state()['ptt'], 'late audio_start must not key the rejected TX'
        second=Client(port)
        assert second.frame()[0]==8, 'second client was not rejected'
        second.close()
        client.send('audio_start:0;trx:0,true;')
        wait_until(lambda:state().get('ptt'))
        client.close()
        client=None
        wait_until(lambda:state().get('ptt') is False)
        assert process.poll() is None, 'disconnect must end PTT, keeping bridge reusable'
        client=Client(port)
        client.frame()
        client.send('audio_start:0;trx:0,true;')
        wait_until(lambda:state().get('ptt'))
        # Missing audio must unkey and discard this frame, preserving RX/CAT.
        wait_until(lambda:state().get('txAborts')==2)
        status=state()
        assert status['ptt'] is False and status['ok'] is True
        assert 'watchdog' in status['lastTxWarning'] and process.poll() is None
        aborted=False;receiving=False
        deadline=time.monotonic()+3
        while not (aborted and receiving) and time.monotonic()<deadline:
            opcode,payload=client.frame()
            if opcode==1 and b'tx_abort:0,audio_timeout;' in payload:
                aborted=True
            if aborted and opcode==2 and struct.unpack_from('<I',payload,24)[0]==1:
                receiving=True
        assert aborted and receiving, 'watchdog must notify WSJT-X and resume audio'
        audio=struct.pack('<16I',0,48000,3,0,0,960,2,*([0]*9))+bytes(960*4)
        # An audio reply that arrives after abort must not rekey or resume TX.
        client.send_payload(2,audio)
        assert not state()['ptt']
        client.send('trx:0,true;')
        wait_until(lambda:state().get('ptt'))
        answered=0
        while answered<30:
            opcode,payload=client.frame()
            if opcode==2 and struct.unpack_from('<I',payload,24)[0]==3:
                client.send_payload(2,audio);answered+=1
        client.send('trx:0,false;')
        wait_until(lambda:not state()['ptt'])
        assert state()['txAborts']==2 and process.poll() is None, 'fresh TX must work after watchdog recovery'
    finally:
        if client:
            client.close()
        if process.poll() is None:
            stop.write_text('stop')
            try:
                process.wait(5)
            except subprocess.TimeoutExpired:
                process.kill()  # This process is a hardware-free simulation.
                process.wait()
        log.close()

run_test()
run_test(graceful=True)
run_test(invalid=True)
print(('PASS HackRF protocol:' if hackrf else 'PASS Pluto protocol:')+' PTT without audio rejected without exit, late audio_start cannot rekey, single client, disconnect unkeys, watchdog abort preserves RX/CAT, late audio ignored, fresh TX succeeds, graceful shutdown; no hardware opened.')
