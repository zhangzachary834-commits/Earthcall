"""§6dz: isolate producer-pass coalescing.

Antigravity / Gemini 2.5 Pro (Sparkly Guy) / 50cf28df-09e9-4c2b-a4d5-1f796abbc73a / 2026-10-26.
Commissioned by Sixth-One Sun (Codex / GPT-6.1 Sol / session 01a148a7-bd85-7550-a416-0e859a729c1a).
Scratch-only diagnostic research on Apple M5.

Isolates a single architectural execution variable:
Coalescing the three indirect producer dispatches (cpS1, cpS2, cpFallback)
into ONE compute pass (WGPUComputePassEncoder) within the qualified array-backed routed path.
Retains identical pipelines, bind groups, indirect offsets, and shaders byte-for-byte.
Preserves Mode 0 (Current), Mode 1 (Guarded Bulk), Mode 2 (Unchanged Routed Unfused Control),
and Mode 3 (Unchanged Terminal S1 Control), testing alongside Candidate Mode 4 (Coalesced Routed Unfused)
and Candidate Mode 5 (Coalesced Terminal S1).
"""
import argparse, ast, hashlib, importlib.util, json, os, re, shlex, subprocess, sys
from pathlib import Path
from collections import deque

def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def load(root, name):
    spec = importlib.util.spec_from_file_location(name, root / 'scratch/probes' / f'{name}.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module

def prove_stack_bound(max_depth=14, seed_count=2):
    """Exhaust the finite abstraction of pending-depth configurations.
    Returns (max_top_seen, total_unique_states, total_transitions).
    """
    if type(max_depth) is not int or type(seed_count) is not int or max_depth < 1 or seed_count < 1:
        raise ValueError('positive integer depth and seed count required')
    initial_state = tuple([1] * seed_count)
    visited = {initial_state}
    queue = deque([initial_state])
    max_top_seen = len(initial_state)

    transitions = 0
    while queue:
        state = queue.popleft()
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

PROOF_FUNCTION_SHA256 = {
    'roots': '453057d9914fcadaec0e555155843c97837e7586f573bbba8ffe605fee28d9c6',
    'getDepth': '27cf285d3c8729c3f1d1b4053d77d9188d254429085b159ea92616ac29c3cae0',
    'setDepth': '47c3286e2615395af82aa1fdff2c0138414de918bbf363e996caa82ce3771cc4',
}

def proof_code(shader):
    """Remove line/nested block comments without accepting comment decoys."""
    out, i, block_depth = [], 0, 0
    while i < len(shader):
        if block_depth:
            if shader.startswith('/*', i): block_depth += 1; i += 2
            elif shader.startswith('*/', i): block_depth -= 1; i += 2
            else: i += 1
            continue
        if shader.startswith('//', i):
            end = shader.find('\n', i + 2)
            i = len(shader) if end < 0 else end
            out.append(' ')
        elif shader.startswith('/*', i):
            block_depth = 1; i += 2; out.append(' ')
        else:
            out.append(shader[i]); i += 1
    if block_depth:
        raise ValueError('unterminated WGSL block comment')
    return ''.join(out)

def proof_function(shader, name):
    code = proof_code(shader)
    starts = list(re.finditer(r'\bfn\s+' + re.escape(name) + r'\s*\(', code))
    if len(starts) != 1:
        raise ValueError(f'proof requires exactly one {name} definition')
    start = starts[0].start()
    opening = code.find('{', start)
    if opening < 0:
        raise ValueError(f'missing {name} body')
    depth, end = 1, opening + 1
    while depth and end < len(code):
        depth += (code[end] == '{') - (code[end] == '}')
        end += 1
    if depth:
        raise ValueError(f'unbalanced {name} body')
    return code[start:end]

def verify_source_matches_proof(shader_wgsl: str):
    """Bind the abstract proof to complete roots and depth-helper tokens."""
    bindings = {}
    for name, expected in PROOF_FUNCTION_SHA256.items():
        body = proof_function(shader_wgsl, name)
        tokens = re.findall(r'\w+|[^\s]', body)
        actual = hashlib.sha256('\x1f'.join(tokens).encode()).hexdigest()
        if actual != expected:
            raise ValueError(f'{name} differs from the source-bound stack proof; renew proof before specializing')
        bindings[name] = actual
    return bindings

def verify_proof_sources(root):
    shaders = load(root, 'ray_transport_gpu_6du_true_terminal_s1').make_terminal_s1_shaders(root)
    results = []
    for name in ['kS1UnfusedProducerWgsl', 'kS1TerminalProducerWgsl']:
        source = shaders[name]
        verify_source_matches_proof(source)
        getter = proof_function(source, 'getDepth')
        bad_getter = re.sub(r'return\s+[^;]+;', 'return 0u;', getter)
        mutations = {
            'missing-pop': source.replace('top--; let cell = stack[top];', 'let cell = stack[top];'),
            'ineffective-cutoff': source.replace('if (f32(depth) >= 14.0 ||', 'if ((false && f32(depth) >= 14.0) ||'),
            'cutoff-depth-15': source.replace('f32(depth) >= 14.0', 'f32(depth) >= 15.0'),
            'seed-comment-decoy': source.replace('top = 2u;', 'top = 3u; // top = 2u;'),
            'push-comment-decoy': source.replace('top += 2u;', 'top += 3u; // top += 2u;'),
            'child-depth-change': source.replace('depth + 1u', 'depth + 2u'),
            'depth-decoder-zero': source.replace(getter, bad_getter),
        }
        for mutation, changed in mutations.items():
            if changed == source:
                raise ValueError(f'{name}: mutation hook absent: {mutation}')
            try:
                verify_source_matches_proof(changed)
            except ValueError:
                results.append({'shader': name, 'mutation': mutation, 'rejected': True})
            else:
                raise ValueError(f'unsound source mutation admitted: {name}/{mutation}')
        verify_source_matches_proof('// unrelated comment\n' + source.replace('top--;', 'top -- ;'))
        verify_source_matches_proof('/* outer /* nested */ fn roots() {{decoy}} */\n' + source)
    return results

def bound_stack_15(shader: str) -> str:
    verify_source_matches_proof(shader)
    declaration = 'var stack: array<vec4f, 20>;'
    if shader.count(declaration) != 1: raise ValueError('expected exactly one stack[20] declaration')
    shader = shader.replace(declaration, 'var stack: array<vec4f, 15>;')
    guard = 'top + 2u > 20u'
    if shader.count(guard) != 1: raise ValueError('expected exactly one top+2u > 20u guard')
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

COALESCED_DISPATCH_SNIPPET = '''                else if(mode==4){
                    // Mode 4: §6dz Candidate Coalesced Routed Unfused (cpS1Unfused + cpS2 + cpFallback in 1 Compute Pass)
                    // Pass 0: Reset Counters
                    {
                        auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                        wgpuComputePassEncoderSetPipeline(p, cpReset);
                        wgpuComputePassEncoderSetBindGroup(p, 0, bgReset, 0, nullptr);
                        wgpuComputePassEncoderDispatchWorkgroups(p, 1, 1, 1);
                        wgpuComputePassEncoderEnd(p);
                        wgpuComputePassEncoderRelease(p);
                    }
                    // Pass 1: Classify Tiles
                    {
                        auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                        wgpuComputePassEncoderSetPipeline(p, cpClassify);
                        wgpuComputePassEncoderSetBindGroup(p, 0, bgClassify, 0, nullptr);
                        wgpuComputePassEncoderDispatchWorkgroups(p, 36, 1, 1);
                        wgpuComputePassEncoderEnd(p);
                        wgpuComputePassEncoderRelease(p);
                    }
                    // Pass 2: Route Args
                    {
                        auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                        wgpuComputePassEncoderSetPipeline(p, cpArgs);
                        wgpuComputePassEncoderSetBindGroup(p, 0, bgArgs, 0, nullptr);
                        wgpuComputePassEncoderDispatchWorkgroups(p, 3, 1, 1);
                        wgpuComputePassEncoderEnd(p);
                        wgpuComputePassEncoderRelease(p);
                    }
                    // Pass 3: Coalesced Producers Pass (Passes 3, 4, 5 in a single WGPUComputePassEncoder!)
                    {
                        auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                        wgpuComputePassEncoderSetPipeline(p, cpS1Unfused);
                        wgpuComputePassEncoderSetBindGroup(p, 0, forced ? bgS1UnfusedForced : bgS1Unfused, 0, nullptr);
                        wgpuComputePassEncoderDispatchWorkgroupsIndirect(p, argsBuf, 0);

                        wgpuComputePassEncoderSetPipeline(p, cpS2);
                        wgpuComputePassEncoderSetBindGroup(p, 0, forced ? bgS2Forced : bgS2, 0, nullptr);
                        wgpuComputePassEncoderDispatchWorkgroupsIndirect(p, argsBuf, 12);

                        wgpuComputePassEncoderSetPipeline(p, cpFallback);
                        wgpuComputePassEncoderSetBindGroup(p, 0, forced ? bgFallbackForced : bgFallback, 0, nullptr);
                        wgpuComputePassEncoderDispatchWorkgroupsIndirect(p, argsBuf, 24);

                        wgpuComputePassEncoderEnd(p);
                        wgpuComputePassEncoderRelease(p);
                    }
                    // Pass 4: Canonical Unfused A* Consumer
                    dispatch(cpUnfused, bgUnfused, count);
                    // Pass 5: Production-Generated Current Marcher Fallback
                    dispatch(fallbackCP, fallbackBG, count);
                }
                else if(mode==5){
                    // Mode 5: §6dz Candidate Coalesced Terminal S1 (cpS1Terminal + cpS2 + cpFallback in 1 Compute Pass)
                    // Pass 0: Reset Counters
                    {
                        auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                        wgpuComputePassEncoderSetPipeline(p, cpReset);
                        wgpuComputePassEncoderSetBindGroup(p, 0, bgReset, 0, nullptr);
                        wgpuComputePassEncoderDispatchWorkgroups(p, 1, 1, 1);
                        wgpuComputePassEncoderEnd(p);
                        wgpuComputePassEncoderRelease(p);
                    }
                    // Pass 1: Classify Tiles
                    {
                        auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                        wgpuComputePassEncoderSetPipeline(p, cpClassify);
                        wgpuComputePassEncoderSetBindGroup(p, 0, bgClassify, 0, nullptr);
                        wgpuComputePassEncoderDispatchWorkgroups(p, 36, 1, 1);
                        wgpuComputePassEncoderEnd(p);
                        wgpuComputePassEncoderRelease(p);
                    }
                    // Pass 2: Route Args
                    {
                        auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                        wgpuComputePassEncoderSetPipeline(p, cpArgs);
                        wgpuComputePassEncoderSetBindGroup(p, 0, bgArgs, 0, nullptr);
                        wgpuComputePassEncoderDispatchWorkgroups(p, 3, 1, 1);
                        wgpuComputePassEncoderEnd(p);
                        wgpuComputePassEncoderRelease(p);
                    }
                    // Pass 3: Coalesced Producers Pass (Passes 3, 4, 5 in a single WGPUComputePassEncoder!)
                    {
                        auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                        wgpuComputePassEncoderSetPipeline(p, cpS1Terminal);
                        wgpuComputePassEncoderSetBindGroup(p, 0, forced ? bgS1TerminalForced : bgS1Terminal, 0, nullptr);
                        wgpuComputePassEncoderDispatchWorkgroupsIndirect(p, argsBuf, 0);

                        wgpuComputePassEncoderSetPipeline(p, cpS2);
                        wgpuComputePassEncoderSetBindGroup(p, 0, forced ? bgS2Forced : bgS2, 0, nullptr);
                        wgpuComputePassEncoderDispatchWorkgroupsIndirect(p, argsBuf, 12);

                        wgpuComputePassEncoderSetPipeline(p, cpFallback);
                        wgpuComputePassEncoderSetBindGroup(p, 0, forced ? bgFallbackForced : bgFallback, 0, nullptr);
                        wgpuComputePassEncoderDispatchWorkgroupsIndirect(p, argsBuf, 24);

                        wgpuComputePassEncoderEnd(p);
                        wgpuComputePassEncoderRelease(p);
                    }
                    // Pass 4: Canonical Unfused A* Consumer
                    dispatch(cpUnfused, bgUnfused, count);
                    // Pass 5: Production-Generated Current Marcher Fallback
                    dispatch(fallbackCP, fallbackBG, count);
                }'''

def inject_coalesced_modes(cpp: str) -> str:
    needle = 'dispatch(fallbackCP, fallbackBG, count);\n                }\n            }\n            auto cmd=wgpuCommandEncoderFinish(enc,nullptr);'
    assert cpp.count(needle) == 1, 'dispatch finish hook not found'
    replacement = 'dispatch(fallbackCP, fallbackBG, count);\n                }\n' + COALESCED_DISPATCH_SNIPPET + '\n            }\n            auto cmd=wgpuCommandEncoderFinish(enc,nullptr);'
    return cpp.replace(needle, replacement)

def clean_cpp(root: Path, variant: str) -> str:
    base = load(root, 'ray_transport_gpu_6du_true_terminal_s1')
    constants = {x.targets[0].id: ast.literal_eval(x.value) for x in ast.parse((root / 'scratch/probes/ray_transport_metal_diagnostic.py').read_text()).body if isinstance(x, ast.Assign) and isinstance(x.value, ast.Constant) and isinstance(x.value.value, str)}
    cpp = base.make_terminal_s1_cpp_source(constants['CPP_SOURCE'], root)
    cpp = inject_coalesced_modes(cpp)

    # Adapt Section C timing loop to test modes 0..5
    old_warmup = 'for(int k=0;k<4;++k){execute(0,1,false);execute(1,1,false);execute(2,1,false);execute(3,1,false);}'
    assert cpp.count(old_warmup) == 1
    new_warmup = 'for(int k=0;k<4;++k){for(int m=0;m<6;++m)execute(m,1,false);}'
    cpp = cpp.replace(old_warmup, new_warmup)

    old_order = 'const std::vector<int> order = block % 2 ? std::vector<int>{3, 2, 1, 0, 0, 1, 2, 3} : std::vector<int>{0, 1, 2, 3, 3, 2, 1, 0};'
    assert cpp.count(old_order) == 1
    new_order = 'const std::vector<int> order = block % 2 ? std::vector<int>{5, 4, 3, 2, 1, 0, 0, 1, 2, 3, 4, 5} : std::vector<int>{0, 1, 2, 3, 4, 5, 5, 4, 3, 2, 1, 0};'
    cpp = cpp.replace(old_order, new_order)

    old_vec = 'std::vector<std::vector<double>> modeSamples(4);'
    assert cpp.count(old_vec) == 1
    new_vec = 'std::vector<std::vector<double>> modeSamples(6);'
    cpp = cpp.replace(old_vec, new_vec)

    old_m_loop = 'for (int m = 0; m < 4; ++m) {'
    assert cpp.count(old_m_loop) == 2
    new_m_loop = 'for (int m = 0; m < 6; ++m) {'
    cpp = cpp.replace(old_m_loop, new_m_loop)

    old_series_push = 'if (sIdx == 1) {\n                    series.push_back({{"repeats", rep}, {"samples", series1Samples}});\n                }'
    assert cpp.count(old_series_push) == 1
    new_series_push = '''if (sIdx == 1) {
                    J legSamples = J::array();
                    for (const auto& s : series1Samples) {
                        if (s["mode"].get<int>() < 4) legSamples.push_back(s);
                    }
                    series.push_back({{"repeats", rep}, {"samples", legSamples}});
                }'''
    cpp = cpp.replace(old_series_push, new_series_push)

    return apply_variant(cpp, variant)

def controls_cpp(root: Path, variant: str, mutation: str = 'fixed') -> str:
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
    text = text.replace('if(mode==3) wgpuCommandEncoderClearBuffer', 'if(mode>=2 && mode<=5) wgpuCommandEncoderClearBuffer')
    ns = {'__name__': 'adapted_6dz_controls'}
    exec(compile(text, str(path), 'exec'), ns)
    branch = ns['BRANCH']
    start = '        auto bitsEqual='
    assert branch.count(start) == 1
    # Run the full 30 cases across ALL 4 routed modes: Mode 2, Mode 3, Mode 4, Mode 5 (120 cases total!)
    branch = branch.replace(start, '        for(int reviewMode:{2,3,4,5}){\n' + start)
    branch = branch.replace('execute(3,rep,false);', 'execute(reviewMode,rep,false);')
    branch = branch.replace('routeChecks.push_back({{"case",label}', 'routeChecks.push_back({{"mode",reviewMode},{"case",label}')
    branch = branch.replace('        std::ofstream reviewOut(outputPath);', '        }\n        std::ofstream reviewOut(outputPath);')
    branch = branch.replace('§6du True Terminal S1 routed correctness and ownership controls', '§6dz coalesced & uncoalesced routed correctness and ownership controls')
    branch = branch.replace('std::cerr<<"ROUTED REVIEW "<<label', 'std::cerr<<"ROUTED REVIEW mode="<<reviewMode<<" "<<label')
    branch = branch.replace('(0 sBuf writes verified)', '(completed-S1 poison check only)')
    ns['BRANCH'] = branch
    cpp = ns['generate'](root, mutation)
    cpp = inject_coalesced_modes(cpp)
    return apply_variant(cpp, variant)

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
    for mode in [0, 2, 3, 4, 5]:
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
    p.add_argument('--variant', choices=['canonical', 'bound-stack-15'], default='bound-stack-15')
    p.add_argument('--mutation', choices=['fixed', 'no-reset', 'no-tail', 'double-publish', 'missing-publish', 'fallback-missing-publish', 'fallback-double-publish', 'terminal-endpoint-corrupt'], default='fixed')
    p.add_argument('--capture', action='store_true')
    p.add_argument('--timing', action='store_true')
    p.add_argument('--run', action='store_true')
    p.add_argument('--prove-only', action='store_true')
    a = p.parse_args()

    max_top, num_states, num_trans = prove_stack_bound(max_depth=14, seed_count=2)
    print(f'PROVE (abstract model): max_top = {max_top} across {num_states} unique reachable DFS stack states ({num_trans} transitions explored).')
    assert max_top == 15, 'proof failed: max_top must be 15'
    m_top, _, _ = prove_stack_bound(max_depth=15, seed_count=2)
    assert m_top == 16, 'mutation test failed: max_depth=15 must yield 16'
    m_seed, _, _ = prove_stack_bound(max_depth=14, seed_count=3)
    assert m_seed == 16, 'mutation test failed: seed=3 must yield 16'
    print('PROVE: Mutation checks passed (cutoff=15 -> 16, seed=3 -> 16). Theorem holds.')

    root = a.root.resolve()
    source_results = verify_proof_sources(root)
    assert len(source_results) == 14 and all(r['rejected'] for r in source_results)
    print('PROVE: both actual source certificates match; 14 emitted-source mutations rejected.')

    if a.prove_only:
        return

    build = a.build_root.resolve()
    work = a.work.resolve()

    if work.exists() or work == root or root in work.parents:
        raise ValueError('fresh external work required')
    work.mkdir(parents=True, exist_ok=False)

    if a.capture and a.timing:
        raise ValueError('capture and clean timing are separate runs')

    cpp = capture_cpp(clean_cpp(root, a.variant), work) if a.capture else clean_cpp(root, a.variant) if a.timing else controls_cpp(root, a.variant, mutation=a.mutation)
    source = work / 'probe.cpp'
    source.write_text(cpp)
    (work / 'source.json').write_text(json.dumps({
        'variant': a.variant,
        'mutation': a.mutation,
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
                m2 = modes['2']
                m3 = modes['3']
                m4 = modes['4']
                m5 = modes['5']
                print(f'REPEAT {rep} Current: mean={current["mean_ms"]:.3f} ms, med={current["median_ms"]:.3f} ms')
                print(f'  Mode 2 Unchanged Unfused: med={m2["median_ms"]:.3f} ms ({(m2["median_ms"]/current["median_ms"]-1)*100:+.2f}%)')
                print(f'  Mode 4 Coalesced Unfused: med={m4["median_ms"]:.3f} ms ({(m4["median_ms"]/current["median_ms"]-1)*100:+.2f}%) | vs M2: {(m4["median_ms"]/m2["median_ms"]-1)*100:+.2f}%')
                print(f'  Mode 3 Unchanged Terminal: med={m3["median_ms"]:.3f} ms ({(m3["median_ms"]/current["median_ms"]-1)*100:+.2f}%)')
                print(f'  Mode 5 Coalesced Terminal: med={m5["median_ms"]:.3f} ms ({(m5["median_ms"]/current["median_ms"]-1)*100:+.2f}%) | vs M3: {(m5["median_ms"]/m3["median_ms"]-1)*100:+.2f}%')
            print('PASS clean fixture/reference gates; one native timing invocation, two internal ordered blocks, 6 modes')
        elif not a.capture:
            data = json.loads((work / 'result.json').read_text())
            modes_present = sorted(list(set(c['mode'] for c in data['cases'])))
            assert modes_present == [2, 3, 4, 5], f'unexpected modes: {modes_present}'
            assert len(data['cases']) == 120, f'expected 120 cases, got {len(data["cases"])}'
            print('PASS', a.variant, 'actual routed modes (2, 3, 4, 5): 120 native cases (all exact bit parity and packet equality)')
        else:
            print('CAPTURED', a.variant, 'modes 0, 2, 3, 4, 5; extracting plist telemetry...')
            spec_replay = importlib.util.spec_from_file_location('replay', root / 'scratch/probes/ray_transport_gpu_6dw_review_replay.py')
            m_replay = importlib.util.module_from_spec(spec_replay)
            spec_replay.loader.exec_module(m_replay)

            trace_dir = work / 'mode5.gputrace'
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
            print(f'Found {len(s1_rows)} s1ProducerMain rows in {trace_dir.name}:')
            for r in s1_rows:
                print('  S1:', r)
            telemetry_report = {
                'variant': a.variant,
                'trace': str(trace_dir),
                's1_rows': s1_rows,
                'all_rows': all_rows
            }
            (work / 'compiler_telemetry.json').write_text(json.dumps(telemetry_report, indent=2) + '\n')

if __name__ == '__main__':
    main()
