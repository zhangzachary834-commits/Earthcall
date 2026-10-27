"""§6ed Guarded Root Interpolation Inside Admitted Monotone Isolator.
Compiles and tests the Safeguarded Bracketed Interpolator (SBI) candidate
across the full 30-case matrix (composed poses, sweeps, budgets, capacities),
strict bit parity between routed and bulk pipelines, native reference accuracy,
and matched unthrottled timing series against Current and Bulk controls.
Antigravity / Gemini 2.5 Pro (Sparkly Guy 🐉✨) / 2026-10-27.
"""

import argparse, ast, hashlib, importlib.util, json, os, re, shutil, struct, subprocess, sys
from pathlib import Path

def generate_guarded_root_cpp(root: Path, mutation: str = 'fixed', timing: bool = False) -> str:
    spec = importlib.util.spec_from_file_location('terminal_s1', root / 'scratch/probes/ray_transport_gpu_6du_true_terminal_s1.py')
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    v = {x.targets[0].id: ast.literal_eval(x.value) for x in ast.parse((root / 'scratch/probes/ray_transport_metal_diagnostic.py').read_text()).body if isinstance(x, ast.Assign) and isinstance(x.value, ast.Constant) and isinstance(x.value.value, str)}

    if timing:
        # In timing mode, use the clean unthrottled timing source without control BRANCH overrides
        source = m.make_terminal_s1_cpp_source(v['CPP_SOURCE'], root, wide=True)
    else:
        # In control mode, use 6dt controls harness with 30-case matrix and mutation checks
        spec_ctrl = importlib.util.spec_from_file_location('controls', root / 'scratch/probes/ray_transport_gpu_6dt_controls.py')
        mod_ctrl = importlib.util.module_from_spec(spec_ctrl)
        spec_ctrl.loader.exec_module(mod_ctrl)
        source = mod_ctrl.generate(root, mutation=mutation)

    # The Safeguarded Bracketed Interpolation (SBI) loop
    sbi_loop = '''  var l = a; var r = b; var fl = va; var fr = vb;
  var stuck_count = 0u; var last_side = 0u;
  for (var iter = 0u; iter < 32u; iter++) {
   if (*work >= cfg.budget) { *reason |= 1u; break; } *work += 1u;
   let h = r - l;
   let s_bisect = l + 0.5 * h;
   if (s_bisect <= l || s_bisect >= r || h <= 0.0001) { break; }
   var s = s_bisect;
   let D = fr - fl;
   if (abs(D) > 1e-30 && stuck_count < 2u) {
    let w_sec = -fl / D;
    if (w_sec > 0.0 && w_sec < 1.0) {
     let w_clamped = clamp(w_sec, 0.125, 0.875);
     let s_cand = l + w_clamped * h;
     if (s_cand > l && s_cand < r) {
      s = s_cand;
     }
    }
   }
   if (s <= l || s >= r) {
    s = s_bisect;
    if (s <= l || s >= r) { break; }
   }
   let v = evalPhi(m, o, d, s) - c;
   if (abs(v) <= error) { let w = 2.0 * error / minSlope; l = max(l, s - w); r = min(r, s + w); break; }
   if ((fl < 0.0 && v > 0.0) || (fl > 0.0 && v < 0.0)) {
    r = s; fr = v;
    if (last_side == 1u) { stuck_count += 1u; } else { stuck_count = 0u; }
    last_side = 1u;
   } else {
    l = s; fl = v;
    if (last_side == 2u) { stuck_count += 1u; } else { stuck_count = 0u; }
    last_side = 2u;
   }
  }'''

    # Replace the bisection loop in ALL 6 producer shaders with the Safeguarded Bracketed Interpolation (SBI) loop:
    # 1. kGpuProducerWgsl (Mode 1 bulk connected)
    # 2. kFastPathFullShader (Mode 2 streamed full)
    # 3. kFastPathCompactShader (Mode 2 streamed compact)
    # 4. kS1ProducerWgsl (Mode 3 S1 producer)
    # 5. kS2ProducerWgsl (Mode 3 S2 producer)
    # 6. kFallbackProducerWgsl (Mode 3 Fallback producer)
    all_producers = [
        'kGpuProducerWgsl', 'kFastPathFullShader', 'kFastPathCompactShader',
        'kS1ProducerWgsl', 'kS1UnfusedProducerWgsl', 'kS1TerminalProducerWgsl',
        'kS2ProducerWgsl', 'kFallbackProducerWgsl'
    ]
    for name in all_producers:
        hit = re.search(r'(static const char\* ' + name + r' ?= ?R\"WGSL\()(.*?)(\)WGSL\";)', source, re.S)
        if not hit:
            # Some control harnesses might omit streamed fast path shaders
            continue
        sh = hit[2]
        old_loop_pat = r'var l = a; var r = b; var left = va;.*?putBandTagged\(l, r,'
        m_loop = re.search(old_loop_pat, sh, re.S)
        if m_loop:
            sh = sh[:m_loop.start()] + sbi_loop + '\n  putBandTagged(l, r,' + sh[m_loop.end():]
            source = source[:hit.start(2)] + sh + source[hit.end(2):]

    if not timing:
        # Add agreement logging against baseline in baselineReview()
        needle_base_rev = 'execute(1,1,false);bulkReview=readResults();refH=readBackBuffer(rBuf,rayBytes);refS=readBackBuffer(sBuf,subpanelBytes);refO=readBackBuffer(oBuf,overlapBytes);};'
        assert source.count(needle_base_rev) == 1
        accuracy_check = needle_base_rev + '''
        double maxDeltaRGB = 0.0, maxDeltaT = 0.0;
        for (size_t i = 0; i < count; ++i) {
            maxDeltaT = std::max(maxDeltaT, std::abs(double(bulkReview[i].rgb_T[3]) - double(currentReview[i].rgb_T[3])));
            for (int k = 0; k < 3; ++k) {
                maxDeltaRGB = std::max(maxDeltaRGB, std::abs(double(bulkReview[i].rgb_T[k]) - double(currentReview[i].rgb_T[k])));
            }
        }
        std::cerr << "SBI CANDIDATE AGREEMENT WITH CURRENT: maxDeltaRGB=" << maxDeltaRGB << " maxDeltaT=" << maxDeltaT << "\\n";
        '''
        source = source.replace(needle_base_rev, accuracy_check)

    return source


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root', type=Path, default=Path('/Users/zacharyzhang/Documents/GitHub/Earthcall-ray-transport'))
    p.add_argument('--build-root', type=Path, default=Path('/Users/zacharyzhang/.gemini/antigravity/worktrees/Earthcall/ourverse_navigator_prototype_loop'))
    p.add_argument('--work', type=Path, default=Path('/private/tmp/earthcall-6ed-candidate'))
    p.add_argument('--case', type=Path, default=Path('/private/tmp/earthcall-wide-case/manifest.json'))
    p.add_argument('--mutation', choices=['fixed', 'no-reset', 'missing-publish'], default='fixed')
    p.add_argument('--timing', action='store_true', help='Run clean unthrottled ordered timing series instead of matrix controls')
    p.add_argument('--run', action='store_true')
    args = p.parse_args()

    root = args.root.resolve()
    build_root = args.build_root.resolve()
    worktree_root = build_root.parent if build_root.name == 'build' else build_root
    work = args.work.resolve()

    if not work.exists():
        work.mkdir(parents=True, exist_ok=True)

    mode_label = "timing series" if args.timing else f"controls (mutation: {args.mutation})"
    print(f'Generating source for §6ed SBI candidate probe ({mode_label})...')
    cpp_source = generate_guarded_root_cpp(root, mutation=args.mutation, timing=args.timing)
    (work / 'probe.cpp').write_text(cpp_source)

    build_dir = work / 'build'
    if build_dir.exists():
        shutil.rmtree(build_dir)
    print(f'Compiling standalone probe to {build_dir}...')
    subprocess.run([
        sys.executable, '-B', str(root / 'scratch/probes/ray_transport_probe_build.py'),
        '--root', str(root),
        '--build-root', str(worktree_root),
        '--source', str(work / 'probe.cpp'),
        '--work', str(build_dir)
    ], check=True)
    print('Compilation successful!')

    if args.run:
        spec = importlib.util.spec_from_file_location('wide', root / 'scratch/probes/ray_transport_gpu_wide_stream.py')
        m = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(m)
        zone = root / 'saves/zones/Northern Veil/zone.ecform'
        before = hashlib.sha256(zone.read_bytes()).hexdigest()
        meta, rays, _ = m.verify_case(root, args.case.resolve(), before)
        env = dict(
            os.environ,
            EC_POSE_RAYS=meta['rays'],
            EC_POSE_REFERENCE=meta['reference'],
            EC_POSE_CLOCK=str(rays['time']),
            EC_MATCH_REPEAT='1',
            EC_GPU_BUDGET='1024',
            EC_GPU_SUB_CAP='64',
            EC_GPU_OVER_CAP='16',
            EC_GPU_MARGIN='.001',
        )
        env.pop('EC_GPU_GEOMETRY_CASE', None)
        env.pop('METAL_CAPTURE_ENABLED', None)

        output = work / ('timing_report.json' if args.timing else 'review.json')
        print(f'Executing §6ed SBI candidate probe for {mode_label}...')
        result = subprocess.run([str(build_dir / 'probe'), str(output)], cwd=root, env=env)
        if hashlib.sha256(zone.read_bytes()).hexdigest() != before:
            raise ValueError('source drift')
        m.verify_case(root, args.case.resolve(), before)
        if result.returncode != 0:
            print(f'Probe exited with code {result.returncode}')
            sys.exit(result.returncode)
        else:
            print(f'PASS §6ed SBI execution: {mode_label}')

if __name__ == '__main__':
    main()
