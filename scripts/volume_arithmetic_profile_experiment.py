"""Scratch-only Metal arithmetic-profile fidelity experiment; never admission.
Codex / GPT-6.1 Sol / 01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca / 2026-10-12 PDT.
Zach's minimum-work/exact-image direction, Astra's backend-contract boundary.
Build earthcall_webgpu first. Read-only frozen save, original probes and renderer.
Copies the observer into a NEW outside-repo directory, overrides copied options
only for modules containing volume evaluators in that scratch process. Does not
change the installed app, original observer, WGSL/MSL, saves or timestamp policy.
Pixels gate cost testing: this driver deliberately records no performance claim.
"""
from pathlib import Path
from datetime import datetime
from zoneinfo import ZoneInfo
import argparse, hashlib, json, os, shlex, subprocess, sys
from volume_backend_contract_probe import ROOT, BUILD, SAVE, CAMERAS, sha

PROFILES = {'safe-fast': (0, 0), 'safe-precise': (0, 1)}


def experiment_observer(text):
    """Generate an explicitly separate option-changing observer, not a new default."""
    marker = '        NSMutableDictionary* record = [NSMutableDictionary dictionary];'
    assert text.count(marker) == 1
    replacement = '''        const char* rawProfile = std::getenv("EC_METAL_PROFILE_EXPERIMENT");
        NSString* profile = rawProfile ? [NSString stringWithUTF8String:rawProfile] : nil;
        const bool volume = [source containsString:@"volumeDensityEval"] || [source containsString:@"volumeShared"];
        MTLCompileOptions* owned = nil;
        NSInteger originalMode = -1, originalFunctions = -1;
        if (@available(macOS 15.0, *)) {
            if (!options || (! [profile isEqualToString:@"safe-fast"] && ! [profile isEqualToString:@"safe-precise"])) std::abort();
            originalMode = options.mathMode;
            originalFunctions = options.mathFloatingPointFunctions;
            if (volume) {
                owned = [options copy];
                owned.mathMode = MTLMathModeSafe;
                owned.mathFloatingPointFunctions = [profile isEqualToString:@"safe-precise"]
                    ? MTLMathFloatingPointFunctionsPrecise : MTLMathFloatingPointFunctionsFast;
                options = owned;
            }
        } else { std::abort(); }
        NSMutableDictionary* record = [NSMutableDictionary dictionary];
        record[@"requestedProfile"] = profile;
        record[@"profileOverride"] = @(volume);
        record[@"originalMathMode"] = @(originalMode);
        record[@"originalMathFloatingPointFunctions"] = @(originalFunctions);'''
    text = text.replace(marker, replacement)
    text = text.replace('// Observation cannot silently choose a different numerical profile.',
                        '// EXPLICIT SCRATCH EXPERIMENT: forward copied volume options once.')
    text = text.replace('        record[@"libraryCreated"] = @(library != nil);',
                        '        [owned release];\n        record[@"libraryCreated"] = @(library != nil);')
    text = text.replace('options unchanged\\n', 'SCRATCH option experiment\\n')
    text = text.replace('// Diagnostic-only observation of the native shader compilation boundary.',
                        '// Scratch-only experiment at the native shader compilation boundary.')
    text = text.replace('// No options/source/pipeline changes. The concrete device method forwards the\n// original arguments once, recording MSL and option values outside the repo.',
                        '// Copies volume-module options; forwards source unchanged once. Records\n// effective settings outside the repo. Never linked into the app.')
    return '// Generated SCRATCH CANDIDATE: volume-only copied option overrides.\n' + text


def image_difference(before, after):
    if len(before) != len(after) or len(before) % 4:
        raise ValueError('invalid capture lengths')
    delta = [abs(a-b) for a,b in zip(before, after)]
    return {'changed_bytes': sum(d != 0 for d in delta),
            'changed_pixels': sum(any(delta[i:i+4]) for i in range(0,len(delta),4)),
            'maximum_channel_difference': max(delta, default=0),
            'bytes_over_one_level': sum(d > 1 for d in delta),
            'baseline_sha256': hashlib.sha256(before).hexdigest(),
            'candidate_sha256': hashlib.sha256(after).hexdigest()}


def modules(directory):
    records = []
    for meta in sorted(directory.glob('*.json')):
        record = json.loads(meta.read_text()); msl = meta.with_suffix('.metal')
        source = msl.read_text()
        record.update(path=str(msl), sha256=sha(msl),
                      volume_module=('volumeDensityEval' in source or 'volumeShared' in source))
        records.append(record)
    assert records and all(x.get('libraryCreated') for x in records)
    assert any(x['volume_module'] for x in records)
    return records


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--width', type=int, default=320)
    p.add_argument('--height', type=int, default=180)
    p.add_argument('--camera', choices=CAMERAS, help='one named view; default: all four')
    p.add_argument('--time', type=float, help='one fixed clock; default: 0 and 7.25')
    p.add_argument('--full-lattice', action='store_true',
                   help='set the scratch renderer budget to 1/255, bypassing the legacy opacity stop')
    a = p.parse_args(); out = a.output.resolve()
    if out == ROOT or ROOT in out.parents: p.error('output must be outside the repository')
    if a.width < 64 or a.height < 36: p.error('minimum dimensions: 64 by 36')
    out.mkdir(parents=True, exist_ok=False)
    observer = ROOT/'scratch/probes/volume_metal_compile_observer.mm'
    inputs = [Path(__file__).resolve(), ROOT/'scripts/volume_backend_contract_probe.py', observer,
              ROOT/'scratch/northern_veil_fragment_demand_probe_2026_10_11.cpp',
              ROOT/'src/Singularity/Screen/WebGPU/SdfWgsl.cpp',
              ROOT/'src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp', SAVE]
    prior = {str(x.relative_to(ROOT)): sha(x) for x in inputs}
    # Reuse the existing unchanged build recipe and read-only observer as control.
    subprocess.run([sys.executable, str(ROOT/'scripts/volume_backend_contract_probe.py'),
                    '--output', str(out/'build'), '--width', str(a.width),
                    '--height', str(a.height), '--build-only'], cwd=ROOT, check=True)
    if a.full_lattice:
        # Both control and candidate consume the same modified COPY of the fixture.
        fixture = out/'build/fixture.cpp'; text = fixture.read_text()
        marker = '    WebGpuRenderer renderer;'
        assert text.count(marker) == 1
        fixture.write_text(text.replace(marker,marker+'\n    renderer.setVolumeOutputErrorBudget(1.0 / 255.0);'))
        db = json.loads((BUILD/'compile_commands.json').read_text())
        c = next(c for c in db if c['file'].endswith('entry.cpp') and 'earthcall_webgpu.dir' in c['command'])
        compile_args = shlex.split(c['command'])
        compile_args[compile_args.index('-c')+1] = str(fixture)
        compile_args[compile_args.index('-o')+1] = str(out/'build/fixture.o')
        subprocess.run(compile_args,cwd=BUILD,check=True)
        for lane, obj in (('plain','stub.o'),('observed','observer.o')):
            link_args = shlex.split((BUILD/'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text())
            i = next(i for i,x in enumerate(link_args) if x.endswith('/src/entry.cpp.o'))
            link_args[i] = str(out/'build/fixture.o')
            link_args[link_args.index('-o')+1] = str(out/'build'/lane)
            link_args.append(str(out/'build'/obj))
            subprocess.run(link_args,cwd=BUILD,check=True)
    candidate_source = out/'candidate-observer.mm'
    candidate_source.write_text(experiment_observer(observer.read_text()))
    subprocess.run(['/usr/bin/clang++', '-std=c++17', '-arch', 'arm64', '-c',
                    str(candidate_source), '-o', str(out/'candidate-observer.o')], check=True)
    args = shlex.split((BUILD/'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text())
    idx = next(i for i,x in enumerate(args) if x.endswith('/src/entry.cpp.o'))
    args[idx] = str(out/'build/fixture.o'); args[args.index('-o')+1] = str(out/'candidate')
    args.append(str(out/'candidate-observer.o'))
    subprocess.run(args, cwd=BUILD, check=True)
    provenance = json.loads((out/'build/provenance.json').read_text())
    provenance.update(at=datetime.now(ZoneInfo('America/Los_Angeles')).isoformat(),
                      experiment='volume-only arithmetic profile overrides in scratch process',
                      inputs_sha256=prior, candidate_binary_sha256=sha(out/'candidate'),
                      candidate_observer_sha256=sha(candidate_source), profiles=PROFILES,
                      fixture_copy_sha256=sha(out/'build/fixture.cpp'),
                      binary_sha256={lane:sha(out/'build'/lane) for lane in ('plain','observed')},
                      reference_budget=(1.0/255.0 if a.full_lattice else 0.0),
                      cameras=([a.camera] if a.camera else list(CAMERAS)),
                      clocks=([a.time] if a.time is not None else [0,7.25]))
    (out/'provenance.json').write_text(json.dumps(provenance,indent=2)+'\n')
    images = []; all_modules = []
    for camera, coords in CAMERAS.items():
        if a.camera and camera != a.camera: continue
        for time in ([a.time] if a.time is not None else [0,7.25]):
            label = f'{camera}-{time:g}'; by_lane = {}
            for lane in ('plain', 'observed', *PROFILES):
                env = dict(os.environ)
                for key in ('EC_METAL_CONTRACT', 'EC_CENSUS', 'EC_METAL_PROFILE_EXPERIMENT'):
                    env.pop(key, None)
                env['EC_CAM'] = ','.join(map(str, coords))
                binary = out/'build'/lane if lane in ('plain','observed') else out/'candidate'
                records = out/(label+'-'+lane+'-modules')
                if lane != 'plain':
                    records.mkdir(); env['EC_METAL_CONTRACT'] = str(records)
                if lane in PROFILES: env['EC_METAL_PROFILE_EXPERIMENT'] = lane
                capture = out/(label+'-'+lane+'.rgba')
                with (out/(label+'-'+lane+'.log')).open('x') as log:
                    subprocess.run([str(binary),str(SAVE),'0123',str(a.width),str(a.height),
                                    str(time),str(capture),'1'],cwd=ROOT,env=env,
                                   stdout=log,stderr=subprocess.STDOUT,check=True)
                if lane != 'plain':
                    by_lane[lane] = modules(records)
                    for record in by_lane[lane]:
                        record.update(camera=camera,time=time,lane=lane)
                    all_modules.extend(by_lane[lane])
                pixels = capture.read_bytes(); assert len(pixels) == a.width*a.height*4
                if lane == 'plain': baseline = pixels
                else:
                    result = image_difference(baseline, pixels)
                    result.update(camera=camera,time=time,lane=lane)
                    images.append(result)
                    if lane == 'observed': assert result['changed_bytes'] == 0
                    print(json.dumps(result),flush=True)
            # Identical MSL across profiles; non-volume compilation settings unchanged.
            reference = by_lane['observed']
            for lane, expected in PROFILES.items():
                actual = by_lane[lane]; assert len(reference) == len(actual)
                for before, after in zip(reference, actual):
                    assert before['sha256'] == after['sha256']
                    assert after['profileOverride'] == after['volume_module']
                    target = expected if after['volume_module'] else (before['mathMode'],before['mathFloatingPointFunctions'])
                    assert (after['mathMode'],after['mathFloatingPointFunctions']) == tuple(target)
                    assert after['languageVersion'] == before['languageVersion']
                    assert after['preserveInvariance'] == before['preserveInvariance']
    assert prior == {str(x.relative_to(ROOT)):sha(x) for x in inputs}, 'inputs changed during experiment'
    summary = {lane: {'exact_capture_gate':all(x['changed_bytes']==0 for x in images if x['lane']==lane),
                      'maximum_channel_difference':max(x['maximum_channel_difference'] for x in images if x['lane']==lane),
                      'finite_one_level_gate':all(x['bytes_over_one_level']==0 for x in images if x['lane']==lane)} for lane in PROFILES}
    report = {'scope':'finite fidelity screen only; not error proof, cost or live FPS',
              'summary':summary,'images':images,'modules':all_modules,
              'production_admitted_skips':0,'performance_run':False}
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(summary,indent=2),flush=True)


if __name__ == '__main__': main()
