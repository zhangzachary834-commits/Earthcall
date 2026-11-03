"""Boundary Guardian (Quest 4): generated-source static checks, native negative-control sources and a Sun-sequenced plan.
Claude Code / Claude Sonnet 5.5 (claude-sonnet-5-5) / session 93555d50-5298-4a0f-8b26-c03a29808834 / 2026-10-27.
Follows docs/audits/GPU_RAY_TRANSPORT_6EM_CORRECTIVE_TAKEOVER_2026-10-27.md (6em); reuses its generators unchanged.

This script NEVER touches a GPU. It (1) regenerates the 6em clean/positive/partial sources through their own
functions, (2) statically checks the generated C++ for the properties 6em claims, proving each static check can
fail by running it on a text-mutated copy, (3) writes NEW native negative-control sources (dispatch omissions,
double publication) and an independent-capacity variant, all produced by exact-anchor mutation (anchor must match
exactly once), (4) optionally runs clang -fsyntax-only on every generated source, and (5) emits index.json with the
commands the Sun may sequence later. Expected native outcomes in index.json are PREDICTIONS, not results.

    python3 -B scratch/probes/sonnet_boundary_native_plan.py --root . --work <fresh external dir> \
        [--syntax-build-root /path/to/engine/checkout-with-build]
"""
import argparse, hashlib, importlib.util, json, re, shlex, subprocess, sys
from pathlib import Path


def load(root, name):
    s = importlib.util.spec_from_file_location(name, root / 'scratch/probes' / (name + '.py'))
    m = importlib.util.module_from_spec(s); s.loader.exec_module(m); return m


def sha(s): return hashlib.sha256(s.encode()).hexdigest()


def wgsl(s): return re.findall(r'R"WGSL\((.*?)\)WGSL"', s, re.S)


# ---------------------------------------------------------------- static checks of generated C++ -------------------------------------
def static_clean(s):
    """6em clean harness: every preflight execute is poison-preceded; every readResults is finite-checked."""
    a = s.index('        // Preflight Parity Assertion outside timed loops at scale K:')
    b = s.index('        // 4 Warmup cycles', a)
    block = s[a:b].splitlines()
    errs = []
    for k, line in enumerate(block):
        if 'execute(' in line and 'auto execute' not in line:
            if 'poisonPreflight' not in line and not (k and 'poisonPreflight();' in block[k - 1]):
                errs.append('execute without poison: ' + line.strip()[:70])
        if 'readResults()' in line and 'auto ' in line and 'finitePreflight' not in line:
            errs.append('readResults without finite check: ' + line.strip()[:70])
    if 'EC_SUN_MISSING_PUBLICATION' not in s: errs.append('missing-publication control hook absent')
    if 'preflight_poisoned' not in s: errs.append('report does not record poisoning')
    return errs


def static_positive(s):
    errs = []
    i = s.find('std::vector<bool> intendedRefusal(count,false);')
    if i < 0: return ['no independent intended set']
    derive = s[i:s.index('uint32_t refused=0', i)]
    if re.search(r'producerFlags|flagBits|fallback|0x80000000', derive): errs.append('intended set reads candidate flags')
    if 's2TileBuf' not in s[s.rfind('auto intendedBytes', 0, i):i]: errs.append('intended set not read from classifier S2 tiles')
    cmp_ = re.findall(r'if\((\w+)!=(\w+)\[i\]\)throw std::runtime_error\("late refusal set differs', s)
    if cmp_ != [('fallback', 'intendedRefusal')]: errs.append('refusal comparison is not flag-vs-intended: ' + str(cmp_))
    for needle in ('partial refused packet', 'refused ray wrote sBuf', 'exactly-once final output publication failure',
                   'exactly-once producer publication failure', 'corrupt mutually exclusive ownership flags'):
        if needle not in s: errs.append('missing check: ' + needle)
    if s.count('poisonSchedule();') < 1 or s.count('poisonOutput();') < 1: errs.append('review does not poison')
    j = s.find('auto checkReview'); k = s.find('execute(4,rep,false)', j)
    if not (0 < j < s.find('poisonOutput();', j) < s.find('poisonSchedule();', j) < k): errs.append('poison does not precede candidate execution')
    return errs


def static_negative(pos, neg):
    errs = static_positive(neg)
    if neg == pos: errs.append('negative identical to positive')
    return errs


STATIC_MUTATIONS = [
    ('clean', 'drop-preflight-poison', lambda s: s.replace('poisonPreflight(); execute(', 'execute('), static_clean),
    ('clean', 'drop-finite-check', lambda s: s.replace('finitePreflight(resM,m); ', ''), static_clean),
    ('positive', 'tautological-refusal-check', lambda s: s.replace('if(fallback!=intendedRefusal[i])', 'if(fallback!=fallback)'), static_positive),
    ('positive', 'intended-from-flags', lambda s: s.replace('intendedRefusal[tileRayIndices[j]]=true;', 'intendedRefusal[tileRayIndices[j]]=true;(void)producerFlags;'), static_positive),
    ('positive', 'drop-sBuf-poison-scan', lambda s: s.replace('throw std::runtime_error("refused ray wrote sBuf")', '(void)0'), static_positive),
    ('positive', 'drop-partial-packet-check', lambda s: s.replace('throw std::runtime_error("partial refused packet")', '(void)0'), static_positive),
    ('positive', 'review-without-output-poison', lambda s: s.replace('            poisonOutput();\n            poisonSchedule();\n            execute(4', '            poisonSchedule();\n            execute(4', 1), static_positive),
]


# ---------------------------------------------------------------- native negative controls ------------------------------------------
def once(s, old, new):
    if s.count(old) != 1: raise ValueError('anchor count %d for %r' % (s.count(old), old[:70]))
    return s.replace(old, new)


def pass_block(s, label):
    a = s.index('// ' + label)
    b = s.find('// Pass', a + 5)
    return a, (b if b > 0 else s.index('\n            }\n            auto cmd=wgpuCommandEncoderFinish', a))


def omit_pass(s, label):
    a, b = pass_block(s, label)
    body = s[a:b]
    if body.count('wgpuComputePassEncoderDispatchWorkgroups') != 1: raise ValueError('pass dispatch count: ' + label)
    return s[:a] + re.sub(r'wgpuComputePassEncoderDispatchWorkgroups(Indirect)?\([^;]*;', '(void)0;', body, count=1) + s[b:]


def omit_current(s):
    a = s.index('// Pass 7: Current Marcher Fallback')
    return s[:a] + s[a:].replace('dispatch(fallbackCP, fallbackBG, count);', '(void)0;', 1)


def double_current(s):
    a = s.index('// Pass 7: Current Marcher Fallback')
    return s[:a] + s[a:].replace('dispatch(fallbackCP, fallbackBG, count);', 'dispatch(fallbackCP, fallbackBG, count); dispatch(fallbackCP, fallbackBG, count);', 1)


NATIVE_CONTROLS = [
    # name, mutator, predicted failure substrings (ALL must exist verbatim in the unmutated source), why it must fail
    ('omit-s1-producer', lambda s: omit_pass(s, 'Pass 3: S1 Terminal Producer'),
     ['routed exactly-once producer publication failure', 'positive late-member1-refusal witness failed'], 'S1 rays never produce -> producer entries != 1 / completed_s1 short'),
    ('omit-s2-terminal-producer', lambda s: omit_pass(s, 'Pass 4: S2 Terminal Producer'),
     ['routed exactly-once producer publication failure', 'positive late-member1-refusal witness failed'], 'late-refusal member never runs -> refusal flags absent for S2 rays'),
    ('omit-fallback-producer', lambda s: omit_pass(s, 'Pass 5: Fallback Producer'),
     ['routed exactly-once producer publication failure', 'positive late-member1-refusal witness failed'], 'routed fallback tiles unowned'),
    ('omit-fallback-consumer', lambda s: omit_pass(s, 'Pass 6: Indirect Fallback Consumer'),
     ['routed exactly-once producer publication failure', 'positive late-member1-refusal witness failed'], 'fallback tile consumer missing'),
    ('omit-current-fallback', omit_current,
     ['positive late-member1-refusal witness failed', 'routed exactly-once final output publication failure'], 'refused rays never reach Current -> output stays poisoned (NaN) -> non-finite / value error'),
    ('double-current-fallback', double_current,
     ['routed exactly-once final output publication failure'], 'Current fallback dispatched twice -> final publisher count 2 on refused rays'),
]

# An independent capacity witness (variant of 6ek's coarse capacity block). The EXISTING block picks `expected` from the
# candidate's own refusal flag (`fallback?resCurrent:resUnfused`), so it cannot detect OVER-refusal. This variant derives
# the intended refusal set from the BULK reference headers (a different pipeline, nominal caps) plus the nominal-cap
# refusal set, using the shader's own stated rules (singles + n > cap  /  overs >= cap on the next append), and requires
# flags == intended exactly. LIMIT: `nominalRefused` (S2/fallback-class refusals at nominal caps) is read from the candidate's own
# flags, so this variant independently validates only the capacity INCREMENT; the nominal set is witnessed by the positive case.
# A mismatch means my rule reading or the candidate is wrong: investigate, never relax.
CAP_OLD_A = '        J capacityControls=J::array();\n'
CAP_NEW_A = '''        J capacityControls=J::array();
        // [SONNET_BOUNDARY] independent reference demand: bulk (mode 1) at NOMINAL caps, before any cap is lowered.
        wgpuQueueWriteBuffer(gpu.queue, cfgBuf, 0, &cfg, sizeof(cfg));
        std::vector<uint32_t> refDemandSub(count,0u), refDemandOver(count,0u);
        {
            poisonOutput(); execute(1,1,false);
            auto hbRef=readBackBuffer(rBuf,rayBytes);const auto* hsRef=reinterpret_cast<const GpuRayHeader*>(hbRef.data());
            for(size_t i=0;i<count;++i){refDemandSub[i]=hsRef[i].subpanelCount;refDemandOver[i]=hsRef[i].overlapCount;
                if(refDemandOver[i]&0x80000000u)throw std::runtime_error("bulk reference ray refused at nominal caps: reference demand undefined");}
        }
        std::vector<bool> nominalRefused(count,false);
        {
            poisonOutput(); execute(3,1,false);auto hbN=readBackBuffer(rBuf,rayBytes);const auto* hsN=reinterpret_cast<const GpuRayHeader*>(hbN.data());
            for(size_t i=0;i<count;++i)nominalRefused[i]=(hsN[i].overlapCount&0x80000000u)!=0;
        }
'''
CAP_OLD_B = '                const auto& expected=fallback?resCurrent[i]:resUnfused[i];for(int k=0;k<4;++k)if(!std::isfinite(res[i].rgb_T[k])'
CAP_NEW_B = '''                const bool intendedCap = nominalRefused[i] || (kind==0 ? refDemandSub[i]>4u : refDemandOver[i]>1u);
                if(fallback!=intendedCap)throw std::runtime_error("capacity refusal set differs from reference-demand derivation: ray="+std::to_string(i)+" kind="+std::to_string(kind));
                if(fallback&&hs[i].subpanelCount!=0)throw std::runtime_error("capacity-refused ray published a partial packet header");
                const auto& expected=intendedCap?resCurrent[i]:resUnfused[i];for(int k=0;k<4;++k)if(!std::isfinite(res[i].rgb_T[k])'''


def capacity_independent(s):
    return once(once(s, CAP_OLD_A, CAP_NEW_A), CAP_OLD_B, CAP_NEW_B)


# ---------------------------------------------------------------- syntax check -----------------------------------------------------
def syntax_cmd(build_root):
    db = json.loads((build_root / 'build/compile_commands.json').read_text())
    e = next(x for x in db if x['file'].endswith('/entry.cpp') and 'earthcall_webgpu' in x.get('command', '') + x.get('output', ''))
    c = shlex.split(e['command']); i = c.index('-o'); del c[i:i + 2]
    c = [x for x in c if x != '-c' and not x.endswith('entry.cpp') and not x.startswith('-std')]
    return c + ['-std=gnu++20', '-fsyntax-only'], e['directory']


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--root', type=Path, required=True); ap.add_argument('--work', type=Path, required=True)
    ap.add_argument('--syntax-build-root', type=Path)
    a = ap.parse_args(); root = a.root.resolve(); work = a.work.resolve()
    if work.exists() or work == root or root in work.parents: raise SystemExit('fresh external work dir required')
    work.mkdir(parents=True)
    t = load(root, 'ray_transport_gpu_6em_corrective_takeover')
    el = load(root, 'ray_transport_gpu_6el_clean_multi_scale')
    clean, pos, neg = t.clean_source(root), t.refusal_source(root), t.refusal_source(root, True)
    report, ok = {'checks': [], 'sources': {}, 'native_controls': {}}, True

    def check(name, cond, detail=''):
        nonlocal ok; ok &= bool(cond); report['checks'].append({'name': name, 'ok': bool(cond), 'detail': detail})
        print(('PASS ' if cond else 'FAIL ') + name + (' :: ' + detail if detail else ''))

    check('clean shader literals byte-identical to a fresh 6el regeneration', wgsl(clean) == wgsl(el.generate(root)))
    for name, s, fn in (('clean', clean, static_clean), ('positive', pos, static_positive), ('partial_negative', neg, lambda x: static_negative(pos, x))):
        e = fn(s); check('static ' + name, not e, '; '.join(e))
    import difflib
    changed = [l for l in difflib.unified_diff(pos.splitlines(), neg.splitlines(), lineterm='', n=0) if l[:1] in '+-' and l[:3] not in ('+++', '---')]
    check('negative differs from positive by exactly the pre-refusal output+audit write and the removed positive assertion',
          any(l.startswith('+') and 'atomicAdd(&auditWrites[id * 2u + 1u], 1u); reason |= 1u' in l for l in changed)
          and sum(l.startswith('+') for l in changed) == 1 and sum(l.startswith('-') for l in changed) == 8, '%d changed lines' % len(changed))
    for src, name, mut, fn in STATIC_MUTATIONS:
        base = {'clean': clean, 'positive': pos}[src]
        m = mut(base)
        check('static-control %s changes the source' % name, m != base)
        e = fn(m); check('static-control %s is flagged by the static check' % name, bool(e), '; '.join(e[:2]))
    sources = {'clean': clean, 'positive': pos, 'partial_negative': neg}
    for name, mut, expect, why in NATIVE_CONTROLS:
        missing = [x for x in expect if x not in pos]
        check('native control %s: predicted failure strings exist verbatim in the source' % name, not missing, ','.join(missing))
        try:
            m = mut(pos); check('native control %s: mutation applied' % name, m != pos)
        except ValueError as ex:
            check('native control %s: mutation applied' % name, False, str(ex)); continue
        sources['ctl_' + name.replace('-', '_')] = m; report['native_controls'][name] = {'predicted_failure_any_of': expect, 'reason': why}
    try:
        sources['capacity_independent'] = capacity_independent(pos); check('capacity-independent variant anchors apply exactly once', True)
    except ValueError as ex:
        check('capacity-independent variant anchors apply exactly once', False, str(ex))
    for k, s in sources.items():
        (work / (k + '.cpp')).write_text(s); report['sources'][k] = sha(s)
    if a.syntax_build_root:
        cmd, cwd = syntax_cmd(a.syntax_build_root.resolve())
        for k in sources:
            r = subprocess.run(cmd + [str(work / (k + '.cpp'))], capture_output=True, text=True, cwd=cwd)
            check('clang -fsyntax-only (gnu++20) ' + k, r.returncode == 0, '\n'.join([l for l in r.stderr.splitlines() if 'error' in l][:3]))
    else:
        print('SKIP syntax check (no --syntax-build-root)')
    report['native_commands_for_the_sun'] = {
        'note': 'NOT EXECUTED. GPU lane is the Sun\'s to sequence. Run one source at a time, fixture hash checked before and after, on the engine checkout whose src tree matches the build.',
        'build_each': 'python3 -B scratch/probes/ray_transport_probe_build.py --root <engine> --build-root <engine> --source <work>/<name>.cpp --work <fresh>/<name>-build',
        'run_env': 'EC_POSE_RAYS EC_POSE_REFERENCE EC_POSE_CLOCK from the 6em verified case; EC_MATCH_REPEAT=1 EC_GPU_BUDGET=1024 EC_GPU_SUB_CAP=64 EC_GPU_OVER_CAP=16 EC_GPU_MARGIN=.001 (same as ray_transport_gpu_6em_corrective_takeover.py run())',
        'run': '<fresh>/<name>-build/probe <out>.json  (cwd=<engine>)',
        'positive_expected_for_ctl_*': 'nonzero exit AND one predicted string in the log; any other text means the control failed at the wrong boundary (a harness defect, not a pass)',
        'capacity_independent': 'expected to run to completion with identical outputs; a "capacity refusal set differs" throw is a FINDING to adjudicate, not to relax',
    }
    (work / 'index.json').write_text(json.dumps(report, indent=1) + '\n')
    print('INDEX', work / 'index.json', 'ALL OK' if ok else 'FAILURES'); return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
