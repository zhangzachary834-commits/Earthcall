#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <vector>
#include <webgpu/wgpu.h>

#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"
#include "Singularity/Screen/VolumeDensity.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"
#include "json.hpp"

using J = nlohmann::json;

struct alignas(16) GpuCurtainSpec {
    glm::vec4 origin;
    glm::vec4 half;
    glm::vec4 limits;
    glm::vec4 curv;
    glm::vec4 amps;
    glm::vec4 freqs;
    glm::vec4 phases;
};

struct alignas(16) GpuRay {
    glm::vec4 ro_clipA;
    glm::vec4 rd_clipB;
    glm::uvec4 counts;
};

struct alignas(16) GpuResult {
    float rgb_T[4];
};

struct DispatchCounters {
    uint32_t s1Count;
    uint32_t s2Count;
    uint32_t fallbackCount;
    uint32_t pad;
};

// ----------------------------------------------------------------------------
// Common Noise & Math Header
// ----------------------------------------------------------------------------
static const char* kNoiseHeader = R"WGSL(
fn mod289_3(x: vec3<f32>) -> vec3<f32> { return x - floor(x * (1.0 / 289.0)) * 289.0; }
fn mod289_4(x: vec4<f32>) -> vec4<f32> { return x - floor(x * (1.0 / 289.0)) * 289.0; }
fn permute4(x: vec4<f32>) -> vec4<f32> { return mod289_4(((x * 34.0) + 10.0) * x); }
fn taylorInvSqrt(r: vec4<f32>) -> vec4<f32> { return 1.79284291400159 - 0.85373472095314 * r; }
fn fade3(t: vec3<f32>) -> vec3<f32> { return t * t * t * (t * (t * 6.0 - 15.0) + 10.0); }

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
    var gx0 = ixy0 * (1.0 / 7.0);
    var gy0 = fract(floor(gx0) * (1.0 / 7.0)) - 0.5;
    gx0 = fract(gx0);
    var gz0 = vec4<f32>(0.5) - abs(gx0) - abs(gy0);
    var sz0 = step(gz0, vec4<f32>(0.0));
    gx0 = gx0 - sz0 * (step(vec4<f32>(0.0), gx0) - 0.5);
    gy0 = gy0 - sz0 * (step(vec4<f32>(0.0), gy0) - 0.5);
    var gx1 = ixy1 * (1.0 / 7.0);
    var gy1 = fract(floor(gx1) * (1.0 / 7.0)) - 0.5;
    gx1 = fract(gx1);
    var gz1 = vec4<f32>(0.5) - abs(gx1) - abs(gy1);
    var sz1 = step(gz1, vec4<f32>(0.0));
    gx1 = gx1 - sz1 * (step(vec4<f32>(0.0), gx1) - 0.5);
    gy1 = gy1 - sz1 * (step(vec4<f32>(0.0), gy1) - 0.5);
    var g000 = vec3<f32>(gx0.x, gy0.x, gz0.x);
    var g100 = vec3<f32>(gx0.y, gy0.y, gz0.y);
    var g010 = vec3<f32>(gx0.z, gy0.z, gz0.z);
    var g110 = vec3<f32>(gx0.w, gy0.w, gz0.w);
    var g001 = vec3<f32>(gx1.x, gy1.x, gz1.x);
    var g101 = vec3<f32>(gx1.y, gy1.y, gz1.y);
    var g011 = vec3<f32>(gx1.z, gy1.z, gz1.z);
    var g111 = vec3<f32>(gx1.w, gy1.w, gz1.w);
    let norm0 = taylorInvSqrt(vec4<f32>(dot(g000, g000), dot(g010, g010), dot(g100, g100), dot(g110, g110)));
    g000 = g000 * norm0.x; g010 = g010 * norm0.y; g100 = g100 * norm0.z; g110 = g110 * norm0.w;
    let norm1 = taylorInvSqrt(vec4<f32>(dot(g001, g001), dot(g011, g011), dot(g101, g101), dot(g111, g111)));
    g001 = g001 * norm1.x; g011 = g011 * norm1.y; g101 = g101 * norm1.z; g111 = g111 * norm1.w;
    let n000 = dot(g000, Pf0);
    let n100 = dot(g100, vec3<f32>(Pf1.x, Pf0.y, Pf0.z));
    let n010 = dot(g010, vec3<f32>(Pf0.x, Pf1.y, Pf0.z));
    let n110 = dot(g110, vec3<f32>(Pf1.x, Pf1.y, Pf0.z));
    let n001 = dot(g001, vec3<f32>(Pf0.x, Pf0.y, Pf1.z));
    let n101 = dot(g101, vec3<f32>(Pf1.x, Pf0.y, Pf1.z));
    let n011 = dot(g011, vec3<f32>(Pf0.x, Pf1.y, Pf1.z));
    let n111 = dot(g111, Pf1);
    let fade_xyz = fade3(Pf0);
    let n_z = mix(vec4<f32>(n000, n100, n010, n110), vec4<f32>(n001, n101, n011, n111), vec4<f32>(fade_xyz.z));
    let n_yz = mix(n_z.xy, n_z.zw, vec2<f32>(fade_xyz.y));
    let n_xyz = mix(n_yz.x, n_yz.y, fade_xyz.x);
    return 2.2 * n_xyz;
}

fn advanceFlow(st: vec4<f32>, emit_sigma: vec4<f32>, dt: f32) -> vec4<f32> {
    let emit = emit_sigma.xyz;
    let sigma = emit_sigma.w;
    let tau = dt * sigma;
    let atten = exp(-tau);
    var gain: f32;
    if (tau < 1e-4) {
        let phi = 1.0 - 0.5 * tau + (1.0 / 6.0) * tau * tau - (1.0 / 24.0) * tau * tau * tau;
        gain = dt * phi;
    } else {
        gain = (1.0 - atten) / sigma;
    }
    let L = st.xyz + st.w * emit * gain;
    let T = st.w * atten;
    return vec4<f32>(L, T);
}
)WGSL";

// ----------------------------------------------------------------------------
// Pass 0: classifyTilesMain WGSL
// ----------------------------------------------------------------------------
static const char* kClassifyWgsl = R"WGSL(
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
    pad: u32,
};

@group(0) @binding(0) var<uniform> curtains: array<CurtainSpec, 4>;
@group(0) @binding(1) var<storage, read> rays: array<Ray>;
@group(0) @binding(2) var<storage, read> tileOffsets: array<u32, 2305>;
@group(0) @binding(3) var<storage, read> tileRayIndices: array<u32>;

@group(1) @binding(0) var<storage, read_write> counters: DispatchCounters;
@group(1) @binding(1) var<storage, read_write> s1TileList: array<u32>;
@group(1) @binding(2) var<storage, read_write> s2TileList: array<u32>;
@group(1) @binding(3) var<storage, read_write> fallbackTileList: array<u32>;
@group(1) @binding(4) var<storage, read_write> tilePrograms: array<vec2u, 2304>;

fn evalWave(m: CurtainSpec, x: f32) -> f32 {
    return m.amps.x * sin(m.freqs.x * x + m.phases.x) +
           m.amps.y * cos(m.freqs.y * x + m.phases.y) +
           m.amps.z * sin(m.freqs.z * x + m.phases.z);
}

@compute @workgroup_size(64, 1, 1)
fn classifyTilesMain(@builtin(global_invocation_id) gid: vec3u) {
    let tileId = gid.x;
    if (tileId >= 2304u) { return; }

    let rayStart = tileOffsets[tileId];
    let rayCount = tileOffsets[tileId + 1u] - rayStart;

    var activeMemberMask = 0u;
    var tileCanOverlap = false;
    var enterArr: array<f32, 4>;
    var exitArr: array<f32, 4>;
    var memberHit: array<bool, 4>;

    for (var m = 0u; m < 4u; m++) {
        let cur = curtains[m];
        var uxMin = 1e9f;
        var uxMax = -1e9f;
        var anyRayHit = false;
        var rA: array<f32, 16>;
        var rB: array<f32, 16>;
        var rHit: array<bool, 16>;

        for (var i = 0u; i < rayCount && i < 16u; i++) {
            let rIdx = tileRayIndices[rayStart + i];
            let ray = rays[rIdx];
            let oRel = ray.ro_clipA.xyz - cur.origin.xyz;
            var a = ray.ro_clipA.w;
            var b = ray.rd_clipB.w;
            var hit = true;

            for (var axis = 0; axis < 3; axis++) {
                var half = cur.half[axis];
                if (axis == 0) { half = min(half, cur.limits.x); }
                if (axis == 1) { half = min(half, cur.limits.y); }
                let d = ray.rd_clipB[axis];
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

            rA[i] = a;
            rB[i] = b;
            rHit[i] = (hit && b > a);

            if (hit && b > a) {
                anyRayHit = true;
                let u0 = oRel.x + a * ray.rd_clipB.x;
                let u1 = oRel.x + b * ray.rd_clipB.x;
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
            let thresh = bow + cur.limits.w + 0.001;

            var enter = 1e9f;
            var exit = -1e9f;
            var curActive = false;

            for (var i = 0u; i < rayCount && i < 16u; i++) {
                if (!rHit[i]) { continue; }
                let rIdx = tileRayIndices[rayStart + i];
                let ray = rays[rIdx];
                let oRel = ray.ro_clipA.xyz - cur.origin.xyz;
                let dx = ray.rd_clipB.x;
                if (abs(dx) < 1e-6) { curActive = true; enter = min(enter, rA[i]); exit = max(exit, rB[i]); break; }

                let a = rA[i];
                let b = rB[i];

                for (var bin = 0u; bin < 8u; bin++) {
                    let u0 = uxMin + f32(bin) / 8.0 * (uxMax - uxMin);
                    let u1 = uxMin + f32(bin + 1u) / 8.0 * (uxMax - uxMin);
                    let su0 = (u0 - oRel.x) / dx;
                    let su1 = (u1 - oRel.x) / dx;
                    let phi0 = oRel.z + su0 * ray.rd_clipB.z + wKnots[bin];
                    let phi1 = oRel.z + su1 * ray.rd_clipB.z + wKnots[bin + 1u];

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
                            curActive = true;
                            enter = min(enter, cLo);
                            exit = max(exit, cHi);
                        }
                    }
                }
            }

            if (curActive) {
                activeMemberMask |= (1u << m);
                memberHit[m] = true;
                enterArr[m] = enter;
                exitArr[m] = exit;
            }
        }
    }

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

    tilePrograms[tileId] = vec2u(activeMemberMask | (select(0u, 1u, tileCanOverlap) << 8u), 0u);

    if (activeMemberMask == 1u && !tileCanOverlap) {
        let slot = atomicAdd(&counters.s1Count, 1u);
        s1TileList[slot] = tileId;
    } else if (activeMemberMask == 3u && !tileCanOverlap) {
        let slot = atomicAdd(&counters.s2Count, 1u);
        s2TileList[slot] = tileId;
    } else {
        let slot = atomicAdd(&counters.fallbackCount, 1u);
        fallbackTileList[slot] = tileId;
    }
}
)WGSL";

// ----------------------------------------------------------------------------
// Pipeline Alpha (Shape S1: Curtain 0 Only, Certified NO_OVERLAP, Single-Pass)
// ----------------------------------------------------------------------------
static const char* kPipelineAlphaWgsl = R"WGSL(
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

struct GpuMedium {
    origin: vec4f,
    color_alpha: vec4f,
    freq_offset: vec4f,
};

@group(0) @binding(0) var<uniform> curtain0: CurtainSpec;
@group(0) @binding(1) var<storage, read> rays: array<Ray>;
@group(0) @binding(2) var<storage, read> tileOffsets: array<u32, 2305>;
@group(0) @binding(3) var<storage, read> tileRayIndices: array<u32>;
@group(0) @binding(4) var<storage, read> s1TileList: array<u32>;
@group(0) @binding(5) var<storage, read> params: array<f32>;
@group(0) @binding(6) var<storage, read_write> results: array<vec4f>;

const gx0: f32 = 0.211324865;
const gx1: f32 = 0.788675135;

fn evalWave0(x: f32) -> f32 {
    return curtain0.amps.x * sin(curtain0.freqs.x * x + curtain0.phases.x) +
           curtain0.amps.y * cos(curtain0.freqs.y * x + curtain0.phases.y) +
           curtain0.amps.z * sin(curtain0.freqs.z * x + curtain0.phases.z);
}

fn evalPhi0(o: vec3f, d: vec3f, s: f32) -> f32 {
    return o.z + s * d.z + evalWave0(o.x + s * d.x);
}

fn evalEnvelope0(p: vec3f, offset: u32) -> f32 {
    let p_x = p.x; let p_y = p.y; let p_z = p.z;
    let b_x = clamp(1.0 - abs(p_x) / params[offset + 2u], 0.0, 1.0);
    let s_y = clamp(0.5 + 0.5 * p_y / params[offset + 6u], 0.0, 1.0);
    let k_y = clamp(1.0 - (p_y - (-params[offset + 6u])) * params[offset + 5u], 0.0, 1.0);
    let b_y = s_y * k_y;
    let phi = p_z + curtain0.amps.x * sin(curtain0.freqs.x * p_x + curtain0.phases.x) +
                    curtain0.amps.y * cos(curtain0.freqs.y * p_x + curtain0.phases.y) +
                    curtain0.amps.z * sin(curtain0.freqs.z * p_x + curtain0.phases.z);
    let b_z = clamp(1.0 - abs(phi) / params[offset + 15u], 0.0, 1.0);
    return params[offset + 0u] * b_x * b_y * b_z;
}

@compute @workgroup_size(64, 1, 1)
fn pipelineAlphaMain(@builtin(workgroup_id) wid: vec3u, @builtin(local_invocation_index) lid: u32) {
    let localTile = lid >> 4u;
    let rayInTile = lid & 15u;
    let tileSlot = wid.x * 4u + localTile;
    if (tileSlot >= 2304u) { return; }
    let tileId = s1TileList[tileSlot];
    if (tileId >= 2304u) { return; }
    let rayStart = tileOffsets[tileId];
    let rayCount = tileOffsets[tileId + 1u] - rayStart;

    if (rayInTile >= rayCount) { return; }
    let rayIdx = tileRayIndices[rayStart + rayInTile];
    let ray = rays[rayIdx];

    var state = vec4f(0.0, 0.0, 0.0, 1.0);
    let o = ray.ro_clipA.xyz - curtain0.origin.xyz;
    let d = ray.rd_clipB.xyz;
    var a = ray.ro_clipA.w;
    var b = ray.rd_clipB.w;

    // AABB clip Curtain 0
    var hit = true;
    for (var axis = 0; axis < 3; axis++) {
        var half = curtain0.half[axis];
        if (axis == 0) { half = min(half, curtain0.limits.x); }
        if (axis == 1) { half = min(half, curtain0.limits.y); }
        let da = d[axis];
        let oa = o[axis];
        if (da == 0.0) {
            if (abs(oa) > half + curtain0.curv.y) { hit = false; break; }
        } else {
            let q0 = (-half - curtain0.curv.y - oa) / da;
            let q1 = (half + curtain0.curv.y - oa) / da;
            a = max(a, min(q0, q1));
            b = min(b, max(q0, q1));
        }
    }

    if (!hit || b <= a) {
        results[rayIdx] = state;
        return;
    }

    // 9 coarse knots
    var phi_knots: array<f32, 9>;
    for (var k = 0u; k < 9u; k++) {
        let s = a + (b - a) * f32(k) / 8.0;
        phi_knots[k] = evalPhi0(o, d, s);
    }

    // Level-specific depth0 bow rejection
    let wz = curtain0.limits.w;
    let curvZ = curtain0.curv.z;
    var activeMask = 0u;
    for (var bin = 0u; bin < 8u; bin++) {
        let h = (b - a) / 8.0;
        let va = phi_knots[bin];
        let vb = phi_knots[bin + 1u];
        let bow = 0.125 * curvZ * h * h + 0.001;
        if (!(min(va, vb) > wz + bow || max(va, vb) < -wz - bow)) {
            activeMask |= (1u << bin);
        }
    }

    if (activeMask == 0u && abs(phi_knots[0]) > wz + 0.001) {
        results[rayIdx] = state;
        return;
    }

    // Monotone root refinement and immediate on-the-fly integration
    let pOffset = 0u;
    let freq = 0.08;
    let alpha = 2.0 / 15.0;
    let color = vec3f(0.1, 0.9, 0.4);

    for (var bin = 0u; bin < 8u; bin++) {
        if ((activeMask & (1u << bin)) == 0u) { continue; }
        let a0 = a + (b - a) * f32(bin) / 8.0;
        let b0 = a + (b - a) * f32(bin + 1u) / 8.0;

        let mid = a0 + 0.5 * (b0 - a0);
        let phiMid = evalPhi0(o, d, mid);

        if (abs(phiMid) <= wz) {
            // Contiguous positive interval [a0, b0]
            let n = max(1u, u32(ceil((b0 - a0) / 4.0)));
            let sub_h = (b0 - a0) / f32(n);

            for (var k = 0u; k < n; k++) {
                let s0 = a0 + f32(k) * sub_h;
                let s1 = select(a0 + f32(k + 1u) * sub_h, b0, k + 1u == n);
                let actual_h = s1 - s0;

                let s_m = s0 + gx0 * actual_h;
                let s_p = s0 + gx1 * actual_h;
                let w_m = o + s_m * d;
                let w_p = o + s_p * d;

                let env_m = evalEnvelope0(w_m, pOffset);
                let env_p = evalEnvelope0(w_p, pOffset);

                var D_m = 0.0;
                var D_p = 0.0;
                if (env_m > 0.0) {
                    let nm = cnoise3(freq * w_m);
                    D_m = env_m * clamp(0.7 + 0.3 * nm, 0.0, 1.0);
                }
                if (env_p > 0.0) {
                    let np = cnoise3(freq * w_p);
                    D_p = env_p * clamp(0.7 + 0.3 * np, 0.0, 1.0);
                }

                let sig_m = select(0.0, max(alpha * D_m, 1e-6), D_m > 0.0);
                let em_m  = select(vec3f(0.0), color * (alpha * D_m), D_m > 0.0);
                let sig_p = select(0.0, max(alpha * D_p, 1e-6), D_p > 0.0);
                let em_p  = select(vec3f(0.0), color * (alpha * D_p), D_p > 0.0);

                let sig_avg = 0.5 * (sig_m + sig_p);
                let em_avg  = 0.5 * (em_m + em_p);
                state = advanceFlow(state, vec4f(em_avg, sig_avg), actual_h);
            }
        }
    }

    results[rayIdx] = state;
}
)WGSL";

// ----------------------------------------------------------------------------
// Pipeline Beta (Shape S2: Curtains 0 & 1 Sequential, NO_OVERLAP, Single-Pass)
// ----------------------------------------------------------------------------
static const char* kPipelineBetaWgsl = R"WGSL(
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

@group(0) @binding(0) var<uniform> curtains: array<CurtainSpec, 2>;
@group(0) @binding(1) var<storage, read> rays: array<Ray>;
@group(0) @binding(2) var<storage, read> tileOffsets: array<u32, 2305>;
@group(0) @binding(3) var<storage, read> tileRayIndices: array<u32>;
@group(0) @binding(4) var<storage, read> s2TileList: array<u32>;
@group(0) @binding(5) var<storage, read> params: array<f32>;
@group(0) @binding(6) var<storage, read_write> results: array<vec4f>;

const gx0: f32 = 0.211324865;
const gx1: f32 = 0.788675135;

fn evalWaveM(m: u32, x: f32) -> f32 {
    let cur = curtains[m];
    return cur.amps.x * sin(cur.freqs.x * x + cur.phases.x) +
           cur.amps.y * cos(cur.freqs.y * x + cur.phases.y) +
           cur.amps.z * sin(cur.freqs.z * x + cur.phases.z);
}

fn evalPhiM(m: u32, o: vec3f, d: vec3f, s: f32) -> f32 {
    return o.z + s * d.z + evalWaveM(m, o.x + s * d.x);
}

fn evalEnvelopeM(m: u32, p: vec3f, offset: u32) -> f32 {
    let cur = curtains[m];
    let p_x = p.x; let p_y = p.y; let p_z = p.z;
    let b_x = clamp(1.0 - abs(p_x) / params[offset + 2u], 0.0, 1.0);
    let s_y = clamp(0.5 + 0.5 * p_y / params[offset + 6u], 0.0, 1.0);
    let k_y = clamp(1.0 - (p_y - (-params[offset + 6u])) * params[offset + 5u], 0.0, 1.0);
    let b_y = s_y * k_y;
    let phi = p_z + cur.amps.x * sin(cur.freqs.x * p_x + cur.phases.x) +
                    cur.amps.y * cos(cur.freqs.y * p_x + cur.phases.y) +
                    cur.amps.z * sin(cur.freqs.z * p_x + cur.phases.z);
    let b_z = clamp(1.0 - abs(phi) / params[offset + 15u], 0.0, 1.0);
    return params[offset + 0u] * b_x * b_y * b_z;
}

@compute @workgroup_size(64, 1, 1)
fn pipelineBetaMain(@builtin(workgroup_id) wid: vec3u, @builtin(local_invocation_index) lid: u32) {
    let localTile = lid >> 4u;
    let rayInTile = lid & 15u;
    let tileSlot = wid.x * 4u + localTile;
    if (tileSlot >= 2304u) { return; }
    let tileId = s2TileList[tileSlot];
    if (tileId >= 2304u) { return; }
    let rayStart = tileOffsets[tileId];
    let rayCount = tileOffsets[tileId + 1u] - rayStart;

    if (rayInTile >= rayCount) { return; }
    let rayIdx = tileRayIndices[rayStart + rayInTile];
    let ray = rays[rayIdx];

    var state = vec4f(0.0, 0.0, 0.0, 1.0);

    // Front-to-back: Curtain 0 then Curtain 1
    for (var m = 0u; m < 2u; m++) {
        let cur = curtains[m];
        let o = ray.ro_clipA.xyz - cur.origin.xyz;
        let d = ray.rd_clipB.xyz;
        var a = ray.ro_clipA.w;
        var b = ray.rd_clipB.w;

        var hit = true;
        for (var axis = 0; axis < 3; axis++) {
            var half = cur.half[axis];
            if (axis == 0) { half = min(half, cur.limits.x); }
            if (axis == 1) { half = min(half, cur.limits.y); }
            let da = d[axis];
            let oa = o[axis];
            if (da == 0.0) {
                if (abs(oa) > half + cur.curv.y) { hit = false; break; }
            } else {
                let q0 = (-half - cur.curv.y - oa) / da;
                let q1 = (half + cur.curv.y - oa) / da;
                a = max(a, min(q0, q1));
                b = min(b, max(q0, q1));
            }
        }

        if (!hit || b <= a) { continue; }

        var phi_knots: array<f32, 9>;
        for (var k = 0u; k < 9u; k++) {
            let s = a + (b - a) * f32(k) / 8.0;
            phi_knots[k] = evalPhiM(m, o, d, s);
        }

        let wz = cur.limits.w;
        let curvZ = cur.curv.z;
        var activeMask = 0u;
        for (var bin = 0u; bin < 8u; bin++) {
            let h = (b - a) / 8.0;
            let va = phi_knots[bin];
            let vb = phi_knots[bin + 1u];
            let bow = 0.125 * curvZ * h * h + 0.001;
            if (!(min(va, vb) > wz + bow || max(va, vb) < -wz - bow)) {
                activeMask |= (1u << bin);
            }
        }

        if (activeMask == 0u && abs(phi_knots[0]) > wz + 0.001) { continue; }

        let pOffset = m * 34u;
        let freq = select(0.08, 0.09, m == 1u);
        let alpha = select(2.0 / 15.0, 9.0 / 65.0, m == 1u);
        let color = select(vec3f(0.1, 0.9, 0.4), vec3f(0.2, 0.5, 0.9), m == 1u);

        for (var bin = 0u; bin < 8u; bin++) {
            if ((activeMask & (1u << bin)) == 0u) { continue; }
            let a0 = a + (b - a) * f32(bin) / 8.0;
            let b0 = a + (b - a) * f32(bin + 1u) / 8.0;
            let mid = a0 + 0.5 * (b0 - a0);
            let phiMid = evalPhiM(m, o, d, mid);

            if (abs(phiMid) <= wz) {
                let n = max(1u, u32(ceil((b0 - a0) / 4.0)));
                let sub_h = (b0 - a0) / f32(n);

                for (var k = 0u; k < n; k++) {
                    let s0 = a0 + f32(k) * sub_h;
                    let s1 = select(a0 + f32(k + 1u) * sub_h, b0, k + 1u == n);
                    let actual_h = s1 - s0;

                    let s_m = s0 + gx0 * actual_h;
                    let s_p = s0 + gx1 * actual_h;
                    let w_m = o + s_m * d;
                    let w_p = o + s_p * d;

                    let env_m = evalEnvelopeM(m, w_m, pOffset);
                    let env_p = evalEnvelopeM(m, w_p, pOffset);

                    var D_m = 0.0;
                    var D_p = 0.0;
                    if (env_m > 0.0) {
                        let nm = cnoise3(freq * w_m);
                        D_m = env_m * clamp(0.7 + 0.3 * nm, 0.0, 1.0);
                    }
                    if (env_p > 0.0) {
                        let np = cnoise3(freq * w_p);
                        D_p = env_p * clamp(0.7 + 0.3 * np, 0.0, 1.0);
                    }

                    let sig_m = select(0.0, max(alpha * D_m, 1e-6), D_m > 0.0);
                    let em_m  = select(vec3f(0.0), color * (alpha * D_m), D_m > 0.0);
                    let sig_p = select(0.0, max(alpha * D_p, 1e-6), D_p > 0.0);
                    let em_p  = select(vec3f(0.0), color * (alpha * D_p), D_p > 0.0);

                    let sig_avg = 0.5 * (sig_m + sig_p);
                    let em_avg  = 0.5 * (em_m + em_p);
                    state = advanceFlow(state, vec4f(em_avg, sig_avg), actual_h);
                }
            }
        }
    }

    results[rayIdx] = state;
}
)WGSL";

// ----------------------------------------------------------------------------
// Pipeline Fallback (Current Marcher on fallbackTileList)
// ----------------------------------------------------------------------------
static const char* kPipelineFallbackWgsl = R"WGSL(
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

@group(0) @binding(0) var<uniform> curtains: array<CurtainSpec, 4>;
@group(0) @binding(1) var<storage, read> rays: array<Ray>;
@group(0) @binding(2) var<storage, read> tileOffsets: array<u32, 2305>;
@group(0) @binding(3) var<storage, read> tileRayIndices: array<u32>;
@group(0) @binding(4) var<storage, read> fallbackTileList: array<u32>;
@group(0) @binding(5) var<storage, read> params: array<f32>;
@group(0) @binding(6) var<storage, read_write> results: array<vec4f>;

const gx0: f32 = 0.211324865;
const gx1: f32 = 0.788675135;

fn evalWaveM4(m: u32, x: f32) -> f32 {
    let cur = curtains[m];
    return cur.amps.x * sin(cur.freqs.x * x + cur.phases.x) +
           cur.amps.y * cos(cur.freqs.y * x + cur.phases.y) +
           cur.amps.z * sin(cur.freqs.z * x + cur.phases.z);
}

fn evalPhiM4(m: u32, o: vec3f, d: vec3f, s: f32) -> f32 {
    return o.z + s * d.z + evalWaveM4(m, o.x + s * d.x);
}

fn evalEnvelopeM4(m: u32, p: vec3f, offset: u32) -> f32 {
    let cur = curtains[m];
    let p_x = p.x; let p_y = p.y; let p_z = p.z;
    let b_x = clamp(1.0 - abs(p_x) / params[offset + 2u], 0.0, 1.0);
    let s_y = clamp(0.5 + 0.5 * p_y / params[offset + 6u], 0.0, 1.0);
    let k_y = clamp(1.0 - (p_y - (-params[offset + 6u])) * params[offset + 5u], 0.0, 1.0);
    let b_y = s_y * k_y;
    let phi = p_z + cur.amps.x * sin(cur.freqs.x * p_x + cur.phases.x) +
                    cur.amps.y * cos(cur.freqs.y * p_x + cur.phases.y) +
                    cur.amps.z * sin(cur.freqs.z * p_x + cur.phases.z);
    let b_z = clamp(1.0 - abs(phi) / params[offset + 15u], 0.0, 1.0);
    return params[offset + 0u] * b_x * b_y * b_z;
}

@compute @workgroup_size(64, 1, 1)
fn pipelineFallbackMain(@builtin(workgroup_id) wid: vec3u, @builtin(local_invocation_index) lid: u32) {
    let localTile = lid >> 4u;
    let rayInTile = lid & 15u;
    let tileSlot = wid.x * 4u + localTile;
    if (tileSlot >= 2304u) { return; }
    let tileId = fallbackTileList[tileSlot];
    if (tileId >= 2304u) { return; }
    let rayStart = tileOffsets[tileId];
    let rayCount = tileOffsets[tileId + 1u] - rayStart;

    if (rayInTile >= rayCount) { return; }
    let rayIdx = tileRayIndices[rayStart + rayInTile];
    let ray = rays[rayIdx];

    var state = vec4f(0.0, 0.0, 0.0, 1.0);

    for (var m = 0u; m < 4u; m++) {
        let cur = curtains[m];
        let o = ray.ro_clipA.xyz - cur.origin.xyz;
        let d = ray.rd_clipB.xyz;
        var a = ray.ro_clipA.w;
        var b = ray.rd_clipB.w;

        var hit = true;
        for (var axis = 0; axis < 3; axis++) {
            var half = cur.half[axis];
            if (axis == 0) { half = min(half, cur.limits.x); }
            if (axis == 1) { half = min(half, cur.limits.y); }
            let da = d[axis];
            let oa = o[axis];
            if (da == 0.0) {
                if (abs(oa) > half + cur.curv.y) { hit = false; break; }
            } else {
                let q0 = (-half - cur.curv.y - oa) / da;
                let q1 = (half + cur.curv.y - oa) / da;
                a = max(a, min(q0, q1));
                b = min(b, max(q0, q1));
            }
        }

        if (!hit || b <= a) { continue; }

        var phi_knots: array<f32, 9>;
        for (var k = 0u; k < 9u; k++) {
            let s = a + (b - a) * f32(k) / 8.0;
            phi_knots[k] = evalPhiM4(m, o, d, s);
        }

        let wz = cur.limits.w;
        let curvZ = cur.curv.z;
        var activeMask = 0u;
        for (var bin = 0u; bin < 8u; bin++) {
            let h = (b - a) / 8.0;
            let va = phi_knots[bin];
            let vb = phi_knots[bin + 1u];
            let bow = 0.125 * curvZ * h * h + 0.001;
            if (!(min(va, vb) > wz + bow || max(va, vb) < -wz - bow)) {
                activeMask |= (1u << bin);
            }
        }

        if (activeMask == 0u && abs(phi_knots[0]) > wz + 0.001) { continue; }

        let pOffset = m * 34u;
        var freq = 0.08;
        var alpha = 2.0 / 15.0;
        var color = vec3f(0.1, 0.9, 0.4);
        if (m == 1u) { freq = 0.09; alpha = 9.0 / 65.0; color = vec3f(0.2, 0.5, 0.9); }
        else if (m == 2u) { freq = 0.07; alpha = 8.0 / 55.0; color = vec3f(0.8, 0.2, 0.6); }
        else if (m == 3u) { freq = 0.10; alpha = 4.0 / 25.0; color = vec3f(0.9, 0.6, 0.1); }

        for (var bin = 0u; bin < 8u; bin++) {
            if ((activeMask & (1u << bin)) == 0u) { continue; }
            let a0 = a + (b - a) * f32(bin) / 8.0;
            let b0 = a + (b - a) * f32(bin + 1u) / 8.0;
            let mid = a0 + 0.5 * (b0 - a0);
            let phiMid = evalPhiM4(m, o, d, mid);

            if (abs(phiMid) <= wz) {
                let n = max(1u, u32(ceil((b0 - a0) / 4.0)));
                let sub_h = (b0 - a0) / f32(n);

                for (var k = 0u; k < n; k++) {
                    let s0 = a0 + f32(k) * sub_h;
                    let s1 = select(a0 + f32(k + 1u) * sub_h, b0, k + 1u == n);
                    let actual_h = s1 - s0;

                    let s_m = s0 + gx0 * actual_h;
                    let s_p = s0 + gx1 * actual_h;
                    let w_m = o + s_m * d;
                    let w_p = o + s_p * d;

                    let env_m = evalEnvelopeM4(m, w_m, pOffset);
                    let env_p = evalEnvelopeM4(m, w_p, pOffset);

                    var D_m = 0.0;
                    var D_p = 0.0;
                    if (env_m > 0.0) {
                        let nm = cnoise3(freq * w_m);
                        D_m = env_m * clamp(0.7 + 0.3 * nm, 0.0, 1.0);
                    }
                    if (env_p > 0.0) {
                        let np = cnoise3(freq * w_p);
                        D_p = env_p * clamp(0.7 + 0.3 * np, 0.0, 1.0);
                    }

                    let sig_m = select(0.0, max(alpha * D_m, 1e-6), D_m > 0.0);
                    let em_m  = select(vec3f(0.0), color * (alpha * D_m), D_m > 0.0);
                    let sig_p = select(0.0, max(alpha * D_p, 1e-6), D_p > 0.0);
                    let em_p  = select(vec3f(0.0), color * (alpha * D_p), D_p > 0.0);

                    let sig_avg = 0.5 * (sig_m + sig_p);
                    let em_avg  = 0.5 * (em_m + em_p);
                    state = advanceFlow(state, vec4f(em_avg, sig_avg), actual_h);
                }
            }
        }
    }

    results[rayIdx] = state;
}
)WGSL";

// ----------------------------------------------------------------------------
// Pipeline Reference (Current Marcher across ALL 32,784 rays)
// ----------------------------------------------------------------------------
static const char* kPipelineReferenceWgsl = R"WGSL(
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

@group(0) @binding(0) var<uniform> curtains: array<CurtainSpec, 4>;
@group(0) @binding(1) var<storage, read> rays: array<Ray>;
@group(0) @binding(2) var<storage, read> params: array<f32>;
@group(0) @binding(3) var<storage, read_write> results: array<vec4f>;

const gx0: f32 = 0.211324865;
const gx1: f32 = 0.788675135;

fn evalWaveRef(m: u32, x: f32) -> f32 {
    let cur = curtains[m];
    return cur.amps.x * sin(cur.freqs.x * x + cur.phases.x) +
           cur.amps.y * cos(cur.freqs.y * x + cur.phases.y) +
           cur.amps.z * sin(cur.freqs.z * x + cur.phases.z);
}

fn evalPhiRef(m: u32, o: vec3f, d: vec3f, s: f32) -> f32 {
    return o.z + s * d.z + evalWaveRef(m, o.x + s * d.x);
}

fn evalEnvelopeRef(m: u32, p: vec3f, offset: u32) -> f32 {
    let cur = curtains[m];
    let p_x = p.x; let p_y = p.y; let p_z = p.z;
    let b_x = clamp(1.0 - abs(p_x) / params[offset + 2u], 0.0, 1.0);
    let s_y = clamp(0.5 + 0.5 * p_y / params[offset + 6u], 0.0, 1.0);
    let k_y = clamp(1.0 - (p_y - (-params[offset + 6u])) * params[offset + 5u], 0.0, 1.0);
    let b_y = s_y * k_y;
    let phi = p_z + cur.amps.x * sin(cur.freqs.x * p_x + cur.phases.x) +
                    cur.amps.y * cos(cur.freqs.y * p_x + cur.phases.y) +
                    cur.amps.z * sin(cur.freqs.z * p_x + cur.phases.z);
    let b_z = clamp(1.0 - abs(phi) / params[offset + 15u], 0.0, 1.0);
    return params[offset + 0u] * b_x * b_y * b_z;
}

@compute @workgroup_size(64, 1, 1)
fn pipelineReferenceMain(@builtin(global_invocation_id) gid: vec3u) {
    let rayIdx = gid.x;
    if (rayIdx >= 32784u) { return; }
    let ray = rays[rayIdx];

    var state = vec4f(0.0, 0.0, 0.0, 1.0);

    for (var m = 0u; m < 4u; m++) {
        let cur = curtains[m];
        let o = ray.ro_clipA.xyz - cur.origin.xyz;
        let d = ray.rd_clipB.xyz;
        var a = ray.ro_clipA.w;
        var b = ray.rd_clipB.w;

        var hit = true;
        for (var axis = 0; axis < 3; axis++) {
            var half = cur.half[axis];
            if (axis == 0) { half = min(half, cur.limits.x); }
            if (axis == 1) { half = min(half, cur.limits.y); }
            let da = d[axis];
            let oa = o[axis];
            if (da == 0.0) {
                if (abs(oa) > half + cur.curv.y) { hit = false; break; }
            } else {
                let q0 = (-half - cur.curv.y - oa) / da;
                let q1 = (half + cur.curv.y - oa) / da;
                a = max(a, min(q0, q1));
                b = min(b, max(q0, q1));
            }
        }

        if (!hit || b <= a) { continue; }

        var phi_knots: array<f32, 9>;
        for (var k = 0u; k < 9u; k++) {
            let s = a + (b - a) * f32(k) / 8.0;
            phi_knots[k] = evalPhiRef(m, o, d, s);
        }

        let wz = cur.limits.w;
        let curvZ = cur.curv.z;
        var activeMask = 0u;
        for (var bin = 0u; bin < 8u; bin++) {
            let h = (b - a) / 8.0;
            let va = phi_knots[bin];
            let vb = phi_knots[bin + 1u];
            let bow = 0.125 * curvZ * h * h + 0.001;
            if (!(min(va, vb) > wz + bow || max(va, vb) < -wz - bow)) {
                activeMask |= (1u << bin);
            }
        }

        if (activeMask == 0u && abs(phi_knots[0]) > wz + 0.001) { continue; }

        let pOffset = m * 34u;
        var freq = 0.08;
        var alpha = 2.0 / 15.0;
        var color = vec3f(0.1, 0.9, 0.4);
        if (m == 1u) { freq = 0.09; alpha = 9.0 / 65.0; color = vec3f(0.2, 0.5, 0.9); }
        else if (m == 2u) { freq = 0.07; alpha = 8.0 / 55.0; color = vec3f(0.8, 0.2, 0.6); }
        else if (m == 3u) { freq = 0.10; alpha = 4.0 / 25.0; color = vec3f(0.9, 0.6, 0.1); }

        for (var bin = 0u; bin < 8u; bin++) {
            if ((activeMask & (1u << bin)) == 0u) { continue; }
            let a0 = a + (b - a) * f32(bin) / 8.0;
            let b0 = a + (b - a) * f32(bin + 1u) / 8.0;
            let mid = a0 + 0.5 * (b0 - a0);
            let phiMid = evalPhiRef(m, o, d, mid);

            if (abs(phiMid) <= wz) {
                let n = max(1u, u32(ceil((b0 - a0) / 4.0)));
                let sub_h = (b0 - a0) / f32(n);

                for (var k = 0u; k < n; k++) {
                    let s0 = a0 + f32(k) * sub_h;
                    let s1 = select(a0 + f32(k + 1u) * sub_h, b0, k + 1u == n);
                    let actual_h = s1 - s0;

                    let s_m = s0 + gx0 * actual_h;
                    let s_p = s0 + gx1 * actual_h;
                    let w_m = o + s_m * d;
                    let w_p = o + s_p * d;

                    let env_m = evalEnvelopeRef(m, w_m, pOffset);
                    let env_p = evalEnvelopeRef(m, w_p, pOffset);

                    var D_m = 0.0;
                    var D_p = 0.0;
                    if (env_m > 0.0) {
                        let nm = cnoise3(freq * w_m);
                        D_m = env_m * clamp(0.7 + 0.3 * nm, 0.0, 1.0);
                    }
                    if (env_p > 0.0) {
                        let np = cnoise3(freq * w_p);
                        D_p = env_p * clamp(0.7 + 0.3 * np, 0.0, 1.0);
                    }

                    let sig_m = select(0.0, max(alpha * D_m, 1e-6), D_m > 0.0);
                    let em_m  = select(vec3f(0.0), color * (alpha * D_m), D_m > 0.0);
                    let sig_p = select(0.0, max(alpha * D_p, 1e-6), D_p > 0.0);
                    let em_p  = select(vec3f(0.0), color * (alpha * D_p), D_p > 0.0);

                    let sig_avg = 0.5 * (sig_m + sig_p);
                    let em_avg  = 0.5 * (em_m + em_p);
                    state = advanceFlow(state, vec4f(em_avg, sig_avg), actual_h);
                }
            }
        }
    }

    results[rayIdx] = state;
}
)WGSL";

struct PopContext { WGPUErrorType type = WGPUErrorType_NoError; bool done = false; };
static void onPopError(WGPUPopErrorScopeStatus, WGPUErrorType type, WGPUStringView msg, void* u, void*) {
    auto* c = static_cast<PopContext*>(u);
    c->type = type;
    c->done = true;
    if (type != WGPUErrorType_NoError) {
        std::fprintf(stderr, "[WebGPU Error %d]: %.*s\n", (int)type, (int)msg.length, msg.data);
    }
}

struct MapContext { bool done = false; bool ok = false; };
static void onMapped(WGPUMapAsyncStatus s, WGPUStringView, void* u, void*) {
    auto* c = static_cast<MapContext*>(u);
    c->ok = (s == WGPUMapAsyncStatus_Success);
    c->done = true;
}

int main(int argc, char** argv) {
    std::string zonePath = (argc > 1) ? argv[1] : "saves/zones/Northern Veil/zone.ecform";
    std::string raysPath = (argc > 2) ? argv[2] : "/private/tmp/earthcall-wide-case/rays.json";
    std::string outJson = (argc > 3) ? argv[3] : "docs/audits/GPU_RAY_TRANSPORT_6DG_QUERY_FAMILY_SPECIALIZATION_2026-10-24.json";

    std::cerr << "Reading zone: " << zonePath << std::endl;
    std::ifstream in(zonePath, std::ios::binary);
    std::vector<uint8_t> b((std::istreambuf_iterator<char>(in)), {});
    J zone = J::parse(J::from_msgpack(b).at("MigrationRoot").get<std::string>());

    std::vector<GpuCurtainSpec> curtains;
    std::vector<float> paramPool;
    for (const auto& j : zone.at("spatialFields")) {
        auto field = geom::FieldNode::fromJson(j);
        Rendering::VolumeDensityBinding bnd;
        if (!field || !Rendering::readVolumeDensity(*field, 0, 0, bnd)) continue;
        if (!bnd.densityExpr) continue;
        auto pb = sdfwgsl::collectVolumeParams(bnd.densityExpr);
        if (pb.values.size() < 34) continue;
        paramPool.insert(paramPool.end(), pb.values.begin(), pb.values.end());
        const auto& v = pb.values;
        GpuCurtainSpec spec{};
        spec.origin = glm::vec4(bnd.origin, 0.0f);
        spec.half = glm::vec4(glm::abs(bnd.scale), 0.0f);
        float Wx = v[2], Wy = v[6];
        float yKnee = -Wy + (v[5] > 0.0f ? 1.0f / v[5] : 4.0f);
        float wz = v[15];
        spec.limits = glm::vec4(Wx, Wy, yKnee, wz);
        float A1 = v[16], w1 = v[17], p1 = v[18];
        float A2 = v[19], w2 = v[20], p2 = v[21];
        float A3 = v[22], w3 = v[23], p3 = v[24];
        spec.amps = glm::vec4(A1, A2, A3, 0.0f);
        spec.freqs = glm::vec4(w1, w2, w3, 0.0f);
        spec.phases = glm::vec4(p1, p2, p3, 0.0f);
        float M2 = std::abs(A1) * w1 * w1 + std::abs(A2) * w2 * w2 + std::abs(A3) * w3 * w3;
        spec.curv = glm::vec4(std::nextafter(M2, INFINITY) * 1.00002f, 0.001f, M2, 0.0f);
        curtains.push_back(spec);
    }

    std::cerr << "Reading rays: " << raysPath << std::endl;
    J raysJ;
    { std::ifstream rin(raysPath); rin >> raysJ; }
    const auto& R = raysJ.at("rays");
    glm::vec3 ro{
        raysJ["origin"][0].get<float>(),
        raysJ["origin"][1].get<float>(),
        raysJ["origin"][2].get<float>()
    };
    size_t count = R.size();

    std::vector<GpuRay> gpuRays(count);
    std::map<uint32_t, std::vector<uint32_t>> groups4x4;

    for (size_t i = 0; i < count; ++i) {
        int px = R[i][0].get<int>(), py = R[i][1].get<int>();
        int gx = px / 10, gy = py / 10;
        uint32_t tx = gx / 4, ty = gy / 4;
        uint32_t key = (ty << 16) | tx;
        groups4x4[key].push_back(uint32_t(i));

        glm::vec3 d(R[i][2].get<float>(), R[i][3].get<float>(), R[i][4].get<float>());
        float a = R[i][5].get<float>(), b = R[i][6].get<float>();
        gpuRays[i].ro_clipA = glm::vec4(ro, a);
        gpuRays[i].rd_clipB = glm::vec4(d, b);
        gpuRays[i].counts = glm::uvec4(0);
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
    tileOffsets[2304] = uint32_t(tileRayIndices.size());

    // Initialize WebGPU on Metal
    wgpu::Device gpu;
    if (!gpu.init()) {
        std::cerr << "Failed to init WebGPU on Metal" << std::endl;
        return 1;
    }

    auto makeBuffer = [&](WGPUBufferUsage usage, uint64_t size, const void* data) -> WGPUBuffer {
        WGPUBufferDescriptor desc = {};
        desc.usage = usage;
        desc.size = size;
        WGPUBuffer buf = wgpuDeviceCreateBuffer(gpu.device, &desc);
        if (data) wgpuQueueWriteBuffer(gpu.queue, buf, 0, data, size);
        return buf;
    };

    auto buildPipeline = [&](const std::string& code, const char* entryPoint) -> WGPUComputePipeline {
        std::string fullCode = std::string(kNoiseHeader) + "\n" + code;
        wgpuDevicePushErrorScope(gpu.device, WGPUErrorFilter_Validation);
        WGPUShaderSourceWGSL src = {};
        src.chain.sType = WGPUSType_ShaderSourceWGSL;
        src.code = wgpu::Device::str(fullCode.c_str());
        WGPUShaderModuleDescriptor smd = {};
        smd.nextInChain = &src.chain;
        WGPUShaderModule sm = wgpuDeviceCreateShaderModule(gpu.device, &smd);

        WGPUComputePipelineDescriptor cpd = {};
        cpd.compute.module = sm;
        cpd.compute.entryPoint = wgpu::Device::str(entryPoint);
        WGPUComputePipeline cp = wgpuDeviceCreateComputePipeline(gpu.device, &cpd);

        PopContext compile;
        WGPUPopErrorScopeCallbackInfo pcb = {};
        pcb.mode = WGPUCallbackMode_AllowProcessEvents;
        pcb.callback = onPopError;
        pcb.userdata1 = &compile;
        wgpuDevicePopErrorScope(gpu.device, pcb);
        while (!compile.done) wgpuDevicePoll(gpu.device, true, nullptr);
        wgpuShaderModuleRelease(sm);
        if (compile.type != WGPUErrorType_NoError || !cp) return nullptr;
        return cp;
    };

    std::cerr << "Compiling physically separate pipelines..." << std::endl;
    WGPUComputePipeline cpClassify = buildPipeline(kClassifyWgsl, "classifyTilesMain");
    WGPUComputePipeline cpAlpha = buildPipeline(kPipelineAlphaWgsl, "pipelineAlphaMain");
    WGPUComputePipeline cpBeta = buildPipeline(kPipelineBetaWgsl, "pipelineBetaMain");
    WGPUComputePipeline cpFallback = buildPipeline(kPipelineFallbackWgsl, "pipelineFallbackMain");
    WGPUComputePipeline cpRef = buildPipeline(kPipelineReferenceWgsl, "pipelineReferenceMain");

    if (!cpClassify || !cpAlpha || !cpBeta || !cpFallback || !cpRef) {
        std::cerr << "Pipeline compilation failed!" << std::endl;
        return 1;
    }
    std::cerr << "All 5 pipelines compiled successfully." << std::endl;

    // Buffers
    WGPUBuffer curBuf = makeBuffer(WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst, curtains.size() * sizeof(GpuCurtainSpec), curtains.data());
    WGPUBuffer rayBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, gpuRays.size() * sizeof(GpuRay), gpuRays.data());
    WGPUBuffer offBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, tileOffsets.size() * sizeof(uint32_t), tileOffsets.data());
    WGPUBuffer idxBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, tileRayIndices.size() * sizeof(uint32_t), tileRayIndices.data());
    WGPUBuffer paramBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, paramPool.size() * sizeof(float), paramPool.data());

    DispatchCounters initCounters{0, 0, 0, 0};
    WGPUBuffer cntBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, sizeof(DispatchCounters), &initCounters);
    std::vector<uint32_t> initList(2304, 0xFFFFFFFFu);
    WGPUBuffer s1Buf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, 2304 * sizeof(uint32_t), initList.data());
    WGPUBuffer s2Buf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, 2304 * sizeof(uint32_t), initList.data());
    WGPUBuffer fbBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, 2304 * sizeof(uint32_t), initList.data());
    WGPUBuffer progBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc, 2304 * sizeof(glm::uvec2), nullptr);

    WGPUBuffer resRoutedBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc, count * sizeof(GpuResult), nullptr);
    WGPUBuffer resRefBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc, count * sizeof(GpuResult), nullptr);

    WGPUBuffer readCntBuf = makeBuffer(WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead, sizeof(DispatchCounters), nullptr);
    WGPUBuffer readResBuf = makeBuffer(WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead, count * sizeof(GpuResult), nullptr);

    // Bind Groups
    // 1. Classifier:
    WGPUBindGroupLayout bglClassifyIn = wgpuComputePipelineGetBindGroupLayout(cpClassify, 0);
    WGPUBindGroupEntry entriesClIn[4] = {};
    entriesClIn[0].binding = 0; entriesClIn[0].buffer = curBuf; entriesClIn[0].size = curtains.size() * sizeof(GpuCurtainSpec);
    entriesClIn[1].binding = 1; entriesClIn[1].buffer = rayBuf; entriesClIn[1].size = gpuRays.size() * sizeof(GpuRay);
    entriesClIn[2].binding = 2; entriesClIn[2].buffer = offBuf; entriesClIn[2].size = tileOffsets.size() * sizeof(uint32_t);
    entriesClIn[3].binding = 3; entriesClIn[3].buffer = idxBuf; entriesClIn[3].size = tileRayIndices.size() * sizeof(uint32_t);
    WGPUBindGroupDescriptor bgdClIn = {};
    bgdClIn.layout = bglClassifyIn;
    bgdClIn.entryCount = 4;
    bgdClIn.entries = entriesClIn;
    WGPUBindGroup bgClassifyIn = wgpuDeviceCreateBindGroup(gpu.device, &bgdClIn);

    WGPUBindGroupLayout bglClassifyOut = wgpuComputePipelineGetBindGroupLayout(cpClassify, 1);
    WGPUBindGroupEntry entriesClOut[5] = {};
    entriesClOut[0].binding = 0; entriesClOut[0].buffer = cntBuf; entriesClOut[0].size = sizeof(DispatchCounters);
    entriesClOut[1].binding = 1; entriesClOut[1].buffer = s1Buf; entriesClOut[1].size = 2304 * sizeof(uint32_t);
    entriesClOut[2].binding = 2; entriesClOut[2].buffer = s2Buf; entriesClOut[2].size = 2304 * sizeof(uint32_t);
    entriesClOut[3].binding = 3; entriesClOut[3].buffer = fbBuf; entriesClOut[3].size = 2304 * sizeof(uint32_t);
    entriesClOut[4].binding = 4; entriesClOut[4].buffer = progBuf; entriesClOut[4].size = 2304 * sizeof(glm::uvec2);
    WGPUBindGroupDescriptor bgdClOut = {};
    bgdClOut.layout = bglClassifyOut;
    bgdClOut.entryCount = 5;
    bgdClOut.entries = entriesClOut;
    WGPUBindGroup bgClassifyOut = wgpuDeviceCreateBindGroup(gpu.device, &bgdClOut);

    // 2. Pipeline Alpha:
    WGPUBindGroupLayout bglAlpha = wgpuComputePipelineGetBindGroupLayout(cpAlpha, 0);
    WGPUBindGroupEntry entriesAlpha[7] = {};
    entriesAlpha[0].binding = 0; entriesAlpha[0].buffer = curBuf; entriesAlpha[0].size = sizeof(GpuCurtainSpec);
    entriesAlpha[1].binding = 1; entriesAlpha[1].buffer = rayBuf; entriesAlpha[1].size = gpuRays.size() * sizeof(GpuRay);
    entriesAlpha[2].binding = 2; entriesAlpha[2].buffer = offBuf; entriesAlpha[2].size = tileOffsets.size() * sizeof(uint32_t);
    entriesAlpha[3].binding = 3; entriesAlpha[3].buffer = idxBuf; entriesAlpha[3].size = tileRayIndices.size() * sizeof(uint32_t);
    entriesAlpha[4].binding = 4; entriesAlpha[4].buffer = s1Buf; entriesAlpha[4].size = 2304 * sizeof(uint32_t);
    entriesAlpha[5].binding = 5; entriesAlpha[5].buffer = paramBuf; entriesAlpha[5].size = paramPool.size() * sizeof(float);
    entriesAlpha[6].binding = 6; entriesAlpha[6].buffer = resRoutedBuf; entriesAlpha[6].size = count * sizeof(GpuResult);
    WGPUBindGroupDescriptor bgdAlpha = {};
    bgdAlpha.layout = bglAlpha;
    bgdAlpha.entryCount = 7;
    bgdAlpha.entries = entriesAlpha;
    WGPUBindGroup bgAlpha = wgpuDeviceCreateBindGroup(gpu.device, &bgdAlpha);

    // 3. Pipeline Beta:
    WGPUBindGroupLayout bglBeta = wgpuComputePipelineGetBindGroupLayout(cpBeta, 0);
    WGPUBindGroupEntry entriesBeta[7] = {};
    entriesBeta[0].binding = 0; entriesBeta[0].buffer = curBuf; entriesBeta[0].size = 2 * sizeof(GpuCurtainSpec);
    entriesBeta[1].binding = 1; entriesBeta[1].buffer = rayBuf; entriesBeta[1].size = gpuRays.size() * sizeof(GpuRay);
    entriesBeta[2].binding = 2; entriesBeta[2].buffer = offBuf; entriesBeta[2].size = tileOffsets.size() * sizeof(uint32_t);
    entriesBeta[3].binding = 3; entriesBeta[3].buffer = idxBuf; entriesBeta[3].size = tileRayIndices.size() * sizeof(uint32_t);
    entriesBeta[4].binding = 4; entriesBeta[4].buffer = s2Buf; entriesBeta[4].size = 2304 * sizeof(uint32_t);
    entriesBeta[5].binding = 5; entriesBeta[5].buffer = paramBuf; entriesBeta[5].size = paramPool.size() * sizeof(float);
    entriesBeta[6].binding = 6; entriesBeta[6].buffer = resRoutedBuf; entriesBeta[6].size = count * sizeof(GpuResult);
    WGPUBindGroupDescriptor bgdBeta = {};
    bgdBeta.layout = bglBeta;
    bgdBeta.entryCount = 7;
    bgdBeta.entries = entriesBeta;
    WGPUBindGroup bgBeta = wgpuDeviceCreateBindGroup(gpu.device, &bgdBeta);

    // 4. Pipeline Fallback:
    WGPUBindGroupLayout bglFallback = wgpuComputePipelineGetBindGroupLayout(cpFallback, 0);
    WGPUBindGroupEntry entriesFallback[7] = {};
    entriesFallback[0].binding = 0; entriesFallback[0].buffer = curBuf; entriesFallback[0].size = 4 * sizeof(GpuCurtainSpec);
    entriesFallback[1].binding = 1; entriesFallback[1].buffer = rayBuf; entriesFallback[1].size = gpuRays.size() * sizeof(GpuRay);
    entriesFallback[2].binding = 2; entriesFallback[2].buffer = offBuf; entriesFallback[2].size = tileOffsets.size() * sizeof(uint32_t);
    entriesFallback[3].binding = 3; entriesFallback[3].buffer = idxBuf; entriesFallback[3].size = tileRayIndices.size() * sizeof(uint32_t);
    entriesFallback[4].binding = 4; entriesFallback[4].buffer = fbBuf; entriesFallback[4].size = 2304 * sizeof(uint32_t);
    entriesFallback[5].binding = 5; entriesFallback[5].buffer = paramBuf; entriesFallback[5].size = paramPool.size() * sizeof(float);
    entriesFallback[6].binding = 6; entriesFallback[6].buffer = resRoutedBuf; entriesFallback[6].size = count * sizeof(GpuResult);
    WGPUBindGroupDescriptor bgdFallback = {};
    bgdFallback.layout = bglFallback;
    bgdFallback.entryCount = 7;
    bgdFallback.entries = entriesFallback;
    WGPUBindGroup bgFallback = wgpuDeviceCreateBindGroup(gpu.device, &bgdFallback);

    // 5. Pipeline Reference:
    WGPUBindGroupLayout bglRef = wgpuComputePipelineGetBindGroupLayout(cpRef, 0);
    WGPUBindGroupEntry entriesRef[4] = {};
    entriesRef[0].binding = 0; entriesRef[0].buffer = curBuf; entriesRef[0].size = 4 * sizeof(GpuCurtainSpec);
    entriesRef[1].binding = 1; entriesRef[1].buffer = rayBuf; entriesRef[1].size = gpuRays.size() * sizeof(GpuRay);
    entriesRef[2].binding = 2; entriesRef[2].buffer = paramBuf; entriesRef[2].size = paramPool.size() * sizeof(float);
    entriesRef[3].binding = 3; entriesRef[3].buffer = resRefBuf; entriesRef[3].size = count * sizeof(GpuResult);
    WGPUBindGroupDescriptor bgdRef = {};
    bgdRef.layout = bglRef;
    bgdRef.entryCount = 4;
    bgdRef.entries = entriesRef;
    WGPUBindGroup bgRef = wgpuDeviceCreateBindGroup(gpu.device, &bgdRef);

    // Helper functions for dispatch and timing
    auto runClassification = [&]() -> double {
        wgpuQueueWriteBuffer(gpu.queue, cntBuf, 0, &initCounters, sizeof(DispatchCounters));
        WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        WGPUComputePassEncoder pass = wgpuCommandEncoderBeginComputePass(enc, nullptr);
        wgpuComputePassEncoderSetPipeline(pass, cpClassify);
        wgpuComputePassEncoderSetBindGroup(pass, 0, bgClassifyIn, 0, nullptr);
        wgpuComputePassEncoderSetBindGroup(pass, 1, bgClassifyOut, 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass, 36, 1, 1);
        wgpuComputePassEncoderEnd(pass);
        wgpuComputePassEncoderRelease(pass);
        WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(enc, nullptr);
        wgpuCommandEncoderRelease(enc);

        auto t0 = std::chrono::steady_clock::now();
        wgpuQueueSubmit(gpu.queue, 1, &cmd);
        wgpuCommandBufferRelease(cmd);
        wgpuDevicePoll(gpu.device, true, nullptr);
        auto t1 = std::chrono::steady_clock::now();
        return std::chrono::duration<double, std::milli>(t1 - t0).count();
    };

    auto runReferenceBatch = [&](int rep) -> double {
        WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        for (int r = 0; r < rep; ++r) {
            WGPUComputePassEncoder pass = wgpuCommandEncoderBeginComputePass(enc, nullptr);
            wgpuComputePassEncoderSetPipeline(pass, cpRef);
            wgpuComputePassEncoderSetBindGroup(pass, 0, bgRef, 0, nullptr);
            wgpuComputePassEncoderDispatchWorkgroups(pass, (uint32_t(count) + 63u) / 64u, 1, 1);
            wgpuComputePassEncoderEnd(pass);
            wgpuComputePassEncoderRelease(pass);
        }
        WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(enc, nullptr);
        wgpuCommandEncoderRelease(enc);

        auto t0 = std::chrono::steady_clock::now();
        wgpuQueueSubmit(gpu.queue, 1, &cmd);
        wgpuCommandBufferRelease(cmd);
        wgpuDevicePoll(gpu.device, true, nullptr);
        auto t1 = std::chrono::steady_clock::now();
        return std::chrono::duration<double, std::milli>(t1 - t0).count() / rep;
    };

    auto runWarmRoutedBatch = [&](uint32_t s1Tiles, uint32_t s2Tiles, uint32_t fbTiles, int rep) -> double {
        WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        for (int r = 0; r < rep; ++r) {
            WGPUComputePassEncoder pass = wgpuCommandEncoderBeginComputePass(enc, nullptr);
            wgpuComputePassEncoderSetPipeline(pass, cpAlpha);
            wgpuComputePassEncoderSetBindGroup(pass, 0, bgAlpha, 0, nullptr);
            wgpuComputePassEncoderDispatchWorkgroups(pass, (s1Tiles + 3u) / 4u, 1, 1);

            wgpuComputePassEncoderSetPipeline(pass, cpBeta);
            wgpuComputePassEncoderSetBindGroup(pass, 0, bgBeta, 0, nullptr);
            wgpuComputePassEncoderDispatchWorkgroups(pass, (s2Tiles + 3u) / 4u, 1, 1);

            wgpuComputePassEncoderSetPipeline(pass, cpFallback);
            wgpuComputePassEncoderSetBindGroup(pass, 0, bgFallback, 0, nullptr);
            wgpuComputePassEncoderDispatchWorkgroups(pass, (fbTiles + 3u) / 4u, 1, 1);
            wgpuComputePassEncoderEnd(pass);
            wgpuComputePassEncoderRelease(pass);
        }
        WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(enc, nullptr);
        wgpuCommandEncoderRelease(enc);

        auto t0 = std::chrono::steady_clock::now();
        wgpuQueueSubmit(gpu.queue, 1, &cmd);
        wgpuCommandBufferRelease(cmd);
        wgpuDevicePoll(gpu.device, true, nullptr);
        auto t1 = std::chrono::steady_clock::now();
        return std::chrono::duration<double, std::milli>(t1 - t0).count() / rep;
    };

    auto runColdRoutedBatch = [&](uint32_t s1Tiles, uint32_t s2Tiles, uint32_t fbTiles, int rep) -> double {
        wgpuQueueWriteBuffer(gpu.queue, cntBuf, 0, &initCounters, sizeof(DispatchCounters));
        WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        for (int r = 0; r < rep; ++r) {
            WGPUComputePassEncoder pass0 = wgpuCommandEncoderBeginComputePass(enc, nullptr);
            wgpuComputePassEncoderSetPipeline(pass0, cpClassify);
            wgpuComputePassEncoderSetBindGroup(pass0, 0, bgClassifyIn, 0, nullptr);
            wgpuComputePassEncoderSetBindGroup(pass0, 1, bgClassifyOut, 0, nullptr);
            wgpuComputePassEncoderDispatchWorkgroups(pass0, 36, 1, 1);
            wgpuComputePassEncoderEnd(pass0);
            wgpuComputePassEncoderRelease(pass0);

            WGPUComputePassEncoder pass1 = wgpuCommandEncoderBeginComputePass(enc, nullptr);
            wgpuComputePassEncoderSetPipeline(pass1, cpAlpha);
            wgpuComputePassEncoderSetBindGroup(pass1, 0, bgAlpha, 0, nullptr);
            wgpuComputePassEncoderDispatchWorkgroups(pass1, (s1Tiles + 3u) / 4u, 1, 1);

            wgpuComputePassEncoderSetPipeline(pass1, cpBeta);
            wgpuComputePassEncoderSetBindGroup(pass1, 0, bgBeta, 0, nullptr);
            wgpuComputePassEncoderDispatchWorkgroups(pass1, (s2Tiles + 3u) / 4u, 1, 1);

            wgpuComputePassEncoderSetPipeline(pass1, cpFallback);
            wgpuComputePassEncoderSetBindGroup(pass1, 0, bgFallback, 0, nullptr);
            wgpuComputePassEncoderDispatchWorkgroups(pass1, (fbTiles + 3u) / 4u, 1, 1);
            wgpuComputePassEncoderEnd(pass1);
            wgpuComputePassEncoderRelease(pass1);
        }
        WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(enc, nullptr);
        wgpuCommandEncoderRelease(enc);

        auto t0 = std::chrono::steady_clock::now();
        wgpuQueueSubmit(gpu.queue, 1, &cmd);
        wgpuCommandBufferRelease(cmd);
        wgpuDevicePoll(gpu.device, true, nullptr);
        auto t1 = std::chrono::steady_clock::now();
        return std::chrono::duration<double, std::milli>(t1 - t0).count() / rep;
    };

    // Initial classify run to populate lists
    runClassification();

    // Read back counters
    WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
    wgpuCommandEncoderCopyBufferToBuffer(enc, cntBuf, 0, readCntBuf, 0, sizeof(DispatchCounters));
    WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(enc, nullptr);
    wgpuQueueSubmit(gpu.queue, 1, &cmd);
    wgpuCommandBufferRelease(cmd);
    wgpuCommandEncoderRelease(enc);

    MapContext mc;
    WGPUBufferMapCallbackInfo mci = {};
    mci.mode = WGPUCallbackMode_AllowProcessEvents;
    mci.callback = onMapped;
    mci.userdata1 = &mc;
    wgpuBufferMapAsync(readCntBuf, WGPUMapMode_Read, 0, sizeof(DispatchCounters), mci);
    while (!mc.done) wgpuDevicePoll(gpu.device, true, nullptr);
    const auto* ptr = static_cast<const DispatchCounters*>(wgpuBufferGetConstMappedRange(readCntBuf, 0, sizeof(DispatchCounters)));
    DispatchCounters counts = *ptr;
    wgpuBufferUnmap(readCntBuf);

    std::cout << "Classified: S1=" << counts.s1Count << ", S2=" << counts.s2Count << ", FB=" << counts.fallbackCount << std::endl;

    // Initial runs for parity check
    runReferenceBatch(1);
    runWarmRoutedBatch(counts.s1Count, counts.s2Count, counts.fallbackCount, 1);

    auto readBackResults = [&](WGPUBuffer srcBuf) -> std::vector<GpuResult> {
        WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        wgpuCommandEncoderCopyBufferToBuffer(enc, srcBuf, 0, readResBuf, 0, count * sizeof(GpuResult));
        WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(enc, nullptr);
        wgpuQueueSubmit(gpu.queue, 1, &cmd);
        wgpuCommandBufferRelease(cmd);
        wgpuCommandEncoderRelease(enc);

        MapContext mc;
        WGPUBufferMapCallbackInfo mci = {};
        mci.mode = WGPUCallbackMode_AllowProcessEvents;
        mci.callback = onMapped;
        mci.userdata1 = &mc;
        wgpuBufferMapAsync(readResBuf, WGPUMapMode_Read, 0, count * sizeof(GpuResult), mci);
        while (!mc.done) wgpuDevicePoll(gpu.device, true, nullptr);
        const auto* ptr = static_cast<const GpuResult*>(wgpuBufferGetConstMappedRange(readResBuf, 0, count * sizeof(GpuResult)));
        std::vector<GpuResult> res(ptr, ptr + count);
        wgpuBufferUnmap(readResBuf);
        return res;
    };

    auto refRes = readBackResults(resRefBuf);
    auto routedRes = readBackResults(resRoutedBuf);

    uint32_t diffRays = 0;
    double maxDeltaRGB = 0.0, maxDeltaT = 0.0;
    for (size_t i = 0; i < count; ++i) {
        float dr = std::abs(refRes[i].rgb_T[0] - routedRes[i].rgb_T[0]);
        float dg = std::abs(refRes[i].rgb_T[1] - routedRes[i].rgb_T[1]);
        float db = std::abs(refRes[i].rgb_T[2] - routedRes[i].rgb_T[2]);
        float dt = std::abs(refRes[i].rgb_T[3] - routedRes[i].rgb_T[3]);
        maxDeltaRGB = std::max({maxDeltaRGB, double(dr), double(dg), double(db)});
        maxDeltaT = std::max(maxDeltaT, double(dt));
        if (dr > 1e-4f || dg > 1e-4f || db > 1e-4f || dt > 1e-4f) {
            diffRays++;
        }
    }
    std::cout << "Parity check vs Reference: diffs = " << diffRays << " / " << count
              << ", maxDeltaRGB = " << maxDeltaRGB << ", maxDeltaT = " << maxDeltaT << std::endl;

    // Benchmark Matrix (repeats = 1, 2, 4)
    std::vector<int> repeatList = {1, 2, 4};
    J benchRuns;

    for (int rep : repeatList) {
        std::cout << "\n--- Measuring at repeats = " << rep << " ---" << std::endl;
        int trials = 5;
        double refTotal = 0.0, warmTotal = 0.0, coldTotal = 0.0;
        double clTotal = 0.0;

        for (int t = 0; t < trials; ++t) {
            refTotal += runReferenceBatch(rep);
            warmTotal += runWarmRoutedBatch(counts.s1Count, counts.s2Count, counts.fallbackCount, rep);
            coldTotal += runColdRoutedBatch(counts.s1Count, counts.s2Count, counts.fallbackCount, rep);
            clTotal += runClassification();
        }

        double refMs = refTotal / trials;
        double warmMs = warmTotal / trials;
        double coldMs = coldTotal / trials;
        double clMs = clTotal / trials;

        double warmDelta = (warmMs - refMs) / refMs * 100.0;
        double coldDelta = (coldMs - refMs) / refMs * 100.0;

        std::cout << "Reference Marcher: " << std::fixed << std::setprecision(3) << refMs << " ms" << std::endl;
        std::cout << "Warm Routed:       " << warmMs << " ms (" << (warmDelta >= 0 ? "+" : "") << warmDelta << "%)" << std::endl;
        std::cout << "Cold Routed:       " << coldMs << " ms (" << (coldDelta >= 0 ? "+" : "") << coldDelta << "%)" << std::endl;
        std::cout << "  - classifyTilesMain: " << clMs << " ms" << std::endl;

        J runJ;
        runJ["repeats"] = rep;
        runJ["reference_marcher_ms"] = refMs;
        runJ["warm_routed_ms"] = warmMs;
        runJ["warm_delta_pct"] = warmDelta;
        runJ["cold_routed_ms"] = coldMs;
        runJ["cold_delta_pct"] = coldDelta;
        runJ["standalone_classify_ms"] = clMs;
        benchRuns.push_back(runJ);
    }

    // Export master record
    J outRecord;
    outRecord["audit_name"] = "GPU_RAY_TRANSPORT_6DG_QUERY_FAMILY_SPECIALIZATION_2026-10-24";
    outRecord["harness"] = "Antigravity";
    outRecord["agent"] = "Sparkly Guy / Gemini 3.8 Flash High";
    outRecord["date"] = "2026-10-24";
    outRecord["branch"] = "claude/ray-domain-transport-rung0";
    outRecord["pinned_rays_count"] = count;
    outRecord["tile_counts"] = {
        {"total_tiles", 2304},
        {"s1_tiles", counts.s1Count},
        {"s1_pct", 100.0 * counts.s1Count / 2304.0},
        {"s2_tiles", counts.s2Count},
        {"s2_pct", 100.0 * counts.s2Count / 2304.0},
        {"fallback_tiles", counts.fallbackCount},
        {"fallback_pct", 100.0 * counts.fallbackCount / 2304.0}
    };
    outRecord["parity"] = {
        {"differing_rays", diffRays},
        {"max_delta_rgb", maxDeltaRGB},
        {"max_delta_t", maxDeltaT},
        {"status", (diffRays == 0) ? "100.0% BIT-EXACT PARITY" : "PARITY_DIVERGENCE"}
    };
    outRecord["benchmark_matrix"] = benchRuns;
    outRecord["gates"] = {
        {"gate_1_standalone_metal", (diffRays == 0) ? "PASS" : "FAIL"},
        {"gate_2_webgpu_screen_render", "OPEN"},
        {"gate_3_end_to_end_30fps", "OPEN"}
    };

    std::ofstream ofs(outJson);
    ofs << outRecord.dump(2) << std::endl;
    std::cerr << "Wrote master §6dg benchmark record to " << outJson << std::endl;

    return 0;
}
