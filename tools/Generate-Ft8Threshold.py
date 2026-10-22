"""Generate a matched FT8 comparison and measure its threshold with production jt9."""
import array
import hashlib
import json
import os
from pathlib import Path
import shlex
import struct
import subprocess
import zipfile

root=Path(__file__).resolve().parents[1]
out=root/'build-custom/ft8-threshold-comparison'
out.mkdir(exist_ok=False)
build=root/'build-custom'
prefix=Path(os.environ.get('WSJTX_MINGW_PREFIX',r'C:\msys64\mingw64'))
env=dict(os.environ,PATH=str(prefix/'bin')+os.pathsep+os.environ['PATH'],OMP_NUM_THREADS='4')
# Share the exact WAV/quantization/noise convention of the preceding QSO/2 set.
original=(root/'tools/Generate-Reist16Threshold.cpp').read_text()
helpers=original.split('int main(){try{',1)[0]
source=helpers+'\n#include "lib/ft8/native/ft8_wave.hpp"\n'+r'''
int main(){try{
 std::mt19937 rng(2026102301u);std::normal_distribution<float> gaussian;
 std::ofstream manifest("generated.jsonl");int slot=0;
 for(std::string message:{"CQ OE3SRO JN88","OE3SRO DL1ABC JO31","DL1ABC OE3SRO -12","OE3SRO DL1ABC R-10","DL1ABC OE3SRO RR73","OE3SRO DL1ABC 73"}){
  ++slot;Frame f;f.message=message;message.resize(37,' ');char sent[37],packed[77];int tones[79],i3=0,n3=0;
  genft8_(&message[0],&i3,&n3,sent,packed,tones,37,37);require(i3==1,"FT8 message packing");
  f.wave.resize(79*1920);ft8_native::waveform(tones,79,1920,2,12000,1500.375,nullptr,f.wave.data(),false,int(f.wave.size()));
  for(auto& x:f.wave)x*=.8f;f.power=0;for(float x:f.wave)f.power+=double(x)*x;f.power/=f.wave.size();
  f.noise.resize(count);double sum=0;for(auto& x:f.noise){x=gaussian(rng);sum+=double(x)*x;}double rms=std::sqrt(sum/count);for(auto& x:f.noise)x=float(x/rms);
  for(int level=0;level<=17;level++){
   double snr=level==0?-12:-18.-.5*(level-1);
   std::string file="ft8_level"+std::to_string(level)+"_slot"+std::to_string(slot)+"_000000.wav";
   wav(file,mix(f,snr),false);
   manifest<<"{\"file\":\""<<file<<"\",\"level\":"<<level<<",\"slot\":"<<slot<<",\"message\":\""<<f.message<<"\",\"snr2500\":"<<snr<<"}\n";
  }
 }
 }catch(std::exception const& e){std::cerr<<e.what()<<std::endl;return 1;}return 0;}
'''
(out/'generate.cpp').write_text(source,encoding='utf-8')
text=(build/'build.ninja').read_text()
section=text.split('build reist16_qso2_checks.exe:',1)[1].split('\n\n',1)[0]
libraries=next(line.split('=',1)[1].strip() for line in section.splitlines() if line.strip().startswith('LINK_LIBRARIES ='))
subprocess.run([str(prefix/'bin/g++.exe'),'-std=c++11','-O3','-fopenmp','-I'+str(root),str(out/'generate.cpp'),'-o',str(out/'generate.exe'),*shlex.split(libraries)],cwd=build,env=env,check=True)
subprocess.run([str(out/'generate.exe')],cwd=out,env=env,check=True)
rows=[json.loads(line) for line in (out/'generated.jsonl').read_text().splitlines()]
last_good=None
scores=[]
for level in range(18):
    current=[r for r in rows if r['level']==level]
    for row in current:
        result=subprocess.run([str(build/'jt9.exe'),'-8','-d','3','-L','100','-H','4000',row['file']],cwd=out,env=env,capture_output=True,text=True,timeout=60,check=True)
        (out/(Path(row['file']).stem+'.decode.txt')).write_text(result.stdout+result.stderr,encoding='utf-8')
        decoded=[line for line in result.stdout.splitlines() if '~' in line and line[:6].isdigit()]
        row['exact']=any(row['message'] in line for line in decoded)
        row['other_decodes']=[line for line in decoded if row['message'] not in line]
    count=sum(r['exact'] for r in current)
    snr=current[0]['snr2500']
    scores.append(dict(snr2500=snr,exact=count,slots=6,level=level))
    print(f'FT8 SNR2500 {snr}: {count}/6',flush=True)
    if count==6 and level>0:last_good=level
    if last_good is not None and level>=last_good+4 and count<6:break
assert last_good is not None
threshold=next(r['snr2500'] for r in rows if r['level']==last_good)
assert any(s['level']==last_good+1 and s['exact']<6 for s in scores)
selected=[]
def save_wav(file,payload,floating=False):
    size=4 if floating else 2
    header=b'RIFF'+struct.pack('<I',36+len(payload))+b'WAVEfmt '+struct.pack('<IHHIIHH',16,3 if floating else 1,1,12000,12000*size,size,size*8)+b'data'+struct.pack('<I',len(payload))
    (out/file).write_bytes(header+payload)
for name,level in [('reference',0),('threshold',last_good),('below',last_good+4)]:
    current=[r for r in rows if r['level']==level]
    assert all('exact' in r for r in current)
    parts=[];quiet_parts=[]
    for row in current:
        payload=(out/row['file']).read_bytes()[44:]
        assert len(payload)==180000*2
        values=array.array('h',payload)
        quiet=array.array('f',(v/32768*(10**(-89/20)/.01) for v in values)).tobytes()
        target=f'{name}_slot{row["slot"]}_000000.wav'
        save_wav(target,payload);save_wav(target.replace('_000000.wav','_minus89dB_Float32.wav'),quiet,True)
        parts.append(payload);quiet_parts.append(quiet)
        selected.append(dict(row,variant=name,export=target))
    save_wav(f'{name}_sequence_90s_PCM16.wav',b''.join(parts))
    save_wav(f'{name}_sequence_90s_minus89dB_Float32.wav',b''.join(quiet_parts),True)
(out/'results.json').write_text(json.dumps(dict(threshold_snr2500=threshold,sweep=scores,slots=selected,seed=2026102301,decoder='jt9 -8 -d 3 -L 100 -H 4000',frequency=1500.375),indent=2),encoding='utf-8')
counts={name:sum(r['exact'] for r in selected if r['variant']==name) for name in ['reference','threshold','below']}
assert counts['reference']==counts['threshold']==6 and counts['below']<6
(out/'README.txt').write_text(f'''FT8-Schwellenvergleich zur REIST16-QSO/2-Testfolge

Empfohlene Einzeldatei: threshold_slot1_000000.wav, CQ OE3SRO JN88.
Vollstaendige Folge: threshold_sequence_90s_PCM16.wav, sechs 15-s-Slots.
Gemessener SNR2500: {threshold} dB; alle sechs Nachrichten dekodiert.
Referenz: -12 dB. Gegenprobe: {threshold-2} dB.
Produktiver jt9-Dateidecoder, Tiefe 3, Suche 100-4000 Hz, ohne mitgegebene
Nachrichten-/Rufzeichenhinweise. Ergebnisse: {json.dumps(counts)}.

Gleiche sechs Nachrichten, Frequenz 1500,375 Hz, Start 0,5 s,
Abtastrate 12 kHz/mono, AWGN-Seed 2026102301 und dieselben normierten
Rauschvektoren wie im zuvor erzeugten REIST16-QSO/2-Satz.
Der Rauschpegel folgt der tatsaechlichen mittleren aktiven FT8-Leistung.
FT8 hat eine andere aktive Signaldauer als REIST16-QSO/2; die Sequenzen
sind unmittelbar zusammengefuegte Nachrichten, kein reales Wechsel-QSO.

PCM16 zum Anhoeren und fuer WSJT-X benutzen. Die zusaetzlichen Float32-
Dateien skalieren Signal UND Rauschen auf nominal 89 dB Signalabsenkung
gegenueber der unskalierten 0,8-Peak-Generatorwelle. Das entspricht 49 dB
zusaetzlicher Absenkung gegenueber diesen PCM16-Dateien (Gain 0,01).
Float32 dient der digitalen Analyse; dessen Dekodierbarkeit wurde hier
nicht gesondert mit dem FT8-Decoder geprueft. Bei 16-Bit-Audioausgabe
koennen die sehr kleinen Signalanteile verloren gehen.

Dies ist der Uebergang fuer sechs feste Rauschrealisierungen in einem
0,5-dB-Raster, keine universelle FT8-Empfindlichkeitsgrenze oder statistische
Gleichwertigkeitspruefung zu REIST16. Ergebnisse und Logs liegen bei.
Keine Hardware geoeffnet, keine HF-Aussendung.
''',encoding='utf-8')
package=[p for p in out.iterdir() if p.name.startswith(('reference_','threshold_','below_')) and p.suffix=='.wav']
package += [out/'README.txt',out/'results.json']
package += [out/(Path(r['file']).stem+'.decode.txt') for r in selected]
(out/'SHA256SUMS.txt').write_text(''.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+p.name+'\n' for p in sorted(package)),encoding='utf-8')
with zipfile.ZipFile(out.with_suffix('.zip'),'x',zipfile.ZIP_DEFLATED) as z:
    for p in package+[out/'SHA256SUMS.txt']:z.write(p,arcname=out.name+'/'+p.name)
with zipfile.ZipFile(out.with_suffix('.zip')) as z:assert z.testzip() is None
print(json.dumps(dict(threshold_snr2500=threshold,counts=counts,output=str(out)),indent=2))
