"""§6db: Depth-0 Register Fast-Path + Universal Cross-Level Bow Pre-Filter + Compact Private State.
Aetherion / The Firmament — continuation after §6cx / §6cy Work Census and §6da Review + Parallel-Roots.

Targets the exact primary bottleneck identified by the §6cx/§6cy Producer Work Census:
1. Universal Cross-Level Depth-0 Bow Pre-Filter (`bowActiveBins` 8-bit mask):
   - Out of 2,490,864 coarse bin visits (103,786 box hits * 8 bins * 3 levels), 2,332,420 (93.64%) are
     bow-rejected at depth == 0.
   - Using the universal cross-level scale upper bound across c in {-w_z, 0, +w_z}:
       scaleMax = 1.0 + abs(phi_a0) + abs(phi_b0) + 2.0 * m.limits.w + abs(s0) + abs(s1)
       bowMax   = 0.125 * M * h0 * h0 + (m.curv.y + 0.000001 * scaleMax)
     any coarse bin with `min(phi_a0, phi_b0) > m.limits.w + bowMax` or
     `max(phi_a0, phi_b0) < -m.limits.w - bowMax` is provably bow-rejected on ALL THREE levels
     (-w_z, 0.0, +w_z) simultaneously, with zero magic constants!
2. Whole-Member Bow Skip (`bowActiveBins == 0u`):
   - When `bowActiveBins == 0u`, all 8 coarse bins are provably free of roots on all 3 levels, so `phi(s)`
     never crosses `{-w_z, 0, +w_z}` anywhere on `[a, b]`.
   - If `abs(phi_knots[0]) > m.limits.w + m.curv.y`, the entire member is provably outside the slab on
     `[a, b]`, skipping all 3 `roots()` calls, all affine knot divisions/sorting, and all `evalPhiPieceMid`
     calls with 0 extra `evalPhi` evaluations!
3. Depth-0 Register Fast-Path Inside `roots()`:
   - Because 98.1% (2,444,326 / 2,490,864) of coarse bins are resolved at depth == 0 without ever
     subdividing (p50 = 0 subdivision splits/ray), `roots()` evaluates depth == 0 bow rejection and
     depth == 0 `minSlope > 0.0` monotone root isolation directly in scalar registers before touching
     `stack` or `depthBits`. Only the 1.87% of coarse bins that actually subdivide at depth 0 push their
     two depth-1 children (`stack[0]`, `stack[1]`).
4. Dynamic-Slack Linear-z Cull (`zSlack = m.limits.w + m.curv.y + (abs(m.amps.x) + abs(m.amps.y) + abs(m.amps.z))`):
   - Derived purely from the member's runtime amplitude parameters with zero magic constants.
5. Compact Private State (`Mode 3`):
   - Shrinks `stack` (`20 -> 8`, census max `6`), `bands` (`32 -> 16`, census max `7`),
     `knots` in `roots` (`96 -> 48`, census max `17`), `pieces` (`48 -> 24`, census max `12`),
     and `mergedKnots` (`96 -> 48`, census max `24`), cutting per-thread private array state from
     2,240 bytes to 960 bytes (-57.1%) while retaining explicit fallback flags if any bound is exceeded.
"""
import argparse, ast, hashlib, importlib.util, json, os, shlex, subprocess
from pathlib import Path


def load(root, name):
    spec = importlib.util.spec_from_file_location(name, root / 'scratch/probes' / f'{name}.py')
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m


def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def make_fastpath_wgsl(compact: bool) -> str:
    stack_cap = 8 if compact else 20
    band_cap = 16 if compact else 32
    knot_cap = 48 if compact else 96
    knot_guard = 44 if compact else 92
    piece_cap = 24 if compact else 48
    merge_knot_cap = 48 if compact else 96
    return f'''
struct CurtainSpec {{
    origin: vec4<f32>,
    half: vec4<f32>,
    limits: vec4<f32>,
    curv: vec4<f32>,
    amps: vec4<f32>,
    freqs: vec4<f32>,
    phases: vec4<f32>,
}};

struct Ray {{
    ro_clipA: vec4<f32>,
    rd_clipB: vec4<f32>,
    counts: vec4<u32>,
}};

struct Subpanel {{
    span: vec2<f32>,
    info: vec2<u32>,
}};

struct Overlap {{
    span: vec2<f32>,
    info: vec2<u32>,
}};

struct Config {{
    rayCount: u32,
    maxSubPerRay: u32,
    maxOverPerRay: u32,
    budget: u32,
}};

const FALLBACK: u32 = 0x80000000u;

@group(0) @binding(0) var<uniform> cfg: Config;
@group(0) @binding(1) var<storage, read> curtains: array<CurtainSpec, 4>;
@group(0) @binding(2) var<storage, read_write> rays: array<Ray>;
@group(0) @binding(3) var<storage, read_write> subpanels: array<Subpanel>;
@group(0) @binding(4) var<storage, read_write> overlaps: array<Overlap>;

fn evalWave(m: CurtainSpec, x: f32) -> f32 {{
    return m.amps.x * sin(m.freqs.x * x + m.phases.x)
         + m.amps.y * cos(m.freqs.y * x + m.phases.y)
         + m.amps.z * sin(m.freqs.z * x + m.phases.z);
}}

fn evalPhi(m: CurtainSpec, o: vec3f, d: vec3f, s: f32) -> f32 {{
    return o.z + s * d.z + evalWave(m, o.x + s * d.x);
}}

fn putBand(lo: f32, hi: f32, bounds: ptr<function, array<f32, {knot_cap}>>, nb: ptr<function, u32>,
           bands: ptr<function, array<vec2f, {band_cap}>>, nr: ptr<function, u32>, reason: ptr<function, u32>) {{
    if (*nr > 0u) {{
        let last = (*bands)[*nr - 1u];
        if (lo <= last.y && hi >= last.x) {{
            let merged = vec2f(min(lo, last.x), max(hi, last.y));
            (*bands)[*nr - 1u] = merged;
            (*bounds)[*nb - 2u] = merged.x;
            (*bounds)[*nb - 1u] = merged.y;
            return;
        }}
    }}
    if (*nb + 2u > {knot_guard}u || *nr >= {band_cap}u) {{ *reason |= 2u; return; }}
    (*bounds)[*nb] = lo; (*bounds)[*nb + 1u] = hi; *nb += 2u;
    (*bands)[*nr] = vec2f(lo, hi); *nr += 1u;
}}

fn mayAbsorb(previous: vec2f, current: vec2f, previousShortBand: bool,
             previousPositive: bool, currentUncertain: bool, protectedBoundary: bool) -> bool {{
    if (protectedBoundary || previous.y != current.x) {{ return false; }}
    let currentShortBand = currentUncertain && current.y - current.x <= 0.02;
    return (previousShortBand && !currentUncertain) || (previousPositive && currentShortBand);
}}

fn putBandTagged(lo: f32, hi: f32, bounds: ptr<function, array<f32, {knot_cap}>>,
                 nb: ptr<function, u32>, bands: ptr<function, array<vec2f, {band_cap}>>,
                 nr: ptr<function, u32>, reason: ptr<function, u32>,
                 zeroBands: ptr<function, u32>, isZeroLevel: bool) {{
    putBand(lo, hi, bounds, nb, bands, nr, reason);
    if (*reason == 0u && isZeroLevel && *nr > 0u) {{ *zeroBands |= 1u << (*nr - 1u); }}
}}

fn getDepth(bits: vec3u, idx: u32) -> u32 {{
    return (bits[idx >> 3u] >> ((idx & 7u) << 2u)) & 15u;
}}

fn setDepth(bits: ptr<function, vec3u>, idx: u32, d: u32) {{
    let w = idx >> 3u; let s = (idx & 7u) << 2u;
    (*bits)[w] = ((*bits)[w] & ~(15u << s)) | ((d & 15u) << s);
}}

fn isolateMonotoneRoot(m: CurtainSpec, o: vec3f, d: vec3f, a: f32, b: f32, c: f32,
                       va: f32, vb: f32, error: f32, minSlope: f32,
                       bounds: ptr<function, array<f32, {knot_cap}>>, nb: ptr<function, u32>,
                       bands: ptr<function, array<vec2f, {band_cap}>>, nr: ptr<function, u32>,
                       reason: ptr<function, u32>, work: ptr<function, u32>,
                       zeroBands: ptr<function, u32>, isZeroLevel: bool) {{
    if (abs(va) <= error) {{
        let w = 2.0 * error / minSlope;
        putBandTagged(a, min(b, a + w), bounds, nb, bands, nr, reason, zeroBands, isZeroLevel);
    }}
    if (abs(vb) <= error) {{
        let w = 2.0 * error / minSlope;
        putBandTagged(max(a, b - w), b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel);
    }}
    if ((va > error && vb < -error) || (va < -error && vb > error)) {{
        var l = a; var r = b; var left = va;
        for (var iter = 0u; iter < 32u; iter++) {{
            if (*work >= cfg.budget) {{ *reason |= 1u; break; }} *work += 1u;
            let mid = l + 0.5 * (r - l);
            if (mid <= l || mid >= r || r - l <= 0.0001) {{ break; }}
            let v = evalPhi(m, o, d, mid) - c;
            if (abs(v) <= error) {{
                let w = 2.0 * error / minSlope;
                l = max(l, mid - w); r = min(r, mid + w);
                break;
            }}
            if ((left < 0.0 && v > 0.0) || (left > 0.0 && v < 0.0)) {{ r = mid; }} else {{ l = mid; left = v; }}
        }}
        putBandTagged(l, r, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel);
    }}
}}

fn rootsFast(m: CurtainSpec, o: vec3f, d: vec3f, lo: f32, hi: f32, c: f32,
             bounds: ptr<function, array<f32, {knot_cap}>>, nb: ptr<function, u32>,
             bands: ptr<function, array<vec2f, {band_cap}>>, nr: ptr<function, u32>,
             reason: ptr<function, u32>, work: ptr<function, u32>,
             phi_knots: ptr<function, array<f32, 9>>, zeroBands: ptr<function, u32>,
             isZeroLevel: bool, bowActiveBins: u32) {{
    let M = m.curv.x * d.x * d.x * 1.00002;
    let err = m.curv.y;
    var stack: array<vec4f, {stack_cap}>;
    var depthBits = vec3u(0u);
    for (var bin = 0u; bin < 8u; bin++) {{
        if (*reason != 0u) {{ break; }}
        var a0 = lo + (hi - lo) * f32(bin) / 8.0;
        var b0 = lo + (hi - lo) * f32(bin + 1u) / 8.0;
        if (bin == 0u) {{ a0 = lo; }}
        if (bin == 7u) {{ b0 = hi; }}
        let h0 = b0 - a0;
        // Charge the depth-0 inspection work unit identically to baseline
        if (*work >= cfg.budget) {{ *reason |= 1u; break; }}
        *work += 1u;
        if (!(h0 > 0.0)) {{ *reason |= 4u; break; }}
        // Fast universal cross-level bow pre-filter: if bit is 0, provably bow-rejected on all 3 levels
        if ((bowActiveBins & (1u << bin)) == 0u) {{ continue; }}

        let phi_a0 = (*phi_knots)[bin];
        let phi_b0 = (*phi_knots)[bin + 1u];
        let va0 = phi_a0 - c;
        let vb0 = phi_b0 - c;
        let scale0 = 1.0 + abs(va0) + abs(vb0) + abs(a0) + abs(b0);
        let error0 = err + 0.000001 * scale0;
        let bow0 = 0.125 * M * h0 * h0 + error0;
        if (min(va0, vb0) > bow0 || max(va0, vb0) < -bow0) {{ continue; }}

        // Depth-0 register fast-path: resolve monotone root isolation without touching stack[]
        let slope0 = (vb0 - va0) / h0;
        let radius0 = 0.5 * M * h0 + 2.0 * error0 / h0;
        let minSlope0 = abs(slope0) - radius0;
        if (minSlope0 > 0.0) {{
            isolateMonotoneRoot(m, o, d, a0, b0, c, va0, vb0, error0, minSlope0,
                                bounds, nb, bands, nr, reason, work, zeroBands, isZeroLevel);
            continue;
        }}
        if (0.125 * M * h0 * h0 <= error0) {{
            putBandTagged(a0, b0, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel);
            continue;
        }}
        let mid0 = a0 + 0.5 * h0;
        if (mid0 <= a0 || mid0 >= b0) {{
            putBandTagged(a0, b0, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel);
            continue;
        }}
        // Only the 1.87% of coarse bins that subdivide at depth 0 ever touch stack[]
        let phi_mid0 = evalPhi(m, o, d, mid0);
        stack[0] = vec4f(mid0, b0, phi_mid0, phi_b0); setDepth(&depthBits, 0u, 1u);
        stack[1] = vec4f(a0, mid0, phi_a0, phi_mid0); setDepth(&depthBits, 1u, 1u);
        var top = 2u;
        loop {{
            if (top == 0u || *reason != 0u) {{ break; }}
            top--;
            let cell = stack[top];
            let a = cell.x; let b = cell.y; let h = b - a;
            let depth = getDepth(depthBits, top);
            if (*work >= cfg.budget) {{ *reason |= 1u; break; }} *work += 1u;
            if (!(h > 0.0)) {{ *reason |= 4u; break; }}
            let va = cell.z - c; let vb = cell.w - c;
            let scale = 1.0 + abs(va) + abs(vb) + abs(a) + abs(b);
            let error = err + 0.000001 * scale;
            let bow = 0.125 * M * h * h + error;
            if (min(va, vb) > bow || max(va, vb) < -bow) {{ continue; }}
            let slope = (vb - va) / h;
            let radius = 0.5 * M * h + 2.0 * error / h;
            let minSlope = abs(slope) - radius;
            if (minSlope > 0.0) {{
                isolateMonotoneRoot(m, o, d, a, b, c, va, vb, error, minSlope,
                                    bounds, nb, bands, nr, reason, work, zeroBands, isZeroLevel);
                continue;
            }}
            if (0.125 * M * h * h <= error) {{
                putBandTagged(a, b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel);
                continue;
            }}
            if (f32(depth) >= 14.0 || top + 2u > {stack_cap}u) {{
                putBandTagged(a, b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel);
                continue;
            }}
            let mid = a + 0.5 * h;
            if (mid <= a || mid >= b) {{
                putBandTagged(a, b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel);
                continue;
            }}
            let phi_mid = evalPhi(m, o, d, mid);
            stack[top] = vec4f(mid, b, phi_mid, cell.w); setDepth(&depthBits, top, depth + 1u);
            stack[top + 1u] = vec4f(a, mid, cell.z, phi_mid); setDepth(&depthBits, top + 1u, depth + 1u);
            top += 2u;
        }}
    }}
}}

@compute @workgroup_size(64)
fn producerFastMain(@builtin(global_invocation_id) gid: vec3u) {{
    let id = gid.x;
    if (id >= cfg.rayCount) {{ return; }}
    let input = rays[id];
    let ro = input.ro_clipA.xyz;
    let d = input.rd_clipB.xyz;
    let subBase = id * cfg.maxSubPerRay;
    let overBase = id * cfg.maxOverPerRay;
    if (cfg.budget == 0u) {{
        rays[id].counts = vec4u(subBase, 0u, overBase, FALLBACK | 1u);
        return;
    }}
    var reason = 0u;
    var work = 0u;
    var pieces: array<vec2f, {piece_cap}>;
    var memStart = vec4u(0u);
    var memCount = vec4u(0u);
    var np = 0u;

    for (var member = 0u; member < 4u; member++) {{
        memStart[member] = np;
        let m = curtains[member];
        let o = ro - m.origin.xyz;
        var a = input.ro_clipA.w;
        var b = input.rd_clipB.w;
        var hit = true;
        for (var axis = 0u; axis < 3u; axis++) {{
            var half = m.half[axis];
            if (axis == 0u) {{ half = min(half, m.limits.x); }}
            if (axis == 1u) {{ half = min(half, m.limits.y); }}
            if (d[axis] == 0.0) {{
                if (abs(o[axis]) > half + m.curv.y) {{ hit = false; break; }}
            }} else {{
                let q0 = (-half - m.curv.y - o[axis]) / d[axis];
                let q1 = (half + m.curv.y - o[axis]) / d[axis];
                a = max(a, min(q0, q1));
                b = min(b, max(q0, q1));
            }}
        }}
        if (!hit || !(b > a)) {{ continue; }}

        // Dynamic-slack linear-z cull using only runtime CurtainSpec parameters (zero magic constants)
        let waveAmpBound = abs(m.amps.x) + abs(m.amps.y) + abs(m.amps.z);
        let zSlack = m.limits.w + m.curv.y + waveAmpBound;
        let zA = o.z + a * d.z;
        let zB = o.z + b * d.z;
        if (min(zA, zB) > zSlack || max(zA, zB) < -zSlack) {{ continue; }}

        let M = m.curv.x * d.x * d.x * 1.00002;
        let err = m.curv.y;
        var activeBins = 0u;
        var bowActiveBins = 0u;
        var prevS = a;
        var prevPhi = 0.0;
        var hasPrev = false;
        var phi_knots: array<f32, 9>;

        for (var bin = 0u; bin < 8u; bin++) {{
            var s0 = a + (b - a) * f32(bin) / 8.0;
            var s1 = a + (b - a) * f32(bin + 1u) / 8.0;
            if (bin == 0u) {{ s0 = a; }}
            if (bin == 7u) {{ s1 = b; }}
            let h0 = s1 - s0;
            let z0 = o.z + s0 * d.z;
            let z1 = o.z + s1 * d.z;
            let binSlack = zSlack + 0.125 * M * h0 * h0;
            if (min(z0, z1) <= binSlack && max(z0, z1) >= -binSlack) {{
                activeBins |= (1u << bin);
                let phi0 = select(evalPhi(m, o, d, s0), prevPhi, hasPrev && s0 == prevS);
                let phi1 = evalPhi(m, o, d, s1);
                phi_knots[bin] = phi0;
                phi_knots[bin + 1u] = phi1;
                prevS = s1;
                prevPhi = phi1;
                hasPrev = true;

                // Universal cross-level depth-0 bow check across c in {{-w_z, 0, +w_z}}
                let scaleMax = 1.0 + abs(phi0) + abs(phi1) + 2.0 * m.limits.w + abs(s0) + abs(s1);
                let bowMax = 0.125 * M * h0 * h0 + (err + 0.000001 * scaleMax);
                if (min(phi0, phi1) <= m.limits.w + bowMax && max(phi0, phi1) >= -m.limits.w - bowMax) {{
                    bowActiveBins |= (1u << bin);
                }}
            }} else {{
                hasPrev = false;
            }}
        }}
        if (activeBins == 0u) {{ continue; }}

        // Whole-member bow skip: if no coarse bin can contain a root on ANY of the 3 levels,
        // and the member is outside the slab at the first active bin, then the entire member
        // has 0 roots and 0 active pieces on [a, b]. Still charge the 24 depth-0 work units!
        if (bowActiveBins == 0u) {{
            let firstBin = firstTrailingBit(activeBins);
            let phiFirst = phi_knots[firstBin];
            if (abs(phiFirst) > m.limits.w + err) {{
                if (work + 24u > cfg.budget) {{
                    reason |= 1u;
                    break;
                }}
                work += 24u;
                continue;
            }}
        }}

        var knots: array<f32, {knot_cap}>;
        var nb = 1u;
        knots[0] = a;
        var bands: array<vec2f, {band_cap}>;
        var nr = 0u;
        var zeroBands = 0u;
        rootsFast(m, o, d, a, b, -m.limits.w, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, false, bowActiveBins);
        if (reason == 0u) {{
            rootsFast(m, o, d, a, b, 0.0, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, true, bowActiveBins);
        }}
        if (reason == 0u) {{
            rootsFast(m, o, d, a, b, m.limits.w, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, false, bowActiveBins);
        }}
        if (reason != 0u) {{ break; }}

        if (d.x != 0.0) {{ let x = -o.x / d.x; if (x > a && x < b) {{ knots[nb] = x; nb++; }} }}
        if (d.y != 0.0) {{
            let y = -o.y / d.y; if (y > a && y < b) {{ knots[nb] = y; nb++; }}
            let k = (m.limits.z - o.y) / d.y; if (k > a && k < b) {{ knots[nb] = k; nb++; }}
        }}
        knots[nb] = b; nb++;
        for (var i = 1u; i < nb; i++) {{
            let v = knots[i]; var j = i;
            loop {{ if (j == 0u || knots[j - 1u] <= v) {{ break; }} knots[j] = knots[j - 1u]; j--; }}
            knots[j] = v;
        }}
        var count = 0u;
        var previousShortBand = false;
        var previousPositive = false;
        for (var i = 1u; i < nb; i++) {{
            let l = knots[i - 1u]; let r = knots[i];
            if (!(r > l)) {{ continue; }}
            let mid = l + 0.5 * (r - l);
            var uncertain = false;
            var protectedBoundary = false;
            if (d.x != 0.0) {{ protectedBoundary = l == -o.x / d.x; }}
            if (d.y != 0.0) {{ protectedBoundary = protectedBoundary || l == -o.y / d.y || l == (m.limits.z - o.y) / d.y; }}
            for (var k = 0u; k < nr; k++) {{
                uncertain = uncertain || (l < bands[k].y && r > bands[k].x);
                if ((zeroBands & (1u << k)) != 0u && (l == bands[k].x || l == bands[k].y)) {{ protectedBoundary = true; }}
            }}
            if (uncertain || abs(evalPhi(m, o, d, mid)) <= m.limits.w + m.curv.y) {{
                if (count > 0u && mayAbsorb(pieces[np - 1u], vec2f(l, r),
                     previousShortBand, previousPositive, uncertain, protectedBoundary)) {{
                    pieces[np - 1u].y = r;
                    previousShortBand = false;
                    previousPositive = true;
                }} else {{
                    if (np >= {piece_cap}u) {{ reason |= 8u; break; }}
                    pieces[np] = vec2f(l, r); np++; count++;
                    previousShortBand = uncertain && r - l <= 0.02;
                    previousPositive = !uncertain;
                }}
            }}
        }}
        memCount[member] = count;
        if (reason != 0u) {{ break; }}
    }}
    if (reason != 0u) {{
        rays[id].counts = vec4u(subBase, 0u, overBase, FALLBACK | reason);
        return;
    }}
    var knots: array<f32, {merge_knot_cap}>;
    var ne = 0u;
    var head = vec4u(0u);
    let totalKnots = np * 2u;
    loop {{
        if (ne >= totalKnots) {{ break; }}
        var bestM = 4u;
        var bestVal = 0.0;
        for (var m = 0u; m < 4u; m++) {{
            let h = head[m];
            if (h < memCount[m] * 2u) {{
                let pIdx = memStart[m] + (h >> 1u);
                let isY = (h & 1u) != 0u;
                let val = select(pieces[pIdx].x, pieces[pIdx].y, isY);
                if (bestM == 4u || val < bestVal) {{
                    bestVal = val;
                    bestM = m;
                }}
            }}
        }}
        knots[ne] = bestVal;
        ne++;
        head[bestM]++;
    }}
    var singles = 0u;
    var overs = 0u;
    var pIdx = vec4u(0u);
    for (var i = 1u; i < ne && reason == 0u; i++) {{
        let a = knots[i - 1u]; let b = knots[i];
        if (!(b > a)) {{ continue; }}
        let mid = a + 0.5 * (b - a);
        var mask = 0u;
        for (var m = 0u; m < 4u; m++) {{
            let start = memStart[m];
            let numPieces = memCount[m];
            var cur = pIdx[m];
            loop {{
                if (cur >= numPieces || pieces[start + cur].y >= mid) {{ break; }}
                cur++;
            }}
            pIdx[m] = cur;
            if (cur < numPieces) {{
                let p = pieces[start + cur];
                if (mid >= p.x && mid <= p.y) {{
                    mask |= (1u << m);
                }}
            }}
        }}
        if (mask == 0u) {{ continue; }}
        if (countOneBits(mask) == 1u) {{
            let n = max(1u, u32(ceil((b - a) / 4.0)));
            if (singles + n > cfg.maxSubPerRay) {{ reason |= 16u; break; }}
            let member = firstTrailingBit(mask);
            for (var k = 0u; k < n; k++) {{
                let l = select(a + f32(k) * (b - a) / f32(n), a, k == 0u);
                let r = select(a + f32(k + 1u) * (b - a) / f32(n), b, k + 1u == n);
                subpanels[subBase + singles] = Subpanel(vec2f(l, r), vec2u(member, 0u));
                singles++;
            }}
        }} else {{
            if (overs >= cfg.maxOverPerRay) {{ reason |= 32u; break; }}
            overlaps[overBase + overs] = Overlap(vec2f(a, b), vec2u(mask, 0u));
            overs++;
        }}
    }}
    if (reason != 0u) {{
        rays[id].counts = vec4u(subBase, 0u, overBase, FALLBACK | reason);
    }} else {{
        rays[id].counts = vec4u(subBase, singles, overBase, overs);
    }}
}}
'''


def source(base, root):
    q = load(root, 'ray_transport_gpu_pose_qualification')
    ms = load(root, 'ray_transport_gpu_producer_merge_sweep')
    s = ms.source(base, root)

    full_wgsl = make_fastpath_wgsl(compact=False).strip()
    compact_wgsl = make_fastpath_wgsl(compact=True).strip()

    shader_decl = (
        'static const char* kFastPathFullShader = R"WGSL(\n' + full_wgsl + '\n)WGSL";\n\n'
        'static const char* kFastPathCompactShader = R"WGSL(\n' + compact_wgsl + '\n)WGSL";\n\n'
    )
    s = q.once(
        s,
        '    auto cpProducer = buildCompute(kGpuProducerWgsl, "producerMain");',
        shader_decl
        + '    auto cpProducer = buildCompute(kGpuProducerWgsl, "producerMain");\n'
        + '    auto cpFastFull = buildCompute(kFastPathFullShader, "producerFastMain");\n'
        + '    auto cpFastCompact = buildCompute(kFastPathCompactShader, "producerFastMain");',
    )

    readback_helper = r'''
    auto readBackBuffer = [&](WGPUBuffer src, uint64_t size) -> std::vector<uint8_t> {
        auto rb = makeBuffer(WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead, size, nullptr);
        WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        wgpuCommandEncoderCopyBufferToBuffer(enc, src, 0, rb, 0, size);
        WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(enc, nullptr);
        wgpuCommandEncoderRelease(enc);
        wgpuQueueSubmit(gpu.queue, 1, &cmd);
        wgpuCommandBufferRelease(cmd);

        MapContext mc;
        WGPUBufferMapCallbackInfo cbInfo = {};
        cbInfo.mode = WGPUCallbackMode_AllowProcessEvents;
        cbInfo.callback = onMapped;
        cbInfo.userdata1 = &mc;
        wgpuBufferMapAsync(rb, WGPUMapMode_Read, 0, size, cbInfo);
        while (!mc.done) wgpuDevicePoll(gpu.device, true, nullptr);
        if (!mc.ok) throw std::runtime_error("buffer mapping failed");

        const uint8_t* ptr = static_cast<const uint8_t*>(wgpuBufferGetConstMappedRange(rb, 0, size));
        std::vector<uint8_t> data(ptr, ptr + size);
        wgpuBufferUnmap(rb);
        wgpuBufferRelease(rb);
        return data;
    };
'''
    s = q.once(s, '    auto readResults = [&]() -> std::vector<GpuResult> {',
               readback_helper + '\n    auto readResults = [&]() -> std::vector<GpuResult> {')

    s = q.once(s, 'for(uint32_t capacity:{8192u,16384u,32768u})', 'for(uint32_t capacity:{32768u})')

    old_tile_struct = r'''        struct Tile {uint32_t start,n;WGPUBindGroup producer,forced,consumer,fallback;};'''
    new_tile_struct = r'''        struct Tile {uint32_t start,n;WGPUBindGroup producer,forced,consumer,fallback,fastFull,forcedFastFull,fastCompact,forcedFastCompact;};'''
    s = q.once(s, old_tile_struct, new_tile_struct)

    old_tile_push = r'''            tiles.push_back({start,n,producer(cb),producer(fb),consumer,current(fallbackCP)});'''
    new_tile_push = r'''            auto prodGroup=[&](WGPUComputePipeline cp, WGPUBuffer configuration){return group(cp,{
                entry(0,configuration,0,sizeof(pcfg)),entry(1,specBuf,0,curtainSpecs.size()*sizeof(GpuCurtainSpec)),
                entry(2,rBuf,rayOffset,uint64_t(n)*sizeof(GpuRayHeader)),entry(3,tileSingles,0,tileSingleBytes),entry(4,tileOverlaps,0,tileOverlapBytes)});};
            tiles.push_back({start,n,producer(cb),producer(fb),consumer,current(fallbackCP),
                prodGroup(cpFastFull,cb),prodGroup(cpFastFull,fb),
                prodGroup(cpFastCompact,cb),prodGroup(cpFastCompact,fb)});'''
    s = q.once(s, old_tile_push, new_tile_push)

    # 4 modes:
    # Mode 0: Current Marcher
    # Mode 1: Guarded Bulk Connected
    # Mode 2: §6db Fast-Path Full-Cap Stream 32,768 (cpFastFull + cpUnfused + fallbackCP)
    # Mode 3: §6db Fast-Path Compact-State Stream 32,768 (cpFastCompact + cpUnfused + fallbackCP)
    old_exec = r'''            for(int k=0;k<repeats;++k){
                if(mode==0)dispatch(currentCP,currentBG,count);
                else if(mode==1){dispatch(cpProducer,bgProducer,count);dispatch(cpUnfused,bgUnfused,count);dispatch(fallbackCP,fallbackBG,count);}
                else for(const auto& tile:tiles){
                    dispatch(cpProducer,forced?tile.forced:tile.producer,tile.n);
                    dispatch(cpUnfused,tile.consumer,tile.n);dispatch(fallbackCP,tile.fallback,tile.n);
                }
            }'''
    new_exec = r'''            for(int k=0;k<repeats;++k){
                if(mode==0)dispatch(currentCP,currentBG,count);
                else if(mode==1){dispatch(cpProducer,bgProducer,count);dispatch(cpUnfused,bgUnfused,count);dispatch(fallbackCP,fallbackBG,count);}
                else if(mode==2)for(const auto& tile:tiles){
                    dispatch(cpFastFull,forced?tile.forcedFastFull:tile.fastFull,tile.n);
                    dispatch(cpUnfused,tile.consumer,tile.n);dispatch(fallbackCP,tile.fallback,tile.n);
                }
                else if(mode==3)for(const auto& tile:tiles){
                    dispatch(cpFastCompact,forced?tile.forcedFastCompact:tile.fastCompact,tile.n);
                    dispatch(cpUnfused,tile.consumer,tile.n);dispatch(fallbackCP,tile.fallback,tile.n);
                }
            }'''
    s = q.once(s, old_exec, new_exec)

    old_corr_section = r'''        // Poison checks and raw output are outside timed batches.
        auto correctness=[&](bool forced){
            std::vector<GpuResult> poison(executionRays);for(auto& p:poison)for(float& v:p.rgb_T)v=NAN;
            wgpuQueueWriteBuffer(gpu.queue,outBuf,0,poison.data(),outBytes);execute(2,1,forced);auto result=readResults();
            const auto& expected=forced?resCurrent:resUnfused;
            for(size_t i=0;i<count;++i)for(int k=0;k<4;++k)
                if(!std::isfinite(result[i].rgb_T[k])||std::bit_cast<uint32_t>(result[i].rgb_T[k])!=std::bit_cast<uint32_t>(expected[i].rgb_T[k]))throw std::runtime_error("stream changed or missed a result");
            J rows=J::array();for(size_t i=0;i<count;++i)rows.push_back({R[i][0],R[i][1],result[i].rgb_T[0],result[i].rgb_T[1],result[i].rgb_T[2],result[i].rgb_T[3]});
            return rows;
        };
        auto normalRows=correctness(false);auto forcedRows=correctness(true);
        auto rb=makeBuffer(WGPUBufferUsage_CopyDst|WGPUBufferUsage_MapRead,rayBytes,nullptr);
        auto enc=wgpuDeviceCreateCommandEncoder(gpu.device,nullptr);wgpuCommandEncoderCopyBufferToBuffer(enc,rBuf,0,rb,0,rayBytes);
        auto cmd=wgpuCommandEncoderFinish(enc,nullptr);wgpuCommandEncoderRelease(enc);wgpuQueueSubmit(gpu.queue,1,&cmd);wgpuCommandBufferRelease(cmd);
        MapContext map;WGPUBufferMapCallbackInfo cb={};cb.mode=WGPUCallbackMode_AllowProcessEvents;cb.callback=onMapped;cb.userdata1=&map;
        wgpuBufferMapAsync(rb,WGPUMapMode_Read,0,rayBytes,cb);while(!map.done)wgpuDevicePoll(gpu.device,true,nullptr);
        if(!map.ok)throw std::runtime_error("stream header readback refused");
        const auto* headers=static_cast<const GpuRayHeader*>(wgpuBufferGetConstMappedRange(rb,0,rayBytes));
        unsigned forcedCount=0;for(size_t i=0;i<count;++i)if(headers[i].overlapCount&0x80000000u)++forcedCount;
        wgpuBufferUnmap(rb);wgpuBufferRelease(rb);
        if(forcedCount!=count)throw std::runtime_error("stream did not force every fallback");'''

    new_corr_section = r'''        auto correctness=[&](int testMode, bool forced){
            std::vector<GpuResult> poison(executionRays);for(auto& p:poison)for(float& v:p.rgb_T)v=NAN;
            wgpuQueueWriteBuffer(gpu.queue,outBuf,0,poison.data(),outBytes);execute(testMode,1,forced);auto result=readResults();
            const auto& expected=forced?resCurrent:resUnfused;
            for(size_t i=0;i<count;++i)for(int k=0;k<4;++k)
                if(!std::isfinite(result[i].rgb_T[k])||std::bit_cast<uint32_t>(result[i].rgb_T[k])!=std::bit_cast<uint32_t>(expected[i].rgb_T[k]))
                    throw std::runtime_error("stream mode " + std::to_string(testMode) + " changed or missed a result at ray " + std::to_string(i));
            J rows=J::array();for(size_t i=0;i<count;++i)rows.push_back({R[i][0],R[i][1],result[i].rgb_T[0],result[i].rgb_T[1],result[i].rgb_T[2],result[i].rgb_T[3]});
            return rows;
        };
        auto mode2NormalRows = correctness(2, false);
        auto mode2ForcedRows = correctness(2, true);
        auto normalRows = correctness(3, false);
        auto forcedRows = correctness(3, true);
        auto mode3ForcedHdrBytes = readBackBuffer(rBuf, rayBytes);
        const auto* mode3ForcedHdrs = reinterpret_cast<const GpuRayHeader*>(mode3ForcedHdrBytes.data());
        unsigned forcedCount = 0;
        for(size_t i=0;i<count;++i)if(mode3ForcedHdrs[i].overlapCount&0x80000000u)++forcedCount;
        if(forcedCount!=count)throw std::runtime_error("mode3 stream did not force every fallback");'''
    s = q.once(s, old_corr_section, new_corr_section)

    old_timing = r'''        // Separate warm correctness from measured ABBA/BAAB complete costs.
        for(int k=0;k<2;++k){execute(0,1,false);execute(1,1,false);execute(2,1,false);}
        J series=J::array();
        for(int repeats:{1,2,4}){
            J samples=J::array();
            for(int block=0;block<4;++block){
                const std::vector<int> order=block%2?std::vector<int>{2,1,0,0,1,2}:std::vector<int>{0,1,2,2,1,0};
                for(int mode:order){std::cout<<"STREAM capacity="<<capacity<<" repeats="<<repeats<<" mode="<<mode<<std::endl;auto t=execute(mode,repeats,false);samples.push_back({{"mode",mode},{"encode_ms",t[0]},{"completion_ms",t[1]},{"total_ms",t[2]}});}
            }
            series.push_back({{"repeats",repeats},{"samples",samples}});
        }'''
    new_timing = r'''        for(int k=0;k<2;++k){execute(0,1,false);execute(1,1,false);execute(2,1,false);execute(3,1,false);}
        J series=J::array();
        for(int repeats:{1,2,4}){
            J samples=J::array();
            for(int block=0;block<4;++block){
                const std::vector<int> order=block%2?std::vector<int>{3,2,1,0,0,1,2,3}:std::vector<int>{0,1,2,3,3,2,1,0};
                for(int mode:order){std::cout<<"STREAM capacity="<<capacity<<" repeats="<<repeats<<" mode="<<mode<<std::endl;auto t=execute(mode,repeats,false);samples.push_back({{"mode",mode},{"encode_ms",t[0]},{"completion_ms",t[1]},{"total_ms",t[2]}});}
            }
            series.push_back({{"repeats",repeats},{"samples",samples}});
        }'''
    s = q.once(s, old_timing, new_timing)

    old_release = r'''        for(auto& tile:tiles)for(auto bg:{tile.producer,tile.forced,tile.consumer,tile.fallback})wgpuBindGroupRelease(bg);
        for(auto b:tileUniforms)wgpuBufferRelease(b);wgpuBufferRelease(tileSingles);wgpuBufferRelease(tileOverlaps);'''
    new_release = r'''        for(auto& tile:tiles)for(auto bg:{tile.producer,tile.forced,tile.consumer,tile.fallback,tile.fastFull,tile.forcedFastFull,tile.fastCompact,tile.forcedFastCompact})wgpuBindGroupRelease(bg);
        for(auto b:tileUniforms)wgpuBufferRelease(b);wgpuBufferRelease(tileSingles);wgpuBufferRelease(tileOverlaps);'''
    s = q.once(s, old_release, new_release)

    old_push = r'''        streamingCases.push_back({{"capacity",capacity},{"tiles",tiles.size()},{"tail_rays",tiles.back().n},
            {"schedule_storage_bytes",tileSingleBytes+tileOverlapBytes},{"uniform_buffer_bytes",tiles.size()*(sizeof(udata)+2*sizeof(cfg)+sizeof(currentU))},
            {"binding_buffer_setup_ms",setupMs},{"normal_rows",normalRows},{"forced_rows",forcedRows},
            {"forced_fallback_rays",forcedCount},{"bit_identical_to_bulk_candidate",true},{"forced_bit_identical_to_native",true},{"series",series}});'''
    new_push = r'''        streamingCases.push_back({{"capacity",capacity},{"tiles",tiles.size()},{"tail_rays",tiles.back().n},
            {"schedule_storage_bytes",tileSingleBytes+tileOverlapBytes},{"uniform_buffer_bytes",tiles.size()*(sizeof(udata)+2*sizeof(cfg)+sizeof(currentU))},
            {"binding_buffer_setup_ms",setupMs},{"normal_rows",normalRows},{"forced_rows",forcedRows},
            {"mode2_normal_rows",mode2NormalRows},{"mode2_forced_rows",mode2ForcedRows},
            {"forced_fallback_rays",forcedCount},{"bit_identical_to_bulk_candidate",true},{"forced_bit_identical_to_native",true},{"series",series}});'''
    s = q.once(s, old_push, new_push)

    meta = r'''
    report["depth0_fastpath_experiment"] = {
        {"protocol", "§6db: Universal Cross-Level Depth-0 Bow Pre-Filter + Whole-Member Bow Skip + Depth-0 Register Fast-Path + Compact Private State"},
        {"modes", {{"0", "Current Marcher"}, {"1", "Guarded Bulk Connected"}, {"2", "FastPath Full-Cap Stream 32768"}, {"3", "FastPath Compact-State Stream 32768"}}}
    };
    wgpuComputePipelineRelease(cpFastFull);
    wgpuComputePipelineRelease(cpFastCompact);
'''
    anchor = '    std::cout << report.dump(2) << std::endl;'
    s = q.once(s, anchor, meta + '\n' + anchor)
    return s


def build_and_run(root, work, case_path, label, build_only=False):
    wide = load(root, 'ray_transport_gpu_wide_stream')
    v = {x.targets[0].id: ast.literal_eval(x.value)
         for x in ast.parse((root / 'scratch/probes/ray_transport_metal_diagnostic.py').read_text()).body
         if isinstance(x, ast.Assign) and isinstance(x.value, ast.Constant) and isinstance(x.value.value, str)}
    zone = root / 'saves/zones/Northern Veil/zone.ecform'
    if sha(zone) != v['ZONE_HASH']:
        raise SystemExit('pinned authored source changed')
    case_meta, rays, (ref_rows, ref_indices) = wide.verify_case(root, case_path, v['ZONE_HASH'])

    cpp = source(v['CPP_SOURCE'], root)
    digest = hashlib.sha256(cpp.encode()).hexdigest()

    work.mkdir(parents=True, exist_ok=True)
    probe_cpp = work / 'probe.cpp'
    probe_o = work / 'probe.o'
    probe_bin = work / 'probe'
    probe_cpp.write_text(cpp)
    (work / 'source.json').write_text(json.dumps({'cpp_sha256': digest, 'zone_sha256': v['ZONE_HASH']}, indent=2) + '\n')

    b = root / 'build'
    tmp_inc = Path('/private/tmp/earthcall-d4963d062-src')
    db = json.loads((b / 'compile_commands.json').read_text())
    c = next(x for x in db if x['file'].endswith('/entry.cpp') and 'earthcall_webgpu.dir' in x['command'])
    cmd = shlex.split(c['command'])
    if tmp_inc.exists():
        inc_idx = next(i for i, x in enumerate(cmd) if x.startswith('-I'))
        cmd.insert(inc_idx, f'-I{tmp_inc}/src')
    cmd[cmd.index('-c') + 1] = str(probe_cpp)
    cmd[cmd.index('-o') + 1] = str(probe_o)
    cmd += ['-std=c++20', '-O3']
    subprocess.run(cmd, cwd=b, check=True)

    link_cmd = shlex.split((b / 'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text())
    link_cmd[next(i for i, x in enumerate(link_cmd) if x.endswith('/src/entry.cpp.o'))] = str(probe_o)
    if (tmp_inc / 'ScalarForm.o').exists():
        link_cmd[next(i for i, x in enumerate(link_cmd) if x.endswith('ScalarForm.cpp.o'))] = str(tmp_inc / 'ScalarForm.o')
        link_cmd[next(i for i, x in enumerate(link_cmd) if x.endswith('SdfWgsl.cpp.o'))] = str(tmp_inc / 'SdfWgsl.o')
        for extra_o in ['LineRestriction.o', 'VolumeRayTransport.o', 'Integral.o', 'EllipticIntegrals.o']:
            link_cmd.insert(link_cmd.index('-o'), str(tmp_inc / extra_o))
    link_cmd[link_cmd.index('-o') + 1] = str(probe_bin)
    subprocess.run(link_cmd, cwd=b, check=True)
    if build_only:
        print('PASS §6db build-only:', probe_bin, 'cpp_sha256:', digest)
        return


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root', type=Path, required=True)
    p.add_argument('--work', type=Path, required=True)
    p.add_argument('--case', type=Path, default=Path('/private/tmp/earthcall-wide-case/manifest.json'))
    p.add_argument('--label', default='depth0_fastpath')
    p.add_argument('--build-only', action='store_true')
    args = p.parse_args()
    build_and_run(args.root.resolve(), args.work.resolve(), args.case.resolve(), args.label, args.build_only)


if __name__ == '__main__':
    main()
