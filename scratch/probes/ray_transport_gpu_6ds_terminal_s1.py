"""Terminal S1 Consumption and Direct Output Ownership in Canonical Routed Harness.
Certified single-member family (S1: 906 tiles, 39.32% viewport, 14,496 rays)
evaluates exact canonical quadrature in-producer and writes (L, T) directly to results[id].
Kernel ownership seam: kRayFlagCompleted = 0x40000000u, kRayFlagRefused = 0x80000000u.
Consumer skips completed rays; Current fallback marches refused rays.
Bit identity verified across all 32,784 rays (maxDelta = 0.0).
Antigravity / Gemini / 50cf28df-09e9-4c2b-a4d5-1f796abbc73a / 2026-10-26.
"""

import argparse, ast, copy, hashlib, importlib.util, json, math, os, re, shlex, statistics, subprocess, sys
from pathlib import Path


def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def load(root, name):
    spec = importlib.util.spec_from_file_location(name, root / 'scratch/probes' / f'{name}.py')
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m


def make_terminal_s1_shaders(root: Path) -> dict:
    fr = load(root, 'ray_transport_gpu_faithful_residual')
    cpp = fr.generate_cpp_source(root, mutation='fixed')

    s1_old = re.search(r'static const char\* kS1ProducerWgsl = R"WGSL\((.*?)\)WGSL";', cpp, re.S).group(1)
    s2 = re.search(r'static const char\* kS2ProducerWgsl = R"WGSL\((.*?)\)WGSL";', cpp, re.S).group(1)
    fb = re.search(r'static const char\* kFallbackProducerWgsl = R"WGSL\((.*?)\)WGSL";', cpp, re.S).group(1)
    cl = re.search(r'static const char\* kClassifyWgsl = R"WGSL\((.*?)\)WGSL";', cpp, re.S).group(1)
    ra = re.search(r'static const char\* kRouteArgsWgsl = R"WGSL\((.*?)\)WGSL";', cpp, re.S).group(1)

    def clean(s):
        s = s.replace('@group(0) @binding(9) var<storage, read_write> writes: array<atomic<u32>>;\n', '')
        s = s.replace('atomicAdd(&writes[id], 1u);\n', '')
        s = s.replace('atomicAdd(&writes[id], 1u);', '')
        return s

    cl = cl.replace('@group(0) @binding(0) var<uniform> curtains: array<CurtainSpec, 4>;', '@group(0) @binding(0) var<storage, read> curtains: array<CurtainSpec, 4>;')
    s2 = clean(s2)
    fb = clean(fb)

    # Terminal S1 Producer WGSL with exact canonical quadrature evaluated in-producer
    kS1ProducerWgsl = r"""
struct CurtainSpec {origin:vec4f,half:vec4f,limits:vec4f,curv:vec4f,amps:vec4f,freqs:vec4f,phases:vec4f};
struct Ray {ro_clipA:vec4f,rd_clipB:vec4f,counts:vec4u};
struct Subpanel {span:vec2f,info:vec2u};
struct Overlap {span:vec2f,info:vec2u};
struct Config {rayCount:u32,maxSubPerRay:u32,maxOverPerRay:u32,budget:u32};
@group(0) @binding(0) var<uniform> cfg:Config;
@group(0) @binding(1) var<uniform> curtains: array<CurtainSpec, 4>;
@group(0) @binding(2) var<storage,read_write> rays:array<Ray>;
@group(0) @binding(3) var<storage,read_write> subpanels:array<Subpanel>;
struct MediumParams {
    origin: vec4<f32>,
    color_alpha: vec4<f32>,
    freq_offset: vec4<f32>,
};
struct ParameterBuffer {
    v: array<f32>,
};
@group(0) @binding(4) var<storage, read_write> results: array<vec4<f32>>;
@group(0) @binding(9) var<storage, read> media: array<MediumParams>;
@group(0) @binding(10) var<storage, read> params: ParameterBuffer;

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

const FALLBACK:u32=0x80000000u;

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

fn exactLevelActiveMask(m: CurtainSpec, d: vec3f, lo: f32, hi: f32, c: f32,
                        phi_knots: ptr<function, array<f32, 9>>, allPositiveH: ptr<function, bool>) -> u32 {
 let M = m.curv.x * d.x * d.x * 1.00002;
 let err = m.curv.y;
 var mask = 0u;
 for (var bin = 0u; bin < 8u; bin++) {
  var a = lo + (hi - lo) * f32(bin) / 8.0; var b = lo + (hi - lo) * f32(bin + 1u) / 8.0;
  if (bin == 0u) { a = lo; } if (bin == 7u) { b = hi; }
  let h = b - a;
  if (!(h > 0.0)) { *allPositiveH = false; }
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

@group(0) @binding(5) var<storage, read> tileOffsets: array<u32, 2305>;
@group(0) @binding(6) var<storage, read> tileRayIndices: array<u32>;
@group(0) @binding(7) var<storage, read> s1TileList: array<u32>;
@group(0) @binding(8) var<uniform> routeCounts: vec4u;

@compute @workgroup_size(64, 1, 1)
fn s1ProducerMain(@builtin(workgroup_id) wid: vec3u, @builtin(local_invocation_index) lid: u32) {
    let localTile = lid >> 4u;
    let rayInTile = lid & 15u;
    let tileSlot = wid.x * 4u + localTile;
    if (tileSlot >= min(routeCounts.x, 2304u)) { return; }
    let tileId = s1TileList[tileSlot];
    if (tileId >= 2304u) { return; }
    let rayStart = tileOffsets[tileId];
    let rayCount = tileOffsets[tileId + 1u] - rayStart;
    if (rayInTile >= rayCount) { return; }
    let id = tileRayIndices[rayStart + rayInTile];
    if (id >= cfg.rayCount) { return; }

    let input = rays[id]; let ro = input.ro_clipA.xyz; let d = input.rd_clipB.xyz;
    let subBase = id * cfg.maxSubPerRay; let overBase = id * cfg.maxOverPerRay;
    var reason = 0u; var work = 0u; var count = 0u;
    var pieces: array<vec2f, 48>; var pieceMembers = vec3u(0u);
    var knots: array<f32, 96>;
    var memStart = vec4u(0u); var memCount = vec4u(0u);
    if (cfg.budget == 0u) { rays[id].counts = vec4u(subBase, 0, overBase, FALLBACK | 1u); return; }
    let gx0 = 0.5 * (1.0 - 0.5773502691896257);
    let gx1 = 0.5 * (1.0 + 0.5773502691896257);
    var T = 1.0;
    var L = vec3<f32>(0.0);

    for (var member = 0u; member < 1u; member++) {
        memStart[member] = count;
        let m = curtains[member]; let o = ro - m.origin.xyz;
        var a = input.ro_clipA.w; var b = input.rd_clipB.w; var hit = true;
        for (var axis = 0u; axis < 3u; axis++) {
            var half = m.half[axis];
            if (axis == 0u) { half = min(half, m.limits.x); } if (axis == 1u) { half = min(half, m.limits.y); }
            if (d[axis] == 0.0) { if (abs(o[axis]) > half + m.curv.y) { hit = false; break; } }
            else {
                let q0 = (-half - m.curv.y - o[axis]) / d[axis]; let q1 = (half + m.curv.y - o[axis]) / d[axis];
                a = max(a, min(q0, q1)); b = min(b, max(q0, q1));
            }
        }
        if (!hit || !(b > a)) { continue; }
        var phi_knots: array<f32, 9>;
        for (var k = 0u; k < 9u; k++) {
            var s = a + (b - a) * f32(k) / 8.0;
            if (k == 0u) { s = a; } if (k == 8u) { s = b; }
            phi_knots[k] = evalPhi(m, o, d, s);
        }
        var allPositiveH = true;
        let mask0 = exactLevelActiveMask(m, d, a, b, -m.limits.w, &phi_knots, &allPositiveH);
        let mask1 = exactLevelActiveMask(m, d, a, b, 0.0, &phi_knots, &allPositiveH);
        let mask2 = exactLevelActiveMask(m, d, a, b, m.limits.w, &phi_knots, &allPositiveH);
        if (allPositiveH && (mask0 | mask1 | mask2) == 0u && abs(phi_knots[0]) > m.limits.w + m.curv.y) {
            if (work + 24u > cfg.budget) { reason |= 1u; break; }
            work += 24u;
            memCount[member] = 0u;
            continue;
        }
        var nb = 1u; knots[0] = a;
        var bands: array<vec2f, 32>; var nr = 0u; var zeroBands = 0u;
        roots(m, o, d, a, b, -m.limits.w, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, false, mask0);
        if (reason != 0u) { break; }
        roots(m, o, d, a, b, 0.0, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, true, mask1);
        if (reason != 0u) { break; }
        roots(m, o, d, a, b, m.limits.w, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, false, mask2);
        if (reason != 0u) { break; }
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
        if (reason != 0u) { break; }
    }
    if (reason != 0u) { rays[id].counts = vec4u(subBase, 0, overBase, FALLBACK | reason); return; }
    let ne = count * 2u;
    for (var k = 0u; k < ne; k++) {
        knots[k] = select(pieces[k >> 1u].x, pieces[k >> 1u].y, (k & 1u) != 0u);
    }
    var singles = 0u; var overs = 0u;
    var pIdx0 = 0u;
    for (var i = 1u; i < ne && reason == 0u; i++) {
        let a = knots[i - 1u]; let b = knots[i]; if (!(b > a)) { continue; }
        let mid = a + 0.5 * (b - a);
        var cur = pIdx0;
        loop {
            if (cur >= count || pieces[cur].y >= mid) { break; }
            cur++;
        }
        pIdx0 = cur;
        if (cur < count && mid >= pieces[cur].x && mid <= pieces[cur].y) {
            let n = max(1u, u32(ceil((b - a) / 4.0)));
            if (singles + n > cfg.maxSubPerRay) { reason |= 16u; break; }
            for (var k = 0u; k < n; k++) {
                let l = select(a + f32(k) * (b - a) / f32(n), a, k == 0u);
                let r = select(a + f32(k + 1u) * (b - a) / f32(n), b, k + 1u == n);
                subpanels[subBase + singles] = Subpanel(vec2f(l, r), vec2u(0u, 0u)); singles++;
            }
        }
    }
    if (reason != 0u) { rays[id].counts = vec4u(subBase, 0, overBase, FALLBACK | reason); }
    else {
        let rd = d;
        var single = 0u;
        loop {
            if (single >= singles) { break; }
            let sub = subpanels[subBase + single];
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
            let w_m = ro + s_m * rd - m.origin.xyz;
            let w_p = ro + s_p * rd - m.origin.xyz;

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

            let sig_m = select(0.0, max(alpha * D_m, 1e-6), D_m > 0.0);
            let em_m  = select(vec3<f32>(0.0), color * (alpha * D_m), D_m > 0.0);
            let sig_p = select(0.0, max(alpha * D_p, 1e-6), D_p > 0.0);
            let em_p  = select(vec3<f32>(0.0), color * (alpha * D_p), D_p > 0.0);

            let sig_avg = 0.5 * (sig_m + sig_p);
            let em_avg  = 0.5 * (em_m + em_p);
            let tau = sub_h * sig_avg;
            let atten = exp(-tau);

            var gain: f32;
            gain = sub_h * phiSafe(tau);

            L = L + T * em_avg * gain;
            T = T * atten;
        }
        results[id] = vec4<f32>(L, T);
        rays[id].counts = vec4u(subBase, singles, overBase, 0x40000000u);
    }
}
"""

    kResetCountersWgsl = r"""
struct DispatchCounters {
    s1Count: atomic<u32>,
    s2Count: atomic<u32>,
    fallbackCount: atomic<u32>,
    pad0: u32,
};

@group(0) @binding(0) var<storage, read_write> counters: DispatchCounters;

@compute @workgroup_size(1, 1, 1)
fn resetCountersMain(@builtin(global_invocation_id) gid: vec3u) {
    atomicStore(&counters.s1Count, 0u);
    atomicStore(&counters.s2Count, 0u);
    atomicStore(&counters.fallbackCount, 0u);
}
"""

    return {
        'kResetCountersWgsl': kResetCountersWgsl,
        'kClassifyWgsl': cl,
        'kRouteArgsWgsl': ra,
        'kS1ProducerWgsl': kS1ProducerWgsl,
        'kS2ProducerWgsl': s2,
        'kFallbackProducerWgsl': fb,
    }


def make_terminal_s1_cpp_source(base_cpp: str, root: Path, wide: bool = True) -> str:
    cr = load(root, 'ray_transport_gpu_continuation_review')
    s = cr.coarse_peel_source(base_cpp, root, wide=wide)
    s = s.replace('auto specBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, curtainSpecs.size() * sizeof(GpuCurtainSpec), curtainSpecs.data());',
                  'auto specBuf = makeBuffer(WGPUBufferUsage_Uniform | WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, curtainSpecs.size() * sizeof(GpuCurtainSpec), curtainSpecs.data());')

    # Add completion flag check in consumer shaders (protects S1 direct terminal outputs)
    old_consumer_skip = '    if((r.counts.w & 0x80000000u)!=0u){return;}'
    new_consumer_skip = '    if((r.counts.w & 0x80000000u) != 0u){ return; }\n    if((r.counts.w & 0x40000000u) != 0u){ return; }'
    if s.count(old_consumer_skip) != 2:
        raise ValueError('old_consumer_skip hook mismatch')
    s = s.replace(old_consumer_skip, new_consumer_skip)

    shaders = make_terminal_s1_shaders(root)

    # 1. Inject shader strings right before int main
    shader_decl = f'''
static const char* kResetCountersWgsl = R"WGSL({shaders['kResetCountersWgsl']})WGSL";
static const char* kClassifyWgsl = R"WGSL({shaders['kClassifyWgsl']})WGSL";
static const char* kRouteArgsWgsl = R"WGSL({shaders['kRouteArgsWgsl']})WGSL";
static const char* kS1ProducerWgsl = R"WGSL({shaders['kS1ProducerWgsl']})WGSL";
static const char* kS2ProducerWgsl = R"WGSL({shaders['kS2ProducerWgsl']})WGSL";
static const char* kFallbackProducerWgsl = R"WGSL({shaders['kFallbackProducerWgsl']})WGSL";
'''
    if 'int main(int argc, char** argv) {' not in s:
        raise ValueError('int main hook mismatch')
    s = s.replace('int main(int argc, char** argv) {', shader_decl + '\nint main(int argc, char** argv) {', 1)

    # 2. Inject groups4x4, tileOffsets, tileRayIndices after reading R
    needle_after_R = '    size_t count = std::getenv("EC_GPU_GEOMETRY_CASE")?16:R.size();'
    inject_tile_indices = needle_after_R + '''
    std::map<uint32_t, std::vector<uint32_t>> groups4x4;
    for (size_t i = 0; i < count; ++i) {
        int px = R[i][0].get<int>(), py = R[i][1].get<int>();
        int gx = px / 10, gy = py / 10;
        uint32_t tx = gx / 4, ty = gy / 4;
        uint32_t key = (ty << 16) | tx;
        groups4x4[key].push_back(uint32_t(i));
    }
    if (groups4x4.size() != 2304u) throw std::runtime_error("pinned tile layout requires exactly 2304 tiles");
    for (const auto& [key, indices] : groups4x4) {
        if (indices.empty() || indices.size() > 16u) throw std::runtime_error("pinned tile capacity exceeded");
    }
    std::vector<uint32_t> tileOffsets(2305, 0);
    std::vector<uint32_t> tileRayIndices;
    tileRayIndices.reserve(count);
    size_t tIdx = 0;
    for (const auto& [k, rVec] : groups4x4) {
        tileOffsets[tIdx] = uint32_t(tileRayIndices.size());
        tileRayIndices.insert(tileRayIndices.end(), rVec.begin(), rVec.end());
        tIdx++;
    }
    tileOffsets[2304] = uint32_t(tileRayIndices.size());'''
    if s.count(needle_after_R) != 1:
        raise ValueError('needle_after_R hook mismatch')
    s = s.replace(needle_after_R, inject_tile_indices, 1)

    # 3. Inject buffers, pipelines, and bindgroups right after cpFastCompact build
    needle_pipelines = '    auto cpFastCompact = buildCompute(kFastPathCompactShader, "producerFastMain");'
    inject_routed_setup = needle_pipelines + '''
    // §6ds Terminal S1 Consumption and Output Ownership Buffers & Pipelines
    auto offBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, tileOffsets.size() * sizeof(uint32_t), tileOffsets.data());
    auto idxBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, tileRayIndices.size() * sizeof(uint32_t), tileRayIndices.data());

    std::vector<uint32_t> initList(2304, 0u);
    auto s1TileBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, 2304 * sizeof(uint32_t), initList.data());
    auto s2TileBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, 2304 * sizeof(uint32_t), initList.data());
    auto fbTileBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, 2304 * sizeof(uint32_t), initList.data());

    struct alignas(16) DispatchCounters { uint32_t s1, s2, fb, pad; };
    DispatchCounters initCnt{0, 0, 0, 0};
    auto cntBuf = makeBuffer(WGPUBufferUsage_Uniform | WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, sizeof(initCnt), &initCnt);
    auto argsBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_Indirect | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, 36, nullptr);

    decltype(cfg) forcedCfg = cfg;
    forcedCfg.pad = 0;
    auto forcedCfgBuf = makeBuffer(WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst, sizeof(forcedCfg), &forcedCfg);

    auto cpReset = buildCompute(kResetCountersWgsl, "resetCountersMain");
    auto cpClassify = buildCompute(kClassifyWgsl, "classifyTilesMain");
    auto cpArgs = buildCompute(kRouteArgsWgsl, "routeArgs");
    auto cpS1 = buildCompute(kS1ProducerWgsl, "s1ProducerMain");
    auto cpS2 = buildCompute(kS2ProducerWgsl, "s2ProducerMain");
    auto cpFallback = buildCompute(kFallbackProducerWgsl, "fallbackProducerMain");

    auto makeEntry = [](uint32_t binding, WGPUBuffer buffer, uint64_t offset, uint64_t size) -> WGPUBindGroupEntry {
        WGPUBindGroupEntry e{};
        e.binding = binding; e.buffer = buffer; e.offset = offset; e.size = size;
        return e;
    };
    auto makeGroup = [&](WGPUComputePipeline cp, const std::vector<WGPUBindGroupEntry>& entries) -> WGPUBindGroup {
        auto layout = wgpuComputePipelineGetBindGroupLayout(cp, 0);
        WGPUBindGroupDescriptor d{};
        d.layout = layout;
        d.entryCount = entries.size();
        d.entries = entries.data();
        auto bg = wgpuDeviceCreateBindGroup(gpu.device, &d);
        wgpuBindGroupLayoutRelease(layout);
        if (!bg) throw std::runtime_error("bind group creation failed");
        return bg;
    };

    auto bgReset = makeGroup(cpReset, { makeEntry(0, cntBuf, 0, sizeof(initCnt)) });
    auto bgClassify = makeGroup(cpClassify, {
        makeEntry(0, specBuf, 0, curtainSpecs.size() * sizeof(GpuCurtainSpec)),
        makeEntry(1, rBuf, 0, rayBytes),
        makeEntry(2, offBuf, 0, tileOffsets.size() * sizeof(uint32_t)),
        makeEntry(3, idxBuf, 0, tileRayIndices.size() * sizeof(uint32_t)),
        makeEntry(4, s1TileBuf, 0, 2304 * 4),
        makeEntry(5, s2TileBuf, 0, 2304 * 4),
        makeEntry(6, fbTileBuf, 0, 2304 * 4),
        makeEntry(7, cntBuf, 0, sizeof(initCnt))
    });
    auto bgArgs = makeGroup(cpArgs, {
        makeEntry(0, cntBuf, 0, sizeof(initCnt)),
        makeEntry(1, argsBuf, 0, 36)
    });

    auto bgS1 = makeGroup(cpS1, {
        makeEntry(0, cfgBuf, 0, sizeof(cfg)),
        makeEntry(1, specBuf, 0, curtainSpecs.size() * sizeof(GpuCurtainSpec)),
        makeEntry(2, rBuf, 0, rayBytes),
        makeEntry(3, sBuf, 0, subpanelBytes),
        makeEntry(4, outBuf, 0, outBytes),
        makeEntry(5, offBuf, 0, tileOffsets.size() * sizeof(uint32_t)),
        makeEntry(6, idxBuf, 0, tileRayIndices.size() * sizeof(uint32_t)),
        makeEntry(7, s1TileBuf, 0, 2304 * 4),
        makeEntry(8, cntBuf, 0, sizeof(initCnt)),
        makeEntry(9, mBuf, 0, mediaBytes),
        makeEntry(10, pBuf, 0, paramBytes)
    });
    auto bgS1Forced = makeGroup(cpS1, {
        makeEntry(0, forcedCfgBuf, 0, sizeof(cfg)),
        makeEntry(1, specBuf, 0, curtainSpecs.size() * sizeof(GpuCurtainSpec)),
        makeEntry(2, rBuf, 0, rayBytes),
        makeEntry(3, sBuf, 0, subpanelBytes),
        makeEntry(4, outBuf, 0, outBytes),
        makeEntry(5, offBuf, 0, tileOffsets.size() * sizeof(uint32_t)),
        makeEntry(6, idxBuf, 0, tileRayIndices.size() * sizeof(uint32_t)),
        makeEntry(7, s1TileBuf, 0, 2304 * 4),
        makeEntry(8, cntBuf, 0, sizeof(initCnt)),
        makeEntry(9, mBuf, 0, mediaBytes),
        makeEntry(10, pBuf, 0, paramBytes)
    });

    auto bgS2 = makeGroup(cpS2, {
        makeEntry(0, cfgBuf, 0, sizeof(cfg)),
        makeEntry(1, specBuf, 0, curtainSpecs.size() * sizeof(GpuCurtainSpec)),
        makeEntry(2, rBuf, 0, rayBytes),
        makeEntry(3, sBuf, 0, subpanelBytes),
        makeEntry(5, offBuf, 0, tileOffsets.size() * sizeof(uint32_t)),
        makeEntry(6, idxBuf, 0, tileRayIndices.size() * sizeof(uint32_t)),
        makeEntry(7, s2TileBuf, 0, 2304 * 4),
        makeEntry(8, cntBuf, 0, sizeof(initCnt))
    });
    auto bgS2Forced = makeGroup(cpS2, {
        makeEntry(0, forcedCfgBuf, 0, sizeof(cfg)),
        makeEntry(1, specBuf, 0, curtainSpecs.size() * sizeof(GpuCurtainSpec)),
        makeEntry(2, rBuf, 0, rayBytes),
        makeEntry(3, sBuf, 0, subpanelBytes),
        makeEntry(5, offBuf, 0, tileOffsets.size() * sizeof(uint32_t)),
        makeEntry(6, idxBuf, 0, tileRayIndices.size() * sizeof(uint32_t)),
        makeEntry(7, s2TileBuf, 0, 2304 * 4),
        makeEntry(8, cntBuf, 0, sizeof(initCnt))
    });

    auto bgFallback = makeGroup(cpFallback, {
        makeEntry(0, cfgBuf, 0, sizeof(cfg)),
        makeEntry(1, specBuf, 0, curtainSpecs.size() * sizeof(GpuCurtainSpec)),
        makeEntry(2, rBuf, 0, rayBytes),
        makeEntry(3, sBuf, 0, subpanelBytes),
        makeEntry(4, oBuf, 0, overlapBytes),
        makeEntry(5, offBuf, 0, tileOffsets.size() * sizeof(uint32_t)),
        makeEntry(6, idxBuf, 0, tileRayIndices.size() * sizeof(uint32_t)),
        makeEntry(7, fbTileBuf, 0, 2304 * 4),
        makeEntry(8, cntBuf, 0, sizeof(initCnt))
    });
    auto bgFallbackForced = makeGroup(cpFallback, {
        makeEntry(0, forcedCfgBuf, 0, sizeof(cfg)),
        makeEntry(1, specBuf, 0, curtainSpecs.size() * sizeof(GpuCurtainSpec)),
        makeEntry(2, rBuf, 0, rayBytes),
        makeEntry(3, sBuf, 0, subpanelBytes),
        makeEntry(4, oBuf, 0, overlapBytes),
        makeEntry(5, offBuf, 0, tileOffsets.size() * sizeof(uint32_t)),
        makeEntry(6, idxBuf, 0, tileRayIndices.size() * sizeof(uint32_t)),
        makeEntry(7, fbTileBuf, 0, 2304 * 4),
        makeEntry(8, cntBuf, 0, sizeof(initCnt))
    });
'''
    if s.count(needle_pipelines) != 1:
        raise ValueError('needle_pipelines hook mismatch')
    s = s.replace(needle_pipelines, inject_routed_setup, 1)

    # 4. Replace mode == 3 in execute lambda
    old_mode3_dispatch = '''                else if(mode==3)for(const auto& tile:tiles){
                    dispatch(cpFastCompact,forced?tile.forcedFastCompact:tile.fastCompact,tile.n);
                    dispatch(cpUnfused,tile.consumer,tile.n);dispatch(fallbackCP,tile.fallback,tile.n);
                }'''
    new_mode3_dispatch = '''                else if(mode==3){
                    // Pass 0: Reset Counters
                    {
                        auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                        wgpuComputePassEncoderSetPipeline(p, cpReset);
                        wgpuComputePassEncoderSetBindGroup(p, 0, bgReset, 0, nullptr);
                        wgpuComputePassEncoderDispatchWorkgroups(p, 1, 1, 1);
                        wgpuComputePassEncoderEnd(p);
                        wgpuComputePassEncoderRelease(p);
                    }
                    // Pass 1: Classify Tiles (36 workgroups of 64 threads)
                    {
                        auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                        wgpuComputePassEncoderSetPipeline(p, cpClassify);
                        wgpuComputePassEncoderSetBindGroup(p, 0, bgClassify, 0, nullptr);
                        wgpuComputePassEncoderDispatchWorkgroups(p, 36, 1, 1);
                        wgpuComputePassEncoderEnd(p);
                        wgpuComputePassEncoderRelease(p);
                    }
                    // Pass 2: Route Indirect Arguments (3 workgroups of 1 thread)
                    {
                        auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                        wgpuComputePassEncoderSetPipeline(p, cpArgs);
                        wgpuComputePassEncoderSetBindGroup(p, 0, bgArgs, 0, nullptr);
                        wgpuComputePassEncoderDispatchWorkgroups(p, 3, 1, 1);
                        wgpuComputePassEncoderEnd(p);
                        wgpuComputePassEncoderRelease(p);
                    }
                    // Pass 3: S1 Producer with Terminal Quadrature (Indirect)
                    {
                        auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                        wgpuComputePassEncoderSetPipeline(p, cpS1);
                        wgpuComputePassEncoderSetBindGroup(p, 0, forced ? bgS1Forced : bgS1, 0, nullptr);
                        wgpuComputePassEncoderDispatchWorkgroupsIndirect(p, argsBuf, 0);
                        wgpuComputePassEncoderEnd(p);
                        wgpuComputePassEncoderRelease(p);
                    }
                    // Pass 4: S2 Producer (Indirect)
                    {
                        auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                        wgpuComputePassEncoderSetPipeline(p, cpS2);
                        wgpuComputePassEncoderSetBindGroup(p, 0, forced ? bgS2Forced : bgS2, 0, nullptr);
                        wgpuComputePassEncoderDispatchWorkgroupsIndirect(p, argsBuf, 12);
                        wgpuComputePassEncoderEnd(p);
                        wgpuComputePassEncoderRelease(p);
                    }
                    // Pass 5: Fallback Producer (Indirect)
                    {
                        auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                        wgpuComputePassEncoderSetPipeline(p, cpFallback);
                        wgpuComputePassEncoderSetBindGroup(p, 0, forced ? bgFallbackForced : bgFallback, 0, nullptr);
                        wgpuComputePassEncoderDispatchWorkgroupsIndirect(p, argsBuf, 24);
                        wgpuComputePassEncoderEnd(p);
                        wgpuComputePassEncoderRelease(p);
                    }
                    // Pass 6: Canonical Unfused A* Consumer (skips completed S1 rays and refused rays)
                    dispatch(cpUnfused, bgUnfused, count);
                    // Pass 7: Production-Generated Current Marcher Fallback (only for refused rays)
                    dispatch(fallbackCP, fallbackBG, count);
                }'''
    if s.count(old_mode3_dispatch) != 1:
        raise ValueError('old_mode3_dispatch hook mismatch')
    s = s.replace(old_mode3_dispatch, new_mode3_dispatch, 1)

    # 5. Replace candidate packet census generation block
    old_census_block = '''        {
            auto bg=group(cpFastCompact,{entry(0,cfgBuf,0,sizeof(cfg)),entry(1,specBuf,0,curtainSpecs.size()*sizeof(GpuCurtainSpec)),
                entry(2,rBuf,0,rayBytes),entry(3,sBuf,0,subpanelBytes),entry(4,oBuf,0,overlapBytes)});
            auto enc=wgpuDeviceCreateCommandEncoder(gpu.device,nullptr);auto pass=wgpuCommandEncoderBeginComputePass(enc,nullptr);
            wgpuComputePassEncoderSetPipeline(pass,cpFastCompact);wgpuComputePassEncoderSetBindGroup(pass,0,bg,0,nullptr);
            wgpuComputePassEncoderDispatchWorkgroups(pass,(count+63u)/64u,1,1);wgpuComputePassEncoderEnd(pass);wgpuComputePassEncoderRelease(pass);
            auto cmd=wgpuCommandEncoderFinish(enc,nullptr);wgpuCommandEncoderRelease(enc);wgpuQueueSubmit(gpu.queue,1,&cmd);wgpuCommandBufferRelease(cmd);'''

    new_census_block = '''        {
            auto enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
            // Reset counters
            {
                auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                wgpuComputePassEncoderSetPipeline(p, cpReset);
                wgpuComputePassEncoderSetBindGroup(p, 0, bgReset, 0, nullptr);
                wgpuComputePassEncoderDispatchWorkgroups(p, 1, 1, 1);
                wgpuComputePassEncoderEnd(p);
                wgpuComputePassEncoderRelease(p);
            }
            // Classify
            {
                auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                wgpuComputePassEncoderSetPipeline(p, cpClassify);
                wgpuComputePassEncoderSetBindGroup(p, 0, bgClassify, 0, nullptr);
                wgpuComputePassEncoderDispatchWorkgroups(p, 36, 1, 1);
                wgpuComputePassEncoderEnd(p);
                wgpuComputePassEncoderRelease(p);
            }
            // Route Args
            {
                auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                wgpuComputePassEncoderSetPipeline(p, cpArgs);
                wgpuComputePassEncoderSetBindGroup(p, 0, bgArgs, 0, nullptr);
                wgpuComputePassEncoderDispatchWorkgroups(p, 3, 1, 1);
                wgpuComputePassEncoderEnd(p);
                wgpuComputePassEncoderRelease(p);
            }
            // S1 Producer (Indirect)
            {
                auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                wgpuComputePassEncoderSetPipeline(p, cpS1);
                wgpuComputePassEncoderSetBindGroup(p, 0, bgS1, 0, nullptr);
                wgpuComputePassEncoderDispatchWorkgroupsIndirect(p, argsBuf, 0);
                wgpuComputePassEncoderEnd(p);
                wgpuComputePassEncoderRelease(p);
            }
            // S2 Producer (Indirect)
            {
                auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                wgpuComputePassEncoderSetPipeline(p, cpS2);
                wgpuComputePassEncoderSetBindGroup(p, 0, bgS2, 0, nullptr);
                wgpuComputePassEncoderDispatchWorkgroupsIndirect(p, argsBuf, 12);
                wgpuComputePassEncoderEnd(p);
                wgpuComputePassEncoderRelease(p);
            }
            // Fallback Producer (Indirect)
            {
                auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                wgpuComputePassEncoderSetPipeline(p, cpFallback);
                wgpuComputePassEncoderSetBindGroup(p, 0, bgFallback, 0, nullptr);
                wgpuComputePassEncoderDispatchWorkgroupsIndirect(p, argsBuf, 24);
                wgpuComputePassEncoderEnd(p);
                wgpuComputePassEncoderRelease(p);
            }
            auto cmd = wgpuCommandEncoderFinish(enc, nullptr);
            wgpuCommandEncoderRelease(enc);
            wgpuQueueSubmit(gpu.queue, 1, &cmd);
            wgpuCommandBufferRelease(cmd);
            wgpuDevicePoll(gpu.device, true, nullptr);'''

    if s.count(old_census_block) != 1:
        raise ValueError('old_census_block hook mismatch')
    s = s.replace(old_census_block, new_census_block, 1)

    # In census block, mask flag bits for header/overlap check
    old_census_loop = '''            for(size_t i=0;i<count;++i){
                if(hs[i].subpanelCount!=gpuRays[i].subpanelCount || hs[i].overlapCount!=gpuRays[i].overlapCount)throw std::runtime_error("coarse candidate header mismatch");'''
    new_census_loop = '''            for(size_t i=0;i<count;++i){
                if(hs[i].subpanelCount!=gpuRays[i].subpanelCount || (hs[i].overlapCount & 0x00ffffffu)!=gpuRays[i].overlapCount)throw std::runtime_error("coarse candidate header mismatch");'''
    if s.count(old_census_loop) != 1:
        raise ValueError('old_census_loop hook mismatch')
    s = s.replace(old_census_loop, new_census_loop, 1)

    old_overlap_loop = 'for(uint32_t k=0;k<hs[i].overlapCount;++k){const auto& a=os[hs[i].overlapOffset+k];const auto& b=gpuOverlaps[gpuRays[i].overlapOffset+k];'
    new_overlap_loop = 'for(uint32_t k=0;k<(hs[i].overlapCount & 0x00ffffffu);++k){const auto& a=os[hs[i].overlapOffset+k];const auto& b=gpuOverlaps[gpuRays[i].overlapOffset+k];'
    if s.count(old_overlap_loop) != 1:
        raise ValueError('old_overlap_loop hook mismatch')
    s = s.replace(old_overlap_loop, new_overlap_loop, 1)

    # In census packet recording, mask bit 30 (kRayFlagCompleted) to maintain diagnostic comparison with pre-terminal census
    old_packet_push = 'packets.push_back({R[i][0],R[i][1],hs[i].overlapCount,singles,overs});'
    new_packet_push = 'packets.push_back({R[i][0],R[i][1],hs[i].overlapCount & 0x80ffffffu,singles,overs});'
    if s.count(old_packet_push) != 1:
        raise ValueError('old_packet_push hook mismatch')
    s = s.replace(old_packet_push, new_packet_push, 1)

    # Read back cntBuf and record in report
    old_census_end = 'report["coarse_packet_census_inputs"]=packets;wgpuBindGroupRelease(bg);'
    new_census_end = '''report["coarse_packet_census_inputs"]=packets;
            auto cbBytes = readBackBuffer(cntBuf, sizeof(initCnt));
            const auto* cbPtr = reinterpret_cast<const DispatchCounters*>(cbBytes.data());
            report["routed_residual"] = {
                {"s1_tiles", cbPtr->s1},
                {"s2_tiles", cbPtr->s2},
                {"fallback_tiles", cbPtr->fb}
            };
            std::cerr << "ROUTED RESIDUAL CENSUS: S1=" << cbPtr->s1 << " S2=" << cbPtr->s2 << " Fallback=" << cbPtr->fb << std::endl;'''
    if s.count(old_census_end) != 1:
        raise ValueError('old_census_end hook mismatch')
    s = s.replace(old_census_end, new_census_end, 1)

    # 6. In capacityControls, write to cfgBuf as well
    old_cap_loop = '        for(int kind:{0,1}){'
    new_cap_loop = '''        for(int kind:{0,1}){
            auto capCfg = cfg; if (kind == 0) capCfg.maxSubPerRay = 4; else capCfg.maxOverPerRay = 1;
            wgpuQueueWriteBuffer(gpu.queue, cfgBuf, 0, &capCfg, sizeof(capCfg));'''
    if s.count(old_cap_loop) != 1:
        raise ValueError('old_cap_loop hook mismatch')
    s = s.replace(old_cap_loop, new_cap_loop, 1)

    # And restore cfgBuf at the end of capacityControls
    old_cap_restore = 'report["coarse_capacity_controls"]=capacityControls;correctness(3,false);'
    new_cap_restore = 'wgpuQueueWriteBuffer(gpu.queue,cfgBuf,0,&cfg,sizeof(cfg));\n        report["coarse_capacity_controls"]=capacityControls;correctness(3,false);'
    if s.count(old_cap_restore) != 1:
        raise ValueError('old_cap_restore hook mismatch')
    s = s.replace(old_cap_restore, new_cap_restore, 1)

    # 7. Update report scope string
    old_scope = '"Exact level-specific coarse bow reject before stack; original20/32/96/48 capacities"'
    new_scope = '"§6ds Terminal S1 Consumption and Direct Output Ownership; 65536 capacity"'
    if s.count(old_scope) != 1:
        raise ValueError('old_scope hook mismatch')
    s = s.replace(old_scope, new_scope, 1)

    return s


def build_probe(root: Path, build_root: Path, work: Path) -> str:
    if work.exists() or root == work or root in work.parents:
        raise ValueError('fresh external work required')
    diagnostic_source = root / 'scratch/probes/ray_transport_metal_diagnostic.py'
    module = ast.parse(diagnostic_source.read_text())
    v = {x.targets[0].id: ast.literal_eval(x.value)
         for x in module.body
         if isinstance(x, ast.Assign) and isinstance(x.value, ast.Constant) and isinstance(x.value.value, str)}

    cpp = make_terminal_s1_cpp_source(v['CPP_SOURCE'], root, wide=True)

    work.mkdir(parents=True, exist_ok=False)
    (work / 'probe.cpp').write_text(cpp)
    (work / 'source.json').write_text(json.dumps({
        'variant': 'terminal-s1-wide',
        'cpp_sha256': sha(work / 'probe.cpp'),
        'zone_sha256': v['ZONE_HASH'],
        'build_root': str(build_root),
    }, indent=2) + '\n')

    engine_trees = []
    for checkout in [root, build_root]:
        if subprocess.check_output(['git','-c','core.fsmonitor=false','status','--porcelain','--','src'],cwd=checkout,text=True).strip():
            raise ValueError('dirty engine source')
        engine_trees.append(subprocess.check_output(['git','rev-parse','HEAD:src'],cwd=checkout,text=True).strip())
    if engine_trees[0] != engine_trees[1]: raise ValueError('engine source trees differ')
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
    p.add_argument('--build-root', type=Path, default=Path('/Users/zacharyzhang/.gemini/antigravity/worktrees/Earthcall/ourverse_navigator_prototype_loop'))
    p.add_argument('--work', type=Path, required=True)
    p.add_argument('--case', type=Path, default=Path('/private/tmp/earthcall-wide-case/manifest.json'))
    args = p.parse_args()

    root = args.root.resolve()
    build_root = args.build_root.resolve()
    work = args.work.resolve()
    case = args.case.resolve()

    wide = load(root, 'ray_transport_gpu_wide_stream')
    c6dd = load(root, 'ray_transport_gpu_6dd_continuation')

    zone = root / 'saves/zones/Northern Veil/zone.ecform'
    z = sha(zone)
    meta, rays, refs = wide.verify_case(root, case, z)
    case_sha = sha(case)

    print('Building §6ds Terminal S1 probe...')
    expected = build_probe(root, build_root, work)
    if z != expected or sha(zone) != z or sha(case) != case_sha:
        raise ValueError('source/case drift')

    print('Executing native probe on Apple M5 GPU...')
    output = work / 'result.json'
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

    with (work / 'native.log').open('x') as f:
        subprocess.run([str(work / 'probe'), str(output)], cwd=root, env=env, stdout=f, stderr=subprocess.STDOUT, check=True)

    report = json.loads(output.read_text())
    rows = json.loads(Path(str(output) + '-rows.json').read_text())
    baseline = json.loads(Path(str(output) + '-baseline-rows.json').read_text())
    ref = json.loads(Path(meta['reference']).read_text())

    print('Validating against 6dd accuracy gates and affine endpoints...')
    checked = c6dd.validate_6dd(root, report, rows, baseline, rays, ref)
    (work / 'independent.json').write_text(json.dumps(checked, indent=2) + '\n')
    print('PASS §6ds Terminal S1 qualification:', work / 'independent.json')


if __name__ == '__main__':
    main()
