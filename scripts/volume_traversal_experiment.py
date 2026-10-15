"""Read-only native volume experiment; build the app first.

Produces control/candidate compiler and renderer objects with a shared current
core and explicit compatible control sources. Original probes/saves stay read-only.
Images and sequential ABBA throughput are separate invocations; no timestamps.
Codex / GPT-6.1 Sol / 01a122d7 / 2026-10-15.
"""
from pathlib import Path
from datetime import datetime
from zoneinfo import ZoneInfo
import argparse
import hashlib
import json
import os
import re
import shlex
import statistics
import subprocess

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / 'build'
NV = {'spawn': (0,2,0,0,60,90), 'inside': (0,50,75,40,62,110),
      'reverse': (0,60,160,0,55,75), 'side': (90,55,90,-30,55,90)}
CLOUD = {'spawn': (0,2,0,0,-5.5,-15), 'inside': (0,-5.5,0,20,-5.5,-15),
         'reverse': (0,-3,30,0,-5.5,0), 'side': (40,-5.5,0,0,-5.5,0)}
SAVES = {'aurora': ROOT/'saves/zones/Northern Veil/zone.ecform',
         'cloud': ROOT/'saves/zones/Sky-Island Sanctuary/zone.json'}


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def write(p, data):
    with p.open('x') as f:
        json.dump(data, f, indent=2, allow_nan=False)
        f.write('\n')


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output', type=Path, required=True, help='fresh external directory')
    p.add_argument('--mode', choices=['build', 'codegen', 'images', 'cost'], default='images')
    p.add_argument('--binaries', type=Path, help='reuse a previous build directory')
    p.add_argument('--control-sources', type=Path, help='compatible frozen SdfWgsl.cpp/WebGpuRenderer.cpp directory')
    p.add_argument('--correctness', type=Path, help='passing image report directory required before cost')
    p.add_argument('--width', type=int, default=2560)
    p.add_argument('--height', type=int, default=1440)
    p.add_argument('--suite', choices=['all', 'aurora', 'cloud'], default='all')
    p.add_argument('--frames', type=int, default=24)
    p.add_argument('--stop-if-primary-misses-gate', action='store_true',
                   help='stop after two complete primary ABBA series if either misses 5 percent improvement')
    a = p.parse_args()
    out = a.output.resolve()
    if out == ROOT or ROOT in out.parents or min(a.width,a.height) < 64 or a.frames < 1:
        p.error('fresh output must be external; dimensions >=64 and frames positive')
    if a.mode == 'cost' and (not a.binaries or not a.correctness):
        p.error('cost requires --binaries and a separate --correctness report')
    if not a.binaries and not a.control_sources:
        p.error('building a candidate requires explicit compatible --control-sources')
    sources = [ROOT/'src/Singularity/Screen/WebGPU/SdfWgsl.cpp',
               ROOT/'src/Singularity/Screen/WebGPU/SdfWgsl.hpp',
               ROOT/'src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp',
               ROOT/'src/Singularity/Screen/WebGPU/WebGpuRenderer.hpp',
               ROOT/'src/Singularity/Screen/VolumeZeroProof.cpp',
               ROOT/'src/Singularity/Screen/VolumeZeroProof.hpp',
               ROOT/'scratch/probes/volume_demand_probe.cpp', *SAVES.values()]
    before = {str(x.relative_to(ROOT)): sha(x) for x in sources}
    out.mkdir(parents=True, exist_ok=False)
    (out/'experiment-used.py').write_bytes(Path(__file__).read_bytes())
    binaries = a.binaries.resolve() if a.binaries else out
    if not a.binaries:
        db = json.loads((BUILD/'compile_commands.json').read_text())
        def compile_one(suffix, source, target):
            c = next(c for c in db if c['file'].endswith(suffix) and 'earthcall_webgpu.dir' in c['command'])
            args = shlex.split(c['command'])
            args[args.index('-c')+1] = str(source)
            args[args.index('-o')+1] = str(target)
            write(out/(target.stem+'-compile.json'), args)
            with (out/(target.stem+'-compile.log')).open('x') as log:
                subprocess.run(args, cwd=BUILD, check=True, stdout=log, stderr=subprocess.STDOUT)
        compile_one('/entry.cpp', ROOT/'scratch/probes/volume_demand_probe.cpp', out/'fixture.o')
        for lane in ['control', 'candidate']:
            for name in ['SdfWgsl','WebGpuRenderer']:
                source = ROOT/f'src/Singularity/Screen/WebGPU/{name}.cpp' if lane == 'candidate' else a.control_sources/(name+'.cpp')
                copied = out/(lane+'-'+name+'.cpp')
                copied.write_bytes(source.read_bytes())
                compile_one('/'+name+'.cpp', copied, out/(lane+'-'+name+'.o'))
            args = shlex.split((BUILD/'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text())
            for suffix, obj in [('/src/entry.cpp.o','fixture.o'),('/SdfWgsl.cpp.o',lane+'-SdfWgsl.o'),('/WebGpuRenderer.cpp.o',lane+'-WebGpuRenderer.o')]:
                args[next(i for i,x in enumerate(args) if x.endswith(suffix))] = str(out/obj)
            args[args.index('-o')+1] = str(out/lane)
            write(out/(lane+'-link.json'), args)
            with (out/(lane+'-link.log')).open('x') as log:
                subprocess.run(args, cwd=BUILD, check=True, stdout=log, stderr=subprocess.STDOUT)
    provenance = {'at': datetime.now(ZoneInfo('America/Los_Angeles')).isoformat(),
                  'mode': a.mode, 'dimensions': [a.width,a.height], 'suite': a.suite,
                  'source_sha256': before, 'binaries': {x: sha(binaries/x) for x in ['control','candidate']},
                  'head': subprocess.check_output(['git','-c','core.fsmonitor=false','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),
                  'control_sources_sha256': {name:sha(out/('control-'+name+'.cpp')) for name in ['SdfWgsl','WebGpuRenderer']} if not a.binaries else json.loads((binaries/'provenance.json').read_text())['control_sources_sha256'],
                  'timestamps': False, 'cost_scope': 'completion-fenced offscreen batch throughput; no isolated volume time or live FPS'}
    provenance['primary_context'] = ['aurora' if a.suite == 'all' else a.suite, 'inside']
    if a.binaries:
        original = json.loads((binaries/'provenance.json').read_text())
        assert original['source_sha256'] == before, 'source changed after binary preparation'
        assert original['binaries'] == provenance['binaries'], 'binary changed after preparation'
    if a.mode == 'cost':
        image_provenance = json.loads((a.correctness/'provenance.json').read_text())
        images = json.loads((a.correctness/'results.json').read_text())
        assert image_provenance['mode'] == 'images'
        assert image_provenance['binaries'] == provenance['binaries']
        assert image_provenance['dimensions'] == provenance['dimensions']
        requested = {'cloud','aurora'} if a.suite == 'all' else {a.suite}
        expected = {f'{suite}-{view}-{time:g}-full{full}' for suite in requested
                    for view in NV for time in [0,7.25] for full in [0,1]}
        relevant = [x for x in images if x['label'] in expected]
        assert {x['label'] for x in relevant} == expected, 'incomplete correctness coverage'
        assert all(x['changed_bytes'] == 0 and x['lit_pixels'] > 0 for x in relevant), 'exact image gate failed'
        provenance['correctness_report_sha256'] = sha(a.correctness/'results.json')
    write(out/'provenance.json', provenance)
    if a.mode == 'build':
        return
    suites = ['aurora','cloud'] if a.suite == 'all' else [a.suite]
    if a.mode == 'codegen':
        for suite in suites:
            for lane in ['control','candidate']:
                dest=out/(suite+'-'+lane); dest.mkdir()
                env=dict(os.environ,EC_VOLUME_CODEGEN_DIR=str(dest))
                subprocess.run([str(binaries/lane),str(SAVES[suite]),'0123','64','64','0','unused','1'],
                               cwd=ROOT,env=env,check=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        assert before == {str(x.relative_to(ROOT)): sha(x) for x in sources}
        return
    results = []
    # Primary target predeclared for the coarse-index experiment: aurora/inside.
    # Other aurora views are controls. Cloud-only experiments declare their own
    # primary context in provenance before measurement; do not select it later.
    # Fixed time and no readback during cost runs; both variants warm in fixture.
    for suite in suites:
        views = CLOUD if suite == 'cloud' else NV
        for view in ['inside','spawn','reverse','side']:
            if a.mode == 'images':
                for full in [False, True]:
                    for time in [0, 7.25]:
                        label = f'{suite}-{view}-{time:g}-full{int(full)}'
                        env = dict(os.environ, EC_CAM=','.join(map(str,views[view])))
                        env.pop('EC_CENSUS',None)
                        env.pop('EC_FULL_LATTICE',None)
                        if full:
                            env['EC_FULL_LATTICE'] = '1'
                        for lane in ['control','candidate']:
                            with (out/(label+'-'+lane+'.log')).open('x') as log:
                                subprocess.run([str(binaries/lane),str(SAVES[suite]),'0123',str(a.width),str(a.height),str(time),
                                                str(out/(label+'-'+lane+'.rgba')),'1'],cwd=ROOT,env=env,check=True,stdout=log,stderr=subprocess.STDOUT)
                        left = (out/(label+'-control.rgba')).read_bytes()
                        right = (out/(label+'-candidate.rgba')).read_bytes()
                        assert len(left) == len(right) == a.width*a.height*4
                        changed = 0 if left == right else sum(x != y for x,y in zip(left,right))
                        max_error = 0 if changed == 0 else max(abs(x-y) for x,y in zip(left,right))
                        lit = sum(bool(left[i] or left[i+1] or left[i+2]) for i in range(0,len(left),4))
                        results.append({'label':label,'changed_bytes':changed,'max_error':max_error,'lit_pixels':lit,
                                        'control_sha256':hashlib.sha256(left).hexdigest(),'candidate_sha256':hashlib.sha256(right).hexdigest()})
                        write(out/(label+'-comparison.json'),results[-1])
                        print(label,'changed',changed,'max',max_error,'lit',lit,flush=True)
                        # Keep every correctness result, including failure; never time it automatically.
            else:
                rows = []
                env = dict(os.environ, EC_CAM=','.join(map(str,views[view])))
                env.pop('EC_CENSUS',None); env.pop('EC_FULL_LATTICE',None)
                for series in range(2):
                    for index,lane in enumerate(['control','candidate','candidate','control']):
                        label = f'{suite}-{view}-s{series}-{index}-{lane}'
                        run = subprocess.run([str(binaries/lane),str(SAVES[suite]),'0123',str(a.width),str(a.height),'7.25','cost',str(a.frames)],
                                             cwd=ROOT,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,check=True)
                        (out/(label+'.log')).write_text(run.stdout)
                        match = re.search(r'COST frames=(\d+) total_ms=([\d.]+) prep_ms=([\d.]+) submit_ms=([\d.]+) compiles=(\d+) lowerings=(\d+) parameters=(\d+) proofs=(\d+)',run.stdout)
                        assert match, 'missing cost record: '+label
                        record = dict(zip(['frames','total_ms','prep_ms','submit_ms','compiles','lowerings','parameters','proofs'],map(float,match.groups())))
                        assert all(record[k] == 0 for k in ['compiles','lowerings','parameters','proofs']), 'warm invariant failed'
                        record.update(lane=lane,series=series)
                        rows.append(record)
                        print(label,record['total_ms'],flush=True)
                deltas = []
                for series in range(2):
                    means = {lane: statistics.mean(x['total_ms'] for x in rows if x['lane']==lane and x['series']==series) for lane in ['control','candidate']}
                    deltas.append((means['candidate']/means['control']-1)*100)
                results.append({'suite':suite,'view':view,'series_change_percent':deltas,'runs':rows})
                write(out/(suite+'-'+view+'-cost.json'),results[-1])
                if a.stop_if_primary_misses_gate and suite == provenance['primary_context'][0] and view == 'inside' and not all(x <= -5 for x in deltas):
                    assert before == {str(x.relative_to(ROOT)): sha(x) for x in sources}
                    write(out/'results.json',results)
                    write(out/'disposition.json',{'retained':False,'reason':'primary misses repeated 5 percent gate',
                                                'remaining_views':'not run after rejection','primary':results[-1]})
                    print('Primary retention gate missed; stopping remaining views.',flush=True)
                    return
    assert before == {str(x.relative_to(ROOT)): sha(x) for x in sources}, 'source/input changed during run'
    write(out/'results.json',results)
    print('Complete; original sources/fixtures unchanged.',flush=True)


if __name__ == '__main__':
    main()
