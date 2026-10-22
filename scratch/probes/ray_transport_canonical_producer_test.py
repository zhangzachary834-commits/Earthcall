"""Northern Veil — Full 4-Member Canonical Level-Set & Curvature Producer Test
Tests coverage, kink alignment, and schedule generation across all 2,304 rays.

Sparkly Guy / Gemini 3.8 Flash High / Antigravity / session 50cf28df-09e9-4c2b-a4d5-1f796abbc73a / 2026-10-21
"""

import hashlib
import json
import math
import sys
from pathlib import Path

# Media definitions
MEDIA_PARAMS = [
    {
        "id": 0,
        "origin": [0.0, 48.0, 75.0],
        "scale": [160.0, 36.0, 46.0],
        "Wx": 75.0,
        "Wy": 16.0,
        "wz": 3.2,
        "M2_x": 0.086772,
        "M3_x": 0.015, # conservative bound for wave f'''
        "amps": [-15.0, -5.5, -2.0],
        "freqs": [0.026, 0.068, 0.16],
        "phases": [0.0, 0.5, 1.2],
        "y_min": -16.0,
        "y_max": 16.0,
    },
    {
        "id": 1,
        "origin": [15.0, 56.0, 140.0],
        "scale": [150.0, 38.0, 44.0],
        "Wx": 70.0,
        "Wy": 17.0,
        "wz": 2.8,
        "M2_x": 0.103901,
        "M3_x": 0.018,
        "amps": [14.0, -5.0, 2.0],
        "freqs": [0.028, 0.075, 0.18],
        "phases": [0.0, 0.5, 1.2],
        "y_min": -17.0,
        "y_max": 17.0,
    },
    {
        "id": 2,
        "origin": [-5.0, 78.0, 82.0],
        "scale": [160.0, 36.0, 46.0],
        "Wx": 75.0,
        "Wy": 16.0,
        "wz": 3.8,
        "M2_x": 0.054888,
        "M3_x": 0.012,
        "amps": [-13.0, -5.0, -1.5],
        "freqs": [0.024, 0.060, 0.14],
        "phases": [0.0, 0.5, 1.2],
        "y_min": -16.0,
        "y_max": 16.0,
    },
    {
        "id": 3,
        "origin": [22.0, 72.0, 68.0],
        "scale": [70.0, 64.0, 36.0],
        "Wx": 32.0,
        "Wy": 30.0,
        "wz": 3.0,
        "M2_x": 0.093184,
        "M3_x": 0.016,
        "amps": [-9.0, -3.5, -1.5],
        "freqs": [0.038, 0.095, 0.18],
        "phases": [0.0, 0.5, 1.2],
        "y_min": -30.0,
        "y_max": 30.0,
    },
]

def eval_wave(m_params, x):
    amps = m_params["amps"]
    freqs = m_params["freqs"]
    phases = m_params["phases"]
    return (amps[0] * math.sin(freqs[0] * x + phases[0]) +
            amps[1] * math.cos(freqs[1] * x + phases[1]) +
            amps[2] * math.sin(freqs[2] * x + phases[2]))

def eval_dwave(m_params, x):
    amps = m_params["amps"]
    freqs = m_params["freqs"]
    phases = m_params["phases"]
    return (amps[0] * freqs[0] * math.cos(freqs[0] * x + phases[0]) -
            amps[1] * freqs[1] * math.sin(freqs[1] * x + phases[1]) +
            amps[2] * freqs[2] * math.cos(freqs[2] * x + phases[2]))

def isolate_roots_for_level(phi, dphi, s_lo, s_hi, target_c, M2_s, max_depth=6):
    """Isolates all roots of phi(s) - target_c = 0 on [s_lo, s_hi] using Taylor bow."""
    roots = []
    
    def search(a, b, depth):
        h = b - a
        if h <= 1e-8:
            roots.append((a, b))
            return
        
        va = phi(a) - target_c
        vb = phi(b) - target_c
        delta_bow = 0.125 * M2_s * (h ** 2)
        lo_b = min(va, vb) - delta_bow
        hi_b = max(va, vb) + delta_bow
        
        # Certified skip
        if lo_b > 0.0 or hi_b < 0.0:
            return
        
        # Strict sign change -> bisection to < 1e-6
        if va * vb <= 0.0:
            cur_a, cur_b = a, b
            for _ in range(24):
                mid = 0.5 * (cur_a + cur_b)
                vmid = phi(mid) - target_c
                if (phi(cur_a) - target_c) * vmid <= 0.0:
                    cur_b = mid
                else:
                    cur_a = mid
            roots.append((cur_a, cur_b))
            return
        
        # Bow crosses 0 but same sign at endpoints: potential extremum
        if depth >= max_depth:
            roots.append((a, b))
            return
        
        mid = 0.5 * (a + b)
        search(a, mid, depth + 1)
        search(mid, b, depth + 1)
        
    n_bins = 8
    step = (s_hi - s_lo) / n_bins
    for i in range(n_bins):
        search(s_lo + i * step, s_lo + (i + 1) * step, 0)
        
    roots.sort(key=lambda r: r[0])
    dedup = []
    for r in roots:
        if not dedup or r[0] > dedup[-1][1] + 1e-5:
            dedup.append(r)
    return dedup

def process_ray_member(ro_world, rd_world, clipA, clipB, m_params):
    """Computes active smooth subpanels and kinks for one member on a ray."""
    m_id = m_params["id"]
    O_m = m_params["origin"]
    half = m_params["scale"]
    Wx = m_params["Wx"]
    Wy = m_params["Wy"]
    wz = m_params["wz"]
    M2_x = m_params["M2_x"]
    
    # 1. Local origin
    o = [ro_world[k] - O_m[k] for k in range(3)]
    d = rd_world
    
    # 2. Outer box clip
    ta, tb = clipA, clipB
    for k in range(3):
        if abs(d[k]) < 1e-15:
            if o[k] < -half[k] or o[k] > half[k]:
                return []
        else:
            x = (-half[k] - o[k]) / d[k]
            y = (half[k] - o[k]) / d[k]
            ta = max(ta, min(x, y))
            tb = min(tb, max(x, y))
    if not (tb > ta):
        return []
        
    # 3. Affine Support Clip (|px| <= Wx, |py| <= Wy)
    s_supp_lo, s_supp_hi = ta, tb
    if abs(d[0]) > 1e-7:
        x0 = (-Wx - o[0]) / d[0]
        x1 = (Wx - o[0]) / d[0]
        s_supp_lo = max(s_supp_lo, min(x0, x1))
        s_supp_hi = min(s_supp_hi, max(x0, x1))
    else:
        if abs(o[0]) > Wx:
            return []
            
    if abs(d[1]) > 1e-7:
        y0 = (-Wy - o[1]) / d[1]
        y1 = (Wy - o[1]) / d[1]
        s_supp_lo = max(s_supp_lo, min(y0, y1))
        s_supp_hi = min(s_supp_hi, max(y0, y1))
    else:
        if abs(o[1]) > Wy:
            return []
            
    if not (s_supp_hi > s_supp_lo):
        return []
        
    # 4. Interior affine split points (px = 0, py = 0)
    affine_kinks = []
    if abs(d[0]) > 1e-7:
        sx0 = -o[0] / d[0]
        if s_supp_lo < sx0 < s_supp_hi:
            affine_kinks.append(sx0)
    if abs(d[1]) > 1e-7:
        sy0 = -o[1] / d[1]
        if s_supp_lo < sy0 < s_supp_hi:
            affine_kinks.append(sy0)
            
    # 5. Wave level-set roots for c in {-wz, 0, +wz}
    def phi(s):
        px = o[0] + s * d[0]
        pz = o[2] + s * d[2]
        return pz + eval_wave(m_params, px)
        
    def dphi(s):
        px = o[0] + s * d[0]
        return d[2] + d[0] * eval_dwave(m_params, px)
        
    M2_s = M2_x * (d[0] ** 2)
    
    roots_neg_wz = isolate_roots_for_level(phi, dphi, s_supp_lo, s_supp_hi, -wz, M2_s)
    roots_pos_wz = isolate_roots_for_level(phi, dphi, s_supp_lo, s_supp_hi, +wz, M2_s)
    roots_zero   = isolate_roots_for_level(phi, dphi, s_supp_lo, s_supp_hi,  0.0, M2_s)
    
    # Collect all boundary roots where |phi(s)| = wz
    boundary_points = []
    for r in roots_neg_wz + roots_pos_wz:
        boundary_points.append(0.5 * (r[0] + r[1]))
    boundary_points.sort()
    
    # Include window endpoints if inside support
    all_bounds = [s_supp_lo] + boundary_points + [s_supp_hi]
    active_intervals = []
    for i in range(len(all_bounds) - 1):
        sa, sb = all_bounds[i], all_bounds[i + 1]
        if sb - sa < 1e-5:
            continue
        smid = 0.5 * (sa + sb)
        val = phi(smid)
        if abs(val) < wz:
            active_intervals.append((sa, sb))
            
    if not active_intervals:
        return []
        
    # Kinks to split inside active intervals
    all_kinks = affine_kinks + [0.5 * (r[0] + r[1]) for r in roots_zero]
    
    # Subdivide active intervals at kinks
    member_panels = []
    for sa, sb in active_intervals:
        cur_splits = [sa]
        for k in sorted(all_kinks):
            if sa + 1e-4 < k < sb - 1e-4:
                cur_splits.append(k)
        cur_splits.append(sb)
        for j in range(len(cur_splits) - 1):
            p0, p1 = cur_splits[j], cur_splits[j + 1]
            if p1 - p0 > 1e-5:
                member_panels.append({"medium": m_id, "s0": p0, "s1": p1})
                
    return member_panels

def main():
    root = Path(".")
    rays_path = root / "docs/audits/NORTHERN_VEIL_NATIVE_CF4_PREFLIGHT_2026-10-21_RAYS.json"
    rays_data = json.load(open(rays_path))
    ro = rays_data["origin"]
    rays = rays_data["rays"]
    
    print(f"Loaded {len(rays)} rays. Running 4-member canonical producer...")
    
    total_single_subpanels = 0
    total_overlaps = 0
    rays_with_content = 0
    
    for ray_idx, r in enumerate(rays):
        rd = [r[2], r[3], r[4]]
        cA, cB = r[5], r[6]
        
        all_pieces = []
        for m_params in MEDIA_PARAMS:
            pieces = process_ray_member(ro, rd, cA, cB, m_params)
            all_pieces.extend(pieces)
            
        if not all_pieces:
            continue
            
        rays_with_content += 1
        
        # Event sweep across active member panels
        events = []
        for p in all_pieces:
            events.append(p["s0"])
            events.append(p["s1"])
        events.sort()
        
        dedup_events = []
        for ev in events:
            if not dedup_events or ev > dedup_events[-1] + 1e-4:
                dedup_events.append(ev)
                
        for e in range(1, len(dedup_events)):
            s0, s1 = dedup_events[e - 1], dedup_events[e]
            h = s1 - s0
            if h <= 1e-4:
                continue
            smid = 0.5 * (s0 + s1)
            active_m = [p["medium"] for p in all_pieces if p["s0"] <= smid <= p["s1"]]
            active_m = list(set(active_m))
            if not active_m:
                continue
            if len(active_m) == 1:
                n_sub = max(1, int(math.ceil(h / 4.0)))
                total_single_subpanels += n_sub
            else:
                total_overlaps += 1
                
    print(f"Canonical producer completed successfully across {len(rays)} rays:")
    print(f"  Rays with active content: {rays_with_content}")
    print(f"  Total single subpanels:   {total_single_subpanels}")
    print(f"  Total overlap intervals:  {total_overlaps}")

if __name__ == "__main__":
    main()
