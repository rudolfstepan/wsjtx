"""Paired CQ/QSO, random-phase and mild-drift comparison through production CLIs."""
import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', required=True, help='New directory under build-custom-make or tmp')
parser.add_argument('--seed', type=int, default=2026102629)
parser.add_argument('--trials', type=int, default=16)
parser.add_argument('--build-directory', default='build-custom-make')
parser.add_argument('--reference-decoder', type=Path, help='Optional older jt9.exe for before/after comparison')
parser.add_argument('--skip-build', action='store_true')
args = parser.parse_args()
build = (ROOT / args.build_directory).resolve()
output = (ROOT / args.output).resolve()
if not any(output.is_relative_to(ROOT / name) for name in ['build-custom-make', 'tmp']):
    raise SystemExit('Output must stay under a build or tmp directory')
if args.trials < 2 or args.trials > 100:
    raise SystemExit('Trials must be 2..100')
output.mkdir(parents=True, exist_ok=False)
prefix = Path(os.environ.get('WSJTX_MINGW_PREFIX', r'C:\msys64\mingw64'))
env = dict(os.environ, PATH=str(prefix / 'bin') + os.pathsep + os.environ['PATH'])
if not args.skip_build:
    subprocess.run([str(prefix / 'bin/cmake.exe'), '--build', str(build), '--target',
                    'jt9', 'reist16_weak_checks', '--parallel', '6'], env=env, check=True)
subprocess.run([str(build / 'reist16_weak_checks.exe'), str(args.seed), str(args.trials), '--generate-only'],
               cwd=output, env=env, check=True)
inputs = [json.loads(line) for line in (output / 'paired.jsonl').read_text().splitlines()]
methods = [('current', build / 'jt9.exe', '--reist16', 'reist_wave', []),
           ('current_no_cq_ap', build / 'jt9.exe', '--reist16', 'reist_wave', ['--no-reist16-ap']),
           ('ft8', build / 'jt9.exe', '-8', 'ft8_wave', [])]
if args.reference_decoder:
    methods.insert(0, ('before', args.reference_decoder.resolve(), '--reist16', 'reist_wave', []))


def evaluate(spec):
    row, method, executable, mode, key, extra = spec
    work = output / method
    work.mkdir(exist_ok=True)
    audio = output / row[key]
    run = subprocess.run([str(executable), mode, *extra, '-d', '3', '-L', '100', '-H', '4000', str(audio)],
                         cwd=work, env=env, capture_output=True, text=True, check=True, timeout=45)
    (work / (audio.stem + '.txt')).write_text(run.stdout + run.stderr, encoding='utf-8')
    marker = '~' if mode == '-8' else '%'
    decoded = [line for line in run.stdout.splitlines() if re.match(r'^\d{6}\s', line) and marker in line]
    return dict(seed=row['seed'], trial=row['trial'], snr2500=row['snr2500'], kind=row['kind'],
                message=row['message'], drift_hz_s=row['drift_hz_s'], method=method,
                exact=any(row['message'] in line for line in decoded),
                foreign=[line for line in decoded if row['message'] not in line], output=decoded)


tasks = [(row, *method) for row in inputs for method in methods]
results = []
with ThreadPoolExecutor(max_workers=2) as pool:
    for future in as_completed([pool.submit(evaluate, task) for task in tasks]):
        results.append(future.result())
        (output / 'results.json').write_text(json.dumps(results, indent=2), encoding='utf-8')
        if len(results) % 32 == 0:
            print(f'Compared {len(results)}/{len(tasks)}', flush=True)
summary = []
for snr in [-20, -22, -24, -26]:
    for kind in ['CQ', 'QSO']:
        entry = dict(snr2500=snr, kind=kind, trials=sum(r['snr2500'] == snr and r['kind'] == kind for r in inputs))
        for method, *_ in methods:
            entry[method] = sum(r['exact'] for r in results if r['snr2500'] == snr and r['kind'] == kind and r['method'] == method)
        summary.append(entry)
        print(entry, flush=True)
(output / 'summary.json').write_text(json.dumps(summary, indent=2), encoding='utf-8')
sources = ['lib/reist16/qso2.cpp', 'lib/reist16/qso2.hpp', 'lib/reist16/reist16.cpp',
           'lib/reist16/reist16.f90', 'lib/decoder.f90', 'lib/jt9.f90', 'tests/reist16/weak_checks.cpp',
           'tools/Test-Reist16WeakUpgrade.py', 'CMakeLists.txt']
hashes = {name: hashlib.sha256((ROOT / name).read_bytes()).hexdigest() for name in sources}
for method, executable, *_ in methods:
    hashes['decoder_' + method] = hashlib.sha256(executable.read_bytes()).hexdigest()
(output / 'sources.json').write_text(json.dumps(hashes, indent=2), encoding='utf-8')
if any(row['foreign'] for row in results):
    raise SystemExit('FAIL: unexpected payload; see results.json')
print('PASS: no foreign payloads in this finite comparison', flush=True)
