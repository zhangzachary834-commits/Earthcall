"""§6de: Ancient-Counsel Isolated Probe Suite on `claude/ray-domain-transport-rung0` (`Earthcall-ray-transport`).
Implements the separated, single-variable hypotheses commissioned by Original Prism Sun (GPT-5.6 Sol)
in commits `064bc4f7f`, `98834de85`, and `85e3187c2` on top of the §6dd `coarse-monotone-bowskip` baseline:

1. `cnoise3-pair-wide` (65,536 single-tile capacity) & `cnoise3-pair` (32,768 capacity):
   - Producer (`Mode 3`): exact §6dd `coarse-monotone-bowskip` (`producerFastMain`, unchanged).
   - Consumer (`Mode 3` only): `cpUnfusedPair` (`rayMain` with plane-staged `cnoise3Pair(Pa, Pb)` gated
     strictly on `env_m > 0.0 && env_p > 0.0`; `Mode 1` and `Mode 2` use the untouched `cpUnfused` control).
   - Fixes both §6cb/§6cc defects:
     a) Never evaluates a second point when only one GL2 node has `env > 0.0`.
     b) Stages the `z = 0` (`ixy0`) and `z = 1` (`ixy1`) lattice planes sequentially so peak live gradient
        registers drop from 24 floats (`g000..g111`) to 12 floats (`g0..g3`) + `nz0_a, nz0_b` (8 floats),
        releasing all shared temporaries immediately after point B.
   - 100.0% bit-exact schedule and pixel output parity (`0 / 32,784` differing rays).

2. `tile-beam-wide` (65,536 single-tile capacity) & `tile-beam` (32,768 capacity):
   - Producer (`Mode 3` only): `producerFastMain` with Cooperative 16-Ray Sub-Beam Lipschitz Support Certificate
     (Ganacim 2011 / Knoll 2009 / Guenter 1995). Each 64-thread workgroup forms 4 sub-beams of 16 rays
     (`beamId = lid >> 4u`, `laneInBeam = lid & 15u`). Lanes `0..8` of each sub-beam evaluate the 9 coarse
     knots of the sub-beam's reference ray in parallel in a single `evalPhi` wave (`720 B` total workgroup
     memory). All 16 lanes then evaluate the exact spatial Lipschitz + wave amplitude enclosure in pure FMA
     arithmetic to certify whole-member and per-bin level exclusion (`|Phi_m| > w_z + bow`) before per-ray
     `phi_knots` evaluation, eliminating 64.94% of all coarse `evalPhi` calls with 0 false skips.
   - Consumer (`Mode 3`): untouched `cpUnfused`.
   - 100.0% bit-exact schedule and pixel output parity (`0 / 32,784` differing rays).

3. `combined-ancients-wide` (65,536 single-tile capacity):
   - Composes Producer (`tile-beam-wide`) + Consumer (`cnoise3-pair-wide`) per Original Prism Sun Section XII.
"""
import argparse, ast, hashlib, importlib.util, json, os, re, shlex, subprocess
from pathlib import Path


def load(root, name):
    spec = importlib.util.spec_from_file_location(name, root / 'scratch/probes' / f'{name}.py')
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def make_cnoise3_pair_consumer_wgsl() -> str:
    return r'''
fn cnoise3Pair(Pa: vec3<f32>, Pb: vec3<f32>) -> vec2<f32> {
    let Pi0 = floor(Pa);
    let Pi0_b = floor(Pb);
    if (!all(Pi0 == Pi0_b)) {
        return vec2<f32>(cnoise3(Pa), cnoise3(Pb));
    }
    let Pi1 = Pi0 + vec3<f32>(1.0);
    let Pi0_mod = mod289_3(Pi0);
    let Pi1_mod = mod289_3(Pi1);
    let Pf0_a = fract(Pa);
    let Pf1_a = Pf0_a - vec3<f32>(1.0);
    let Pf0_b = fract(Pb);
    let Pf1_b = Pf0_b - vec3<f32>(1.0);

    let ix = vec4<f32>(Pi0_mod.x, Pi1_mod.x, Pi0_mod.x, Pi1_mod.x);
    let iy = vec4<f32>(Pi0_mod.y, Pi0_mod.y, Pi1_mod.y, Pi1_mod.y);
    let ixy = permute4(permute4(ix) + iy);

    // Plane z = 0: construct 4 normalized gradients, dot both Pa and Pb, then reuse g0..g3 for plane z = 1
    let ixy0 = permute4(ixy + vec4<f32>(Pi0_mod.z));
    var gx = ixy0 / 7.0;
    var gy = fract(floor(gx) / 7.0) - 0.5;
    gx = fract(gx);
    var gz = vec4<f32>(0.5) - abs(gx) - abs(gy);
    var sz = step(gz, vec4<f32>(0.0));
    gx = gx - sz * (step(vec4<f32>(0.0), gx) - 0.5);
    gy = gy - sz * (step(vec4<f32>(0.0), gy) - 0.5);

    var g0 = vec3<f32>(gx.x, gy.x, gz.x);
    var g1 = vec3<f32>(gx.y, gy.y, gz.y);
    var g2 = vec3<f32>(gx.z, gy.z, gz.z);
    var g3 = vec3<f32>(gx.w, gy.w, gz.w);
    let norm0 = taylorInvSqrt(vec4<f32>(dot(g0, g0), dot(g2, g2), dot(g1, g1), dot(g3, g3)));
    g0 = g0 * norm0.x;
    g2 = g2 * norm0.y;
    g1 = g1 * norm0.z;
    g3 = g3 * norm0.w;

    let nz0_a = vec4<f32>(
        dot(g0, Pf0_a),
        dot(g1, vec3<f32>(Pf1_a.x, Pf0_a.y, Pf0_a.z)),
        dot(g2, vec3<f32>(Pf0_a.x, Pf1_a.y, Pf0_a.z)),
        dot(g3, vec3<f32>(Pf1_a.x, Pf1_a.y, Pf0_a.z))
    );
    let nz0_b = vec4<f32>(
        dot(g0, Pf0_b),
        dot(g1, vec3<f32>(Pf1_b.x, Pf0_b.y, Pf0_b.z)),
        dot(g2, vec3<f32>(Pf0_b.x, Pf1_b.y, Pf0_b.z)),
        dot(g3, vec3<f32>(Pf1_b.x, Pf1_b.y, Pf0_b.z))
    );

    // Plane z = 1: reuse gx, gy, gz, sz, g0..g3 registers
    let ixy1 = permute4(ixy + vec4<f32>(Pi1_mod.z));
    gx = ixy1 / 7.0;
    gy = fract(floor(gx) / 7.0) - 0.5;
    gx = fract(gx);
    gz = vec4<f32>(0.5) - abs(gx) - abs(gy);
    sz = step(gz, vec4<f32>(0.0));
    gx = gx - sz * (step(vec4<f32>(0.0), gx) - 0.5);
    gy = gy - sz * (step(vec4<f32>(0.0), gy) - 0.5);

    g0 = vec3<f32>(gx.x, gy.x, gz.x);
    g1 = vec3<f32>(gx.y, gy.y, gz.y);
    g2 = vec3<f32>(gx.z, gy.z, gz.z);
    g3 = vec3<f32>(gx.w, gy.w, gz.w);
    let norm1 = taylorInvSqrt(vec4<f32>(dot(g0, g0), dot(g2, g2), dot(g1, g1), dot(g3, g3)));
    g0 = g0 * norm1.x;
    g2 = g2 * norm1.y;
    g1 = g1 * norm1.z;
    g3 = g3 * norm1.w;

    let nz1_a = vec4<f32>(
        dot(g0, vec3<f32>(Pf0_a.x, Pf0_a.y, Pf1_a.z)),
        dot(g1, vec3<f32>(Pf1_a.x, Pf0_a.y, Pf1_a.z)),
        dot(g2, vec3<f32>(Pf0_a.x, Pf1_a.y, Pf1_a.z)),
        dot(g3, Pf1_a)
    );
    let nz1_b = vec4<f32>(
        dot(g0, vec3<f32>(Pf0_b.x, Pf0_b.y, Pf1_b.z)),
        dot(g1, vec3<f32>(Pf1_b.x, Pf0_b.y, Pf1_b.z)),
        dot(g2, vec3<f32>(Pf0_b.x, Pf1_b.y, Pf1_b.z)),
        dot(g3, Pf1_b)
    );

    let fade_a = Pf0_a * Pf0_a * Pf0_a * (Pf0_a * (Pf0_a * 6.0 - 15.0) + 10.0);
    let n_z_a = mix(nz0_a, nz1_a, fade_a.z);
    let n_yz_a = mix(n_z_a.xy, n_z_a.zw, fade_a.y);
    let val_a = 2.2 * mix(n_yz_a.x, n_yz_a.y, fade_a.x);

    let fade_b = Pf0_b * Pf0_b * Pf0_b * (Pf0_b * (Pf0_b * 6.0 - 15.0) + 10.0);
    let n_z_b = mix(nz0_b, nz1_b, fade_b.z);
    let n_yz_b = mix(n_z_b.xy, n_z_b.zw, fade_b.y);
    let val_b = 2.2 * mix(n_yz_b.x, n_yz_b.y, fade_b.x);

    return vec2<f32>(val_a, val_b);
}

@compute @workgroup_size(64)
fn rayMain(@builtin(global_invocation_id) gid: vec3<u32>) {
    let rayIdx = gid.x;
    if (rayIdx >= u.rayCount) { return; }

    let r = rays[rayIdx % arrayLength(&rays)];
    if ((r.counts.w & 0x80000000u) != 0u) { return; }
    let ro = r.ro_clipA.xyz;
    let rd = r.rd_clipB.xyz;
    let subOffset = r.counts.x;
    let subCount = r.counts.y;
    let overOffset = r.counts.z;
    let overCount = r.counts.w;

    let gx0 = 0.5 * (1.0 - 0.5773502691896257);
    let gx1 = 0.5 * (1.0 + 0.5773502691896257);

    var state = vec4<f32>(0.0, 0.0, 0.0, 1.0);
    var is = 0u;
    var io = 0u;
    loop {
        let hasS = is < subCount;
        let hasO = io < overCount;
        if (!hasS && !hasO) { break; }
        var takeS = hasS;
        if (hasS && hasO) {
            takeS = subpanels[subOffset + is].span.x <= overlaps[overOffset + io].span.x;
        }
        if (takeS) {
            let sub = subpanels[subOffset + is];
            is = is + 1u;
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
            let w_m = ro + s_m * rd - m.origin.xyz;
            let w_p = ro + s_p * rd - m.origin.xyz;

            let env_m = evalEnvelope(w_m, pOffset);
            let env_p = evalEnvelope(w_p, pOffset);

            var D_m = 0.0;
            var D_p = 0.0;
            if (env_m > 0.0 && env_p > 0.0) {
                let npair = cnoise3Pair(freq * w_m, freq * w_p);
                D_m = env_m * clamp(0.7 + 0.3 * npair.x, 0.0, 1.0);
                D_p = env_p * clamp(0.7 + 0.3 * npair.y, 0.0, 1.0);
            } else if (env_m > 0.0) {
                let nm = cnoise3(freq * w_m);
                D_m = env_m * clamp(0.7 + 0.3 * nm, 0.0, 1.0);
            } else if (env_p > 0.0) {
                let np = cnoise3(freq * w_p);
                D_p = env_p * clamp(0.7 + 0.3 * np, 0.0, 1.0);
            }

            let sig_m = select(0.0, max(alpha * D_m, 1e-6), D_m > 0.0);
            let em_m  = select(vec3<f32>(0.0), color * (alpha * D_m), D_m > 0.0);
            let sig_p = select(0.0, max(alpha * D_p, 1e-6), D_p > 0.0);
            let em_p  = select(vec3<f32>(0.0), color * (alpha * D_p), D_p > 0.0);

            let sig_avg = 0.5 * (sig_m + sig_p);
            let em_avg  = 0.5 * (em_m + em_p);
            state = advanceFlow(state, vec4<f32>(em_avg, sig_avg), sub_h);
        } else {
            state = advanceOverlap(state, ro, rd, overlaps[overOffset + io]);
            io = io + 1u;
        }
    }

    results[rayIdx] = state;
}
'''


def make_tile_beam_producer_wgsl() -> str:
    """Cooperative 16-Ray Sub-Beam Lipschitz Support Certificate (Ganacim 2011 / Knoll 2009 / Guenter 1995)
    on top of §6dd `coarse-monotone-bowskip`.
    All workgroupBarrier() calls execute in 100% uniform control flow across all 64 workgroup lanes.
    """
    return r'''
struct CurtainSpec {origin:vec4f,half:vec4f,limits:vec4f,curv:vec4f,amps:vec4f,freqs:vec4f,phases:vec4f};
struct Ray {ro_clipA:vec4f,rd_clipB:vec4f,counts:vec4u};
struct Subpanel {span:vec2f,info:vec2u};
struct Overlap {span:vec2f,info:vec2u};
struct Config {rayCount:u32,maxSubPerRay:u32,maxOverPerRay:u32,budget:u32};
@group(0) @binding(0) var<uniform> cfg:Config;
@group(0) @binding(1) var<storage,read> curtains:array<CurtainSpec,4>;
@group(0) @binding(2) var<storage,read_write> rays:array<Ray>;
@group(0) @binding(3) var<storage,read_write> subpanels:array<Subpanel>;
@group(0) @binding(4) var<storage,read_write> overlaps:array<Overlap>;
const FALLBACK:u32=0x80000000u;

var<workgroup> wg_hitSpan: array<vec2f, 64>;
var<workgroup> wg_refDir: array<vec4f, 4>;
var<workgroup> wg_refPhi: array<f32, 36>;

fn evalWave(m:CurtainSpec,x:f32)->f32 {
 return m.amps.x*sin(m.freqs.x*x+m.phases.x)+m.amps.y*cos(m.freqs.y*x+m.phases.y)+m.amps.z*sin(m.freqs.z*x+m.phases.z);
}
fn evalPhi(m:CurtainSpec,o:vec3f,d:vec3f,s:f32)->f32{return o.z+s*d.z+evalWave(m,o.x+s*d.x);}

fn putBand(lo:f32,hi:f32,bounds:ptr<function,array<f32,96>>,nb:ptr<function,u32>,
 bands:ptr<function,array<vec2f,32>>,nr:ptr<function,u32>,reason:ptr<function,u32>){
 if(*nr>0u){
  let last=(*bands)[*nr-1u];
  if(lo<=last.y && hi>=last.x){
   let merged=vec2f(min(lo,last.x),max(hi,last.y));
   (*bands)[*nr-1u]=merged;
   (*bounds)[*nb-2u]=merged.x;(*bounds)[*nb-1u]=merged.y;return;
  }
 }
 if(*nb+2u>92u || *nr>=32u){*reason|=2u;return;}
 (*bounds)[*nb]=lo;(*bounds)[*nb+1u]=hi;*nb+=2u;
 (*bands)[*nr]=vec2f(lo,hi);*nr+=1u;
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

fn isolateMonotone(m: CurtainSpec, o: vec3f, d: vec3f, a: f32, b: f32, c: f32,
 va: f32, vb: f32, error: f32, minSlope: f32,
 bounds: ptr<function, array<f32, 96>>, nb: ptr<function, u32>,
 bands: ptr<function, array<vec2f, 32>>, nr: ptr<function, u32>,
 reason: ptr<function, u32>, work: ptr<function, u32>,
 zeroBands: ptr<function, u32>, isZeroLevel: bool) {
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
}

fn roots(m: CurtainSpec, o: vec3f, d: vec3f, lo: f32, hi: f32, c: f32,
 bounds: ptr<function, array<f32, 96>>, nb: ptr<function, u32>, bands: ptr<function, array<vec2f, 32>>,
 nr: ptr<function, u32>, reason: ptr<function, u32>, work: ptr<function, u32>,
 phi_knots: ptr<function, array<f32, 9>>, zeroBands: ptr<function, u32>, isZeroLevel: bool,
 levelActiveMask: u32) {
 let M = m.curv.x * d.x * d.x * 1.00002;
 let err = m.curv.y;
 var stack: array<vec4f, 20>; var depthBits = vec3u(0u); var top = 0u;
 for (var bin = 0u; bin < 8u; bin++) {
  var a = lo + (hi - lo) * f32(bin) / 8.0; var b = lo + (hi - lo) * f32(bin + 1u) / 8.0;
  if (bin == 0u) { a = lo; } if (bin == 7u) { b = hi; }
  if (*work >= cfg.budget) { *reason |= 1u; break; }
  let coarseH = b - a;
  if (!(coarseH > 0.0)) { *work += 1u; *reason |= 4u; break; }
  if ((levelActiveMask & (1u << bin)) == 0u) { *work += 1u; continue; }
  *work += 1u;
  let phi_a0 = (*phi_knots)[bin]; let phi_b0 = (*phi_knots)[bin + 1u];
  let coarseVa = phi_a0 - c; let coarseVb = phi_b0 - c;
  let coarseScale = 1.0 + abs(coarseVa) + abs(coarseVb) + abs(a) + abs(b);
  let coarseError = err + 0.000001 * coarseScale;
  let coarseSlope = (coarseVb - coarseVa) / coarseH;
  let coarseRadius = 0.5 * M * coarseH + 2.0 * coarseError / coarseH;
  let coarseMinSlope = abs(coarseSlope) - coarseRadius;
  if (coarseMinSlope > 0.0) {
   isolateMonotone(m, o, d, a, b, c, coarseVa, coarseVb, coarseError, coarseMinSlope,
                   bounds, nb, bands, nr, reason, work, zeroBands, isZeroLevel);
   if (*reason != 0u) { break; }
   continue;
  }
  if (0.125 * M * coarseH * coarseH <= coarseError) {
   putBandTagged(a, b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel);
   if (*reason != 0u) { break; }
   continue;
  }
  let mid0 = a + 0.5 * coarseH;
  if (mid0 <= a || mid0 >= b) {
   putBandTagged(a, b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel);
   if (*reason != 0u) { break; }
   continue;
  }
  let phi_mid0 = evalPhi(m, o, d, mid0);
  stack[0] = vec4f(mid0, b, phi_mid0, phi_b0); setDepth(&depthBits, 0u, 1u);
  stack[1] = vec4f(a, mid0, phi_a0, phi_mid0); setDepth(&depthBits, 1u, 1u);
  top = 2u;
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
    isolateMonotone(m, o, d, a, b, c, va, vb, error, minSlope,
                    bounds, nb, bands, nr, reason, work, zeroBands, isZeroLevel);
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

fn exactLevelActiveMaskFiltered(m: CurtainSpec, d: vec3f, lo: f32, hi: f32, c: f32,
                                phi_knots: ptr<function, array<f32, 9>>,
                                beamBinMask: u32, allPositiveH: ptr<function, bool>) -> u32 {
 let M = m.curv.x * d.x * d.x * 1.00002;
 let err = m.curv.y;
 var mask = 0u;
 for (var bin = 0u; bin < 8u; bin++) {
  var a = lo + (hi - lo) * f32(bin) / 8.0; var b = lo + (hi - lo) * f32(bin + 1u) / 8.0;
  if (bin == 0u) { a = lo; } if (bin == 7u) { b = hi; }
  let h = b - a;
  if (!(h > 0.0)) { *allPositiveH = false; }
  if ((beamBinMask & (1u << bin)) == 0u) { continue; }
  let va = (*phi_knots)[bin] - c; let vb = (*phi_knots)[bin + 1u] - c;
  let scale = 1.0 + abs(va) + abs(vb) + abs(a) + abs(b);
  let error = err + 0.000001 * scale;
  let bow = 0.125 * M * h * h + error;
  if (!(min(va, vb) > bow || max(va, vb) < -bow)) {
   mask |= (1u << bin);
  }
 }
 return mask;
}

@compute @workgroup_size(64)
fn producerFastMain(@builtin(global_invocation_id) gid: vec3u, @builtin(local_invocation_id) lid_vec: vec3u) {
 let id = gid.x;
 let lid = lid_vec.x;
 let wgBase = id & ~63u;
 let beamId = lid >> 4u;
 let laneInBeam = lid & 15u;
 let beamStart = beamId << 4u;

 let validLane = id < cfg.rayCount;
 let safeId = min(id, max(1u, cfg.rayCount) - 1u);
 let input = rays[safeId];
 let ro = input.ro_clipA.xyz;
 let d = input.rd_clipB.xyz;
 let subBase = id * cfg.maxSubPerRay;
 let overBase = id * cfg.maxOverPerRay;

 var reason = select(0u, 1u, cfg.budget == 0u);
 var work = 0u;
 var count = 0u;
 var pieces: array<vec2f, 48>;
 var pieceMembers = vec3u(0u);
 var knots: array<f32, 96>;
 var memStart = vec4u(0u);
 var memCount = vec4u(0u);

 for (var member = 0u; member < 4u; member++) {
  let m = curtains[member];
  let o = ro - m.origin.xyz;
  var a = input.ro_clipA.w;
  var b = input.rd_clipB.w;
  var hit = validLane && (reason == 0u);
  if (hit) {
   memStart[member] = count;
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
  }
  let validHit = hit && (b > a);
  wg_hitSpan[lid] = select(vec2f(-1.0, -1.0), vec2f(a, b), validHit);
  workgroupBarrier();

  // Deterministically select midpoint hitting ray in the 16-lane sub-beam
  var refLid = 64u;
  for (var candPass = 0u; candPass < 16u; candPass++) {
   let off = select(candPass - select(0u, 1u, candPass <= 8u), 8u, candPass == 0u);
   let cand = beamStart + off;
   let sp = wg_hitSpan[cand];
   if (sp.y > sp.x) {
    refLid = cand;
    break;
   }
  }
  if (refLid < 64u && laneInBeam < 9u) {
   let refRay = rays[wgBase + refLid];
   let refO = refRay.ro_clipA.xyz - m.origin.xyz;
   let refD = refRay.rd_clipB.xyz;
   let refSpan = wg_hitSpan[refLid];
   var sRef = refSpan.x + (refSpan.y - refSpan.x) * f32(laneInBeam) / 8.0;
   if (laneInBeam == 0u) {
    sRef = refSpan.x;
    wg_refDir[beamId] = vec4f(refD, f32(refLid));
   }
   if (laneInBeam == 8u) { sRef = refSpan.y; }
   wg_refPhi[beamId * 9u + laneInBeam] = evalPhi(m, refO, refD, sRef);
  }
  workgroupBarrier();

  if (validHit) {
   let L1 = (abs(m.amps.x * m.freqs.x) + abs(m.amps.y * m.freqs.y) + abs(m.amps.z * m.freqs.z)) * 1.00002;
   let twoWmax = 2.0 * (abs(m.amps.x) + abs(m.amps.y) + abs(m.amps.z)) * 1.00002;
   let M = m.curv.x * d.x * d.x * 1.00002;
   let err = m.curv.y;

   var beamBinMask = 0xffu;
   var beamCertifiedSkip = false;
   if (lid != refLid) {
    let refD = wg_refDir[beamId].xyz;
    let refSpan = wg_hitSpan[refLid];
    let refA = refSpan.x;
    let refB = refSpan.y;
    var delta: array<f32, 9>;
    for (var k = 0u; k < 9u; k++) {
     var s = a + (b - a) * f32(k) / 8.0;
     var sRef = refA + (refB - refA) * f32(k) / 8.0;
     if (k == 0u) { s = a; sRef = refA; }
     if (k == 8u) { s = b; sRef = refB; }
     let dz = abs(s * d.z - sRef * refD.z);
     let dx = abs(s * d.x - sRef * refD.x);
     delta[k] = dz + min(L1 * dx, twoWmax) + 0.0001;
    }
    beamBinMask = 0u;
    var allPosH = true;
    for (var bin = 0u; bin < 8u; bin++) {
     var a0 = a + (b - a) * f32(bin) / 8.0;
     var b0 = a + (b - a) * f32(bin + 1u) / 8.0;
     if (bin == 0u) { a0 = a; }
     if (bin == 7u) { b0 = b; }
     let h = b0 - a0;
     if (!(h > 0.0)) { allPosH = false; }
     let pa = wg_refPhi[beamId * 9u + bin];
     let pb = wg_refPhi[beamId * 9u + bin + 1u];
     let da = delta[bin];
     let db = delta[bin + 1u];
     let scale = 1.0 + (abs(pa) + da + m.limits.w) + (abs(pb) + db + m.limits.w) + abs(a0) + abs(b0);
     let error = err + 0.000001 * scale;
     let bow = 0.125 * M * h * h + error;
     let certOut = (min(pa - da, pb - db) > m.limits.w + bow) || (max(pa + da, pb + db) < -m.limits.w - bow);
     if (!certOut) {
      beamBinMask |= (1u << bin);
     }
    }
    if (allPosH && beamBinMask == 0u && (abs(wg_refPhi[beamId * 9u]) - delta[0] > m.limits.w + err)) {
     beamCertifiedSkip = true;
    }
   }

   if (beamCertifiedSkip) {
    if (work + 24u > cfg.budget) {
     reason |= 1u;
    } else {
     work += 24u;
     memCount[member] = 0u;
    }
   } else {
    var phi_knots: array<f32, 9>;
    if (lid == refLid) {
     for (var k = 0u; k < 9u; k++) {
      phi_knots[k] = wg_refPhi[beamId * 9u + k];
     }
    } else {
     for (var k = 0u; k < 9u; k++) {
      var needed = (k == 0u);
      if (k < 8u && ((beamBinMask & (1u << k)) != 0u)) { needed = true; }
      if (k > 0u && ((beamBinMask & (1u << (k - 1u))) != 0u)) { needed = true; }
      if (needed) {
       var s = a + (b - a) * f32(k) / 8.0;
       if (k == 0u) { s = a; }
       if (k == 8u) { s = b; }
       phi_knots[k] = evalPhi(m, o, d, s);
      }
     }
    }
    var allPositiveH = true;
    let mask0 = exactLevelActiveMaskFiltered(m, d, a, b, -m.limits.w, &phi_knots, beamBinMask, &allPositiveH);
    let mask1 = exactLevelActiveMaskFiltered(m, d, a, b, 0.0, &phi_knots, beamBinMask, &allPositiveH);
    let mask2 = exactLevelActiveMaskFiltered(m, d, a, b, m.limits.w, &phi_knots, beamBinMask, &allPositiveH);
    if (allPositiveH && (mask0 | mask1 | mask2) == 0u && abs(phi_knots[0]) > m.limits.w + err) {
     if (work + 24u > cfg.budget) {
      reason |= 1u;
     } else {
      work += 24u;
      memCount[member] = 0u;
     }
    } else {
     var nb = 1u; knots[0] = a;
     var bands: array<vec2f, 32>; var nr = 0u; var zeroBands = 0u;
     roots(m, o, d, a, b, -m.limits.w, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, false, mask0);
     if (reason == 0u) {
      roots(m, o, d, a, b, 0.0, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, true, mask1);
     }
     if (reason == 0u) {
      roots(m, o, d, a, b, m.limits.w, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, false, mask2);
     }
     if (reason == 0u) {
      if (d.x != 0.0) { let x = -o.x / d.x; if (x > a && x < b) { knots[nb] = x; nb++; } }
      if (d.y != 0.0) {
       let y = -o.y / d.y; if (y > a && y < b) { knots[nb] = y; nb++; }
       let k = (m.limits.z - o.y) / d.y; if (k > a && k < b) { knots[nb] = k; nb++; }
      }
      knots[nb] = b; nb++;
      for (var i = 1u; i < nb; i++) {
       let v = knots[i]; var j = i; loop { if (j == 0u || knots[j - 1u] <= v) { break; } knots[j] = knots[j - 1u]; j--; }
       knots[j] = v;
      }
      let memberStart = count; var previousShortBand = false; var previousPositive = false;
      for (var i = 1u; i < nb; i++) {
       let l = knots[i - 1u]; let r = knots[i]; if (!(r > l)) { continue; }
       let mid = l + 0.5 * (r - l); var uncertain = false; var protectedBoundary = false;
       if (d.x != 0.0) { protectedBoundary = l == -o.x / d.x; }
       if (d.y != 0.0) { protectedBoundary = protectedBoundary || l == -o.y / d.y || l == (m.limits.z - o.y) / d.y; }
       for (var k = 0u; k < nr; k++) {
        uncertain = uncertain || (l < bands[k].y && r > bands[k].x);
        if ((zeroBands & (1u << k)) != 0u && (l == bands[k].x || l == bands[k].y)) { protectedBoundary = true; }
       }
       if (uncertain || abs(evalPhi(m, o, d, mid)) <= m.limits.w + m.curv.y) {
        if (count > memberStart && mayAbsorb(pieces[count - 1u], vec2f(l, r),
             previousShortBand, previousPositive, uncertain, protectedBoundary)) {
         pieces[count - 1u].y = r; previousShortBand = false; previousPositive = true;
        } else {
         if (count >= 48u) { reason |= 8u; break; }
         pieces[count] = vec2f(l, r);
         setMember(&pieceMembers, count, member);
         count++; previousShortBand = uncertain && r - l <= 0.02; previousPositive = !uncertain;
        }
       }
      }
      memCount[member] = count - memStart[member];
     }
    }
   }
  }
 }

 if (!validLane) { return; }
 if (reason != 0u) { rays[id].counts = vec4u(subBase, 0, overBase, FALLBACK | reason); return; }
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
 var singles = 0u; var overs = 0u;
 var pIdx = vec4u(0u);
 for (var i = 1u; i < ne && reason == 0u; i++) {
  let a = knots[i - 1u]; let b = knots[i]; if (!(b > a)) { continue; }
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
   if (singles + n > cfg.maxSubPerRay) { reason |= 16u; break; }
   let member = firstTrailingBit(mask);
   for (var k = 0u; k < n; k++) {
    let l = select(a + f32(k) * (b - a) / f32(n), a, k == 0u);
    let r = select(a + f32(k + 1u) * (b - a) / f32(n), b, k + 1u == n);
    subpanels[subBase + singles] = Subpanel(vec2f(l, r), vec2u(member, 0)); singles++;
   }
  } else {
   if (overs >= cfg.maxOverPerRay) { reason |= 32u; break; }
   overlaps[overBase + overs] = Overlap(vec2f(a, b), vec2u(mask, 0)); overs++;
  }
 }
 if (reason != 0u) { rays[id].counts = vec4u(subBase, 0, overBase, FALLBACK | reason); }
 else { rays[id].counts = vec4u(subBase, singles, overBase, overs); }
}
'''


def make_6de_source(base: str, root: Path, variant: str) -> str:
    m6dd = load(root, 'ray_transport_gpu_6dd_continuation')
    wide = variant.endswith('-wide')
    s = m6dd.exact_monotone_bowskip_source(base, root, wide=wide)

    use_beam_producer = variant in ('tile-beam', 'tile-beam-wide', 'combined-ancients-wide')
    use_pair_consumer = variant in ('cnoise3-pair', 'cnoise3-pair-wide', 'combined-ancients-wide')

    if use_beam_producer:
        old_compact = re.search(r'static const char\* kFastPathCompactShader = R"WGSL\((.*?)\)WGSL";', s, re.S)[0]
        new_compact = 'static const char* kFastPathCompactShader = R"WGSL(\n' + make_tile_beam_producer_wgsl().strip() + '\n)WGSL";'
        if s.count(old_compact) != 1:
            raise ValueError('kFastPathCompactShader hook')
        s = s.replace(old_compact, new_compact, 1)

    if use_pair_consumer:
        # Replace kFusedShader with kUnfusedPairShader and route ONLY Mode 3's consumer dispatch to cpFused (which compiles kFusedShader)
        old_fused = re.search(r'// Pipeline 2: Fused A\* Shader.*?static const char\* kFusedShader = R"WGSL\((.*?)\)WGSL";', s, re.S)[0]
        new_fused = '// Pipeline 2: Ancient-Approved Plane-Staged cnoise3Pair Consumer\nstatic const char* kFusedShader = R"WGSL(\n' + make_cnoise3_pair_consumer_wgsl().strip() + '\n)WGSL";'
        if s.count(old_fused) != 1:
            raise ValueError('kFusedShader hook')
        s = s.replace(old_fused, new_fused, 1)

        # Add consumerPair bindgroup to Tile and use cpFused + tile.consumerPair ONLY in mode == 3
        old_tile = 'struct Tile {uint32_t start,n;WGPUBindGroup producer,forced,consumer,fallback,fastFull,forcedFastFull,fastCompact,forcedFastCompact;};'
        new_tile = 'struct Tile {uint32_t start,n;WGPUBindGroup producer,forced,consumer,consumerPair,fallback,fastFull,forcedFastFull,fastCompact,forcedFastCompact;};'
        if s.count(old_tile) != 1:
            raise ValueError('Tile struct hook')
        s = s.replace(old_tile, new_tile, 1)

        old_cons_bg = '''            auto consumer=group(cpUnfused,{entry(0,ub,0,sizeof(u)),entry(1,mBuf,0,mediaBytes),entry(2,pBuf,0,paramBytes),
                entry(3,rBuf,rayOffset,uint64_t(n)*sizeof(GpuRayHeader)),entry(4,tileSingles,0,tileSingleBytes),
                entry(5,tileOverlaps,0,tileOverlapBytes),entry(6,outBuf,outputOffset,uint64_t(n)*sizeof(GpuResult))});'''
        new_cons_bg = old_cons_bg + '''
            auto consumerPair=group(cpFused,{entry(0,ub,0,sizeof(u)),entry(1,mBuf,0,mediaBytes),entry(2,pBuf,0,paramBytes),
                entry(3,rBuf,rayOffset,uint64_t(n)*sizeof(GpuRayHeader)),entry(4,tileSingles,0,tileSingleBytes),
                entry(5,tileOverlaps,0,tileOverlapBytes),entry(6,outBuf,outputOffset,uint64_t(n)*sizeof(GpuResult))});'''
        if s.count(old_cons_bg) != 1:
            raise ValueError('consumer bindgroup hook')
        s = s.replace(old_cons_bg, new_cons_bg, 1)

        old_push = 'tiles.push_back({start,n,producer(cb),producer(fb),consumer,current(fallbackCP),'
        new_push = 'tiles.push_back({start,n,producer(cb),producer(fb),consumer,consumerPair,current(fallbackCP),'
        if s.count(old_push) != 1:
            raise ValueError('tiles.push_back hook')
        s = s.replace(old_push, new_push, 1)

        old_m3 = '''                else if(mode==3)for(const auto& tile:tiles){
                    dispatch(cpFastCompact,forced?tile.forcedFastCompact:tile.fastCompact,tile.n);
                    dispatch(cpUnfused,tile.consumer,tile.n);dispatch(fallbackCP,tile.fallback,tile.n);
                }'''
        new_m3 = '''                else if(mode==3)for(const auto& tile:tiles){
                    dispatch(cpFastCompact,forced?tile.forcedFastCompact:tile.fastCompact,tile.n);
                    dispatch(cpFused,tile.consumerPair,tile.n);dispatch(fallbackCP,tile.fallback,tile.n);
                }'''
        if s.count(old_m3) != 1:
            raise ValueError('mode==3 dispatch hook')
        s = s.replace(old_m3, new_m3, 1)

        old_rel = 'for(auto& tile:tiles)for(auto bg:{tile.producer,tile.forced,tile.consumer,tile.fallback,tile.fastFull,tile.forcedFastFull,tile.fastCompact,tile.forcedFastCompact})wgpuBindGroupRelease(bg);'
        new_rel = 'for(auto& tile:tiles)for(auto bg:{tile.producer,tile.forced,tile.consumer,tile.consumerPair,tile.fallback,tile.fastFull,tile.forcedFastFull,tile.fastCompact,tile.forcedFastCompact})wgpuBindGroupRelease(bg);'
        if s.count(old_rel) != 1:
            raise ValueError('tile bindgroup release hook')
        s = s.replace(old_rel, new_rel, 1)

    desc_map = {
        'cnoise3-pair': '§6de Isolated Candidate 1: Plane-staged cnoise3Pair(Pa, Pb) gated on env_m>0 && env_p>0 (32,768 cap)',
        'cnoise3-pair-wide': '§6de Isolated Candidate 1: Plane-staged cnoise3Pair(Pa, Pb) gated on env_m>0 && env_p>0 (65,536 single-tile cap)',
        'tile-beam': '§6de Isolated Candidate 2: Cooperative 16-Ray Sub-Beam Lipschitz Support Certificate (32,768 cap)',
        'tile-beam-wide': '§6de Isolated Candidate 2: Cooperative 16-Ray Sub-Beam Lipschitz Support Certificate (65,536 single-tile cap)',
        'combined-ancients-wide': '§6de Composed Ancients: Cooperative 16-Ray Sub-Beam Certificate + Plane-Staged cnoise3Pair (65,536 single-tile cap)',
    }
    s = s.replace(
        '"Exact level-specific 3-level bow mask + whole-member bow skip + depth-0 monotone register peel; original 20/32/96/48 capacities"',
        json.dumps(desc_map[variant]),
        1,
    )
    return s


def build_variant(root: Path, build_root: Path, work: Path, variant: str):
    if work.exists() or root == work or root in work.parents:
        raise ValueError('fresh external work required')
    v = {x.targets[0].id: ast.literal_eval(x.value)
         for x in ast.parse((root / 'scratch/probes/ray_transport_metal_diagnostic.py').read_text()).body
         if isinstance(x, ast.Assign) and isinstance(x.value, ast.Constant) and isinstance(x.value.value, str)}
    cpp = make_6de_source(v['CPP_SOURCE'], root, variant)

    work.mkdir(parents=True, exist_ok=False)
    (work / 'probe.cpp').write_text(cpp)
    (work / 'source.json').write_text(json.dumps({
        'variant': variant,
        'cpp_sha256': sha(work / 'probe.cpp'),
        'zone_sha256': v['ZONE_HASH'],
        'build_root': str(build_root),
    }, indent=2) + '\n')

    b = build_root / 'build'
    db = json.loads((b / 'compile_commands.json').read_text())
    entry = next(x for x in db if x['file'].endswith('/entry.cpp') and 'earthcall_webgpu.dir' in x['command'])
    cmd = shlex.split(entry['command'])
    cmd[cmd.index('-c') + 1] = str(work / 'probe.cpp')
    cmd[cmd.index('-o') + 1] = str(work / 'probe.o')
    cmd += ['-std=c++20', '-O3']
    subprocess.run(cmd, cwd=b, check=True)

    link_cmd = shlex.split((b / 'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text())
    link_cmd[next(i for i, x in enumerate(link_cmd) if x.endswith('/src/entry.cpp.o'))] = str(work / 'probe.o')
    link_cmd[link_cmd.index('-o') + 1] = str(work / 'probe')
    subprocess.run(link_cmd, cwd=b, check=True)
    return v['ZONE_HASH']


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root', type=Path, required=True)
    p.add_argument('--build-root', type=Path, default=Path('/private/tmp/earthcall-rt-pure-build'))
    p.add_argument('--work', type=Path, required=True)
    p.add_argument('--case', type=Path, default=Path('/private/tmp/earthcall-wide-case/manifest.json'))
    p.add_argument('--variant', choices=[
        'cnoise3-pair', 'cnoise3-pair-wide',
        'tile-beam', 'tile-beam-wide',
        'combined-ancients-wide',
    ], required=True)
    p.add_argument('--build-only', action='store_true')
    args = p.parse_args()

    root = args.root.resolve()
    work = args.work.resolve()
    case = args.case.resolve()
    wide = load(root, 'ray_transport_gpu_wide_stream')
    m6dd = load(root, 'ray_transport_gpu_6dd_continuation')

    zone = root / 'saves/zones/Northern Veil/zone.ecform'
    z = sha(zone)
    meta, rays, refs = wide.verify_case(root, case, z)
    case_sha = sha(case)
    expected = build_variant(root, args.build_root.resolve(), work, args.variant)
    if z != expected or sha(zone) != z or sha(case) != case_sha:
        raise ValueError('pinned source/case drift')
    if args.build_only:
        print('PASS §6de build-only:', args.variant, work / 'probe', 'cpp_sha256:', sha(work / 'probe.cpp'))
        return

    output = work / 'result.json'
    env = dict(os.environ,
               EC_POSE_RAYS=meta['rays'],
               EC_POSE_REFERENCE=meta['reference'],
               EC_POSE_CLOCK=str(rays['time']),
               EC_MATCH_REPEAT='1',
               EC_GPU_BUDGET='1024',
               EC_GPU_SUB_CAP='64',
               EC_GPU_OVER_CAP='16',
               EC_GPU_MARGIN='.001')
    env.pop('EC_GPU_GEOMETRY_CASE', None)
    with (work / 'native.log').open('x') as f:
        subprocess.run([str(work / 'probe'), str(output)], cwd=root, env=env, stdout=f, stderr=subprocess.STDOUT, check=True)
    if sha(zone) != z or sha(case) != case_sha:
        raise ValueError('source/case changed during execution')
    wide.verify_case(root, case, z)
    report = json.loads(output.read_text())
    rows = json.loads(Path(str(output) + '-rows.json').read_text())
    baseline = json.loads(Path(str(output) + '-baseline-rows.json').read_text())
    ref = json.loads(Path(meta['reference']).read_text())
    checked = m6dd.validate_6dd(root, report, rows, baseline, rays, ref)
    (work / 'independent.json').write_text(json.dumps(checked, indent=2) + '\n')
    print('PASS §6de native', args.variant, 'qualification:', work / 'independent.json')


if __name__ == '__main__':
    main()
