#pragma once
// Qualified §6et backend kernels; no authored field evaluation is replaced.
// Producer admission is mathematical ClampedWaveSupport, never Zone identity.
// Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-28.
namespace routed_wgsl {
inline constexpr const char* kResetCountersWgsl=R"WGSL(
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
)WGSL";
inline constexpr const char* kClassifyWgsl=R"WGSL(
struct CurtainSpec {
    origin: vec4f,
    half: vec4f,
    limits: vec4f,
    curv: vec4f,
    amps: vec4f,
    freqs: vec4f,
    phases: vec4f,
};

struct Ray {
    ro_clipA: vec4f,
    rd_clipB: vec4f,
    counts: vec4u,
};

struct DispatchCounters {
    s1Count: atomic<u32>,
    s2Count: atomic<u32>,
    fallbackCount: atomic<u32>,
    pad0: u32,
};

@group(0) @binding(0) var<storage, read> curtains: array<CurtainSpec, 4>;
@group(0) @binding(1) var<storage, read> rays: array<Ray>;
@group(0) @binding(2) var<storage, read> tileOffsets: array<u32>;
@group(0) @binding(3) var<storage, read> tileRayIndices: array<u32>;
@group(0) @binding(4) var<storage, read_write> s1TileList: array<u32>;
@group(0) @binding(5) var<storage, read_write> s2TileList: array<u32>;
@group(0) @binding(6) var<storage, read_write> fallbackTileList: array<u32>;
@group(0) @binding(7) var<storage, read_write> counters: DispatchCounters;

var<workgroup> wgUxMin: array<f32, 64>;
var<workgroup> wgUxMax: array<f32, 64>;
var<workgroup> wgHit: array<u32, 64>;
var<workgroup> wgWKnots: array<f32, 36>;
var<workgroup> wgTileUxMin: array<f32, 4>;
var<workgroup> wgTileUxMax: array<f32, 4>;
var<workgroup> wgTileAnyHit: array<u32, 4>;
var<workgroup> wgEnter: array<f32, 64>;
var<workgroup> wgExit: array<f32, 64>;
var<workgroup> wgState: array<u32, 64>;

fn evalWave(cur: CurtainSpec, x: f32) -> f32 {
    return cur.amps.x * sin(cur.freqs.x * x + cur.phases.x) +
           cur.amps.y * cos(cur.freqs.y * x + cur.phases.y) +
           cur.amps.z * sin(cur.freqs.z * x + cur.phases.z);
}

@compute @workgroup_size(64, 1, 1)
fn classifyTilesMain(@builtin(workgroup_id) wid: vec3u, @builtin(local_invocation_index) lid: u32) {
    let totalTiles = arrayLength(&tileOffsets) - 1u;
    let localTile = lid >> 4u;
    let lane = lid & 15u;
    let tileId = wid.x * 4u + localTile;
    let base = localTile * 16u;
    let validTile = tileId < totalTiles;

    var rayStart = 0u;
    var rayCount = 0u;
    if (validTile) {
        rayStart = tileOffsets[tileId];
        rayCount = min(tileOffsets[tileId + 1u] - rayStart, 16u);
    }
    let validRay = validTile && (lane < rayCount);

    var ro = vec3f(0.0);
    var rd = vec3f(0.0);
    var clipA = 0.0;
    var clipB = 0.0;
    if (validRay) {
        let rIdx = tileRayIndices[rayStart + lane];
        let ray = rays[rIdx];
        ro = ray.ro_clipA.xyz;
        clipA = ray.ro_clipA.w;
        rd = ray.rd_clipB.xyz;
        clipB = ray.rd_clipB.w;
    }

    var activeMemberMask = 0u;
    var tileCanOverlap = false;
    var enterArr: array<f32, 4>;
    var exitArr: array<f32, 4>;
    var memberHit: array<bool, 4>;

    for (var m = 0u; m < 4u; m++) {
        let cur = curtains[m];
        let oRel = ro - cur.origin.xyz;
        var a = clipA;
        var b = clipB;
        var hit = validRay;

        if (validRay) {
            for (var axis = 0; axis < 3; axis++) {
                var half = cur.half[axis];
                if (axis == 0) { half = min(half, cur.limits.x); }
                if (axis == 1) { half = min(half, cur.limits.y); }
                let d = rd[axis];
                let o = oRel[axis];
                if (d == 0.0) {
                    if (abs(o) > half + cur.curv.y) { hit = false; break; }
                } else {
                    let q0 = (-half - cur.curv.y - o) / d;
                    let q1 = (half + cur.curv.y - o) / d;
                    a = max(a, min(q0, q1));
                    b = min(b, max(q0, q1));
                }
            }
        }

        let rHit = hit && (b > a);
        var myUxMin = 1e9f;
        var myUxMax = -1e9f;
        if (rHit) {
            let u0 = oRel.x + a * rd.x;
            let u1 = oRel.x + b * rd.x;
            myUxMin = min(myUxMin, min(u0, u1));
            myUxMax = max(myUxMax, max(u0, u1));
        }
        wgUxMin[lid] = myUxMin;
        wgUxMax[lid] = myUxMax;
        wgHit[lid] = select(0u, 1u, rHit);
        workgroupBarrier();

        if (lane < 9u) {
            var tUxMin = 1e9f;
            var tUxMax = -1e9f;
            var tAnyHit = false;
            for (var i = 0u; i < 16u; i++) {
                if (wgHit[base + i] != 0u) {
                    tAnyHit = true;
                    tUxMin = min(tUxMin, wgUxMin[base + i]);
                    tUxMax = max(tUxMax, wgUxMax[base + i]);
                }
            }
            if (tAnyHit && tUxMax > tUxMin) {
                let ux = tUxMin + f32(lane) / 8.0 * (tUxMax - tUxMin);
                wgWKnots[localTile * 9u + lane] = evalWave(cur, ux);
            }
            if (lane == 0u) {
                wgTileUxMin[localTile] = tUxMin;
                wgTileUxMax[localTile] = tUxMax;
                wgTileAnyHit[localTile] = select(0u, 1u, tAnyHit);
            }
        }
        workgroupBarrier();

        let anyRayHit = wgTileAnyHit[localTile] != 0u;
        let uxMin = wgTileUxMin[localTile];
        let uxMax = wgTileUxMax[localTile];
        let hasUxSpan = anyRayHit && (uxMax > uxMin);

        var myEnter = 1e9f;
        var myExit = -1e9f;
        var myState = 0u;

        if (hasUxSpan && rHit) {
            let dx = rd.x;
            if (abs(dx) < 1e-6) {
                myState = 2u;
                myEnter = a;
                myExit = b;
            } else {
                let h_ux = (uxMax - uxMin) / 8.0;
                let bow = 0.125 * cur.curv.x * h_ux * h_ux;
                let thresh = bow + cur.limits.w + 0.001;
                let knotBase = localTile * 9u;

                for (var bin = 0u; bin < 8u; bin++) {
                    let u0 = uxMin + f32(bin) / 8.0 * (uxMax - uxMin);
                    let u1 = uxMin + f32(bin + 1u) / 8.0 * (uxMax - uxMin);
                    let su0 = (u0 - oRel.x) / dx;
                    let su1 = (u1 - oRel.x) / dx;
                    let phi0 = oRel.z + su0 * rd.z + wgWKnots[knotBase + bin];
                    let phi1 = oRel.z + su1 * rd.z + wgWKnots[knotBase + bin + 1u];

                    let sMin = min(su0, su1);
                    let sMax = max(su0, su1);
                    let cLo = max(a, sMin);
                    let cHi = min(b, sMax);

                    if (cHi > cLo) {
                        let tA = (cLo - su0) / (su1 - su0);
                        let tB = (cHi - su0) / (su1 - su0);
                        let pA = phi0 + tA * (phi1 - phi0);
                        let pB = phi0 + tB * (phi1 - phi0);

                        if (!(min(pA, pB) > thresh || max(pA, pB) < -thresh)) {
                            myState = 1u;
                            myEnter = min(myEnter, cLo);
                            myExit = max(myExit, cHi);
                        }
                    }
                }
            }
        } else if (anyRayHit && rHit) {
            myState = 1u;
            myEnter = a;
            myExit = b;
        }

        wgEnter[lid] = myEnter;
        wgExit[lid] = myExit;
        wgState[lid] = myState;
        workgroupBarrier();

        if (lane == 0u && validTile) {
            var enter = 1e9f;
            var exit = -1e9f;
            var curActive = false;
            for (var i = 0u; i < 16u; i++) {
                let st = wgState[base + i];
                if (st != 0u) {
                    curActive = true;
                    enter = min(enter, wgEnter[base + i]);
                    exit = max(exit, wgExit[base + i]);
                    if (st == 2u) { break; }
                }
            }
        if (anyRayHit && uxMax > uxMin) {
            if (curActive) {
                activeMemberMask |= (1u << m);
                memberHit[m] = true;
                enterArr[m] = enter;
                exitArr[m] = exit;
            }
        } else if (anyRayHit) {
            activeMemberMask |= (1u << m);
            memberHit[m] = true;
            enterArr[m] = enter;
            exitArr[m] = exit;
        }
        }
    }

    if (!validTile || lane != 0u) { return; }

    let activeCount = countOneBits(activeMemberMask);
    if (activeCount > 1u) {
        for (var m1 = 0u; m1 < 4u; m1++) {
            if (!memberHit[m1]) { continue; }
            for (var m2 = m1 + 1u; m2 < 4u; m2++) {
                if (!memberHit[m2]) { continue; }
                let lo = max(enterArr[m1], enterArr[m2]);
                let hi = min(exitArr[m1], exitArr[m2]);
                if (hi > lo) { tileCanOverlap = true; }
            }
        }
    }

    if (activeCount == 0u) {
        // wave-proved empty: no producer work needed
    } else if (activeCount == 1u) {
        let onlyMember = firstTrailingBit(activeMemberMask);
        let slot = atomicAdd(&counters.s1Count, 1u);
        s1TileList[slot] = (onlyMember << 28u) | tileId;
    } else if (activeCount == 2u && !tileCanOverlap) {
        let m0 = firstTrailingBit(activeMemberMask);
        let m1 = firstTrailingBit(activeMemberMask & ~(1u << m0));
        let order01 = exitArr[m0] <= enterArr[m1];
        let order10 = exitArr[m1] <= enterArr[m0];
        if (order01 || order10) {
            let m1First = select(0u, 1u, order10 && !order01);
            let slot = atomicAdd(&counters.s2Count, 1u);
            s2TileList[slot] = (m0 << 28u) | (m1 << 24u) | (m1First << 20u) | tileId;
        } else {
            let slot = atomicAdd(&counters.fallbackCount, 1u);
            fallbackTileList[slot] = tileId;
        }
    } else {
        let slot = atomicAdd(&counters.fallbackCount, 1u);
        fallbackTileList[slot] = tileId;
    }
}
)WGSL";
inline constexpr const char* kRouteArgsWgsl=R"WGSL(
@group(0) @binding(0) var<storage, read> counts: array<u32>;
@group(0) @binding(1) var<storage, read_write> args: array<u32>;
@compute @workgroup_size(1) fn routeArgs(@builtin(global_invocation_id) id: vec3u) {
    if (id.x >= 3u) { return; }
    let n = counts[id.x];
    args[3u * id.x] = (n + 3u) / 4u;
    args[3u * id.x + 1u] = 1u;
    args[3u * id.x + 2u] = 1u;
}
)WGSL";
inline constexpr const char* kS1UnfusedProducerWgsl=R"WGSL(

struct CurtainSpec {origin:vec4f,half:vec4f,limits:vec4f,curv:vec4f,amps:vec4f,freqs:vec4f,phases:vec4f};
struct Ray {ro_clipA:vec4f,rd_clipB:vec4f,counts:vec4u};
struct Subpanel {span:vec2f,info:vec2u};
struct Config {rayCount:u32,maxSubPerRay:u32,maxOverPerRay:u32,budget:u32};
@group(0) @binding(0) var<uniform> cfg:Config;
@group(0) @binding(1) var<uniform> curtains: array<CurtainSpec, 4>;
@group(0) @binding(2) var<storage,read_write> rays:array<Ray>;
@group(0) @binding(3) var<storage,read_write> subpanels:array<Subpanel>;
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

@group(0) @binding(5) var<storage, read> tileOffsets: array<u32>;
@group(0) @binding(6) var<storage, read> tileRayIndices: array<u32>;
@group(0) @binding(7) var<storage, read> s1TileList: array<u32>;
@group(0) @binding(8) var<storage, read> routeCounts: array<u32>;

@compute @workgroup_size(64, 1, 1)
fn s1ProducerMain(@builtin(workgroup_id) wid: vec3u, @builtin(local_invocation_index) lid: u32) {
    let totalTiles = arrayLength(&tileOffsets) - 1u;
    let localTile = lid >> 4u;
    let rayInTile = lid & 15u;
    let tileSlot = wid.x * 4u + localTile;
    if (tileSlot >= min(routeCounts[0u], totalTiles)) { return; }
    let packedTile = s1TileList[tileSlot];
    let tileId = packedTile & 0x0FFFFFFFu;
    let activeMember = packedTile >> 28u;
    if (tileId >= totalTiles) { return; }
    let rayStart = tileOffsets[tileId];
    let rayCount = tileOffsets[tileId + 1u] - rayStart;
    if (rayInTile >= rayCount) { return; }
    let id = tileRayIndices[rayStart + rayInTile];
    if (id >= cfg.rayCount) { return; }

 let input = rays[id];
    if((input.counts.w & 0x40000000u)!=0u){return;}
 let ro = input.ro_clipA.xyz; let d = input.rd_clipB.xyz;
 var reason = 0u; var work = 0u; var count = 0u;
 var pieces: array<vec2f, 48>;
 var knots: array<f32, 96>;
 if (cfg.budget == 0u) { rays[id].counts = vec4u(0, 0, 0, FALLBACK | 1u); return; }
 for (var _i = 0u; _i < 1u; _i++) {
  let m = curtains[activeMember]; let o = ro - m.origin.xyz;
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
     count++; previousShortBand = uncertain && r - l <= 0.02; previousPositive = !uncertain;
    }
   }
  }
  if (reason != 0u) { break; }
 }
 if (reason != 0u) { rays[id].counts = vec4u(0, 0, 0, FALLBACK | reason); return; }
 var singles = 0u;
 for (var p = 0u; p < count; p++) {
  let a = pieces[p].x; let b = pieces[p].y;
  let n = max(1u, u32(ceil((b - a) / 4.0)));
  if (singles + n > cfg.maxSubPerRay) { reason |= 16u; break; }
  for (var k = 0u; k < n; k++) {
   let l = select(a + f32(k) * (b - a) / f32(n), a, k == 0u);
   let r = select(a + f32(k + 1u) * (b - a) / f32(n), b, k + 1u == n);
   subpanels[singles * cfg.rayCount + id] = Subpanel(vec2f(l, r), vec2u(activeMember, 0u)); singles++;
  }
 }
 if (reason != 0u) { rays[id].counts = vec4u(0, 0, 0, FALLBACK | reason); }
 else { rays[id].counts = vec4u(0, singles, 0, 0u); }
}
)WGSL";
inline constexpr const char* kS2ProducerWgsl=R"WGSL(

struct CurtainSpec {origin:vec4f,half:vec4f,limits:vec4f,curv:vec4f,amps:vec4f,freqs:vec4f,phases:vec4f};
struct Ray {ro_clipA:vec4f,rd_clipB:vec4f,counts:vec4u};
struct Subpanel {span:vec2f,info:vec2u};
struct Config {rayCount:u32,maxSubPerRay:u32,maxOverPerRay:u32,budget:u32};
@group(0) @binding(0) var<uniform> cfg:Config;
@group(0) @binding(1) var<uniform> curtains: array<CurtainSpec, 4>;
@group(0) @binding(2) var<storage,read_write> rays:array<Ray>;
@group(0) @binding(3) var<storage,read_write> subpanels:array<Subpanel>;
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

@group(0) @binding(5) var<storage, read> tileOffsets: array<u32>;
@group(0) @binding(6) var<storage, read> tileRayIndices: array<u32>;
@group(0) @binding(7) var<storage, read> s2TileList: array<u32>;
@group(0) @binding(8) var<storage, read> routeCounts: array<u32>;

@compute @workgroup_size(64, 1, 1)
fn s2ProducerMain(@builtin(workgroup_id) wid: vec3u, @builtin(local_invocation_index) lid: u32) {
    let totalTiles = arrayLength(&tileOffsets) - 1u;
    let localTile = lid >> 4u;
    let rayInTile = lid & 15u;
    let tileSlot = wid.x * 4u + localTile;
    if (tileSlot >= min(routeCounts[1u], totalTiles)) { return; }
    let packedTile = s2TileList[tileSlot];
    let tileId = packedTile & 0x000FFFFFu;
    let m1First = ((packedTile >> 20u) & 1u) != 0u;
    let rawM0 = (packedTile >> 28u) & 0xFu;
    let rawM1 = (packedTile >> 24u) & 0xFu;
    if (tileId >= totalTiles) { return; }
    let rayStart = tileOffsets[tileId];
    let rayCount = tileOffsets[tileId + 1u] - rayStart;
    if (rayInTile >= rayCount) { return; }
    let id = tileRayIndices[rayStart + rayInTile];
    if (id >= cfg.rayCount) { return; }

    let input = rays[id];
    if((input.counts.w & 0x40000000u)!=0u){return;}
 let ro = input.ro_clipA.xyz; let d = input.rd_clipB.xyz;
    var reason = 0u; var work = 0u; var count = 0u;
    var pieces: array<vec2f, 48>;
    var knots: array<f32, 96>;
    if (cfg.budget == 0u) { rays[id].counts = vec4u(0, 0, 0, FALLBACK | 1u); return; }

    var count0 = 0u;
    for (var memberIdx = 0u; memberIdx < 2u; memberIdx++) {
        let member = select(rawM0, rawM1, memberIdx == 1u);
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
        if (!hit || !(b > a)) {
            if (memberIdx == 0u) { count0 = count; }
            continue;
        }
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
            if (memberIdx == 0u) { count0 = count; }
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
        let memberStart = count;
        var previousShortBand = false; var previousPositive = false;
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
                    count++; previousShortBand = uncertain && r - l <= 0.02; previousPositive = !uncertain;
                }
            }
        }
        if (memberIdx == 0u) { count0 = count; }
        if (reason != 0u) { break; }
    }
    if (reason != 0u) { rays[id].counts = vec4u(0, 0, 0, FALLBACK | reason); return; }
    var singles = 0u;
    for (var pass = 0u; pass < 2u; pass++) {
        let emitM1 = (pass == 0u) == m1First;
        let emitMember = select(rawM0, rawM1, emitM1);
        let pStart = select(0u, count0, emitM1);
        let pEnd = select(count0, count, emitM1);
        for (var p = pStart; p < pEnd; p++) {
            let pa = pieces[p].x; let pb = pieces[p].y;
            let n = max(1u, u32(ceil((pb - pa) / 4.0)));
            if (singles + n > cfg.maxSubPerRay) { reason |= 16u; break; }
            for (var k = 0u; k < n; k++) {
                let sl = select(pa + f32(k) * (pb - pa) / f32(n), pa, k == 0u);
                let sr = select(pa + f32(k + 1u) * (pb - pa) / f32(n), pb, k + 1u == n);
                subpanels[singles * cfg.rayCount + id] = Subpanel(vec2f(sl, sr), vec2u(emitMember, 0u));
                singles++;
            }
        }
        if (reason != 0u) { break; }
    }
    if (reason != 0u) { rays[id].counts = vec4u(0, 0, 0, FALLBACK | reason); }
    else { rays[id].counts = vec4u(0, singles, 0, 0u); }
}
)WGSL";
inline constexpr const char* kFallbackProducerWgsl=R"WGSL(

struct CurtainSpec {origin:vec4f,half:vec4f,limits:vec4f,curv:vec4f,amps:vec4f,freqs:vec4f,phases:vec4f};
struct Ray {ro_clipA:vec4f,rd_clipB:vec4f,counts:vec4u};
struct Subpanel {span:vec2f,info:vec2u};
struct Overlap {span:vec2f,info:vec2u};
struct Config {rayCount:u32,maxSubPerRay:u32,maxOverPerRay:u32,budget:u32};
@group(0) @binding(0) var<uniform> cfg:Config;
@group(0) @binding(1) var<uniform> curtains: array<CurtainSpec, 4>;
@group(0) @binding(2) var<storage,read_write> rays:array<Ray>;
@group(0) @binding(3) var<storage,read_write> subpanels:array<Subpanel>;
@group(0) @binding(4) var<storage,read_write> overlaps:array<Overlap>;
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

@group(0) @binding(5) var<storage, read> tileOffsets: array<u32>;
@group(0) @binding(6) var<storage, read> tileRayIndices: array<u32>;
@group(0) @binding(7) var<storage, read> fallbackTileList: array<u32>;
@group(0) @binding(8) var<storage, read> routeCounts: array<u32>;

@compute @workgroup_size(64, 1, 1)
fn fallbackProducerMain(@builtin(workgroup_id) wid: vec3u, @builtin(local_invocation_index) lid: u32) {
    let totalTiles = arrayLength(&tileOffsets) - 1u;
    let localTile = lid >> 4u;
    let rayInTile = lid & 15u;
    let tileSlot = wid.x * 4u + localTile;
    if (tileSlot >= min(routeCounts[2u], totalTiles)) { return; }
    let tileId = fallbackTileList[tileSlot];
    if (tileId >= totalTiles) { return; }
    let rayStart = tileOffsets[tileId];
    let rayCount = tileOffsets[tileId + 1u] - rayStart;
    if (rayInTile >= rayCount) { return; }
    let id = tileRayIndices[rayStart + rayInTile];
    if (id >= cfg.rayCount) { return; }

    
 let input = rays[id];
    if((input.counts.w & 0x40000000u)!=0u){return;}
 let ro = input.ro_clipA.xyz; let d = input.rd_clipB.xyz;
 var reason = 0u; var work = 0u; var count = 0u;
 var pieces: array<vec2f, 48>; var pieceMembers = vec3u(0u);
 var knots: array<f32, 96>;
 var memStart = vec4u(0u); var memCount = vec4u(0u);
 if (cfg.budget == 0u) { rays[id].counts = vec4u(0, 0, 0, FALLBACK | 1u); return; }
 for (var member = 0u; member < 4u; member++) {
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
 if (reason != 0u) { rays[id].counts = vec4u(0, 0, 0, FALLBACK | reason); return; }
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
    subpanels[singles * cfg.rayCount + id] = Subpanel(vec2f(l, r), vec2u(member, 0)); singles++;
   }
  } else {
   if (overs >= cfg.maxOverPerRay) { reason |= 32u; break; }
   overlaps[overs * cfg.rayCount + id] = Overlap(vec2f(a, b), vec2u(mask, 0)); overs++;
  }
 }
 if (reason != 0u) { rays[id].counts = vec4u(0, 0, 0, FALLBACK | reason); }
 else { rays[id].counts = vec4u(0, singles, 0, overs); }
}
)WGSL";
}
