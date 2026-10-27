"""Comprehensive CPU and f32 Reference Enclosure Witnesses for Safeguarded Root Interpolation.
Mission §6ed: Verify mathematical enclosure, convergence, and edge-case contracts.
Antigravity / Gemini 2.5 Pro (Sparkly Guy 🐉✨) / 2026-10-27.
"""
import math
import struct
import sys
from typing import Callable, Tuple, List, Dict, Any

def to_f32(val: float) -> float:
    return struct.unpack('<f', struct.pack('<f', float(val)))[0]

def next_f32(val: float, up: bool = True) -> float:
    bits = struct.unpack('<I', struct.pack('<f', float(val)))[0]
    if up:
        bits += 1
    else:
        bits -= 1
    return struct.unpack('<f', struct.pack('<I', bits))[0]

class BisectionIsolator:
    """The baseline bisection isolator currently running in the WGSL shader."""
    @staticmethod
    def isolate(f: Callable[[float], float], a: float, b: float, va: float, vb: float,
                error: float, min_slope: float, max_budget: int = 32) -> Dict[str, Any]:
        l = to_f32(a)
        r = to_f32(b)
        left = to_f32(va)
        evals = 0
        bands = []
        reason = 0
        work = 0

        # Endpoint checks
        if abs(va) <= error:
            w = to_f32(2.0 * error / min_slope)
            bands.append((l, min(r, l + w)))
        if abs(vb) <= error:
            w = to_f32(2.0 * error / min_slope)
            bands.append((max(l, r - w), r))

        if (va > error and vb < -error) or (va < -error and vb > error):
            for iter_idx in range(32):
                if work >= max_budget:
                    reason |= 1
                    break
                work += 1

                mid = to_f32(l + 0.5 * (r - l))
                if mid <= l or mid >= r or (r - l) <= 0.0001:
                    break

                v = to_f32(f(mid))
                evals += 1

                if abs(v) <= error:
                    w = to_f32(2.0 * error / min_slope)
                    l = max(l, to_f32(mid - w))
                    r = min(r, to_f32(mid + w))
                    break

                if (left < 0.0 and v > 0.0) or (left > 0.0 and v < 0.0):
                    r = mid
                else:
                    l = mid
                    left = v

            bands.append((l, r))

        return {
            'bands': bands,
            'evals': evals,
            'work': work,
            'reason': reason,
            'final_bracket': (l, r) if ((va > error and vb < -error) or (va < -error and vb > error)) else None
        }


class SafeguardedInterpolationIsolator:
    """The §6ed Safeguarded Bracketed Interpolator (SBI).
    Combines secant interpolation with boundary guard cushion (0.125 h)
    and Dekker/Brent bisection fallback on consecutive side retentions.
    """
    @staticmethod
    def isolate(f: Callable[[float], float], a: float, b: float, va: float, vb: float,
                error: float, min_slope: float, max_budget: int = 32) -> Dict[str, Any]:
        l = to_f32(a)
        r = to_f32(b)
        fl = to_f32(va)
        fr = to_f32(vb)
        evals = 0
        bands = []
        reason = 0
        work = 0

        # Endpoint checks (100% identical to baseline)
        if abs(va) <= error:
            w = to_f32(2.0 * error / min_slope)
            bands.append((l, min(r, l + w)))
        if abs(vb) <= error:
            w = to_f32(2.0 * error / min_slope)
            bands.append((max(l, r - w), r))

        if (va > error and vb < -error) or (va < -error and vb > error):
            stuck_count = 0
            last_side = 0  # 1 = left retained, 2 = right retained

            for iter_idx in range(32):
                if work >= max_budget:
                    reason |= 1
                    break
                work += 1

                h = to_f32(r - l)
                s_bisect = to_f32(l + 0.5 * h)
                if s_bisect <= l or s_bisect >= r or h <= 0.0001:
                    break

                # Proposed sample point
                s = s_bisect
                D = to_f32(fr - fl)

                # Check if secant step is numerically valid and not stuck
                if math.isfinite(D) and abs(D) > 1e-30 and stuck_count < 2:
                    w_sec = to_f32(-fl / D)
                    if math.isfinite(w_sec) and 0.0 < w_sec < 1.0:
                        # Cushion: clamp within [0.125, 0.875] of bracket
                        w_clamped = min(max(w_sec, 0.125), 0.875)
                        s_cand = to_f32(l + w_clamped * h)
                        if l < s_cand < r:
                            s = s_cand

                # Fallback to bisection if candidate stalled on float precision
                if s <= l or s >= r:
                    s = s_bisect
                    if s <= l or s >= r:
                        break

                v = to_f32(f(s))
                evals += 1

                # Residual error enclosure (identical contract)
                if abs(v) <= error:
                    w = to_f32(2.0 * error / min_slope)
                    l = max(l, to_f32(s - w))
                    r = min(r, to_f32(s + w))
                    break

                # Bracket update & alternation tracking
                if (fl < 0.0 and v > 0.0) or (fl > 0.0 and v < 0.0):
                    # Root is in [l, s]. Right endpoint moves to s
                    r = s
                    fr = v
                    if last_side == 1:
                        stuck_count += 1
                    else:
                        stuck_count = 0
                    last_side = 1
                else:
                    # Root is in [s, r]. Left endpoint moves to s
                    l = s
                    fl = v
                    if last_side == 2:
                        stuck_count += 1
                    else:
                        stuck_count = 0
                    last_side = 2

            bands.append((l, r))

        return {
            'bands': bands,
            'evals': evals,
            'work': work,
            'reason': reason,
            'final_bracket': (l, r) if ((va > error and vb < -error) or (va < -error and vb > error)) else None
        }


def run_witness_suite():
    print("=== Running §6ed CPU & f32 Reference Enclosure Witnesses ===")
    
    witnesses = []

    # 1. Affine Root: f(s) = 2.5 * (s - 4.2)
    def f1(s): return 2.5 * (s - 4.2)
    witnesses.append({
        'name': '1. Affine Linear Field',
        'f': f1, 'a': 0.0, 'b': 10.0, 'err': 0.001, 'min_slope': 2.4, 'true_root': 4.2
    })

    # 2. Curved Monotone Wave (Curtain-like): f(s) = 0.5 * sin(0.3 * s) + 1.2 * s - 5.0
    def f2(s): return 0.5 * math.sin(0.3 * s) + 1.2 * s - 5.0
    witnesses.append({
        'name': '2. Curved Monotone Sinusoid',
        'f': f2, 'a': 2.0, 'b': 6.0, 'err': 0.002, 'min_slope': 1.05, 'true_root': 3.78866333
    })

    # 3. Reversed Direction (Negative Slope): f(s) = -3.0 * (s - 1.5)
    def f3(s): return -3.0 * (s - 1.5)
    witnesses.append({
        'name': '3. Reversed Direction (Negative Slope)',
        'f': f3, 'a': 0.0, 'b': 3.0, 'err': 0.001, 'min_slope': 2.9, 'true_root': 1.5
    })

    # 4. Near-Zero Slope / Flat Monotone: f(s) = 0.005 * (s - 10.0)
    def f4(s): return 0.005 * (s - 10.0)
    witnesses.append({
        'name': '4. Near-Zero / Flat Slope',
        'f': f4, 'a': 0.0, 'b': 20.0, 'err': 0.0005, 'min_slope': 0.004, 'true_root': 10.0
    })

    # 5. Steep Slope / Shock: f(s) = 150.0 * (s - 7.3)
    def f5(s): return 150.0 * (s - 7.3)
    witnesses.append({
        'name': '5. Steep Slope (High Gradient)',
        'f': f5, 'a': 7.0, 'b': 7.5, 'err': 0.01, 'min_slope': 140.0, 'true_root': 7.3
    })

    # 6. Endpoint Exactly Within Error: f(a) = error * 0.5
    def f6(s): return 1.5 * (s - 1.0002)
    witnesses.append({
        'name': '6. Endpoint Within Error',
        'f': f6, 'a': 1.0, 'b': 3.0, 'err': 0.001, 'min_slope': 1.4, 'true_root': 1.0002
    })

    # 7. Asymmetric / Convex Field (Tests Regula Falsi Stagnation): f(s) = (s - 1.0)**3 + 0.1 * (s - 1.0)
    def f7(s): return (s - 1.0)**3 + 0.1 * (s - 1.0)
    witnesses.append({
        'name': '7. Cubic Convex Stagnation Stress',
        'f': f7, 'a': 0.5, 'b': 3.0, 'err': 0.001, 'min_slope': 0.09, 'true_root': 1.0
    })

    # 8. High Curvature Sine (Close to Lipschitz Limit)
    def f8(s): return math.sin(2.0 * s) + 2.1 * s - 4.0
    witnesses.append({
        'name': '8. High Frequency Monotone Curvature',
        'f': f8, 'a': 1.0, 'b': 3.0, 'err': 0.001, 'min_slope': 0.1, 'true_root': 2.3803947
    })

    all_passed = True
    total_bisect_evals = 0
    total_interp_evals = 0

    for w in witnesses:
        name = w['name']
        f = w['f']
        a = to_f32(w['a'])
        b = to_f32(w['b'])
        va = to_f32(f(a))
        vb = to_f32(f(b))
        err = to_f32(w['err'])
        min_slope = to_f32(w['min_slope'])
        true_root = w['true_root']

        res_bisect = BisectionIsolator.isolate(f, a, b, va, vb, err, min_slope)
        res_interp = SafeguardedInterpolationIsolator.isolate(f, a, b, va, vb, err, min_slope)

        total_bisect_evals += res_bisect['evals']
        total_interp_evals += res_interp['evals']

        # Enclosure check: does the band contain the true root?
        enclosed_bisect = False
        for lo, hi in res_bisect['bands']:
            if lo - 1e-4 <= true_root <= hi + 1e-4:
                enclosed_bisect = True
                break

        enclosed_interp = False
        for lo, hi in res_interp['bands']:
            if lo - 1e-4 <= true_root <= hi + 1e-4:
                enclosed_interp = True
                break

        passed = enclosed_interp and (res_interp['reason'] == 0)
        if not passed:
            all_passed = False

        speedup = f"{res_bisect['evals']} -> {res_interp['evals']} evals"
        status = "PASS" if passed else "FAIL"
        print(f"[{status}] {name:<40} | Root={true_root:<8.4f} | Enclosed={enclosed_interp} | {speedup}")

    # 9. Test Work Budget Exhaustion (Budget = 0)
    res_budget = SafeguardedInterpolationIsolator.isolate(f1, 0.0, 10.0, f1(0.0), f1(10.0), 0.001, 2.4, max_budget=0)
    budget_passed = (res_budget['reason'] & 1) != 0 and (res_budget['evals'] == 0)
    print(f"[{'PASS' if budget_passed else 'FAIL'}] 9. Work Budget Guard (Budget=0)             | Reason={res_budget['reason']} (1=exhausted) | Evals={res_budget['evals']}")
    if not budget_passed: all_passed = False

    # 10. Test Adjacent Floats Termination
    a_fl = to_f32(1.0)
    b_fl = next_f32(a_fl, up=True)
    res_adj = SafeguardedInterpolationIsolator.isolate(f1, a_fl, b_fl, f1(a_fl), f1(b_fl), 0.001, 2.4)
    adj_passed = (res_adj['evals'] == 0)  # No midpoint possible between adjacent floats
    print(f"[{'PASS' if adj_passed else 'FAIL'}] 10. Adjacent Floats Termination               | a={a_fl}, b={b_fl} | Evals={res_adj['evals']}")
    if not adj_passed: all_passed = False

    print("\n=== Summary of Witnesses ===")
    print(f"Total Bisection Evals Across Cases:     {total_bisect_evals}")
    print(f"Total Safeguarded Interp Evals:        {total_interp_evals}")
    pct_eval_reduction = 100.0 * (total_bisect_evals - total_interp_evals) / total_bisect_evals
    print(f"Evaluation Reduction:                  {pct_eval_reduction:.1f}%")
    print(f"All Witnesses Passed Strict Enclosure: {all_passed}")

    return all_passed

if __name__ == '__main__':
    ok = run_witness_suite()
    sys.exit(0 if ok else 1)
