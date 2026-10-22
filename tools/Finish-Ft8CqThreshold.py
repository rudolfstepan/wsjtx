"""Find the individual CQ transition in the existing matched FT8 test corpus."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import zipfile

root=Path(__file__).resolve().parents[1]
out=root/'build-custom/ft8-threshold-comparison'
env=dict(os.environ,PATH=r'C:\msys64\mingw64\bin'+os.pathsep+os.environ['PATH'],OMP_NUM_THREADS='4')
rows=[]
for level in range(1,18):
    file=f'ft8_level{level}_slot1_000000.wav'
    log=out/(Path(file).stem+'.decode.txt')
    if not log.exists():
        result=subprocess.run([str(root/'build-custom/jt9.exe'),'-8','-d','3','-L','100','-H','4000',file],cwd=out,env=env,capture_output=True,text=True,check=True,timeout=60)
        log.write_text(result.stdout+result.stderr,encoding='utf-8')
    text=log.read_text(encoding='utf-8')
    exact='CQ OE3SRO JN88' in text
    rows.append(dict(level=level,snr2500=-18-.5*(level-1),file=file,exact=exact,log=log.name))
    print(f'CQ {rows[-1]["snr2500"]} dB: {exact}',flush=True)
good=[r for r in rows if r['exact']]
best=good[-1]
below=next(r for r in rows if r['level']==best['level']+1)
assert not below['exact']
added=[]
for name,row in [('ft8_CQ_threshold',best),('ft8_CQ_below_threshold',below)]:
    target=out/(name+'_000000.wav');shutil.copyfile(out/row['file'],target);added.append(target)
    log=out/(name+'.decode.txt');shutil.copyfile(out/row['log'],log);added.append(log)
verification=out/'cq-threshold-results.json'
verification.write_text(json.dumps(dict(message='CQ OE3SRO JN88',threshold=best,below=below,sweep=rows,decoder='jt9 -8 -d 3 -L 100 -H 4000',normal_ap_enabled=True,payload_hints_supplied=False),indent=2),encoding='utf-8');added.append(verification)
readme=out/'README-CQ.txt'
readme.write_text(f'''Einzelnes FT8-CQ direkt am gemessenen Uebergang

ft8_CQ_threshold_000000.wav: {best['snr2500']} dB SNR2500, CQ OE3SRO JN88 dekodiert.
ft8_CQ_below_threshold_000000.wav: {below['snr2500']} dB SNR2500, kein CQ-Decode.
Beide Dateien: 15 Sekunden, 12 kHz mono PCM16, gleicher Rauschvektor.
Pruefung: produktiver jt9 -8 -d 3 -L 100 -H 4000, normale AP-Funktion,
keine vorgegebenen Nutzdaten, kein eigener Rufzeichen-/DX-Hinweis.
Der Pegel bezieht sich auf 2500 Hz Rauschbandbreite.
Diese Grenze gilt fuer diese eine feste Nachricht/Rauschrealisierung
und die genannten Decodereinstellungen, nicht universell fuer FT8.
Die 90-s-Sequenz hat eine andere gemeinsame Grenze: -20,5 dB (6/6).
''',encoding='utf-8');added.append(readme)
hashes=out/'CQ-SHA256SUMS.txt';hashes.write_text(''.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+p.name+'\n' for p in added),encoding='utf-8');added.append(hashes)
with zipfile.ZipFile(out.with_suffix('.zip'),'a',zipfile.ZIP_DEFLATED) as z:
    for p in added:
        archive=out.name+'/'+p.name
        assert archive not in z.namelist()
        z.write(p,archive)
with zipfile.ZipFile(out.with_suffix('.zip')) as z:assert z.testzip() is None
print(json.dumps(dict(threshold=best,below=below),indent=2))
