import argparse, importlib.util, sys
from pathlib import Path

def load(root, name):
    p = root / 'scratch/probes' / (name + '.py')
    spec = importlib.util.spec_from_file_location(name, p)
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m

FUSED_WGSL = r'''
struct CurtainSpec {
    origin: vec4<f32>,
    half: vec4<f32>,
    limits: vec4<f32>,
    curv: vec4<f32>,
    amps: vec4<f32>,
    freqs: vec4<f32>,
    phases: vec4<f32>,
};

struct Ray {
    ro_clipA: vec4<f32>,
    rd_clipB: vec4<f32>,
    counts: vec4<u32>,
};

struct Subpanel {
    span: vec2<f32>,
    info: vec2<u32>,
};

struct Overlap {
    span: vec2<f32>,
    info: vec2<u32>,
};

struct Config {
    rayCount: u32,
    maxSubPerRay: u32,
    maxOverPerRay: u32,
    budget: u32,
};

struct MediumParams {
    origin: vec4<f32>,
    color_alpha: vec4<f32>,
    freq_offset: vec4<f32>,
};

struct ParameterBuffer {
    v: array<f32>,
};

const FALLBACK: u32 = 0x80000000u;

@group(0) @binding(0) var<uniform> cfg: Config;
@group(0) @binding(1) var<storage, read> curtains: array<CurtainSpec, 4>;
@group(0) @binding(2) var<storage, read_write> rays: array<Ray>;
@group(0) @binding(3) var<storage, read> media: array<MediumParams>;
@group(0) @binding(4) var<storage, read> params: ParameterBuffer;
@group(0) @binding(5) var<storage, read_write> results: array<vec4<f32>>;

fn mod289(x: vec4<f32>) -> vec4<f32> {
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}
fn mod289_3(x: vec3<f32>) -> vec3<f32> {
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}
fn permute4(x: vec4<f32>) -> vec4<f32> {
    return mod289(((x * 34.0) + 1.0) * x);
}
fn taylorInvSqrt(r: vec4<f32>) -> vec4<f32> {
    return 1.79284291400159 - 0.85373472095314 * r;
}

fn cnoise3(P: vec3<f32>) -> f32 {
    let Pi0 = floor(P);
    let Pi1 = Pi0 + vec3<f32>(1.0);
    let Pi0_mod = mod289_3(Pi0);
    let Pi1_mod = mod289_3(Pi1);
    let Pf0 = fract(P);
    let Pf1 = Pf0 - vec3<f32>(1.0);
    let ix = vec4<f32>(Pi0_mod.x, Pi1_mod.x, Pi0_mod.x, Pi1_mod.x);
    let iy = vec4<f32>(Pi0_mod.y, Pi0_mod.y, Pi1_mod.y, Pi1_mod.y);
    let iz0 = vec4<f32>(Pi0_mod.z);
    let iz1 = vec4<f32>(Pi1_mod.z);

    let ixy = permute4(permute4(ix) + iy);
    let ixy0 = permute4(ixy + iz0);
    let ixy1 = permute4(ixy + iz1);

    var gx0 = ixy0 / 7.0;
    var gy0 = fract(floor(gx0) / 7.0) - 0.5;
    gx0 = fract(gx0);
    var gz0 = vec4<f32>(0.5) - abs(gx0) - abs(gy0);
    let sz0 = step(gz0, vec4<f32>(0.0));
    gx0 = gx0 - sz0 * (step(vec4<f32>(0.0), gx0) - 0.5);
    gy0 = gy0 - sz0 * (step(vec4<f32>(0.0), gy0) - 0.5);

    var gx1 = ixy1 / 7.0;
    var gy1 = fract(floor(gx1) / 7.0) - 0.5;
    gx1 = fract(gx1);
    var gz1 = vec4<f32>(0.5) - abs(gx1) - abs(gy1);
    let sz1 = step(gz1, vec4<f32>(0.0));
    gx1 = gx1 - sz1 * (step(vec4<f32>(0.0), gx1) - 0.5);
    gy1 = gy1 - sz1 * (step(vec4<f32>(0.0), gy1) - 0.5);

    var g000 = vec3<f32>(gx0.x,gy0.x,gz0.x);
    var g100 = vec3<f32>(gx0.y,gy0.y,gz0.y);
    var g010 = vec3<f32>(gx0.z,gy0.z,gz0.z);
    var g110 = vec3<f32>(gx0.w,gy0.w,gz0.w);
    var g001 = vec3<f32>(gx1.x,gy1.x,gz1.x);
    var g101 = vec3<f32>(gx1.y,gy1.y,gz1.y);
    var g011 = vec3<f32>(gx1.z,gy1.z,gz1.z);
    var g111 = vec3<f32>(gx1.w,gy1.w,gz1.w);

    let norm0 = taylorInvSqrt(vec4<f32>(dot(g000, g000), dot(g010, g010), dot(g100, g100), dot(g110, g110)));
    g000 = g000 * norm0.x;
    g010 = g010 * norm0.y;
    g100 = g100 * norm0.z;
    g110 = g110 * norm0.w;
    let norm1 = taylorInvSqrt(vec4<f32>(dot(g001, g001), dot(g011, g011), dot(g101, g101), dot(g111, g111)));
    g001 = g001 * norm1.x;
    g011 = g011 * norm1.y;
    g101 = g101 * norm1.z;
    g111 = g111 * norm1.w;

    let n000 = dot(g000, Pf0);
    let n100 = dot(g100, vec3<f32>(Pf1.x, Pf0.y, Pf0.z));
    let n010 = dot(g010, vec3<f32>(Pf0.x, Pf1.y, Pf0.z));
    let n110 = dot(g110, vec3<f32>(Pf1.x, Pf1.y, Pf0.z));
    let n001 = dot(g001, vec3<f32>(Pf0.x, Pf0.y, Pf1.z));
    let n101 = dot(g101, vec3<f32>(Pf1.x, Pf0.y, Pf1.z));
    let n011 = dot(g011, vec3<f32>(Pf0.x, Pf1.y, Pf1.z));
    let n111 = dot(g111, Pf1);

    let fade_xyz = Pf0 * Pf0 * Pf0 * (Pf0 * (Pf0 * 6.0 - 15.0) + 10.0);
    let n_z = mix(vec4<f32>(n000, n100, n010, n110), vec4<f32>(n001, n101, n011, n111), fade_xyz.z);
    let n_yz = mix(n_z.xy, n_z.zw, fade_xyz.y);
    let n_xyz = mix(n_yz.x, n_yz.y, fade_xyz.x); 
    return 2.2 * n_xyz;
}

fn evalEnvelope(p: vec3<f32>, paramOffset: u32) -> f32 {
    let f0 = params.v[paramOffset + 0u];
    if (f0 == 0.0) { return 0.0; }
    let f1 = clamp(params.v[paramOffset + 1u] - select(0.0, abs(p.x) / params.v[paramOffset + 2u], abs(params.v[paramOffset + 2u]) >= 1e-06), params.v[paramOffset + 3u], params.v[paramOffset + 4u]);
    if (f1 == 0.0) { return 0.0; }
    let f2 = clamp(params.v[paramOffset + 5u] * (p.y + params.v[paramOffset + 6u]), params.v[paramOffset + 7u], params.v[paramOffset + 8u]);
    if (f2 == 0.0) { return 0.0; }
    let f3 = clamp(params.v[paramOffset + 9u] - select(0.0, (p.y + params.v[paramOffset + 11u]) / params.v[paramOffset + 10u], abs(params.v[paramOffset + 10u]) >= 1e-06), params.v[paramOffset + 12u], params.v[paramOffset + 13u]);
    if (f3 == 0.0) { return 0.0; }
    let z_curve = p.z + params.v[paramOffset + 16u] * sin(params.v[paramOffset + 17u] * p.x + params.v[paramOffset + 18u])
                      + params.v[paramOffset + 19u] * cos(params.v[paramOffset + 20u] * p.x + params.v[paramOffset + 21u])
                      + params.v[paramOffset + 22u] * sin(params.v[paramOffset + 23u] * p.x + params.v[paramOffset + 24u]);
    let f4 = clamp(params.v[paramOffset + 14u] - select(0.0, abs(z_curve) / params.v[paramOffset + 15u], abs(params.v[paramOffset + 15u]) >= 1e-06), params.v[paramOffset + 25u], params.v[paramOffset + 26u]);
    if (f4 == 0.0) { return 0.0; }
    let f5 = params.v[paramOffset + 27u] + params.v[paramOffset + 28u] * cos(params.v[paramOffset + 29u] * p.x + params.v[paramOffset + 30u])
                                         + params.v[paramOffset + 31u] * cos(params.v[paramOffset + 32u] * p.x + params.v[paramOffset + 33u]);
    if (f5 == 0.0) { return 0.0; }
    return f0 * f1 * f2 * f3 * f4 * f5;
}

fn phiSafe(tau: f32) -> f32 {
    if (abs(tau) < 0.01) {
        return 1.0 + tau * (-0.5 + tau * (1.0/6.0 - tau/24.0));
    }
    return (1.0 - exp(-tau)) / tau;
}

fn projectedAt(ro: vec3<f32>, rd: vec3<f32>, s: f32, mask: u32) -> vec4<f32> {
    var result = vec4<f32>(0.0);
    for (var member = 0u; member < arrayLength(&media); member += 1u) {
        if ((mask & (1u << member)) == 0u) { continue; }
        let m = media[member];
        let local = ro + s * rd - m.origin.xyz;
        let env = evalEnvelope(local, u32(m.freq_offset.y));
        if (env <= 0.0) { continue; }
        let D = env * clamp(0.7 + 0.3 * cnoise3(m.freq_offset.x * local), 0.0, 1.0);
        if (D <= 0.0) { continue; }
        let raw = m.color_alpha.w * D;
        result += vec4<f32>(m.color_alpha.xyz * raw, max(raw, 1e-6));
    }
    return result;
}

fn advanceFlow(state: vec4<f32>, sourceSigma: vec4<f32>, h: f32) -> vec4<f32> {
    let tau = h * sourceSigma.w;
    return vec4<f32>(state.xyz + state.w * sourceSigma.xyz * (h * phiSafe(tau)), state.w * exp(-tau));
}

fn advanceOverlap(stateIn: vec4<f32>, ro: vec3<f32>, rd: vec3<f32>, o: Overlap) -> vec4<f32> {
    var state = stateIn;
    let h = (o.span.y - o.span.x) * 0.5;
    let a = 0.25 - sqrt(3.0) / 6.0;
    let b = 0.5 - a;
    for (var panel = 0u; panel < 2u; panel += 1u) {
        let lo = o.span.x + f32(panel) * h;
        let minus = projectedAt(ro, rd, lo + (0.5 - sqrt(3.0) / 6.0) * h, o.info.x);
        let plus  = projectedAt(ro, rd, lo + (0.5 + sqrt(3.0) / 6.0) * h, o.info.x);
        state = advanceFlow(state, b * minus + a * plus, h);
        state = advanceFlow(state, a * minus + b * plus, h);
    }
    return state;
}

fn evalWave(m: CurtainSpec, x: f32) -> f32 {
    return m.amps.x * sin(m.freqs.x * x + m.phases.x)
         + m.amps.y * cos(m.freqs.y * x + m.phases.y)
         + m.amps.z * sin(m.freqs.z * x + m.phases.z);
}

fn evalPhi(m: CurtainSpec, o: vec3f, d: vec3f, s: f32) -> f32 {
    return o.z + s * d.z + evalWave(m, o.x + s * d.x);
}

fn putBand(lo: f32, hi: f32, bounds: ptr<function, array<f32, 96>>, nb: ptr<function, u32>,
           bands: ptr<function, array<vec2f, 32>>, nr: ptr<function, u32>, reason: ptr<function, u32>) {
    if (*nr > 0u) {
        let last = (*bands)[*nr - 1u];
        if (lo <= last.y && hi >= last.x) {
            let merged = vec2f(min(lo, last.x), max(hi, last.y));
            (*bands)[*nr - 1u] = merged;
            (*bounds)[*nb - 2u] = merged.x;
            (*bounds)[*nb - 1u] = merged.y;
            return;
        }
    }
    if (*nb + 2u > 92u || *nr >= 32u) { *reason |= 2u; return; }
    (*bounds)[*nb] = lo; (*bounds)[*nb + 1u] = hi; *nb += 2u;
    (*bands)[*nr] = vec2f(lo, hi); *nr += 1u;
}

fn mayAbsorb(previous: vec2f, current: vec2f, previousShortBand: bool,
             previousPositive: bool, currentUncertain: bool, protectedBoundary: bool) -> bool {
    if (protectedBoundary || previous.y != current.x) { return false; }
    let currentShortBand = currentUncertain && current.y - current.x <= 0.02;
    return (previousShortBand && !currentUncertain) || (previousPositive && currentShortBand);
}

fn putBandTagged(lo: f32, hi: f32, bounds: ptr<function, array<f32, 96>>,
                 nb: ptr<function, u32>, bands: ptr<function, array<vec2f, 32>>,
                 nr: ptr<function, u32>, reason: ptr<function, u32>,
                 zeroBands: ptr<function, u32>, isZeroLevel: bool) {
    putBand(lo, hi, bounds, nb, bands, nr, reason);
    if (*reason == 0u && isZeroLevel && *nr > 0u) { *zeroBands |= 1u << (*nr - 1u); }
}

fn getDepth(bits: vec3u, idx: u32) -> u32 {
    return (bits[idx >> 3u] >> ((idx & 7u) << 2u)) & 15u;
}

fn setDepth(bits: ptr<function, vec3u>, idx: u32, d: u32) {
    let w = idx >> 3u; let s = (idx & 7u) << 2u;
    (*bits)[w] = ((*bits)[w] & ~(15u << s)) | ((d & 15u) << s);
}

fn roots(m: CurtainSpec, o: vec3f, d: vec3f, lo: f32, hi: f32, c: f32,
         bounds: ptr<function, array<f32, 96>>, nb: ptr<function, u32>, bands: ptr<function, array<vec2f, 32>>,
         nr: ptr<function, u32>, reason: ptr<function, u32>, work: ptr<function, u32>,
         phi_knots: ptr<function, array<f32, 9>>, zeroBands: ptr<function, u32>, isZeroLevel: bool) {
    let M = m.curv.x * d.x * d.x * 1.00002;
    let err = m.curv.y;
    var stack: array<vec4f, 20>; var depthBits = vec3u(0u); var top = 0u;
    for (var bin = 0u; bin < 8u; bin++) {
        var a = lo + (hi - lo) * f32(bin) / 8.0; var b = lo + (hi - lo) * f32(bin + 1u) / 8.0;
        if (bin == 0u) { a = lo; } if (bin == 7u) { b = hi; }
        let phi_a0 = (*phi_knots)[bin]; let phi_b0 = (*phi_knots)[bin + 1u];
        stack[0] = vec4f(a, b, phi_a0, phi_b0); setDepth(&depthBits, 0u, 0u); top = 1u;
        loop {
            if (top == 0u || *reason != 0u) { break; }
            top--; let cell = stack[top]; a = cell.x; b = cell.y; let h = b - a;
            let depth = getDepth(depthBits, top);
            if (*work >= cfg.budget) { *reason |= 1u; break; } *work += 1u;
            if (!(h > 0.0)) { *reason |= 4u; break; }
            let va = cell.z - c; let vb = cell.w - c;
            let scale = 1.0 + abs(va) + abs(vb) + abs(a) + abs(b);
            let arithmetic = 0.000001 * scale;
            let error = err + arithmetic;
            let bow = 0.125 * M * h * h + error;
            if (min(va, vb) > bow || max(va, vb) < -bow) { continue; }
            let slope = (vb - va) / h; let radius = 0.5 * M * h + 2.0 * error / h;
            let minSlope = abs(slope) - radius;
            if (minSlope > 0.0) {
                if (abs(va) <= error) { let w = 2.0 * error / minSlope; putBandTagged(a, min(b, a + w), bounds, nb, bands, nr, reason, zeroBands, isZeroLevel); }
                if (abs(vb) <= error) { let w = 2.0 * error / minSlope; putBandTagged(max(a, b - w), b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel); }
                if ((va > error && vb < -error) || (va < -error && vb > error)) {
                    var l = a; var r = b; var left = va;
                    for (var iter = 0u; iter < 32u; iter++) {
                        if (*work >= cfg.budget) { *reason |= 1u; break; } *work += 1u;
                        let mid = l + 0.5 * (r - l);
                        if (mid <= l || mid >= r || r - l <= 0.0001) { break; }
                        let v = evalPhi(m, o, d, mid) - c;
                        if (abs(v) <= error) { let w = 2.0 * error / minSlope; l = max(l, mid - w); r = min(r, mid + w); break; }
                        if ((left < 0.0 && v > 0.0) || (left > 0.0 && v < 0.0)) { r = mid; } else { l = mid; left = v; }
                    }
                    putBandTagged(l, r, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel);
                }
                continue;
            }
            if (0.125 * M * h * h <= error) { putBandTagged(a, b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel); continue; }
            if (f32(depth) >= 14.0 || top + 2u > 20u) { putBandTagged(a, b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel); continue; }
            let mid = a + 0.5 * h; if (mid <= a || mid >= b) { putBandTagged(a, b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel); continue; }
            let phi_mid = evalPhi(m, o, d, mid);
            stack[top] = vec4f(mid, b, phi_mid, cell.w); setDepth(&depthBits, top, depth + 1u);
            stack[top + 1u] = vec4f(a, mid, cell.z, phi_mid); setDepth(&depthBits, top + 1u, depth + 1u);
            top += 2u;
        }
        if (*reason != 0u) { break; }
    }
}

fn getMember(pm: vec3u, k: u32) -> u32 {
    return (pm[k >> 4u] >> ((k & 15u) << 1u)) & 3u;
}

fn setMember(pm: ptr<function, vec3u>, k: u32, m: u32) {
    let w = k >> 4u; let s = (k & 15u) << 1u;
    (*pm)[w] = ((*pm)[w] & ~(3u << s)) | ((m & 3u) << s);
}

@compute @workgroup_size(64)
fn fusedMain(@builtin(global_invocation_id) gid: vec3u) {
    let id = gid.x;
    if (id >= cfg.rayCount) { return; }

    let input = rays[id];
    let ro = input.ro_clipA.xyz;
    let d = input.rd_clipB.xyz;

    var reason = 0u;
    var work = 0u;
    var count = 0u;
    var pieces: array<vec2f, 48>;
    var pieceMembers = vec3u(0u);
    var knots: array<f32, 96>;
    var memStart = vec4u(0u);
    var memCount = vec4u(0u);

    // Forced fallback check (budget = 0 forces fallback for all rays)
    if (cfg.budget == 0u) {
        // Fallback flag writer: bit 31 set indicates fallback to native marcher
        rays[id].counts = vec4u(0u, 0u, 0u, FALLBACK | 1u);
        return;
    }

    for (var member = 0u; member < 4u; member++) {
        memStart[member] = count;
        let m = curtains[member];
        let o = ro - m.origin.xyz;
        var a = input.ro_clipA.w;
        var b = input.rd_clipB.w;
        var hit = true;
        for (var axis = 0u; axis < 3u; axis++) {
            var half = m.half[axis];
            if (axis == 0u) { half = min(half, m.limits.x); }
            if (axis == 1u) { half = min(half, m.limits.y); }
            if (d[axis] == 0.0) {
                if (abs(o[axis]) > half + m.curv.y) { hit = false; break; }
            } else {
                let q0 = (-half - m.curv.y - o[axis]) / d[axis];
                let q1 = (half + m.curv.y - o[axis]) / d[axis];
                a = max(a, min(q0, q1));
                b = min(b, max(q0, q1));
            }
        }
        if (!hit || !(b > a)) { continue; }
        var nb = 1u;
        knots[0] = a;
        var bands: array<vec2f, 32>;
        var nr = 0u;
        var zeroBands = 0u;
        var phi_knots: array<f32, 9>;
        for (var k = 0u; k < 9u; k++) {
            var s = a + (b - a) * f32(k) / 8.0;
            if (k == 0u) { s = a; }
            if (k == 8u) { s = b; }
            phi_knots[k] = evalPhi(m, o, d, s);
        }
        roots(m, o, d, a, b, -m.limits.w, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, false);
        if (reason != 0u) { break; }
        roots(m, o, d, a, b, 0.0, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, true);
        if (reason != 0u) { break; }
        roots(m, o, d, a, b, m.limits.w, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, false);
        if (reason != 0u) { break; }
        if (d.x != 0.0) { let x = -o.x / d.x; if (x > a && x < b) { knots[nb] = x; nb++; } }
        if (d.y != 0.0) {
            let y = -o.y / d.y; if (y > a && y < b) { knots[nb] = y; nb++; }
            let k = (m.limits.z - o.y) / d.y; if (k > a && k < b) { knots[nb] = k; nb++; }
        }
        knots[nb] = b; nb++;
        for (var i = 1u; i < nb; i++) {
            let v = knots[i]; var j = i;
            loop { if (j == 0u || knots[j - 1u] <= v) { break; } knots[j] = knots[j - 1u]; j--; }
            knots[j] = v;
        }
        let memberStart = count;
        var previousShortBand = false;
        var previousPositive = false;
        for (var i = 1u; i < nb; i++) {
            let l = knots[i - 1u]; let r = knots[i];
            if (!(r > l)) { continue; }
            let mid = l + 0.5 * (r - l);
            var uncertain = false;
            var protectedBoundary = false;
            if (d.x != 0.0) { protectedBoundary = l == -o.x / d.x; }
            if (d.y != 0.0) { protectedBoundary = protectedBoundary || l == -o.y / d.y || l == (m.limits.z - o.y) / d.y; }
            for (var k = 0u; k < nr; k++) {
                uncertain = uncertain || (l < bands[k].y && r > bands[k].x);
                if ((zeroBands & (1u << k)) != 0u && (l == bands[k].x || l == bands[k].y)) { protectedBoundary = true; }
            }
            if (uncertain || abs(evalPhi(m, o, d, mid)) <= m.limits.w + m.curv.y) {
                if (count > memberStart && mayAbsorb(pieces[count - 1u], vec2f(l, r),
                     previousShortBand, previousPositive, uncertain, protectedBoundary)) {
                    pieces[count - 1u].y = r;
                    previousShortBand = false;
                    previousPositive = true;
                } else {
                    if (count >= 48u) { reason |= 8u; break; }
                    pieces[count] = vec2f(l, r);
                    setMember(&pieceMembers, count, member);
                    count++;
                    previousShortBand = uncertain && r - l <= 0.02;
                    previousPositive = !uncertain;
                }
            }
        }
        memCount[member] = count - memStart[member];
        if (reason != 0u) { break; }
    }

    // 4-way merge of member knots
    var ne = 0u;
    var head = vec4u(0u);
    let totalKnots = (memCount.x + memCount.y + memCount.z + memCount.w) * 2u;
    loop {
        if (ne >= totalKnots) { break; }
        var bestM = 4u;
        var bestVal = 0.0;
        for (var m = 0u; m < 4u; m++) {
            let h = head[m];
            if (h < memCount[m] * 2u) {
                let pIdx = memStart[m] + (h >> 1u);
                let isY = (h & 1u) != 0u;
                let val = select(pieces[pIdx].x, pieces[pIdx].y, isY);
                if (bestM == 4u || val < bestVal) {
                    bestVal = val;
                    bestM = m;
                }
            }
        }
        knots[ne] = bestVal;
        ne++;
        head[bestM]++;
    }

    // Thread-local interval storage
    var local_subpanels: array<Subpanel, 32>;
    var local_overlaps: array<Overlap, 8>;
    var singles = 0u;
    var overs = 0u;
    var pIdx = vec4u(0u);

    // Linear mask sweep
    for (var i = 1u; i < ne && reason == 0u; i++) {
        let a = knots[i - 1u];
        let b = knots[i];
        if (!(b > a)) { continue; }
        let mid = a + 0.5 * (b - a);
        var mask = 0u;
        for (var m = 0u; m < 4u; m++) {
            let start = memStart[m];
            let numPieces = memCount[m];
            var cur = pIdx[m];
            loop {
                if (cur >= numPieces || pieces[start + cur].y >= mid) { break; }
                cur++;
            }
            pIdx[m] = cur;
            if (cur < numPieces) {
                let p = pieces[start + cur];
                if (mid >= p.x && mid <= p.y) {
                    mask |= (1u << m);
                }
            }
        }
        if (mask == 0u) { continue; }
        if (countOneBits(mask) == 1u) {
            let n = max(1u, u32(ceil((b - a) / 4.0)));
            if (singles + n > 32u || singles + n > cfg.maxSubPerRay) { reason |= 16u; break; }
            let member = firstTrailingBit(mask);
            for (var k = 0u; k < n; k++) {
                let l = select(a + f32(k) * (b - a) / f32(n), a, k == 0u);
                let r = select(a + f32(k + 1u) * (b - a) / f32(n), b, k + 1u == n);
                local_subpanels[singles] = Subpanel(vec2f(l, r), vec2u(member, 0u));
                singles++;
            }
        } else {
            if (overs >= 8u || overs >= cfg.maxOverPerRay) { reason |= 32u; break; }
            local_overlaps[overs] = Overlap(vec2f(a, b), vec2u(mask, 0u));
            overs++;
        }
    }

    // Branch on reason: fallback path vs normal path
    if (reason != 0u) {
        // Fallback flag writer: write FALLBACK flag; discard partial transport so fallback writes entire result
        rays[id].counts = vec4u(0u, 0u, 0u, FALLBACK | reason);
        return;
    }

    // Normal path flag writer: clear FALLBACK flag (counts.w bit 31 = 0); fallbackMain will skip
    rays[id].counts = vec4u(0u, singles, 0u, overs);

    // Consumer quadrature integration directly from thread-local intervals
    let gx0 = 0.5 * (1.0 - 0.5773502691896257);
    let gx1 = 0.5 * (1.0 + 0.5773502691896257);

    var T = 1.0;
    var L = vec3<f32>(0.0);

    // Two sorted streams must be consumed in ray order, including overlap.
    var single = 0u;
    var overlap = 0u;
    loop {
        if (single >= singles && overlap >= overs) { break; }
        var useOverlap = false;
        if (overlap < overs) {
            if (single >= singles) { useOverlap = true; }
            else { useOverlap = local_overlaps[overlap].span.x < local_subpanels[single].span.x; }
        }
        if (useOverlap) {
            let state = advanceOverlap(vec4<f32>(L, T), ro, d, local_overlaps[overlap]);
            L = state.xyz; T = state.w;
            overlap += 1u;
            continue;
        }
        let sub = local_subpanels[single];
        single += 1u;
        let s0 = sub.span.x;
        let s1 = sub.span.y;
        let sub_h = s1 - s0;
        let mid = sub.info.x;
        let m = media[mid];
        let pOffset = u32(m.freq_offset.y);
        let freq = m.freq_offset.x;
        let alpha = m.color_alpha.w;
        let color = m.color_alpha.xyz;

        let s_m = s0 + gx0 * sub_h;
        let s_p = s0 + gx1 * sub_h;
        let w_m = ro + s_m * d - m.origin.xyz;
        let w_p = ro + s_p * d - m.origin.xyz;

        let env_m = evalEnvelope(w_m, pOffset);
        let env_p = evalEnvelope(w_p, pOffset);

        var D_m = 0.0;
        if (env_m > 0.0) {
            let nm = cnoise3(freq * w_m);
            D_m = env_m * clamp(0.7 + 0.3 * nm, 0.0, 1.0);
        }
        var D_p = 0.0;
        if (env_p > 0.0) {
            let np = cnoise3(freq * w_p);
            D_p = env_p * clamp(0.7 + 0.3 * np, 0.0, 1.0);
        }

        // Sixth-one Sun: Strict D > 0 gate
        let sig_m = select(0.0, max(alpha * D_m, 1e-6), D_m > 0.0);
        let em_m  = select(vec3<f32>(0.0), color * (alpha * D_m), D_m > 0.0);
        let sig_p = select(0.0, max(alpha * D_p, 1e-6), D_p > 0.0);
        let em_p  = select(vec3<f32>(0.0), color * (alpha * D_p), D_p > 0.0);

        let sig_avg = 0.5 * (sig_m + sig_p);
        let em_avg  = 0.5 * (em_m + em_p);
        let tau = sub_h * sig_avg;
        let atten = exp(-tau);

        // Sixth-one Sun: Numerically safe small-depth gain in f32
        var gain: f32;
        gain = sub_h * phiSafe(tau);

        L = L + T * em_avg * gain;
        T = T * atten;
    }

    results[id] = vec4<f32>(L, T);
}
'''

def source(base, root):
    q = load(root, 'ray_transport_gpu_pose_qualification')
    ms = load(root, 'ray_transport_gpu_producer_merge_sweep')
    s = ms.source(base, root)

    # 1. Add kFusedNormalShader right before cpProducer build
    fused_shader_decl = 'static const char* kFusedNormalShader = R"WGSL(\n' + FUSED_WGSL.strip() + '\n)WGSL";\n\n'
    s = q.once(s, '    auto cpProducer = buildCompute(kGpuProducerWgsl, "producerMain");',
                  fused_shader_decl + '    auto cpProducer = buildCompute(kGpuProducerWgsl, "producerMain");\n    auto cpFusedNormal = buildCompute(kFusedNormalShader, "fusedMain");')

    # 2. Add bgFusedNormal definition
    bg_fused_setup = r'''
    WGPUBindGroupEntry bgFusedNormalEntries[6] = {};
    bgFusedNormalEntries[0].binding = 0; bgFusedNormalEntries[0].buffer = cfgBuf; bgFusedNormalEntries[0].size = sizeof(cfg);
    bgFusedNormalEntries[1].binding = 1; bgFusedNormalEntries[1].buffer = specBuf; bgFusedNormalEntries[1].size = curtainSpecs.size() * sizeof(GpuCurtainSpec);
    bgFusedNormalEntries[2].binding = 2; bgFusedNormalEntries[2].buffer = rBuf; bgFusedNormalEntries[2].size = rayBytes;
    bgFusedNormalEntries[3].binding = 3; bgFusedNormalEntries[3].buffer = mBuf; bgFusedNormalEntries[3].size = mediaBytes;
    bgFusedNormalEntries[4].binding = 4; bgFusedNormalEntries[4].buffer = pBuf; bgFusedNormalEntries[4].size = paramBytes;
    bgFusedNormalEntries[5].binding = 5; bgFusedNormalEntries[5].buffer = outBuf; bgFusedNormalEntries[5].size = outBytes;
    WGPUBindGroupDescriptor bgfnd = {};
    bgfnd.layout = wgpuComputePipelineGetBindGroupLayout(cpFusedNormal, 0);
    bgfnd.entryCount = 6;
    bgfnd.entries = bgFusedNormalEntries;
    WGPUBindGroup bgFusedNormal = wgpuDeviceCreateBindGroup(gpu.device, &bgfnd);
'''
    s = q.once(s, '    WGPUBindGroup bgProducer = wgpuDeviceCreateBindGroup(gpu.device, &bgpd);',
                  '    WGPUBindGroup bgProducer = wgpuDeviceCreateBindGroup(gpu.device, &bgpd);\n' + bg_fused_setup)

    # 3. Update Tile struct to include fused and forcedFused bind groups
    s = q.once(s, 'struct Tile {uint32_t start,n;WGPUBindGroup producer,forced,consumer,fallback;};',
                  'struct Tile {uint32_t start,n;WGPUBindGroup producer,forced,consumer,fallback,fused,forcedFused;};')

    # 4. In tile creation loop, create fused and forcedFused bind groups
    old_tile_push = 'tiles.push_back({start,n,producer(cb),producer(fb),consumer,current(fallbackCP)});'
    new_tile_push = r'''auto fused = [&](WGPUBuffer configuration){return group(cpFusedNormal,{
                entry(0,configuration,0,sizeof(pcfg)),entry(1,specBuf,0,curtainSpecs.size()*sizeof(GpuCurtainSpec)),
                entry(2,rBuf,rayOffset,uint64_t(n)*sizeof(GpuRayHeader)),entry(3,mBuf,0,mediaBytes),
                entry(4,pBuf,0,paramBytes),entry(5,outBuf,outputOffset,uint64_t(n)*sizeof(GpuResult))});};
            tiles.push_back({start,n,producer(cb),producer(fb),consumer,current(fallbackCP),fused(cb),fused(fb)});'''
    s = q.once(s, old_tile_push, new_tile_push)

    # 5. Update execute lambda: in mode 2, dispatch fused normal + fallback passes
    old_execute_loop = r'''            for(int k=0;k<repeats;++k){
                if(mode==0)dispatch(currentCP,currentBG,count);
                else if(mode==1){dispatch(cpProducer,bgProducer,count);dispatch(cpUnfused,bgUnfused,count);dispatch(fallbackCP,fallbackBG,count);}
                else for(const auto& tile:tiles){
                    dispatch(cpProducer,forced?tile.forced:tile.producer,tile.n);
                    dispatch(cpUnfused,tile.consumer,tile.n);dispatch(fallbackCP,tile.fallback,tile.n);
                }
            }'''
    new_execute_loop = r'''            for(int k=0;k<repeats;++k){
                if(mode==0)dispatch(currentCP,currentBG,count);
                else if(mode==1){dispatch(cpProducer,bgProducer,count);dispatch(cpUnfused,bgUnfused,count);dispatch(fallbackCP,fallbackBG,count);}
                else for(const auto& tile:tiles){
                    dispatch(cpFusedNormal,forced?tile.forcedFused:tile.fused,tile.n);
                    dispatch(fallbackCP,tile.fallback,tile.n);
                }
            }'''
    s = q.once(s, old_execute_loop, new_execute_loop)

    # Diagnostic in correctness check
    old_correctness = r'''            for(size_t i=0;i<count;++i)for(int k=0;k<4;++k)
                if(!std::isfinite(result[i].rgb_T[k])||std::bit_cast<uint32_t>(result[i].rgb_T[k])!=std::bit_cast<uint32_t>(expected[i].rgb_T[k]))throw std::runtime_error("stream changed or missed a result");'''
    new_correctness = r'''            for(size_t i=0;i<count;++i) {
                bool mismatch = false;
                for(int k=0;k<4;++k) {
                    if(!std::isfinite(result[i].rgb_T[k])||std::bit_cast<uint32_t>(result[i].rgb_T[k])!=std::bit_cast<uint32_t>(expected[i].rgb_T[k])) mismatch = true;
                }
                if (mismatch) {
                    std::cerr << "MISMATCH forced=" << forced << " ray=" << i << " (" << R[i][0] << "," << R[i][1] << ")"
                              << " res=[" << result[i].rgb_T[0] << "," << result[i].rgb_T[1] << "," << result[i].rgb_T[2] << "," << result[i].rgb_T[3] << "]"
                              << " exp=[" << expected[i].rgb_T[0] << "," << expected[i].rgb_T[1] << "," << expected[i].rgb_T[2] << "," << expected[i].rgb_T[3] << "]"
                              << " resBits=[" << std::hex << std::bit_cast<uint32_t>(result[i].rgb_T[0]) << "," << std::bit_cast<uint32_t>(result[i].rgb_T[1]) << "," << std::bit_cast<uint32_t>(result[i].rgb_T[2]) << "," << std::bit_cast<uint32_t>(result[i].rgb_T[3]) << "]"
                              << " expBits=[" << std::hex << std::bit_cast<uint32_t>(expected[i].rgb_T[0]) << "," << std::bit_cast<uint32_t>(expected[i].rgb_T[1]) << "," << std::bit_cast<uint32_t>(expected[i].rgb_T[2]) << "," << std::bit_cast<uint32_t>(expected[i].rgb_T[3]) << "]" << std::dec << "\n";
                    throw std::runtime_error("stream changed or missed a result");
                }
            }'''
    s = q.once(s, old_correctness, new_correctness)

    # 6. In tile release, release fused and forcedFused bind groups
    s = q.once(s, 'for(auto bg:{tile.producer,tile.forced,tile.consumer,tile.fallback})wgpuBindGroupRelease(bg);',
                  'for(auto bg:{tile.producer,tile.forced,tile.consumer,tile.fallback,tile.fused,tile.forcedFused})wgpuBindGroupRelease(bg);')

    # 7. Add report metadata
    meta = '''
    report["fused_normal_path"] = {
        {"producer_consumer_fused", true},
        {"global_schedule_storage_eliminated", true},
        {"fallback_pass_retained", true},
        {"thread_private_subpanels_capacity", 32},
        {"thread_private_overlaps_capacity", 8},
        {"bit_identical_to_guarded_baseline_required", true},
        {"scope", "§6cv: Fused normal producer+consumer execution with retained native fallback dispatch"}
    };
'''
    anchor = '    std::cout << report.dump(2) << std::endl;'
    s = q.once(s, anchor, meta + '\n' + anchor)
    return s

def main():
    p = argparse.ArgumentParser(add_help=False)
    p.add_argument('--root', type=Path, required=True)
    args, _ = p.parse_known_args()
    root = args.root.resolve()
    # §6cw review: preserve the rejected historical shader for replay, but do
    # not let the executable silently certify changed capacities/old packets.
    review = load(root, 'ray_transport_gpu_fused_review')
    review.require_contract(FUSED_WGSL)
    q = load(root, 'ray_transport_gpu_pose_qualification')
    wide = load(root, 'ray_transport_gpu_wide_stream')
    q.source = lambda base, r: source(base, r)
    q.verify_case = lambda r, p, z: wide.verify_case(r, p, z)
    q.independent_check = lambda report, rows, rays, refs: review.admission(root, report, rows, rays, refs)
    q.main()

if __name__ == '__main__':
    main()
