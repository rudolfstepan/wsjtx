"""Build and run the QSO/2 threshold WAV generator against local WSJT-X libraries."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess

root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser()
parser.add_argument('--output',default='build-custom-make/reist16-threshold-minus89db')
args=parser.parse_args()
out=(root/args.output).resolve()
if not out.is_relative_to((root/'build-custom-make').resolve()):
    raise SystemExit('Output must be under build-custom-make')
out.mkdir(parents=True,exist_ok=False)
build=root/'build-custom-make'
prefix=Path(os.environ.get('WSJTX_MINGW_PREFIX',r'C:\msys64\mingw64'))
env=dict(os.environ,PATH=str(prefix/'bin')+os.pathsep+os.environ['PATH'],OMP_NUM_THREADS='4')
libraries=(build/'CMakeFiles/reist16_qso2_checks.dir/linkLibs.rsp').read_text().strip()
if not (build/'libwsjt_fort.a').exists():
    libraries=libraries.replace('libwsjt_fort.a','libwsjt_fort_omp.a')
command=[str(prefix/'bin/g++.exe'),'-std=c++11','-O3','-fopenmp','-I'+str(root),str(root/'tools/Generate-Reist16Threshold.cpp'),'-o',str(out/'generate.exe'),*shlex.split(libraries)]
subprocess.run(command,cwd=build,env=env,check=True)
with (out/'generator.log').open('w') as log:
    subprocess.run([str(out/'generate.exe')],cwd=out,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
# Independently exercise the production CLI on every individual PCM16 slot.
rows=[json.loads(line) for line in (out/'slots.jsonl').read_text().splitlines()]
for row in rows:
    result=subprocess.run([str(build/'jt9.exe'),'--reist16','-d','3',row['pcm16']],cwd=out,env=env,capture_output=True,text=True,timeout=60,check=True)
    stem=Path(row['pcm16']).stem
    (out/(stem+'.decode.txt')).write_text(result.stdout+result.stderr,encoding='utf-8')
    row['jt9_exact']=row['message'] in result.stdout
    if row['variant']!='below' and not row['jt9_exact']:
        raise RuntimeError('Production decoder mismatch: '+row['pcm16'])
(out/'production-results.json').write_text(json.dumps(rows,indent=2),encoding='utf-8')
hashes={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in out.iterdir() if p.is_file()}
hashes['generator_source.cpp']=hashlib.sha256((root/'tools/Generate-Reist16Threshold.cpp').read_bytes()).hexdigest()
(out/'sha256.json').write_text(json.dumps(hashes,indent=2),encoding='utf-8')
print((out/'generator.log').read_text())
print(out)
