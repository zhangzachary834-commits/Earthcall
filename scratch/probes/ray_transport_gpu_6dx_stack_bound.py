"""§6dx: prove pending-stack bound, then specialize allocation without switch expansion.

Antigravity / Gemini 2.5 Pro (Sparkly Guy) / 50cf28df-09e9-4c2b-a4d5-1f796abbc73a / 2026-10-26.
Commissioned by Sixth-One Sun (Codex / GPT-6.1 Sol / session 01a148a7-bd85-7550-a416-0e859a729c1a).
Scratch-only diagnostic research on Apple M5.
"""
import argparse, ast, hashlib, importlib.util, json, os, re, shlex, subprocess, sys
from pathlib import Path

def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def load(root, name):
    spec = importlib.util.spec_from_file_location(name, root / 'scratch/probes' / f'{name}.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module

def prove_stack_bound(max_depth=14, seed_count=2):
    """Exhaustive abstract state-space exploration over all reachable DFS stack configurations.
    Proves that for any possible execution trace:
    initial stack has `seed_count` elements at depth 1.
    popped elements subdivide into 2 children at depth `d + 1` if and only if `d < max_depth`.
    Returns (max_top_seen, total_unique_states, total_transitions).
    """
    initial_state = tuple([1] * seed_count)
    visited = {initial_state}
    queue = [initial_state]
    max_top_seen = len(initial_state)

    transitions = 0
    while queue:
        state = queue.pop(0)
        k = len(state)
        if k == 0:
            continue

        popped_depth = state[-1]
        base_state = state[:-1]

        # Transition 1: Terminal / Prune / Monotone / Band / Depth-cutoff (pops without push)
        t1 = base_state
        if t1 not in visited:
            visited.add(t1)
            queue.append(t1)
        transitions += 1

        # Transition 2: Subdivide into 2 children (only reachable when popped_depth < max_depth)
        if popped_depth < max_depth:
            child_depth = popped_depth + 1
            t2 = base_state + (child_depth, child_depth)
            if len(t2) > max_top_seen:
                max_top_seen = len(t2)
            if t2 not in visited:
                visited.add(t2)
                queue.append(t2)
            transitions += 1

    return max_top_seen, len(visited), transitions

def verify_source_matches_proof(shader_wgsl: str):
    """Verifies that the WGSL shader source strictly satisfies the mathematical proof assumptions.
    Refuses if seeds, cutoff depth, or pop/push increments diverge.
    """
    assert 'top = 2u;' in shader_wgsl, 'proof assumption violated: initial top != 2'
    assert 'setDepth(&depthBits, 0u, 1u);' in shader_wgsl, 'proof assumption violated: seed 0 depth != 1'
    assert 'setDepth(&depthBits, 1u, 1u);' in shader_wgsl, 'proof assumption violated: seed 1 depth != 1'
    assert 'f32(depth) >= 14.0' in shader_wgsl, 'proof assumption violated: cutoff depth != 14'
    assert 'setDepth(&depthBits, top, depth + 1u);' in shader_wgsl, 'proof assumption violated: child 0 depth != depth + 1'
    assert 'setDepth(&depthBits, top + 1u, depth + 1u);' in shader_wgsl, 'proof assumption violated: child 1 depth != depth + 1'
    assert 'top += 2u;' in shader_wgsl, 'proof assumption violated: push step != 2'

def bound_stack_15(shader: str) -> str:
    """Specializes S1 stack allocation from 20 to the proven bound 15 without switch expansion."""
    verify_source_matches_proof(shader)
    declaration = 'var stack: array<vec4f, 20>;'
    assert shader.count(declaration) == 1, 'expected exactly one stack[20] declaration'
    shader = shader.replace(declaration, 'var stack: array<vec4f, 15>;')
    guard = 'top + 2u > 20u'
    assert shader.count(guard) == 1, 'expected exactly one top+2u > 20u guard'
    shader = shader.replace(guard, 'top + 2u > 15u')
    return shader

def apply_variant(cpp: str, variant: str) -> str:
    if variant == 'canonical':
        return cpp
    if variant == 'bound-stack-15':
        names = ['kS1UnfusedProducerWgsl', 'kS1TerminalProducerWgsl' if 'kS1TerminalProducerWgsl = R"WGSL(' in cpp else 'kS1ProducerWgsl']
        for name in names:
            match = re.search(r'(static const char\* ' + name + r' = R"WGSL\()(.*?)(\)WGSL";)', cpp, re.S)
            assert match, name
            cpp = cpp[:match.start(2)] + bound_stack_15(match[2]) + cpp[match.end(2):]
        return cpp
    raise ValueError(f'unknown variant: {variant}')

def clean_cpp(root: Path, variant: str) -> str:
    base = load(root, 'ray_transport_gpu_6du_true_terminal_s1')
    constants = {x.targets[0].id: ast.literal_eval(x.value) for x in ast.parse((root / 'scratch/probes/ray_transport_metal_diagnostic.py').read_text()).body if isinstance(x, ast.Assign) and isinstance(x.value, ast.Constant) and isinstance(x.value.value, str)}
    return apply_variant(base.make_terminal_s1_cpp_source(constants['CPP_SOURCE'], root), variant)

def controls_cpp(root: Path, variant: str) -> str:
    path = root / 'scratch/probes/ray_transport_gpu_6du_controls.py'
    text = path.read_text()
    target = "root / 'scratch/probes/ray_transport_gpu_6dt_true_terminal_s1.py'"
    assert text.count(target) == 1
    text = text.replace(target, "root / 'scratch/probes/ray_transport_gpu_6du_true_terminal_s1.py'")
    hook = "source = m.make_terminal_s1_cpp_source(v['CPP_SOURCE'], root, wide=True)"
    assert text.count(hook) == 1
    text = text.replace(hook, hook + ".replace('kS1TerminalProducerWgsl', 'kS1ProducerWgsl')")
    text = text.replace("for name in ['kS2ProducerWgsl', 'kFallbackProducerWgsl']:", "for name in ['kS1UnfusedProducerWgsl', 'kS2ProducerWgsl', 'kFallbackProducerWgsl']:")
    text = text.replace('assert source.count(needle_bg_s2_fb) == 4', 'assert source.count(needle_bg_s2_fb) == 6')
    text = text.replace('if(mode==3) wgpuCommandEncoderClearBuffer', 'if(mode==2||mode==3) wgpuCommandEncoderClearBuffer')
    ns = {'__name__': 'adapted_6dx_controls'}
    exec(compile(text, str(path), 'exec'), ns)
    branch = ns['BRANCH']
    start = '        auto bitsEqual='
    assert branch.count(start) == 1
    branch = branch.replace(start, '        for(int reviewMode:{2,3}){\n' + start)
    branch = branch.replace('execute(3,rep,false);', 'execute(reviewMode,rep,false);')
    branch = branch.replace('routeChecks.push_back({{"case",label}', 'routeChecks.push_back({{"mode",reviewMode},{"case",label}')
    branch = branch.replace('        std::ofstream reviewOut(outputPath);', '        }\n        std::ofstream reviewOut(outputPath);')
    branch = branch.replace('§6du True Terminal S1 routed correctness and ownership controls', '§6dx actual Mode2 and Mode3 routed correctness and ownership controls')
    branch = branch.replace('std::cerr<<"ROUTED REVIEW "<<label', 'std::cerr<<"ROUTED REVIEW mode="<<reviewMode<<" "<<label')
    branch = branch.replace('(0 sBuf writes verified)', '(completed-S1 poison check only)')
    ns['BRANCH'] = branch
    return apply_variant(ns['generate'](root, 'fixed'), variant)

def capture_cpp(cpp: str, work: Path) -> str:
    main_func = 'int main(int argc, char** argv) {'
    assert cpp.count(main_func) == 1
    cpp = cpp.replace(main_func, 'extern "C" void rtInstallMetalObserver();\nextern "C" void rtBeginCapture(const char*);\nextern "C" void rtEndCapture();\nextern "C" void rtMetalSetContext(const char*,const char*,size_t);\n' + main_func + '\n rtInstallMetalObserver();')
    for needle, prefix in [
        ('WGPUComputePipeline cp = wgpuDeviceCreateComputePipeline(gpu.device, &cpd);', 'rtMetalSetContext("rayMain",fullCode.data(),fullCode.size());\n'),
        ('auto pipeline = wgpuDeviceCreateComputePipeline(gpu.device, &pd);', 'rtMetalSetContext(entry,text.data(),text.size());\n'),
        ('auto pipeline=wgpuDeviceCreateComputePipeline(gpu.device,&pd);', 'rtMetalSetContext(entry,text.data(),text.size());\n')
    ]:
        assert cpp.count(needle) == 1
        cpp = cpp.replace(needle, prefix + needle)
    hook = '        auto correctness='
    assert cpp.count(hook) == 1
    captures = ''
    for mode in [0, 2, 3]:
        captures += f'        rtBeginCapture("{work}/mode{mode}.gputrace"); execute({mode},1,false); rtEndCapture();\n'
    return cpp.replace(hook, captures + '        return 0;\n' + hook)

def build_capture(root: Path, build: Path, work: Path, source: Path) -> Path:
    trees = []
    for checkout in [root, build]:
        assert not subprocess.check_output(['git', '-c', 'core.fsmonitor=false', 'status', '--porcelain', '--', 'src'], cwd=checkout, text=True).strip(), 'dirty engine source'
        trees.append(subprocess.check_output(['git', 'rev-parse', 'HEAD:src'], cwd=checkout, text=True).strip())
    assert trees[0] == trees[1], 'source/build tree mismatch'
    b = build / 'build'
    observer = root / 'scratch/probes/ray_transport_metal_pipeline_observer.mm'
    subprocess.run(['/usr/bin/clang++', '-std=c++20', '-O2', '-fobjc-arc', '-c', str(observer), '-o', str(work / 'observer.o')], check=True)
    db = json.loads((b / 'compile_commands.json').read_text())
    entry = next(x for x in db if x['file'].endswith('/entry.cpp') and 'earthcall_webgpu.dir' in x['command'])
    cmd = shlex.split(entry['command'])
    cmd[cmd.index('-c') + 1] = str(source)
    cmd[cmd.index('-o') + 1] = str(work / 'probe.o')
    cmd += ['-std=c++20', '-O3', '-I' + str(root / 'scratch/probes')]
    subprocess.run(cmd, cwd=b, check=True)
    cmd = shlex.split((b / 'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text())
    cmd[next(i for i, x in enumerate(cmd) if x.endswith('/src/entry.cpp.o'))] = str(work / 'probe.o')
    cmd[cmd.index('-o') + 1] = str(work / 'probe')
    cmd += [str(work / 'observer.o'), '-framework', 'Metal', '-framework', 'Foundation']
    subprocess.run(cmd, cwd=b, check=True)
    return work / 'probe'

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root', type=Path, required=True)
    p.add_argument('--build-root', type=Path, required=True)
    p.add_argument('--work', type=Path, required=True)
    p.add_argument('--variant', choices=['canonical', 'bound-stack-15'], required=True)
    p.add_argument('--capture', action='store_true')
    p.add_argument('--timing', action='store_true')
    p.add_argument('--run', action='store_true')
    p.add_argument('--prove-only', action='store_true')
    a = p.parse_args()

    # Step 1: Run and log the formal proof
    max_top, num_states, num_trans = prove_stack_bound(max_depth=14, seed_count=2)
    print(f'PROVE: max_top = {max_top} across {num_states} unique reachable DFS stack states ({num_trans} transitions explored).')
    assert max_top == 15, 'proof failed: max_top must be 15'

    # Verify mutations detect deviations:
    m_top, _, _ = prove_stack_bound(max_depth=15, seed_count=2)
    assert m_top == 16, 'mutation test failed: max_depth=15 must yield 16'
    m_seed, _, _ = prove_stack_bound(max_depth=14, seed_count=3)
    assert m_seed == 16, 'mutation test failed: seed=3 must yield 16'
    print('PROVE: Mutation checks passed (cutoff=15 -> 16, seed=3 -> 16). Theorem holds.')

    if a.prove_only:
        return

    root = a.root.resolve()
    build = a.build_root.resolve()
    work = a.work.resolve()

    if work.exists() or work == root or root in work.parents:
        raise ValueError('fresh external work required')
    work.mkdir(parents=True, exist_ok=False)

    if a.capture and a.timing:
        raise ValueError('capture and clean timing are separate runs')

    cpp = capture_cpp(clean_cpp(root, a.variant), work) if a.capture else clean_cpp(root, a.variant) if a.timing else controls_cpp(root, a.variant)
    source = work / 'probe.cpp'
    source.write_text(cpp)
    (work / 'source.json').write_text(json.dumps({
        'variant': a.variant,
        'capture': a.capture,
        'timing': a.timing,
        'cpp_sha256': sha(source),
        'engine_src_tree': subprocess.check_output(['git', 'rev-parse', 'HEAD:src'], cwd=root, text=True).strip()
    }, indent=2) + '\n')

    if a.capture:
        binary = build_capture(root, build, work, source)
    else:
        subprocess.run([
            sys.executable, '-B', str(root / 'scratch/probes/ray_transport_probe_build.py'),
            '--root', str(root),
            '--build-root', str(build),
            '--source', str(source),
            '--work', str(work / 'build')
        ], check=True)
        binary = work / 'build/probe'

    if a.run:
        wide = load(root, 'ray_transport_gpu_wide_stream')
        zone = root / 'saves/zones/Northern Veil/zone.ecform'
        before = sha(zone)
        meta, rays, _ = wide.verify_case(root, Path('/private/tmp/earthcall-wide-case/manifest.json'), before)
        env = dict(
            os.environ,
            EC_POSE_RAYS=meta['rays'],
            EC_POSE_REFERENCE=meta['reference'],
            EC_POSE_CLOCK=str(rays['time']),
            EC_MATCH_REPEAT='1',
            EC_GPU_BUDGET='1024',
            EC_GPU_SUB_CAP='64',
            EC_GPU_OVER_CAP='16',
            EC_GPU_MARGIN='.001'
        )
        env.pop('EC_GPU_GEOMETRY_CASE', None)
        env.pop('METAL_CAPTURE_ENABLED', None)
        if a.capture:
            env.update(METAL_CAPTURE_ENABLED='1', EC_RAY_PIPELINE_FACTS=str(work / 'pipeline-facts.jsonl'))

        with (work / 'native.log').open('xb') as f:
            result = subprocess.run([str(binary), str(work / 'result.json')], cwd=root, env=env, stdout=f, stderr=subprocess.STDOUT)
        assert sha(zone) == before, 'fixture drift'
        if result.returncode:
            raise SystemExit(result.returncode)

        if a.timing:
            report = json.loads((work / 'result.json').read_text())
            rows = json.loads((work / 'result.json-rows.json').read_text())
            baseline = json.loads((work / 'result.json-baseline-rows.json').read_text())
            ref = json.loads(Path(meta['reference']).read_text())
            checked = load(root, 'ray_transport_gpu_6dd_continuation').validate_6dd(root, report, rows, baseline, rays, ref)
            (work / 'independent.json').write_text(json.dumps(checked, indent=2) + '\n')
            for rep, modes in report['clean_timing_series']['pooled'].items():
                current = modes['0']
                print('REPEAT', rep, 'Current mean/median', current['mean_ms'], current['median_ms'])
                for mode in ['2', '3']:
                    candidate = modes[mode]
                    print('MODE', mode, 'mean/median percent', 100 * (candidate['mean_ms'] / current['mean_ms'] - 1), 100 * (candidate['median_ms'] / current['median_ms'] - 1))
            print('PASS clean fixture/reference gates; one native timing invocation, two internal ordered blocks')
        elif not a.capture:
            data = json.loads((work / 'result.json').read_text())
            assert len(data['cases']) == 60 and [c['mode'] for c in data['cases']] == [2] * 30 + [3] * 30
            print('PASS', a.variant, 'actual routed modes: 60 native cases')
        else:
            print('CAPTURED', a.variant, 'modes 0/2/3; extracting plist telemetry...')
            spec_replay = importlib.util.spec_from_file_location('replay', root / 'scratch/probes/ray_transport_gpu_6dw_review_replay.py')
            m_replay = importlib.util.module_from_spec(spec_replay)
            spec_replay.loader.exec_module(m_replay)

            trace_dir = work / 'mode3.gputrace'
            all_rows = []
            s1_rows = []
            for f in sorted(trace_dir.glob('*')):
                if f.is_file():
                    try:
                        row = m_replay.decode_table(f.read_bytes())
                        if row:
                            entry = dict(row, file_hash=f.name)
                            all_rows.append(entry)
                            if row.get('function') == 's1ProducerMain':
                                s1_rows.append(entry)
                    except Exception:
                        pass
            print(f'Found {len(s1_rows)} s1ProducerMain rows (out of {len(all_rows)} total compiled pipelines) in {trace_dir.name}:')
            for r in s1_rows:
                print('  S1:', r)
            for r in all_rows:
                if r.get('function') != 's1ProducerMain':
                    print(' ', r.get('function'), r)
            telemetry_report = {
                'variant': a.variant,
                'trace': str(trace_dir),
                's1_rows': s1_rows,
                'all_rows': all_rows
            }
            (work / 'compiler_telemetry.json').write_text(json.dumps(telemetry_report, indent=2) + '\n')

if __name__ == '__main__':
    main()
