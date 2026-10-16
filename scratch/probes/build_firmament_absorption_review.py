"""Build the isolated review witness with the configured app toolchain.

Codex / GPT-6 Astra / 01a12cb8-ac95-7b13-827d-2c7f289aa560 / 2026-10-16.
The C++ witness adapts the existing shared-subexpression native test's plumbing.
No saves, production sources or existing build objects are modified.
"""
import hashlib
import json
from pathlib import Path
import shlex
import subprocess
import sys

root = Path(__file__).resolve().parents[2]
build = root / 'build'
out = Path(sys.argv[1]).resolve()
if out == root or root in out.parents:
    raise SystemExit('Use a fresh external output directory')
out.mkdir(parents=True, exist_ok=False)
db = json.loads((build / 'compile_commands.json').read_text())
specs = [
    ('/entry.cpp', root / 'scratch/probes/firmament_absorption_review.cpp', 'probe'),
    ('/ScalarForm.cpp', root / 'src/Singularity/OntoMath/ScalarForm.cpp', 'ScalarForm'),
    ('/VolumeZeroProof.cpp', root / 'src/Singularity/Screen/VolumeZeroProof.cpp', 'VolumeZeroProof'),
    ('/SdfWgsl.cpp', root / 'src/Singularity/Screen/WebGPU/SdfWgsl.cpp', 'SdfWgsl'),
    ('/WebGpuRenderer.cpp', root / 'src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp', 'WebGpuRenderer'),
]
records = []
for suffix, source, name in specs:
    c = next(c for c in db if c['file'].endswith(suffix) and 'earthcall_webgpu.dir' in c['command'])
    args = shlex.split(c['command'])
    args[args.index('-c') + 1] = str(source)
    args[args.index('-o') + 1] = str(out / (name + '.o'))
    records.append({'source': str(source), 'sha256': hashlib.sha256(source.read_bytes()).hexdigest(), 'argv': args})
    with (out / (name + '-compile.log')).open('w') as log:
        subprocess.run(args, cwd=build, check=True, stdout=log, stderr=subprocess.STDOUT)
args = shlex.split((build / 'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text())
for suffix, _, name in specs:
    args[next(i for i, x in enumerate(args) if x.endswith(suffix + '.o'))] = str(out / (name + '.o'))
args[args.index('-o') + 1] = str(out / 'absorption-probe')
with (out / 'link.log').open('w') as log:
    subprocess.run(args, cwd=build, check=True, stdout=log, stderr=subprocess.STDOUT)
(out / 'provenance.json').write_text(json.dumps({'compile': records, 'link': args}, indent=2) + '\n')
print(out / 'absorption-probe')
