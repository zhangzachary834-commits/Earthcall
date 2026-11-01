#!/usr/bin/env python3
"""Reproduce three source/CPU findings. Never launches Earthcall or a GPU.

Usage: python3 scratch/probes/ray_transport_p1_p11_structural_review.py \
  --out /private/tmp/ec-p1-p11-review
Requires the existing build/compile_commands.json for local dependency headers.
This is a regression-preservation witness: exit 0 means all three defects were
reproduced in the reviewed source, NOT that the renderer passed correctness.
Codex / GPT-6 / session 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-11-01.
"""
import argparse
import hashlib
import json
from pathlib import Path
import platform
import shlex
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[2])
parser.add_argument('--out', type=Path, required=True)
a = parser.parse_args()
root = a.root.resolve()
out = a.out.resolve()
out.mkdir(parents=True, exist_ok=True)
paths = {
    'cpu_mirror': root/'scratch/probes/positive_e2e_cpu_verify.cpp',
    'kernel': root/'src/Singularity/Screen/WebGPU/VolumeRoutedKernels.hpp',
    'transport': root/'src/Singularity/Screen/WebGPU/VolumeRoutedTransport.inc',
    'header': root/'src/Singularity/Screen/WebGPU/VolumeRoutedTransport.hpp',
    'adversary': Path(__file__).with_name('ray_transport_p1_p11_near_parallel_case.cpp.inc'),
}
src = {name: p.read_text() for name, p in paths.items()}
# Fail closed when the relevant source changes; do not present this model as
# continued evidence after repair without updating the named obligations.
assert 'args[9u] = select(0u, CAPACITY_WORKGROUPS, totalActiveTiles > 0u);' in src['kernel']
assert 'if (activeCount == 0u) {\n        // wave-proved empty: no producer work needed\n    }' in src['kernel']
ordinary_setup = src['transport'].split('if(t1>2048.0)', 1)[1].split(')WGSL";', 1)[0]
assert 'ecRays[id] = ECRay(' in ordinary_setup
assert 'ecPublication[ecPixel(id)] = vec4<f32>(0.0);' not in ordinary_setup
assert 'wgpuCommandEncoderClearBuffer' not in src['transport']
assert 'maxStrips=(h+rows-1)/rows;if(maxStrips>64)maxStrips=64;' in src['transport']
assert 'start<height;start+=stripRows,++stripIdx)' in src['transport']
assert 'std::array<WGPUBindGroup,64> localGroups' in src['header']

# Integer admission/indexing calculation taken from ensure()/encode().
w, h, binding_limit = 1024, 4096, 128*1024*1024
rows = min(48, binding_limit//(w*64*16))//4*4
needed = (h+rows-1)//rows
allocated = min(needed, 64)
assert w*h*16 <= min(64*1024*1024, binding_limit) and needed == 86 and allocated == 64

# Compile only the repository CPU helper prefix plus an independent camera and
# true-density oracle. No production objects, native device, or saves are used.
prefix = src['cpu_mirror'].split('\nint main(', 1)[0]
start = prefix.index('static TileRouteResult classifyTileSerial(')
end = prefix.index('static void classifyWorkgroupCooperative(', start)
serial = prefix[start:end]
branch = '''                if (std::abs(dx) < 1e-6f) {
                    curActive = true;
                    enter = std::min(enter, rA[i]);
                    exit = std::max(exit, rB[i]);
                    break;
                }'''
assert serial.count(branch) == 1
control = serial.replace('classifyTileSerial(', 'classifyTileSerialKeepAllRays(', 1)
control = control.replace(branch, branch.replace('break;', 'continue;'), 1)
cpp = out/'near_parallel_witness.cpp'
cpp.write_text('#include <iostream>\n'+prefix+'\n'+control+'\n'+src['adversary'])
entries = json.loads((root/'build/compile_commands.json').read_text())
e = next(e for e in entries if e['file'].endswith('/entry.cpp'))
raw = e.get('arguments') or shlex.split(e['command'])
compile_cmd = []
i = 0
while i < len(raw):
    arg = raw[i]
    if arg == '-o':
        i += 2
        continue
    if arg == '-c' or arg == e['file'] or arg.startswith('-O') or arg.startswith('-g'):
        i += 1
        continue
    compile_cmd.append(arg)
    i += 1
obj = out/'near_parallel_witness.o'
binary = out/'near_parallel_witness'
compile_cmd += ['-O3', '-DNDEBUG', '-Wno-unused-function', '-c', str(cpp), '-o', str(obj)]
with (out/'cpu-build.log').open('w') as log:
    subprocess.run(compile_cmd, cwd=e['directory'], stdout=log, stderr=log, check=True)
    link = ['/usr/bin/c++', '-arch', platform.machine(), '-Wl,-dead_strip', str(obj), '-o', str(binary)]
    subprocess.run(link, stdout=log, stderr=log, check=True)
r = subprocess.run([str(binary)], capture_output=True, text=True, check=True)
near = json.loads(r.stdout)
assert near['accepted_overlap_rays'] == 12 and near['fixed_serial_cohort'] == 3
# The old mirror lacks P7 zEnv AABB clipping. Record that drift separately.
assert near['legacy_cpu_mirror_empty_tile_cohort'] == 3
assert 'if (axis == 2) { half = min(half, zEnv); }' in src['kernel']
assert 10 > min(20, .1+.1+.1+1)+.001
(out/'near_parallel_result.json').write_text(json.dumps(near, indent=2)+'\n')
report = {
    'scope': 'source inspection plus synthetic CPU mirror execution; no GPU, app, frame capture or timing',
    'source_head': subprocess.check_output(['git','rev-parse','HEAD'], cwd=root, text=True).strip(),
    'source_sha256': {name: hashlib.sha256(p.read_bytes()).hexdigest() for name,p in paths.items()},
    'near_parallel': near,
    'tall_viewport': {'width':w,'height':h,'binding_limit':binding_limit,'publication_bytes':w*h*16,
                      'strip_rows':rows,'needed_strips':needed,'allocated_cached_strips':allocated,
                      'first_out_of_bounds_strip':allocated,'scope':'integer/source model, not native execution'},
    'empty_strip': {'source_proven_tile_cohort':0, 'legacy_cpu_mirror_tile_cohort':near['legacy_cpu_mirror_empty_tile_cohort'],
                    'camera':[0,0,10], 'direction':[0,1,0], 'box_half':[20,20,20],
                    'z_support_envelope':1.3, 'margin':.001,
                    'producer_workgroups':[0,0,0], 'consumer_workgroups':0,
                    'normal_setup_publication_writes':0,
                    'scope':'independent support geometry plus inspection of actual WGSL dispatch/write ownership; not GPU readback'},
    'meaning_of_exit_0':'all three reviewed defects reproduced, not renderer approval',
}
(out/'structural_review_result.json').write_text(json.dumps(report, indent=2)+'\n')
print(json.dumps({'reproduced_findings':3,'cpu_overlap_rays':12,'needed_strips':needed,
                  'cached_strips':allocated,'report':str(out/'structural_review_result.json')}))
