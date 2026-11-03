"""Oracle Keeper small CPU controls for sonnet_oracle_capture_gate. No GPU, no engine, no fixture reads.
Claude Code / Claude Sonnet 5.5 (claude-sonnet-5-5) / session 37985354-1fde-47c7-9707-50ceed335c11 / 2026-10-27.
The synthetic reference mirrors veil_projected_reference.cpp's scheme: sigma = max(X,1e-6)*[D>0] (strict),
sample positions rounded to f32, cumulative 4th-order Simpson on one finest grid read back at stride 2 for
the half level. These are controls of the GATE LOGIC and of the reference's behaviour at kinks/support edges,
not evidence about Northern Veil. Grid is 2^12 (Python speed); the refusal ceiling is unchanged (1e-6).
"""
import copy, json, math, struct, sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
import sonnet_oracle_capture_gate as G

def f32(x): return struct.unpack('<f', struct.pack('<f', x))[0]
LOG2N = 12

def reference_ray(Dfn, Xfn, E, a, b, log2n=LOG2N):
    N = 1 << log2n; h = (b - a) / N; sig = []; src = []
    for k in range(N + 1):
        s = f32(a + k * h); d = Dfn(s)
        if d > 0: sig.append(max(Xfn(s), 1e-6)); src.append(max(E, 0.0))
        else: sig.append(0.0); src.append(0.0)
    out = []
    for lev in (0, 1):
        st = 1 << lev; H = h * st; n = N // st; tau = 0.0; L = 0.0
        g = lambda k, t: math.exp(-t) * src[k * st]
        ga = g(0, 0.0)
        for j in range(0, n, 2):
            s0, s1, s2 = sig[j * st], sig[(j + 1) * st], sig[(j + 2) * st]
            tm = tau + H * (5 * s0 + 8 * s1 - s2) / 12; tb = tau + H * (s0 + 4 * s1 + s2) / 3
            gm, gb = g(j + 1, tm), g(j + 2, tb); L += H * (ga + 4 * gm + gb) / 3; ga = gb; tau = tb
        out.append((math.exp(-tau), L))
    return out  # [(T, L) finest, (T, L) half]

def checker_ref(rays, ref_rows, sha):
    return {'rays_sha256': sha, 'columns': ['px', 'py', 'T', 'Lr', 'Lg', 'Lb', 'T_half', 'Lr_half', 'Lg_half', 'Lb_half'],
            'rows': [[r[0], r[1], *x] for r, x in zip(rays, ref_rows)]}

def row_from(T, L): return [T, L, L, L, T, L, L, L]

def make_capture(n=400, clock=7.25, zone='z' * 64):
    rays = [[i % 20, i // 20, 0.0, 0.0, 1.0, 0.0, 100.0] + [0.0] * 16 for i in range(n)]
    return {'schema': G.SCHEMA, 'zone_sha256': zone, 'time': clock, 'medium_clocks': {'m0': clock, 'm1': clock},
            'origin': [0.0, 50.0, 75.0], 'ray_count': n, 'rays': rays}

def expect_refusal(name, fn, needle, log):
    try: fn()
    except ValueError as e:
        assert needle in str(e), f'{name}: refused for the wrong reason: {e}'; log.append(name); return
    raise AssertionError('admitted: ' + name)

def main():
    log = []; report = {}
    # 1. smooth continuous tent (sigma proportional to D) vs analytic: convergence guard passes, value matches closed form.
    tent = lambda s: 1 - abs(s - 50) / 30
    (T0, L0), (T1, L1) = reference_ray(tent, lambda s: 0.02 * tent(s), 0.5, 0.0, 100.0)
    tau = 0.6 + 2 * 7.5e-10  # area 30 * 0.02, plus the two sub-floor edge slivers (analytic)
    assert abs(T0 - math.exp(-tau)) < 1e-6 and abs(T0 - T1) < G.CONVERGENCE_CEILING, (T0, T1)
    report['smooth_tent'] = {'T_finest': T0, 'T_analytic': math.exp(-tau), 'finest_vs_half': abs(T0 - T1)}; log.append('smooth tent matches closed form and converges')
    # 2. density-zero boundary: D==0 everywhere (strict >0) -> T=1, L=0 even though X would be huge; X=0 with D>0 floors to 1e-6.
    (Tz, Lz), _ = reference_ray(lambda s: 0.0, lambda s: 1e3, 7.0, 0.0, 100.0); assert Tz == 1.0 and Lz == 0.0
    (Tf, _), _ = reference_ray(lambda s: 1.0, lambda s: 0.0, 0.0, 0.0, 100.0); assert abs(Tf - math.exp(-1e-6 * 100)) < 1e-12
    (Tt, _), _ = reference_ray(lambda s: -1e-30 if s < 50 else 0.0, lambda s: 1.0, 1.0, 0.0, 100.0); assert Tt == 1.0
    log.append('D==0 / D<0 contribute nothing; D>0 with X=0 floors to 1e-6'); report['density_zero'] = {'T_zero': Tz, 'T_floor': Tf}
    # 3. grazing: support narrower than the half grid, centred on an odd finest node -> half level misses it. Guard must REFUSE.
    N = 1 << LOG2N; h = 100.0 / N; s0 = f32(1001 * h); w = 0.4 * h
    graze = lambda s: w - abs(s - s0)
    (Tg, _), (Tgh, _) = reference_ray(graze, lambda s: 0.5, 0.5, 0.0, 100.0)
    assert abs(Tg - Tgh) > 10 * G.CONVERGENCE_CEILING, (Tg, Tgh)
    cap = make_capture(4); rays = cap['rays']; sha = 'a' * 64
    rows = [row_from(Tg, 0.0)[:4] + row_from(Tgh, 0.0)[4:] for _ in rays]
    ref = checker_ref(rays, [[Tg, 0.1, 0.1, 0.1, Tgh, 0.1, 0.1, 0.1]] * 4, sha)
    expect_refusal('grazing support: reference not converged', lambda: G.check_convergence(ref), 'failed convergence', log)
    report['grazing'] = {'T_finest': Tg, 'T_half': Tgh, 'delta': abs(Tg - Tgh)}
    # 4. gate logic on a synthetic full workload. Layout of candidate/current rows: [px,py,Lr,Lg,Lb,T].
    n = 400; cap = make_capture(n); rays = cap['rays']; sha = 'b' * 64; zone = cap['zone_sha256']
    refvals = [[0.5 + 0.4 * math.sin(i), 0.1 + 0.05 * math.cos(i)] for i in range(n)]  # (T, L)
    ref = checker_ref(rays, [row_from(T, L) for T, L in refvals], sha)
    cur_err = [1e-4 * (1 + (i * 37 % 100) / 100) for i in range(n)]       # Current: 1e-4..2e-4, max just under 2e-4
    cur_err[7] = 2e-4
    def rows(errs):
        return [[r[0], r[1], L + e, L + e, L + e, T + e] for r, (T, L), e in zip(rays, refvals, errs)]
    current = rows(cur_err); better = rows([e * 0.5 for e in cur_err])
    ident = dict(rays_sha256=sha, zone_sha256=zone, expected_count=n, expected_clock=7.25, expected_origin=[0.0, 50.0, 75.0])
    ok = G.evaluate_capture(cap, better, current, ref, **ident); assert ok['both_resolutions_pass'] and ok['finest']['rays_beyond_current_max'] == 0
    same = G.evaluate_capture(cap, current, current, ref, **ident); assert same['both_resolutions_pass']
    log.append('strictly better candidate and identical-Current pass at both resolutions')
    # 5. localized tail error: ONE ray 30x worse; mean and p99 still pass, max fails; a 1-in-20 sampled gate would miss it.
    tail_errs = [e * 0.5 for e in cur_err]; tail_errs[203] = 6e-3
    tail = rows(tail_errs); d = G.evaluate_capture(cap, tail, current, ref, **ident)
    g = d['finest']['gates']; assert g['RGB_pooled_channels.mean'] and g['RGB_pooled_channels.p99'] and not g['RGB_pooled_channels.max'] and not g['T.max']
    assert not d['both_resolutions_pass'] and d['finest']['beyond_current_max_indices'] == [203]
    sampled = [i for i in range(n) if i % 20 == 0]; assert 203 not in sampled   # what a stride-20 holdout would never see
    sub = {'schema': G.SCHEMA}  # sampled-subset gate through the SAME evaluator passes -> proves sampled max is not acceptance
    sref = dict(ref, rows=[ref['rows'][i] for i in sampled]); sd = G.shared_gate().evaluate([tail[i] for i in sampled], [current[i] for i in sampled], sref, sha)
    assert sd['both_resolutions_pass']; log.append('localized tail error rejected on full set; same error passes a stride-20 sample (sampled max NOT accepted)')
    report['tail'] = {'full_pass': d['both_resolutions_pass'], 'sampled_pass': sd['both_resolutions_pass'], 'failing_index': 203}
    # 6. missing / reordered / duplicate / stale / failed-convergence / nonfinite / truncated.
    E = lambda **kw: (lambda: G.evaluate_capture(cap if 'cap' not in kw else kw['cap'], kw.get('cand', better), kw.get('cur', current), kw.get('ref', ref), **{**ident, **kw.get('id', {})}))
    expect_refusal('missing candidate ray', E(cand=better[:-1]), 'coverage', log)
    expect_refusal('missing Current ray', E(cur=current[:-1]), 'coverage', log)
    sw = copy.deepcopy(better); sw[10], sw[11] = sw[11], sw[10]
    expect_refusal('reordered candidate rays', E(cand=sw), 'reordered', log)
    expect_refusal('reordered Current rays', E(cur=sw), 'reordered', log)
    expect_refusal('stale fixture identity', E(id={'zone_sha256': 'y' * 64}), 'stale fixture', log)
    expect_refusal('stale clock identity', E(id={'expected_clock': 7.0}), 'stale clock', log)
    expect_refusal('stale pose identity', E(id={'expected_origin': [0.0, 50.0, 76.0]}), 'stale pose', log)
    expect_refusal('stale ray-file identity', E(id={'rays_sha256': 'c' * 64}), 'identity mismatch', log)
    expect_refusal('count differs from frozen plan', E(id={'expected_count': n + 1}), 'frozen capture plan', log)
    c2 = copy.deepcopy(cap); c2['rays'] = c2['rays'][:-1]
    expect_refusal('truncated capture (declared count kept)', E(cap=c2), 'ray_count', log)
    c3 = copy.deepcopy(cap); c3['medium_clocks'] = {'m0': 7.25, 'm1': 7.5}
    expect_refusal('differing medium clocks', E(cap=c3), 'medium-specific clocks differ', log)
    c4 = copy.deepcopy(cap); c4['rays'][5][:2] = c4['rays'][4][:2]
    expect_refusal('duplicate pixel (not distinct)', E(cap=c4), 'duplicate pixel', log)
    c6 = copy.deepcopy(cap); c6['rays'][2] = c6['rays'][2][:7]
    expect_refusal('ray row narrower than probe width', E(cap=c6), 'width', log)
    c5 = copy.deepcopy(cap); c5['rays'][3][2] = float('nan')
    expect_refusal('nonfinite ray', E(cap=c5), 'nonfinite', log)
    nan = copy.deepcopy(better); nan[9][5] = float('inf')
    expect_refusal('nonfinite candidate value', E(cand=nan), 'nonfinite', log)
    bad = copy.deepcopy(ref); bad['rows'][50][6] += 1e-3
    expect_refusal('failed reference convergence', E(ref=bad), 'failed convergence', log)
    try: G.check_convergence(ref, ceiling=1e-3)
    except ValueError as e: assert 'never widened' in str(e); log.append('convergence ceiling widening refused')
    else: raise AssertionError('ceiling widening admitted')
    report.update(controls_passed=log, count=len(log))
    print(json.dumps(report, indent=2)); print('ORACLE CONTROLS PASS:', len(log))

if __name__ == '__main__': main()
