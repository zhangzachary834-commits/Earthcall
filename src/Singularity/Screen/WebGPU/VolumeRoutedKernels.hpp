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

@group(0) @binding(0) var<uniform> curtains: array<CurtainSpec, 4>;
@group(0) @binding(1) var<storage, read> rays: array<Ray>;
@group(0) @binding(2) var<storage, read> tileOffsets: array<u32>;
@group(0) @binding(3) var<storage, read> tileRayIndices: array<u32>;
@group(0) @binding(4) var<storage, read_write> s1TileList: array<u32>;
@group(0) @binding(5) var<storage, read_write> s2TileList: array<u32>;
@group(0) @binding(6) var<storage, read_write> fallbackTileList: array<u32>;
@group(0) @binding(7) var<storage, read_write> counters: DispatchCounters;

fn evalWave(cur: CurtainSpec, x: f32) -> f32 {
    return cur.amps.x * sin(cur.freqs.x * x + cur.phases.x) +
           cur.amps.y * cos(cur.freqs.y * x + cur.phases.y) +
           cur.amps.z * sin(cur.freqs.z * x + cur.phases.z);
}

@compute @workgroup_size(64, 1, 1)
fn classifyTilesMain(@builtin(global_invocation_id) gid: vec3u) {
    let tileId = gid.x;
    let totalTiles = arrayLength(&tileOffsets) - 1u;
    if (tileId >= totalTiles) { return; }

    let rayStart = tileOffsets[tileId];
    let rayCount = min(tileOffsets[tileId + 1u] - rayStart, 16u);

    var activeMemberMask = 0u;
    var tileCanOverlap = false;
    var enterArr: array<f32, 4>;
    var exitArr: array<f32, 4>;
    var memberHit: array<bool, 4>;

    var tileRoClipA: array<vec4f, 16>;
    var tileRdClipB: array<vec4f, 16>;
    var rayValid: array<bool, 16>;
    var rEnter: array<vec4f, 16>;
    var rExit: array<vec4f, 16>;
    for (var i = 0u; i < rayCount; i++) {
        let rIdx = tileRayIndices[rayStart + i];
        let ray = rays[rIdx];
        tileRoClipA[i] = ray.ro_clipA;
        tileRdClipB[i] = ray.rd_clipB;
        rayValid[i] = ((ray.counts.w & 0x40000000u) == 0u) && (ray.rd_clipB.w > ray.ro_clipA.w);
        rEnter[i] = vec4f(1e9f);
        rExit[i] = vec4f(-1e9f);
    }

    for (var m = 0u; m < 4u; m++) {
        let cur = curtains[m];
        let zEnv = abs(cur.amps.x) + abs(cur.amps.y) + abs(cur.amps.z) + cur.limits.w;
        // Expand derived support only; authored physical boxes remain hard bounds.
        let halfVec = min(cur.half.xyz, vec3f(cur.limits.x, cur.limits.y, zEnv) + vec3f(cur.curv.y));
        var uxMin = 1e9f;
        var uxMax = -1e9f;
        var anyRayHit = false;
        var rA: array<f32, 16>;
        var rB: array<f32, 16>;
        var rHit: array<bool, 16>;

        for (var i = 0u; i < rayCount; i++) {
            var a = tileRoClipA[i].w;
            var b = tileRdClipB[i].w;
            var hit = rayValid[i];
            let oRel = tileRoClipA[i].xyz - cur.origin.xyz;

            if (hit) {
                for (var axis = 0; axis < 3; axis++) {
                    let half = halfVec[axis];
                    let d = tileRdClipB[i][axis];
                    let o = oRel[axis];
                    if (d == 0.0) {
                        if (abs(o) > half) { hit = false; break; }
                    } else {
                        let q0 = (-half - o) / d;
                        let q1 = (half - o) / d;
                        a = max(a, min(q0, q1));
                        b = min(b, max(q0, q1));
                    }
                }
            }

            let h = hit && (b > a);
            rA[i] = a;
            rB[i] = b;
            rHit[i] = h;

            if (h) {
                anyRayHit = true;
                let u0 = oRel.x + a * tileRdClipB[i].x;
                let u1 = oRel.x + b * tileRdClipB[i].x;
                uxMin = min(uxMin, min(u0, u1));
                uxMax = max(uxMax, max(u0, u1));
            }
        }

        if (anyRayHit && uxMax > uxMin) {
            var wKnots: array<f32, 9>;
            for (var k = 0u; k < 9u; k++) {
                let ux = uxMin + f32(k) / 8.0 * (uxMax - uxMin);
                wKnots[k] = evalWave(cur, ux);
            }
            let h_ux = (uxMax - uxMin) / 8.0;
            let bow = 0.125 * cur.curv.x * h_ux * h_ux;
            let thresh = bow + cur.limits.w + cur.curv.y + 0.001;

            var enter = 1e9f;
            var exit = -1e9f;
            var curActive = false;

            for (var i = 0u; i < rayCount; i++) {
                if (!rHit[i]) { continue; }
                let oRel = tileRoClipA[i].xyz - cur.origin.xyz;
                let dx = tileRdClipB[i].x;
                if (abs(dx) < 1e-6) {
                    curActive = true;
                    enter = min(enter, rA[i]);
                    exit = max(exit, rB[i]);
                    rEnter[i][m] = rA[i];
                    rExit[i][m] = rB[i];
                    continue;
                }

                let a = rA[i];
                let b = rB[i];
                var iEnter = 1e9f;
                var iExit = -1e9f;
                var iHit = false;

                for (var bin = 0u; bin < 8u; bin++) {
                    let u0 = uxMin + f32(bin) / 8.0 * (uxMax - uxMin);
                    let u1 = uxMin + f32(bin + 1u) / 8.0 * (uxMax - uxMin);
                    let su0 = (u0 - oRel.x) / dx;
                    let su1 = (u1 - oRel.x) / dx;
                    let phi0 = oRel.z + su0 * tileRdClipB[i].z + wKnots[bin];
                    let phi1 = oRel.z + su1 * tileRdClipB[i].z + wKnots[bin + 1u];

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
                            iHit = true;
                            let dp = pB - pA;
                            var sLo = cLo;
                            var sHi = cHi;
                            if (abs(dp) > 1e-6) {
                                let q0 = (-thresh - pA) / dp;
                                let q1 = (thresh - pA) / dp;
                                let uLo = clamp(min(q0, q1), 0.0, 1.0);
                                let uHi = clamp(max(q0, q1), 0.0, 1.0);
                                sLo = max(cLo, cLo + uLo * (cHi - cLo) - 0.2);
                                sHi = min(cHi, cLo + uHi * (cHi - cLo) + 0.2);
                            }
                            iEnter = min(iEnter, sLo);
                            iExit = max(iExit, sHi);
                        }
                    }
                }

                if (iHit) {
                    curActive = true;
                    enter = min(enter, iEnter);
                    exit = max(exit, iExit);
                    rEnter[i][m] = iEnter;
                    rExit[i][m] = iExit;
                }
            }

            if (curActive) {
                activeMemberMask |= (1u << m);
                memberHit[m] = true;
                enterArr[m] = enter;
                exitArr[m] = exit;
            }
        } else if (anyRayHit) {
            activeMemberMask |= (1u << m);
            memberHit[m] = true;
            var enter = 1e9f; var exit = -1e9f;
            for (var i = 0u; i < rayCount; i++) {
                if (rHit[i]) {
                    enter = min(enter, rA[i]);
                    exit = max(exit, rB[i]);
                    rEnter[i][m] = rA[i];
                    rExit[i][m] = rB[i];
                }
            }
            enterArr[m] = enter; exitArr[m] = exit;
        }
    }

    let activeCount = countOneBits(activeMemberMask);
    var precMask = 0u;
    if (activeCount > 1u) {
        for (var m1 = 0u; m1 < 4u; m1++) {
            if (!memberHit[m1]) { continue; }
            for (var m2 = m1 + 1u; m2 < 4u; m2++) {
                if (!memberHit[m2]) { continue; }
                var pairOrder12 = true;
                var pairOrder21 = true;
                var anyBothHit = false;
                for (var i = 0u; i < rayCount; i++) {
                    let h1 = rEnter[i][m1] <= rExit[i][m1];
                    let h2 = rEnter[i][m2] <= rExit[i][m2];
                    if (h1 && h2) {
                        anyBothHit = true;
                        if (!(rExit[i][m1] + 0.01 <= rEnter[i][m2])) { pairOrder12 = false; }
                        if (!(rExit[i][m2] + 0.01 <= rEnter[i][m1])) { pairOrder21 = false; }
                    }
                }
                if (!anyBothHit) {
                    pairOrder12 = (exitArr[m1] + 0.01 <= enterArr[m2]);
                    pairOrder21 = (exitArr[m2] + 0.01 <= enterArr[m1]);
                }
                if (pairOrder12 && !pairOrder21) {
                    precMask |= (1u << (m1 * 4u + m2));
                } else if (pairOrder21 && !pairOrder12) {
                    precMask |= (1u << (m2 * 4u + m1));
                } else {
                    tileCanOverlap = true;
                }
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
        let order01 = (precMask & (1u << (m0 * 4u + m1))) != 0u;
        let order10 = (precMask & (1u << (m1 * 4u + m0))) != 0u;
        if (order01 || order10) {
            let m1First = select(0u, 1u, order10 && !order01);
            let slot = atomicAdd(&counters.s2Count, 1u);
            s2TileList[slot] = (m0 << 28u) | (m1 << 24u) | (m1First << 20u) | tileId;
        } else {
            let slot = atomicAdd(&counters.fallbackCount, 1u);
            fallbackTileList[slot] = (activeMemberMask << 28u) | tileId;
        }
    } else if (activeCount >= 3u && !tileCanOverlap) {
        var ord = array<u32, 4>(0u, 0u, 0u, 0u);
        var inDegMask = 0u;
        for (var m = 0u; m < 4u; m++) {
            if (!memberHit[m]) { continue; }
            var inDeg = 0u;
            for (var mPrev = 0u; mPrev < 4u; mPrev++) {
                if (mPrev != m && memberHit[mPrev] && ((precMask & (1u << (mPrev * 4u + m))) != 0u)) {
                    inDeg++;
                }
            }
            if (inDeg < 4u) {
                ord[inDeg] = m;
                inDegMask |= (1u << inDeg);
            }
        }
        if (inDegMask == ((1u << activeCount) - 1u)) {
            let isFour = select(0u, 1u, activeCount == 4u);
            let slot = atomicAdd(&counters.s2Count, 1u);
            s2TileList[slot] = (1u << 21u) | (isFour << 20u) | (ord[0] << 22u) | (ord[1] << 24u) | (ord[2] << 26u) | (ord[3] << 28u) | tileId;
        } else {
            let slot = atomicAdd(&counters.fallbackCount, 1u);
            fallbackTileList[slot] = (activeMemberMask << 28u) | tileId;
        }
    } else {
        let slot = atomicAdd(&counters.fallbackCount, 1u);
        fallbackTileList[slot] = (activeMemberMask << 28u) | tileId;
    }
}
)WGSL";
inline constexpr const char* kFirstActivityMaskWgsl=R"WGSL(
struct ECWindow {control:vec4<u32>,window:vec4<u32>,flags:vec4<u32>};
struct ECRay {ro_clipA:vec4<f32>,rd_clipB:vec4<f32>,counts:vec4<u32>};
struct ECSpan {span:vec2<f32>,info:vec2<u32>};

@group(0) @binding(0) var<uniform> ecWindow:ECWindow;
@group(0) @binding(1) var<storage,read> ecRays:array<ECRay>;
@group(0) @binding(2) var<storage,read> ecSingles:array<ECSpan>;
@group(0) @binding(3) var<storage,read> ecOverlaps:array<ECSpan>;
@group(0) @binding(4) var<storage,read_write> ecFirstActivity:array<u32>;

fn markSpan(mask:ptr<function,u32>, lo:f32, hi:f32,
            quantLo:f32, quantHi:f32) -> bool {
    // C5b-2c: quantize over one host-derived conservative radial range shared
    // with the fused consumer. If the range is malformed or fails to enclose a
    // produced span, refuse the certificate instead of manufacturing absence.
    if (!(quantHi > quantLo) || quantLo < 0.0 ||
        !(hi > lo) || lo < quantLo - 1e-3 || hi > quantHi + 1e-3) {
        return false;
    }
    let inv = 1.0 / (quantHi - quantLo);
    let q0 = clamp((lo - quantLo) * inv, 0.0, 1.0);
    let q1 = clamp((hi - quantLo) * inv, 0.0, 1.0);
    let b0 = min(u32(floor(q0 * 32.0)), 31u);
    let b1 = min(u32(floor(q1 * 32.0)), 31u);
    var b = b0;
    loop {
        *mask |= (1u << b);
        if (b >= b1 || b == 31u) { break; }
        b++;
    }
    return true;
}

@compute @workgroup_size(64)
fn compactFirstActivityMain(@builtin(global_invocation_id) gid:vec3<u32>) {
    let id = gid.x;
    if (id >= ecWindow.control.x) { return; }
    let x = id % ecWindow.window.x;
    let localY = id / ecWindow.window.x;
    let y = localY + ecWindow.window.z;
    if (y >= ecWindow.window.y) { return; }
    let pixel = y * ecWindow.window.x + x;
    let ray = ecRays[id];

    // Invalid/refused route state is never interpreted as empty support.
    if ((ray.counts.w & 0xC0000000u) != 0u ||
        ray.counts.y > 64u ||
        (ray.counts.w & 0x3fffffffu) > 16u) {
        ecFirstActivity[pixel] = 0xffffffffu;
        return;
    }

    let overlapCount = ray.counts.w & 0x3fffffffu;
    if ((ray.counts.y | overlapCount) == 0u) {
        ecFirstActivity[pixel] = 0u;
        return;
    }

    if (!(ray.rd_clipB.w > ray.ro_clipA.w) ||
        ray.ro_clipA.w < -1e-4 || ray.rd_clipB.w > 2048.0001) {
        ecFirstActivity[pixel] = 0xffffffffu;
        return;
    }

    let quantLo = bitcast<f32>(ecWindow.flags.y);
    let quantHi = bitcast<f32>(ecWindow.flags.z);
    if (!(quantHi > quantLo) || quantLo < 0.0) {
        ecFirstActivity[pixel] = 0xffffffffu;
        return;
    }

    let stride = ecWindow.control.x;
    var mask = 0u;
    for (var i = 0u; i < ray.counts.y; i++) {
        let sp = ecSingles[i * stride + id];
        if (!markSpan(&mask, sp.span.x, sp.span.y, quantLo, quantHi)) {
            ecFirstActivity[pixel] = 0xffffffffu;
            return;
        }
    }
    for (var i = 0u; i < overlapCount; i++) {
        let sp = ecOverlaps[i * stride + id];
        if (!markSpan(&mask, sp.span.x, sp.span.y, quantLo, quantHi)) {
            ecFirstActivity[pixel] = 0xffffffffu;
            return;
        }
    }
    ecFirstActivity[pixel] = mask;
}
)WGSL";

inline constexpr const char* kRouteArgsWgsl=R"WGSL(
@group(0) @binding(0) var<storage, read> counts: array<u32>;
@group(0) @binding(1) var<storage, read_write> args: array<u32>;
@compute @workgroup_size(1) fn routeArgs(@builtin(global_invocation_id) id: vec3u) {
    if (id.x >= 4u) { return; }
    if (id.x < 3u) {
        args[3u * id.x] = (counts[id.x] + 3u) / 4u;
        args[3u * id.x + 1u] = 1u;
        args[3u * id.x + 2u] = 1u;
    } else {
        let totalActiveTiles = counts[0] + counts[1] + counts[2];
        args[9u] = select(0u, CAPACITY_WORKGROUPS, totalActiveTiles > 0u);
        args[10u] = 1u;
        args[11u] = 1u;
    }
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

fn putBand(lo:f32,hi:f32,bounds:ptr<function,array<f32,48>>,nb:ptr<function,u32>,
 bands:ptr<function,array<vec2f,24>>,nr:ptr<function,u32>,reason:ptr<function,u32>){
 if(*nr>0u){
  let last=(*bands)[*nr-1u];
  if(lo<=last.y && hi>=last.x){
   let merged=vec2f(min(lo,last.x),max(hi,last.y));
   (*bands)[*nr-1u]=merged;
   (*bounds)[*nb-2u]=merged.x;(*bounds)[*nb-1u]=merged.y;return;
  }
 }
 if(*nb+2u>44u || *nr>=24u){*reason|=2u;return;}
 (*bounds)[*nb]=lo;(*bounds)[*nb+1u]=hi;*nb+=2u;
 (*bands)[*nr]=vec2f(lo,hi);*nr+=1u;
}

fn mayAbsorb(previous: vec2f, current: vec2f, previousShortBand: bool,
 previousPositive: bool, currentUncertain: bool, protectedBoundary: bool) -> bool {
 if (protectedBoundary || previous.y != current.x) { return false; }
 let currentShortBand = currentUncertain && current.y - current.x <= 0.02;
 return (previousShortBand && !currentUncertain) || (previousPositive && currentShortBand);
}

fn putBandTagged(lo: f32, hi: f32, bounds: ptr<function, array<f32, 48>>,
 nb: ptr<function, u32>, bands: ptr<function, array<vec2f, 24>>,
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
 va: f32, vb: f32, error: f32, minSlope: f32, M: f32,
 bounds: ptr<function, array<f32, 48>>, nb: ptr<function, u32>,
 bands: ptr<function, array<vec2f, 24>>, nr: ptr<function, u32>,
 reason: ptr<function, u32>, work: ptr<function, u32>,
 zeroBands: ptr<function, u32>, isZeroLevel: bool) {
 if (abs(va) <= error) { let w = 2.0 * error / minSlope; putBandTagged(a, min(b, a + w), bounds, nb, bands, nr, reason, zeroBands, isZeroLevel); }
 if (abs(vb) <= error) { let w = 2.0 * error / minSlope; putBandTagged(max(a, b - w), b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel); }
 if ((va > error && vb < -error) || (va < -error && vb > error)) {
  var l = a; var r = b; var left = va; var right = vb;
  var rawLeft = va; var rawRight = vb;
  var curMinSlope = minSlope;
  var prevSide = 0i;
  for (var iter = 0u; iter < 32u; iter++) {
   if (*work >= cfg.budget) { *reason |= 1u; break; } *work += 1u;
   var mid = l + 0.5 * (r - l);
   if ((iter < 2u || (iter & 1u) == 0u) && abs(right - left) > 1e-6) {
    let tSec = clamp(-left / (right - left), 0.002, 0.998);
    mid = l + tSec * (r - l);
   }
   if (mid <= l || mid >= r || r - l <= 0.0001) { break; }
   let v = evalPhi(m, o, d, mid) - c;
   if (abs(v) <= error) {
    let w = 2.0 * error / curMinSlope;
    // An unresolved sign cannot discard either side, even at an exact zero.
    // Enclose it with the existing slope/error bound; never refine on that sign.
    l = max(l, mid - w); r = min(r, mid + w); break;
   }
   if ((rawLeft < 0.0 && v > 0.0) || (rawLeft > 0.0 && v < 0.0)) {
    r = mid; right = v; rawRight = v;
    if (prevSide == -1i) { left *= 0.5; }
    prevSide = -1i;
   } else {
    l = mid; left = v; rawLeft = v;
    if (prevSide == 1i) { right *= 0.5; }
    prevSide = 1i;
   }
   let h = r - l;
   if (h > 1e-5) {
    let localMinSlope = (abs(rawRight - rawLeft) - 2.0 * error) / h - 0.5 * M * h;
    curMinSlope = max(curMinSlope, localMinSlope);
   }
  }
  putBandTagged(l, r, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel);
 }
}

fn emitPieceSubpanels(p: vec2f, member: u32, rayId: u32, singles: ptr<function, u32>, reason: ptr<function, u32>) {
 let a = p.x; let b = p.y;
 let n = max(1u, u32(ceil((b - a) / 4.0)));
 if (*singles + n > cfg.maxSubPerRay) { *reason |= 16u; return; }
 for (var k = 0u; k < n; k++) {
  let l = select(a + f32(k) * (b - a) / f32(n), a, k == 0u);
  let r = select(a + f32(k + 1u) * (b - a) / f32(n), b, k + 1u == n);
  subpanels[(*singles) * cfg.rayCount + rayId] = Subpanel(vec2f(l, r), vec2u(member, 0u));
  *singles += 1u;
 }
}

fn roots(m: CurtainSpec, o: vec3f, d: vec3f, lo: f32, hi: f32, c: f32,
 bounds: ptr<function, array<f32, 48>>, nb: ptr<function, u32>, bands: ptr<function, array<vec2f, 24>>,
 nr: ptr<function, u32>, reason: ptr<function, u32>, work: ptr<function, u32>,
 phi_knots: ptr<function, array<f32, 9>>,
 zeroBands: ptr<function, u32>, isZeroLevel: bool,
 levelActiveMask: u32, allPositiveH: bool) {
 let M = m.curv.x * d.x * d.x * 1.00002;
 let err = m.curv.y;
 var stack: array<vec4f, 15>; var depthBits = vec3u(0u); var top = 0u;
 var binMask = select(0xFFu, levelActiveMask, allPositiveH);
 var nextChargeBin = 0u;
 while (binMask != 0u) {
  let bin = firstTrailingBit(binMask);
  binMask &= binMask - 1u;
  if (allPositiveH) {
   let skipped = bin - nextChargeBin;
   if (*work + skipped >= cfg.budget) { *reason |= 1u; return; }
   *work += skipped;
   nextChargeBin = bin + 1u;
  }
  var a = lo + (hi - lo) * f32(bin) / 8.0; var b = lo + (hi - lo) * f32(bin + 1u) / 8.0;
  if (bin == 0u) { a = lo; } if (bin == 7u) { b = hi; }
  if (*work >= cfg.budget) { *reason |= 1u; return; }
  let coarseH = b - a;
  if (!(coarseH > 0.0)) { *work += 1u; *reason |= 4u; return; }
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
   isolateMonotone(m, o, d, a, b, c, coarseVa, coarseVb, coarseError, coarseMinSlope, M,
                   bounds, nb, bands, nr, reason, work, zeroBands, isZeroLevel);
   if (*reason != 0u) { return; }
   continue;
  }
  if (0.125 * M * coarseH * coarseH <= coarseError) {
   putBandTagged(a, b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel);
   if (*reason != 0u) { return; }
   continue;
  }
  let mid0 = a + 0.5 * coarseH;
  if (mid0 <= a || mid0 >= b) {
   putBandTagged(a, b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel);
   if (*reason != 0u) { return; }
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
    isolateMonotone(m, o, d, a, b, c, va, vb, error, minSlope, M,
                    bounds, nb, bands, nr, reason, work, zeroBands, isZeroLevel);
    continue;
   }
   if (0.125 * M * h * h <= error) { putBandTagged(a, b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel); continue; }
   if (f32(depth) >= 14.0 || top + 2u > 15u) { putBandTagged(a, b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel); continue; }
   let mid = a + 0.5 * h; if (mid <= a || mid >= b) { putBandTagged(a, b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel); continue; }
   let phi_mid = evalPhi(m, o, d, mid);
   stack[top] = vec4f(mid, b, phi_mid, cell.w); setDepth(&depthBits, top, depth + 1u);
   stack[top + 1u] = vec4f(a, mid, cell.z, phi_mid); setDepth(&depthBits, top + 1u, depth + 1u);
   top += 2u;
  }
  if (*reason != 0u) { return; }
 }
 if (allPositiveH && nextChargeBin < 8u) {
  let trailing = 8u - nextChargeBin;
  if (*work + trailing > cfg.budget) { *reason |= 1u; return; }
  *work += trailing;
 }
}

fn exactThreeLevelMasks(m: CurtainSpec, d: vec3f, lo: f32, hi: f32,
                        phi_knots: ptr<function, array<f32, 9>>, allPositiveH: ptr<function, bool>) -> vec3u {
 let M = m.curv.x * d.x * d.x * 1.00002;
 let err = m.curv.y;
 let w = m.limits.w;
 var masks = vec3u(0u);
 for (var bin = 0u; bin < 8u; bin++) {
  var a = lo + (hi - lo) * f32(bin) / 8.0; var b = lo + (hi - lo) * f32(bin + 1u) / 8.0;
  if (bin == 0u) { a = lo; } if (bin == 7u) { b = hi; }
  let h = b - a;
  if (!(h > 0.0)) { *allPositiveH = false; }
  let pa = (*phi_knots)[bin]; let pb = (*phi_knots)[bin + 1u];
  let ab = abs(a) + abs(b);
  let mh = 0.125 * M * h * h;
  {
   let va = pa + w; let vb = pb + w;
   let scale = 1.0 + abs(va) + abs(vb) + ab;
   let bow = mh + (err + 0.000001 * scale);
   if (!(min(va, vb) > bow || max(va, vb) < -bow)) { masks.x |= (1u << bin); }
  }
  {
   let scale = 1.0 + abs(pa) + abs(pb) + ab;
   let bow = mh + (err + 0.000001 * scale);
   if (!(min(pa, pb) > bow || max(pa, pb) < -bow)) { masks.y |= (1u << bin); }
  }
  {
   let va = pa - w; let vb = pb - w;
   let scale = 1.0 + abs(va) + abs(vb) + ab;
   let bow = mh + (err + 0.000001 * scale);
   if (!(min(va, vb) > bow || max(va, vb) < -bow)) { masks.z |= (1u << bin); }
  }
 }
 return masks;
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
 var reason = 0u; var work = 0u; var singles = 0u;
 var knots: array<f32, 48>;
 if (cfg.budget == 0u) { rays[id].counts = vec4u(0, 0, 0, FALLBACK | 1u); return; }
 for (var _i = 0u; _i < 1u; _i++) {
  let m = curtains[activeMember]; let o = ro - m.origin.xyz;
  var a = input.ro_clipA.w; var b = input.rd_clipB.w; var hit = true;
  let zEnv = abs(m.amps.x) + abs(m.amps.y) + abs(m.amps.z) + m.limits.w;
  let halfVec = min(m.half.xyz, vec3f(m.limits.x, m.limits.y, zEnv) + vec3f(m.curv.y));
  for (var axis = 0u; axis < 3u; axis++) {
   let half = halfVec[axis];
   if (d[axis] == 0.0) { if (abs(o[axis]) > half) { hit = false; break; } }
   else {
    let q0 = (-half - o[axis]) / d[axis]; let q1 = (half - o[axis]) / d[axis];
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
  let masks = exactThreeLevelMasks(m, d, a, b, &phi_knots, &allPositiveH);
  if (allPositiveH && (masks.x | masks.y | masks.z) == 0u && abs(phi_knots[0]) > m.limits.w + m.curv.y) {
   if (work + 24u > cfg.budget) { reason |= 1u; break; }
   work += 24u;
   continue;
  }
  var nb = 1u; knots[0] = a;
  var bands: array<vec2f, 24>; var nr = 0u; var zeroBands = 0u;
  let levels = vec3f(-m.limits.w, 0.0, m.limits.w);
  for (var lvl = 0u; lvl < 3u; lvl++) {
   let lMask = masks[lvl];
   if (allPositiveH && lMask == 0u) {
    if (work + 8u > cfg.budget) { reason |= 1u; break; }
    work += 8u;
    continue;
   }
   roots(m, o, d, a, b, levels[lvl], &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, lvl == 1u, lMask, allPositiveH);
   if (reason != 0u) { break; }
  }
  if (reason != 0u) { break; }
  if (allPositiveH && nr == 0u && abs(phi_knots[0]) > m.limits.w + m.curv.y) { continue; }
  var supportLo = a;
  var supportHi = b;
  if (allPositiveH && nr > 0u) {
   var minBandLo = bands[0].x;
   var maxBandHi = bands[0].y;
   for (var k = 1u; k < nr; k++) {
    let bnd = bands[k];
    minBandLo = min(minBandLo, bnd.x);
    maxBandHi = max(maxBandHi, bnd.y);
   }
   if (abs(phi_knots[0]) > m.limits.w + m.curv.y) {
    supportLo = minBandLo;
    knots[0] = minBandLo;
   }
   if (abs(phi_knots[8]) > m.limits.w + m.curv.y) {
    supportHi = maxBandHi;
   }
  }
  var kx = 0.0; var ky0 = 0.0; var ky1 = 0.0;
  if (d.x != 0.0) { kx = -o.x / d.x; if (kx > supportLo && kx < supportHi) { knots[nb] = kx; nb++; } }
  if (d.y != 0.0) {
   ky0 = -o.y / d.y; if (ky0 > supportLo && ky0 < supportHi) { knots[nb] = ky0; nb++; }
   ky1 = (m.limits.z - o.y) / d.y; if (ky1 > supportLo && ky1 < supportHi) { knots[nb] = ky1; nb++; }
  }
  if (supportHi == b) { knots[nb] = b; nb++; }
  for (var i = 1u; i < nb; i++) {
   let v = knots[i]; var j = i; loop { if (j == 0u || knots[j - 1u] <= v) { break; } knots[j] = knots[j - 1u]; j--; }
   knots[j] = v;
  }
  var pieces: array<vec2f, 8>;
  var count = 0u;
  var previousShortBand = false; var previousPositive = false;
  for (var i = 1u; i < nb; i++) {
   let l = knots[i - 1u]; let r = knots[i]; if (!(r > l)) { continue; }
   let mid = l + 0.5 * (r - l); var uncertain = false; var protectedBoundary = false;
   if (d.x != 0.0) { protectedBoundary = (l == kx); }
   if (d.y != 0.0) { protectedBoundary = protectedBoundary || (l == ky0) || (l == ky1); }
   for (var k = 0u; k < nr; k++) {
    let bnd = bands[k];
    if (l < bnd.y && r > bnd.x) { uncertain = true; }
    if ((zeroBands & (1u << k)) != 0u && l == bnd.x) { protectedBoundary = true; }
    if (uncertain && (protectedBoundary || zeroBands == 0u)) { break; }
   }
   if (uncertain || abs(evalPhi(m, o, d, mid)) <= m.limits.w + m.curv.y) {
    if (count > 0u && mayAbsorb(pieces[count - 1u], vec2f(l, r),
         previousShortBand, previousPositive, uncertain, protectedBoundary)) {
     pieces[count - 1u].y = r; previousShortBand = false; previousPositive = true;
    } else if (count < 8u) {
     pieces[count] = vec2f(l, r); count++;
     previousShortBand = uncertain && r - l <= 0.02; previousPositive = !uncertain;
    } else {
     emitPieceSubpanels(pieces[0], activeMember, id, &singles, &reason);
     if (reason != 0u) { break; }
     for (var k = 1u; k < 8u; k++) { pieces[k - 1u] = pieces[k]; }
     pieces[7u] = vec2f(l, r);
     previousShortBand = uncertain && r - l <= 0.02; previousPositive = !uncertain;
    }
   }
  }
  for (var p = 0u; p < count; p++) {
   emitPieceSubpanels(pieces[p], activeMember, id, &singles, &reason);
   if (reason != 0u) { break; }
  }
  if (reason != 0u) { break; }
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

fn putBand(lo:f32,hi:f32,bounds:ptr<function,array<f32,48>>,nb:ptr<function,u32>,
 bands:ptr<function,array<vec2f,24>>,nr:ptr<function,u32>,reason:ptr<function,u32>){
 if(*nr>0u){
  let last=(*bands)[*nr-1u];
  if(lo<=last.y && hi>=last.x){
   let merged=vec2f(min(lo,last.x),max(hi,last.y));
   (*bands)[*nr-1u]=merged;
   (*bounds)[*nb-2u]=merged.x;(*bounds)[*nb-1u]=merged.y;return;
  }
 }
 if(*nb+2u>44u || *nr>=24u){*reason|=2u;return;}
 (*bounds)[*nb]=lo;(*bounds)[*nb+1u]=hi;*nb+=2u;
 (*bands)[*nr]=vec2f(lo,hi);*nr+=1u;
}

fn mayAbsorb(previous: vec2f, current: vec2f, previousShortBand: bool,
 previousPositive: bool, currentUncertain: bool, protectedBoundary: bool) -> bool {
 if (protectedBoundary || previous.y != current.x) { return false; }
 let currentShortBand = currentUncertain && current.y - current.x <= 0.02;
 return (previousShortBand && !currentUncertain) || (previousPositive && currentShortBand);
}

fn putBandTagged(lo: f32, hi: f32, bounds: ptr<function, array<f32, 48>>,
 nb: ptr<function, u32>, bands: ptr<function, array<vec2f, 24>>,
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
 va: f32, vb: f32, error: f32, minSlope: f32, M: f32,
 bounds: ptr<function, array<f32, 48>>, nb: ptr<function, u32>,
 bands: ptr<function, array<vec2f, 24>>, nr: ptr<function, u32>,
 reason: ptr<function, u32>, work: ptr<function, u32>,
 zeroBands: ptr<function, u32>, isZeroLevel: bool) {
 if (abs(va) <= error) { let w = 2.0 * error / minSlope; putBandTagged(a, min(b, a + w), bounds, nb, bands, nr, reason, zeroBands, isZeroLevel); }
 if (abs(vb) <= error) { let w = 2.0 * error / minSlope; putBandTagged(max(a, b - w), b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel); }
 if ((va > error && vb < -error) || (va < -error && vb > error)) {
  var l = a; var r = b; var left = va; var right = vb;
  var rawLeft = va; var rawRight = vb;
  var curMinSlope = minSlope;
  var prevSide = 0i;
  for (var iter = 0u; iter < 32u; iter++) {
   if (*work >= cfg.budget) { *reason |= 1u; break; } *work += 1u;
   var mid = l + 0.5 * (r - l);
   if ((iter < 2u || (iter & 1u) == 0u) && abs(right - left) > 1e-6) {
    let tSec = clamp(-left / (right - left), 0.002, 0.998);
    mid = l + tSec * (r - l);
   }
   if (mid <= l || mid >= r || r - l <= 0.0001) { break; }
   let v = evalPhi(m, o, d, mid) - c;
   if (abs(v) <= error) {
    let w = 2.0 * error / curMinSlope;
    // An unresolved sign cannot discard either side, even at an exact zero.
    // Enclose it with the existing slope/error bound; never refine on that sign.
    l = max(l, mid - w); r = min(r, mid + w); break;
   }
   if ((rawLeft < 0.0 && v > 0.0) || (rawLeft > 0.0 && v < 0.0)) {
    r = mid; right = v; rawRight = v;
    if (prevSide == -1i) { left *= 0.5; }
    prevSide = -1i;
   } else {
    l = mid; left = v; rawLeft = v;
    if (prevSide == 1i) { right *= 0.5; }
    prevSide = 1i;
   }
   let h = r - l;
   if (h > 1e-5) {
    let localMinSlope = (abs(rawRight - rawLeft) - 2.0 * error) / h - 0.5 * M * h;
    curMinSlope = max(curMinSlope, localMinSlope);
   }
  }
  putBandTagged(l, r, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel);
 }
}

fn emitPieceSubpanels(p: vec2f, member: u32, rayId: u32, singles: ptr<function, u32>, reason: ptr<function, u32>) {
 let a = p.x; let b = p.y;
 let n = max(1u, u32(ceil((b - a) / 4.0)));
 if (*singles + n > cfg.maxSubPerRay) { *reason |= 16u; return; }
 for (var k = 0u; k < n; k++) {
  let l = select(a + f32(k) * (b - a) / f32(n), a, k == 0u);
  let r = select(a + f32(k + 1u) * (b - a) / f32(n), b, k + 1u == n);
  subpanels[(*singles) * cfg.rayCount + rayId] = Subpanel(vec2f(l, r), vec2u(member, 0u));
  *singles += 1u;
 }
}

fn roots(m: CurtainSpec, o: vec3f, d: vec3f, lo: f32, hi: f32, c: f32,
 bounds: ptr<function, array<f32, 48>>, nb: ptr<function, u32>, bands: ptr<function, array<vec2f, 24>>,
 nr: ptr<function, u32>, reason: ptr<function, u32>, work: ptr<function, u32>,
 phi_knots: ptr<function, array<f32, 9>>,
 zeroBands: ptr<function, u32>, isZeroLevel: bool,
 levelActiveMask: u32, allPositiveH: bool) {
 let M = m.curv.x * d.x * d.x * 1.00002;
 let err = m.curv.y;
 var stack: array<vec4f, 15>; var depthBits = vec3u(0u); var top = 0u;
 var binMask = select(0xFFu, levelActiveMask, allPositiveH);
 var nextChargeBin = 0u;
 while (binMask != 0u) {
  let bin = firstTrailingBit(binMask);
  binMask &= binMask - 1u;
  if (allPositiveH) {
   let skipped = bin - nextChargeBin;
   if (*work + skipped >= cfg.budget) { *reason |= 1u; return; }
   *work += skipped;
   nextChargeBin = bin + 1u;
  }
  var a = lo + (hi - lo) * f32(bin) / 8.0; var b = lo + (hi - lo) * f32(bin + 1u) / 8.0;
  if (bin == 0u) { a = lo; } if (bin == 7u) { b = hi; }
  if (*work >= cfg.budget) { *reason |= 1u; return; }
  let coarseH = b - a;
  if (!(coarseH > 0.0)) { *work += 1u; *reason |= 4u; return; }
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
   isolateMonotone(m, o, d, a, b, c, coarseVa, coarseVb, coarseError, coarseMinSlope, M,
                   bounds, nb, bands, nr, reason, work, zeroBands, isZeroLevel);
   if (*reason != 0u) { return; }
   continue;
  }
  if (0.125 * M * coarseH * coarseH <= coarseError) {
   putBandTagged(a, b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel);
   if (*reason != 0u) { return; }
   continue;
  }
  let mid0 = a + 0.5 * coarseH;
  if (mid0 <= a || mid0 >= b) {
   putBandTagged(a, b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel);
   if (*reason != 0u) { return; }
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
    isolateMonotone(m, o, d, a, b, c, va, vb, error, minSlope, M,
                    bounds, nb, bands, nr, reason, work, zeroBands, isZeroLevel);
    continue;
   }
   if (0.125 * M * h * h <= error) { putBandTagged(a, b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel); continue; }
   if (f32(depth) >= 14.0 || top + 2u > 15u) { putBandTagged(a, b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel); continue; }
   let mid = a + 0.5 * h; if (mid <= a || mid >= b) { putBandTagged(a, b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel); continue; }
   let phi_mid = evalPhi(m, o, d, mid);
   stack[top] = vec4f(mid, b, phi_mid, cell.w); setDepth(&depthBits, top, depth + 1u);
   stack[top + 1u] = vec4f(a, mid, cell.z, phi_mid); setDepth(&depthBits, top + 1u, depth + 1u);
   top += 2u;
  }
  if (*reason != 0u) { return; }
 }
 if (allPositiveH && nextChargeBin < 8u) {
  let trailing = 8u - nextChargeBin;
  if (*work + trailing > cfg.budget) { *reason |= 1u; return; }
  *work += trailing;
 }
}

fn exactThreeLevelMasks(m: CurtainSpec, d: vec3f, lo: f32, hi: f32,
                        phi_knots: ptr<function, array<f32, 9>>, allPositiveH: ptr<function, bool>) -> vec3u {
 let M = m.curv.x * d.x * d.x * 1.00002;
 let err = m.curv.y;
 let w = m.limits.w;
 var masks = vec3u(0u);
 for (var bin = 0u; bin < 8u; bin++) {
  var a = lo + (hi - lo) * f32(bin) / 8.0; var b = lo + (hi - lo) * f32(bin + 1u) / 8.0;
  if (bin == 0u) { a = lo; } if (bin == 7u) { b = hi; }
  let h = b - a;
  if (!(h > 0.0)) { *allPositiveH = false; }
  let pa = (*phi_knots)[bin]; let pb = (*phi_knots)[bin + 1u];
  let ab = abs(a) + abs(b);
  let mh = 0.125 * M * h * h;
  {
   let va = pa + w; let vb = pb + w;
   let scale = 1.0 + abs(va) + abs(vb) + ab;
   let bow = mh + (err + 0.000001 * scale);
   if (!(min(va, vb) > bow || max(va, vb) < -bow)) { masks.x |= (1u << bin); }
  }
  {
   let scale = 1.0 + abs(pa) + abs(pb) + ab;
   let bow = mh + (err + 0.000001 * scale);
   if (!(min(pa, pb) > bow || max(pa, pb) < -bow)) { masks.y |= (1u << bin); }
  }
  {
   let va = pa - w; let vb = pb - w;
   let scale = 1.0 + abs(va) + abs(vb) + ab;
   let bow = mh + (err + 0.000001 * scale);
   if (!(min(va, vb) > bow || max(va, vb) < -bow)) { masks.z |= (1u << bin); }
  }
 }
 return masks;
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
    let isMulti = ((packedTile >> 21u) & 1u) != 0u;
    let numMembers = select(2u, 3u + ((packedTile >> 20u) & 1u), isMulti);
    var ord = array<u32, 4>(0u, 0u, 0u, 0u);
    if (!isMulti) {
        let m1First = ((packedTile >> 20u) & 1u) != 0u;
        let rawM0 = (packedTile >> 28u) & 0xFu;
        let rawM1 = (packedTile >> 24u) & 0xFu;
        ord[0] = select(rawM0, rawM1, m1First);
        ord[1] = select(rawM1, rawM0, m1First);
    } else {
        ord[0] = (packedTile >> 22u) & 3u;
        ord[1] = (packedTile >> 24u) & 3u;
        ord[2] = (packedTile >> 26u) & 3u;
        ord[3] = (packedTile >> 28u) & 3u;
    }
    if (tileId >= totalTiles) { return; }
    let rayStart = tileOffsets[tileId];
    let rayCount = tileOffsets[tileId + 1u] - rayStart;
    if (rayInTile >= rayCount) { return; }
    let id = tileRayIndices[rayStart + rayInTile];
    if (id >= cfg.rayCount) { return; }

    let input = rays[id];
    if((input.counts.w & 0x40000000u)!=0u){return;}
    let ro = input.ro_clipA.xyz; let d = input.rd_clipB.xyz;
    var reason = 0u; var work = 0u; var singles = 0u;
    var knots: array<f32, 48>;
    if (cfg.budget == 0u) { rays[id].counts = vec4u(0, 0, 0, FALLBACK | 1u); return; }

    var prevMemberMaxS = -1e9f;
    for (var memberIdx = 0u; memberIdx < numMembers; memberIdx++) {
        let member = ord[memberIdx];
        var currMemberMaxS = prevMemberMaxS;
        let m = curtains[member]; let o = ro - m.origin.xyz;
        var a = input.ro_clipA.w; var b = input.rd_clipB.w; var hit = true;
        let zEnv = abs(m.amps.x) + abs(m.amps.y) + abs(m.amps.z) + m.limits.w;
        let halfVec = min(m.half.xyz, vec3f(m.limits.x, m.limits.y, zEnv) + vec3f(m.curv.y));
        for (var axis = 0u; axis < 3u; axis++) {
            let half = halfVec[axis];
            if (d[axis] == 0.0) { if (abs(o[axis]) > half) { hit = false; break; } }
            else {
                let q0 = (-half - o[axis]) / d[axis]; let q1 = (half - o[axis]) / d[axis];
                a = max(a, min(q0, q1)); b = min(b, max(q0, q1));
            }
        }
        if (!hit || !(b > a)) {
            continue;
        }
        var phi_knots: array<f32, 9>;
        for (var k = 0u; k < 9u; k++) {
            var s = a + (b - a) * f32(k) / 8.0;
            if (k == 0u) { s = a; } if (k == 8u) { s = b; }
            phi_knots[k] = evalPhi(m, o, d, s);
        }
        var allPositiveH = true;
        let masks = exactThreeLevelMasks(m, d, a, b, &phi_knots, &allPositiveH);
        if (allPositiveH && (masks.x | masks.y | masks.z) == 0u && abs(phi_knots[0]) > m.limits.w + m.curv.y) {
            if (work + 24u > cfg.budget) { reason |= 1u; break; }
            work += 24u;
            continue;
        }
        var nb = 1u; knots[0] = a;
        var bands: array<vec2f, 24>; var nr = 0u; var zeroBands = 0u;
        let levels = vec3f(-m.limits.w, 0.0, m.limits.w);
        for (var lvl = 0u; lvl < 3u; lvl++) {
            let lMask = masks[lvl];
            if (allPositiveH && lMask == 0u) {
                if (work + 8u > cfg.budget) { reason |= 1u; break; }
                work += 8u;
                continue;
            }
            roots(m, o, d, a, b, levels[lvl], &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, lvl == 1u, lMask, allPositiveH);
            if (reason != 0u) { break; }
        }
        if (reason != 0u) { break; }
        if (allPositiveH && nr == 0u && abs(phi_knots[0]) > m.limits.w + m.curv.y) { continue; }
        var supportLo = a;
        var supportHi = b;
        if (allPositiveH && nr > 0u) {
            var minBandLo = bands[0].x;
            var maxBandHi = bands[0].y;
            for (var k = 1u; k < nr; k++) {
                let bnd = bands[k];
                minBandLo = min(minBandLo, bnd.x);
                maxBandHi = max(maxBandHi, bnd.y);
            }
            if (abs(phi_knots[0]) > m.limits.w + m.curv.y) {
                supportLo = minBandLo;
                knots[0] = minBandLo;
            }
            if (abs(phi_knots[8]) > m.limits.w + m.curv.y) {
                supportHi = maxBandHi;
            }
        }
        var kx = 0.0; var ky0 = 0.0; var ky1 = 0.0;
        if (d.x != 0.0) { kx = -o.x / d.x; if (kx > supportLo && kx < supportHi) { knots[nb] = kx; nb++; } }
        if (d.y != 0.0) {
            ky0 = -o.y / d.y; if (ky0 > supportLo && ky0 < supportHi) { knots[nb] = ky0; nb++; }
            ky1 = (m.limits.z - o.y) / d.y; if (ky1 > supportLo && ky1 < supportHi) { knots[nb] = ky1; nb++; }
        }
        if (supportHi == b) { knots[nb] = b; nb++; }
        for (var i = 1u; i < nb; i++) {
            let v = knots[i]; var j = i; loop { if (j == 0u || knots[j - 1u] <= v) { break; } knots[j] = knots[j - 1u]; j--; }
            knots[j] = v;
        }
        var pieces: array<vec2f, 8>;
        var count = 0u;
        var previousShortBand = false; var previousPositive = false;
        for (var i = 1u; i < nb; i++) {
            let l = knots[i - 1u]; let r = knots[i]; if (!(r > l)) { continue; }
            let mid = l + 0.5 * (r - l); var uncertain = false; var protectedBoundary = false;
            if (d.x != 0.0) { protectedBoundary = (l == kx); }
            if (d.y != 0.0) { protectedBoundary = protectedBoundary || (l == ky0) || (l == ky1); }
            for (var k = 0u; k < nr; k++) {
                let bnd = bands[k];
                if (l < bnd.y && r > bnd.x) { uncertain = true; }
                if ((zeroBands & (1u << k)) != 0u && l == bnd.x) { protectedBoundary = true; }
                if (uncertain && (protectedBoundary || zeroBands == 0u)) { break; }
            }
            if (uncertain || abs(evalPhi(m, o, d, mid)) <= m.limits.w + m.curv.y) {
                if (memberIdx > 0u && l < prevMemberMaxS) {
                    reason |= 64u;
                    break;
                }
                currMemberMaxS = max(currMemberMaxS, r);
                if (count > 0u && mayAbsorb(pieces[count - 1u], vec2f(l, r),
                     previousShortBand, previousPositive, uncertain, protectedBoundary)) {
                    pieces[count - 1u].y = r; previousShortBand = false; previousPositive = true;
                } else if (count < 8u) {
                    pieces[count] = vec2f(l, r); count++;
                    previousShortBand = uncertain && r - l <= 0.02; previousPositive = !uncertain;
                } else {
                    emitPieceSubpanels(pieces[0], member, id, &singles, &reason);
                    if (reason != 0u) { break; }
                    for (var k = 1u; k < 8u; k++) { pieces[k - 1u] = pieces[k]; }
                    pieces[7u] = vec2f(l, r);
                    previousShortBand = uncertain && r - l <= 0.02; previousPositive = !uncertain;
                }
            }
        }
        if (reason != 0u) { break; }
        for (var p = 0u; p < count; p++) {
            emitPieceSubpanels(pieces[p], member, id, &singles, &reason);
            if (reason != 0u) { break; }
        }
        if (reason != 0u) { break; }
        prevMemberMaxS = currMemberMaxS;
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

fn putBand(lo:f32,hi:f32,bounds:ptr<function,array<f32,48>>,nb:ptr<function,u32>,
 bands:ptr<function,array<vec2f,24>>,nr:ptr<function,u32>,reason:ptr<function,u32>){
 if(*nr>0u){
  let last=(*bands)[*nr-1u];
  if(lo<=last.y && hi>=last.x){
   let merged=vec2f(min(lo,last.x),max(hi,last.y));
   (*bands)[*nr-1u]=merged;
   (*bounds)[*nb-2u]=merged.x;(*bounds)[*nb-1u]=merged.y;return;
  }
 }
 if(*nb+2u>44u || *nr>=24u){*reason|=2u;return;}
 (*bounds)[*nb]=lo;(*bounds)[*nb+1u]=hi;*nb+=2u;
 (*bands)[*nr]=vec2f(lo,hi);*nr+=1u;
}

fn mayAbsorb(previous: vec2f, current: vec2f, previousShortBand: bool,
 previousPositive: bool, currentUncertain: bool, protectedBoundary: bool) -> bool {
 if (protectedBoundary || previous.y != current.x) { return false; }
 let currentShortBand = currentUncertain && current.y - current.x <= 0.02;
 return (previousShortBand && !currentUncertain) || (previousPositive && currentShortBand);
}

fn putBandTagged(lo: f32, hi: f32, bounds: ptr<function, array<f32, 48>>,
 nb: ptr<function, u32>, bands: ptr<function, array<vec2f, 24>>,
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
 va: f32, vb: f32, error: f32, minSlope: f32, M: f32,
 bounds: ptr<function, array<f32, 48>>, nb: ptr<function, u32>,
 bands: ptr<function, array<vec2f, 24>>, nr: ptr<function, u32>,
 reason: ptr<function, u32>, work: ptr<function, u32>,
 zeroBands: ptr<function, u32>, isZeroLevel: bool) {
 if (abs(va) <= error) { let w = 2.0 * error / minSlope; putBandTagged(a, min(b, a + w), bounds, nb, bands, nr, reason, zeroBands, isZeroLevel); }
 if (abs(vb) <= error) { let w = 2.0 * error / minSlope; putBandTagged(max(a, b - w), b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel); }
 if ((va > error && vb < -error) || (va < -error && vb > error)) {
  var l = a; var r = b; var left = va; var right = vb;
  var rawLeft = va; var rawRight = vb;
  var curMinSlope = minSlope;
  var prevSide = 0i;
  for (var iter = 0u; iter < 32u; iter++) {
   if (*work >= cfg.budget) { *reason |= 1u; break; } *work += 1u;
   var mid = l + 0.5 * (r - l);
   if ((iter < 2u || (iter & 1u) == 0u) && abs(right - left) > 1e-6) {
    let tSec = clamp(-left / (right - left), 0.002, 0.998);
    mid = l + tSec * (r - l);
   }
   if (mid <= l || mid >= r || r - l <= 0.0001) { break; }
   let v = evalPhi(m, o, d, mid) - c;
   if (abs(v) <= error) {
    let w = 2.0 * error / curMinSlope;
    // An unresolved sign cannot discard either side, even at an exact zero.
    // Enclose it with the existing slope/error bound; never refine on that sign.
    l = max(l, mid - w); r = min(r, mid + w); break;
   }
   if ((rawLeft < 0.0 && v > 0.0) || (rawLeft > 0.0 && v < 0.0)) {
    r = mid; right = v; rawRight = v;
    if (prevSide == -1i) { left *= 0.5; }
    prevSide = -1i;
   } else {
    l = mid; left = v; rawLeft = v;
    if (prevSide == 1i) { right *= 0.5; }
    prevSide = 1i;
   }
   let h = r - l;
   if (h > 1e-5) {
    let localMinSlope = (abs(rawRight - rawLeft) - 2.0 * error) / h - 0.5 * M * h;
    curMinSlope = max(curMinSlope, localMinSlope);
   }
  }
  putBandTagged(l, r, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel);
 }
}

fn roots(m: CurtainSpec, o: vec3f, d: vec3f, lo: f32, hi: f32, c: f32,
 bounds: ptr<function, array<f32, 48>>, nb: ptr<function, u32>, bands: ptr<function, array<vec2f, 24>>,
 nr: ptr<function, u32>, reason: ptr<function, u32>, work: ptr<function, u32>,
 phi_knots: ptr<function, array<f32, 9>>,
 zeroBands: ptr<function, u32>, isZeroLevel: bool,
 levelActiveMask: u32, allPositiveH: bool) {
 let M = m.curv.x * d.x * d.x * 1.00002;
 let err = m.curv.y;
 var stack: array<vec4f, 15>; var depthBits = vec3u(0u); var top = 0u;
 var binMask = select(0xFFu, levelActiveMask, allPositiveH);
 var nextChargeBin = 0u;
 while (binMask != 0u) {
  let bin = firstTrailingBit(binMask);
  binMask &= binMask - 1u;
  if (allPositiveH) {
   let skipped = bin - nextChargeBin;
   if (*work + skipped >= cfg.budget) { *reason |= 1u; return; }
   *work += skipped;
   nextChargeBin = bin + 1u;
  }
  var a = lo + (hi - lo) * f32(bin) / 8.0; var b = lo + (hi - lo) * f32(bin + 1u) / 8.0;
  if (bin == 0u) { a = lo; } if (bin == 7u) { b = hi; }
  if (*work >= cfg.budget) { *reason |= 1u; return; }
  let coarseH = b - a;
  if (!(coarseH > 0.0)) { *work += 1u; *reason |= 4u; return; }
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
   isolateMonotone(m, o, d, a, b, c, coarseVa, coarseVb, coarseError, coarseMinSlope, M,
                   bounds, nb, bands, nr, reason, work, zeroBands, isZeroLevel);
   if (*reason != 0u) { return; }
   continue;
  }
  if (0.125 * M * coarseH * coarseH <= coarseError) {
   putBandTagged(a, b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel);
   if (*reason != 0u) { return; }
   continue;
  }
  let mid0 = a + 0.5 * coarseH;
  if (mid0 <= a || mid0 >= b) {
   putBandTagged(a, b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel);
   if (*reason != 0u) { return; }
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
    isolateMonotone(m, o, d, a, b, c, va, vb, error, minSlope, M,
                    bounds, nb, bands, nr, reason, work, zeroBands, isZeroLevel);
    continue;
   }
   if (0.125 * M * h * h <= error) { putBandTagged(a, b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel); continue; }
   if (f32(depth) >= 14.0 || top + 2u > 15u) { putBandTagged(a, b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel); continue; }
   let mid = a + 0.5 * h; if (mid <= a || mid >= b) { putBandTagged(a, b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel); continue; }
   let phi_mid = evalPhi(m, o, d, mid);
   stack[top] = vec4f(mid, b, phi_mid, cell.w); setDepth(&depthBits, top, depth + 1u);
   stack[top + 1u] = vec4f(a, mid, cell.z, phi_mid); setDepth(&depthBits, top + 1u, depth + 1u);
   top += 2u;
  }
  if (*reason != 0u) { return; }
 }
 if (allPositiveH && nextChargeBin < 8u) {
  let trailing = 8u - nextChargeBin;
  if (*work + trailing > cfg.budget) { *reason |= 1u; return; }
  *work += trailing;
 }
}


fn exactThreeLevelMasks(m: CurtainSpec, d: vec3f, lo: f32, hi: f32,
                        phi_knots: ptr<function, array<f32, 9>>, allPositiveH: ptr<function, bool>) -> vec3u {
 let M = m.curv.x * d.x * d.x * 1.00002;
 let err = m.curv.y;
 let w = m.limits.w;
 var masks = vec3u(0u);
 for (var bin = 0u; bin < 8u; bin++) {
  var a = lo + (hi - lo) * f32(bin) / 8.0; var b = lo + (hi - lo) * f32(bin + 1u) / 8.0;
  if (bin == 0u) { a = lo; } if (bin == 7u) { b = hi; }
  let h = b - a;
  if (!(h > 0.0)) { *allPositiveH = false; }
  let pa = (*phi_knots)[bin]; let pb = (*phi_knots)[bin + 1u];
  let ab = abs(a) + abs(b);
  let mh = 0.125 * M * h * h;
  {
   let va = pa + w; let vb = pb + w;
   let scale = 1.0 + abs(va) + abs(vb) + ab;
   let bow = mh + (err + 0.000001 * scale);
   if (!(min(va, vb) > bow || max(va, vb) < -bow)) { masks.x |= (1u << bin); }
  }
  {
   let scale = 1.0 + abs(pa) + abs(pb) + ab;
   let bow = mh + (err + 0.000001 * scale);
   if (!(min(pa, pb) > bow || max(pa, pb) < -bow)) { masks.y |= (1u << bin); }
  }
  {
   let va = pa - w; let vb = pb - w;
   let scale = 1.0 + abs(va) + abs(vb) + ab;
   let bow = mh + (err + 0.000001 * scale);
   if (!(min(va, vb) > bow || max(va, vb) < -bow)) { masks.z |= (1u << bin); }
  }
 }
 return masks;
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
    let packedTile = fallbackTileList[tileSlot];
    let tileId = packedTile & 0x0FFFFFFFu;
    let activeMask = packedTile >> 28u;
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
 var pieces: array<vec2f, 32>;
 var knots: array<f32, 48>;
 var memStart = vec4u(0u); var memCount = vec4u(0u);
 if (cfg.budget == 0u) { rays[id].counts = vec4u(0, 0, 0, FALLBACK | 1u); return; }
 for (var member = 0u; member < 4u; member++) {
  memStart[member] = count;
  if ((activeMask & (1u << member)) == 0u) { memCount[member] = 0u; continue; }
  let m = curtains[member]; let o = ro - m.origin.xyz;
  var a = input.ro_clipA.w; var b = input.rd_clipB.w; var hit = true;
  let zEnv = abs(m.amps.x) + abs(m.amps.y) + abs(m.amps.z) + m.limits.w;
  let halfVec = min(m.half.xyz, vec3f(m.limits.x, m.limits.y, zEnv) + vec3f(m.curv.y));
  for (var axis = 0u; axis < 3u; axis++) {
   let half = halfVec[axis];
   if (d[axis] == 0.0) { if (abs(o[axis]) > half) { hit = false; break; } }
   else {
    let q0 = (-half - o[axis]) / d[axis]; let q1 = (half - o[axis]) / d[axis];
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
  let masks = exactThreeLevelMasks(m, d, a, b, &phi_knots, &allPositiveH);
  if (allPositiveH && (masks.x | masks.y | masks.z) == 0u && abs(phi_knots[0]) > m.limits.w + m.curv.y) {
   if (work + 24u > cfg.budget) { reason |= 1u; break; }
   work += 24u;
   memCount[member] = 0u;
   continue;
  }
  var nb = 1u; knots[0] = a;
  var bands: array<vec2f, 24>; var nr = 0u; var zeroBands = 0u;
  let levels = vec3f(-m.limits.w, 0.0, m.limits.w);
  for (var lvl = 0u; lvl < 3u; lvl++) {
   let lMask = masks[lvl];
   if (allPositiveH && lMask == 0u) {
    if (work + 8u > cfg.budget) { reason |= 1u; break; }
    work += 8u;
    continue;
   }
   roots(m, o, d, a, b, levels[lvl], &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, lvl == 1u, lMask, allPositiveH);
   if (reason != 0u) { break; }
  }
  if (reason != 0u) { break; }
  if (allPositiveH && nr == 0u && abs(phi_knots[0]) > m.limits.w + m.curv.y) {
   memCount[member] = 0u;
   continue;
  }
  var supportLo = a;
  var supportHi = b;
  if (allPositiveH && nr > 0u) {
   var minBandLo = bands[0].x;
   var maxBandHi = bands[0].y;
   for (var k = 1u; k < nr; k++) {
    let bnd = bands[k];
    minBandLo = min(minBandLo, bnd.x);
    maxBandHi = max(maxBandHi, bnd.y);
   }
   if (abs(phi_knots[0]) > m.limits.w + m.curv.y) {
    supportLo = minBandLo;
    knots[0] = minBandLo;
   }
   if (abs(phi_knots[8]) > m.limits.w + m.curv.y) {
    supportHi = maxBandHi;
   }
  }
  var kx = 0.0; var ky0 = 0.0; var ky1 = 0.0;
  if (d.x != 0.0) { kx = -o.x / d.x; if (kx > supportLo && kx < supportHi) { knots[nb] = kx; nb++; } }
  if (d.y != 0.0) {
   ky0 = -o.y / d.y; if (ky0 > supportLo && ky0 < supportHi) { knots[nb] = ky0; nb++; }
   ky1 = (m.limits.z - o.y) / d.y; if (ky1 > supportLo && ky1 < supportHi) { knots[nb] = ky1; nb++; }
  }
  if (supportHi == b) { knots[nb] = b; nb++; }
  for (var i = 1u; i < nb; i++) {
   let v = knots[i]; var j = i; loop { if (j == 0u || knots[j - 1u] <= v) { break; } knots[j] = knots[j - 1u]; j--; }
   knots[j] = v;
  }
  let memberStart = count; var previousShortBand = false; var previousPositive = false;
  for (var i = 1u; i < nb; i++) {
   let l = knots[i - 1u]; let r = knots[i]; if (!(r > l)) { continue; }
   let mid = l + 0.5 * (r - l); var uncertain = false; var protectedBoundary = false;
   if (d.x != 0.0) { protectedBoundary = (l == kx); }
   if (d.y != 0.0) { protectedBoundary = protectedBoundary || (l == ky0) || (l == ky1); }
   for (var k = 0u; k < nr; k++) {
    let bnd = bands[k];
    if (l < bnd.y && r > bnd.x) { uncertain = true; }
    if ((zeroBands & (1u << k)) != 0u && l == bnd.x) { protectedBoundary = true; }
    if (uncertain && (protectedBoundary || zeroBands == 0u)) { break; }
   }
   if (uncertain || abs(evalPhi(m, o, d, mid)) <= m.limits.w + m.curv.y) {
    if (count > memberStart && mayAbsorb(pieces[count - 1u], vec2f(l, r),
         previousShortBand, previousPositive, uncertain, protectedBoundary)) {
     pieces[count - 1u].y = r; previousShortBand = false; previousPositive = true;
    } else {
     if (count >= 32u) { reason |= 8u; break; }
     pieces[count] = vec2f(l, r);
     count++; previousShortBand = uncertain && r - l <= 0.02; previousPositive = !uncertain;
    }
   }
  }
  memCount[member] = count - memStart[member];
  if (reason != 0u) { break; }
 }
 if (reason != 0u) { rays[id].counts = vec4u(0, 0, 0, FALLBACK | reason); return; }
 if (count == 0u) { rays[id].counts = vec4u(0, 0, 0, 0u); return; }
 var singles = 0u; var overs = 0u;
 var nonEmptyMask = 0u;
 for (var m = 0u; m < 4u; m++) {
  if (memCount[m] > 0u) { nonEmptyMask |= (1u << m); }
 }
 var ord = array<u32, 4>(0u, 0u, 0u, 0u);
 var nAct = 0u;
 var tempMask = nonEmptyMask;
 while (tempMask != 0u) {
  let m = firstTrailingBit(tempMask);
  tempMask &= tempMask - 1u;
  let startM = pieces[memStart[m]].x;
  var ins = nAct;
  while (ins > 0u && pieces[memStart[ord[ins - 1u]]].x > startM) {
   ord[ins] = ord[ins - 1u];
   ins--;
  }
  ord[ins] = m;
  nAct++;
 }

 var allDisjoint = true;
 for (var i = 0u; i + 1u < nAct; i++) {
  let mCurr = ord[i];
  let mNext = ord[i + 1u];
  let endCurr = pieces[memStart[mCurr] + memCount[mCurr] - 1u].y;
  let startNext = pieces[memStart[mNext]].x;
  if (endCurr > startNext) {
   allDisjoint = false;
   break;
  }
 }

 if (allDisjoint) {
  for (var idx = 0u; idx < nAct; idx++) {
   let m = ord[idx];
   let pStart = memStart[m];
   let pEnd = pStart + memCount[m];
   for (var p = pStart; p < pEnd; p++) {
    let a = pieces[p].x; let b = pieces[p].y;
    let n = max(1u, u32(ceil((b - a) / 4.0)));
    if (singles + n > cfg.maxSubPerRay) { reason |= 16u; break; }
    for (var k = 0u; k < n; k++) {
     let l = select(a + f32(k) * (b - a) / f32(n), a, k == 0u);
     let r = select(a + f32(k + 1u) * (b - a) / f32(n), b, k + 1u == n);
     subpanels[singles * cfg.rayCount + id] = Subpanel(vec2f(l, r), vec2u(m, 0u));
     singles++;
    }
   }
   if (reason != 0u) { break; }
  }
  if (reason != 0u) { rays[id].counts = vec4u(0, 0, 0, FALLBACK | reason); }
  else { rays[id].counts = vec4u(0, singles, 0, 0u); }
  return;
 }
 var head = vec4u(0u);
 var pIdx = vec4u(0u);
 var prevKnot = 0.0;
 let totalKnots = count * 2u;
 for (var ne = 0u; ne < totalKnots && reason == 0u; ne++) {
  var bestM = 4u;
  var bestVal = 0.0;
  for (var idx = 0u; idx < nAct; idx++) {
   let m = ord[idx];
   let h = head[m];
   if (h < memCount[m] * 2u) {
    let p_idx = memStart[m] + (h >> 1u);
    let val = select(pieces[p_idx].x, pieces[p_idx].y, (h & 1u) != 0u);
    if (bestM == 4u || val < bestVal) {
     bestVal = val;
     bestM = m;
    }
   }
  }
  head[bestM]++;
  if (ne > 0u && bestVal > prevKnot) {
   let a = prevKnot; let b = bestVal;
   let mid = a + 0.5 * (b - a);
   var mask = 0u;
   for (var idx = 0u; idx < nAct; idx++) {
    let m = ord[idx];
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
     if (mid >= p.x && mid <= p.y) { mask |= (1u << m); }
    }
   }
   if (mask != 0u) {
    if (countOneBits(mask) == 1u) {
     let n = max(1u, u32(ceil((b - a) / 4.0)));
     if (singles + n > cfg.maxSubPerRay) { reason |= 16u; break; }
     let member = firstTrailingBit(mask);
     for (var k = 0u; k < n; k++) {
      let l = select(a + f32(k) * (b - a) / f32(n), a, k == 0u);
      let r = select(a + f32(k + 1u) * (b - a) / f32(n), b, k + 1u == n);
      subpanels[singles * cfg.rayCount + id] = Subpanel(vec2f(l, r), vec2u(member, 0u));
      singles++;
     }
    } else {
     if (overs >= cfg.maxOverPerRay) { reason |= 32u; break; }
     overlaps[overs * cfg.rayCount + id] = Overlap(vec2f(a, b), vec2u(mask, 0u));
     overs++;
    }
   }
  }
  prevKnot = bestVal;
 }
 if (reason != 0u) { rays[id].counts = vec4u(0, 0, 0, FALLBACK | reason); }
 else { rays[id].counts = vec4u(0, singles, 0, overs); }
}
)WGSL";
}
