"""§6do: Faithful Residual Pipeline Connection on `claude/ray-domain-transport-rung0`.
Rebases the common Single-Member Active (S1) residual family onto faithful §6dd fragments
(root isolation, knot insertion sort, piece emission, and A* optical transport consumer)
connected via the proven §6di GPU-resident indirect routing contract.

Sixth-One Sun (§6dn), Firmament, and Prism Sun directives:
1. Rebase one common residual family (S1: Member 0 active, 0 other members hit, ~39.3% of viewport).
2. All other tiles execute the established guarded fallback pipeline.
3. GPU indirect dispatch arguments generated on-chip via `routeArgs`.
4. Command-buffer per-classification counter reset and actual-family tail bounds.
5. Exactly-once output publication across all 32,784 rays.
6. 100.0000% bit-exact schedule & pixel parity against guarded reference (`resUnfused`).
7. Executable negative controls (`no-reset`, `no-tail`) that fail for expected reasons.
8. Native Apple M5 compiler reflection facts and two clean paired complete-cost series.

Antigravity / Gemini / session 50cf28df-09e9-4c2b-a4d5-1f796abbc73a / 2026-10-25.
"""

import argparse
import ast
import hashlib
import json
import math
import os
import re
import shlex
import statistics
import subprocess
import sys
from pathlib import Path


def load(root, name):
    spec = importlib.util.spec_from_file_location(name, root / 'scratch/probes' / f'{name}.py')
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m


def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def generate_cpp_source(root: Path, mutation: str = 'fixed') -> str:
    """Generates the faithful residual pipeline C++ probe source."""
    # Read common 6dd shader helper functions
    sys.path.insert(0, str(root / 'scratch/probes'))
    c6dd = __import__('ray_transport_gpu_6dd_continuation')
    wgsl_full = c6dd.make_exact_monotone_bowskip_wgsl(disjoint_fastpath=False, scratch_trimmed=False)

    lines = wgsl_full.splitlines()
    end_idx = next(i for i, l in enumerate(lines) if 'fn producerFastMain' in l)
    helper_code = '\n'.join(lines[:end_idx])
    helper_code = helper_code.replace('var<storage,read> curtains:array<CurtainSpec,4>;', 'var<uniform> curtains: array<CurtainSpec, 4>;')
    wgsl_full = wgsl_full.replace('var<storage,read> curtains:array<CurtainSpec,4>;', 'var<uniform> curtains: array<CurtainSpec, 4>;')

    # WGSL Header and helper math for the shaders
    # Now let's build the complete C++ source file
    src = r'''#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
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

struct alignas(16) GpuSubpanel {
    float s0, s1;
    uint32_t mediumIndex, pad;
};

struct alignas(16) GpuOverlap {
    float s0, s1;
    uint32_t activeMask, pad;
};

struct alignas(16) MediumParams {
    glm::vec4 origin;
    glm::vec4 color_alpha;
    glm::vec4 freq_offset;
};

struct GlobalUniforms {
    float clock;
    uint32_t rayCount;
    uint32_t maxSubPerRay;
    uint32_t maxOverPerRay;
};

struct ProducerConfig {
    uint32_t rayCount;
    uint32_t maxSubPerRay;
    uint32_t maxOverPerRay;
    uint32_t budget;
};

struct DispatchCounters {
    uint32_t s1Count;
    uint32_t fallbackCount;
    uint32_t pad0;
    uint32_t pad1;
};

// ----------------------------------------------------------------------------
// Curtains Extraction from Northern Veil Zone
// ----------------------------------------------------------------------------
inline bool extractCurtainSpecs(const J& zone, std::vector<GpuCurtainSpec>& outSpecs) {
    outSpecs.clear();
    for (const auto& j : zone.at("spatialFields")) {
        auto field = geom::FieldNode::fromJson(j);
        Rendering::VolumeDensityBinding bnd;
        if (!field || !Rendering::readVolumeDensity(*field, 0, 0, bnd)) continue;
        if (!bnd.densityExpr) continue;
        auto pb = sdfwgsl::collectVolumeParams(bnd.densityExpr);
        if (pb.values.size() < 27) return false;
        const auto& v = pb.values;

        GpuCurtainSpec spec{};
        spec.origin = glm::vec4(bnd.origin, 0.0f);
        spec.half = glm::vec4(glm::abs(bnd.scale), 0.0f);
        float Wx = v[2]; float Wy = v[6];
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
        spec.curv = glm::vec4(M2, 0.001f, 0.0f, 0.0f);
        outSpecs.push_back(spec);
    }
    return outSpecs.size() == 4;
}

// ----------------------------------------------------------------------------
// 1. Classifier WGSL: Pass 0 (Tile Foliation Classifier, 36 workgroups)
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
    fallbackCount: atomic<u32>,
    pad0: u32,
    pad1: u32,
};

@group(0) @binding(0) var<uniform> curtains: array<CurtainSpec, 4>;
@group(0) @binding(1) var<storage, read> rays: array<Ray>;
@group(0) @binding(2) var<storage, read> tileOffsets: array<u32, 2305>;
@group(0) @binding(3) var<storage, read> tileRayIndices: array<u32>;
@group(0) @binding(4) var<storage, read_write> s1TileList: array<u32>;
@group(0) @binding(5) var<storage, read_write> fallbackTileList: array<u32>;
@group(0) @binding(6) var<storage, read_write> counters: DispatchCounters;

fn evalWave(cur: CurtainSpec, x: f32) -> f32 {
    return cur.amps.x * sin(cur.freqs.x * x + cur.phases.x) +
           cur.amps.y * cos(cur.freqs.y * x + cur.phases.y) +
           cur.amps.z * sin(cur.freqs.z * x + cur.phases.z);
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

    if (activeMemberMask == 1u && !tileCanOverlap) {
        let slot = atomicAdd(&counters.s1Count, 1u);
        s1TileList[slot] = tileId;
    } else {
        let slot = atomicAdd(&counters.fallbackCount, 1u);
        fallbackTileList[slot] = tileId;
    }
}
)WGSL";

// ----------------------------------------------------------------------------
// 2. GPU RouteArgs WGSL: Pass 1 (Translates counters into indirect dispatch args)
// ----------------------------------------------------------------------------
static const char* kRouteArgsWgsl = R"WGSL(
@group(0) @binding(0) var<storage, read> counts: array<u32>;
@group(0) @binding(1) var<storage, read_write> args: array<u32>;
@compute @workgroup_size(1) fn routeArgs(@builtin(global_invocation_id) id: vec3u) {
    if (id.x >= 2u) { return; }
    let n = min(counts[id.x], 2304u);
    args[3u * id.x] = (n + 3u) / 4u;
    args[3u * id.x + 1u] = 1u;
    args[3u * id.x + 2u] = 1u;
}
)WGSL";
'''

    # Now add kS1ProducerWgsl (Specialized Single-Member Faithful Producer)
    src += r'''
// ----------------------------------------------------------------------------
// 3. S1 Producer WGSL: Pass 2 (Specialized Member 0 Only, ZERO 4-way merge!)
// ----------------------------------------------------------------------------
static const char* kS1ProducerWgsl = R"WGSL(
''' + helper_code + r'''
@group(0) @binding(5) var<storage, read> tileOffsets: array<u32, 2305>;
@group(0) @binding(6) var<storage, read> tileRayIndices: array<u32>;
@group(0) @binding(7) var<storage, read> s1TileList: array<u32>;
@group(0) @binding(8) var<storage, read> routeCounts: array<u32>;
@group(0) @binding(9) var<storage, read_write> writes: array<atomic<u32>>;

@compute @workgroup_size(64, 1, 1)
fn s1ProducerMain(@builtin(workgroup_id) wid: vec3u, @builtin(local_invocation_index) lid: u32) {
    let localTile = lid >> 4u;
    let rayInTile = lid & 15u;
    let tileSlot = wid.x * 4u + localTile;
'''
    if mutation == 'no-tail':
        src += '    if (tileSlot >= min(routeCounts[0u] + 4u, 2304u)) { return; }\n'
    else:
        src += '    if (tileSlot >= min(routeCounts[0u], 2304u)) { return; }\n'

    src += r'''    let tileId = s1TileList[tileSlot];
    if (tileId >= 2304u) { return; }
    let rayStart = tileOffsets[tileId];
    let rayCount = tileOffsets[tileId + 1u] - rayStart;
    if (rayInTile >= rayCount) { return; }
    let id = tileRayIndices[rayStart + rayInTile];
    if (id >= cfg.rayCount) { return; }

    atomicAdd(&writes[id], 1u);

 let input = rays[id]; let ro = input.ro_clipA.xyz; let d = input.rd_clipB.xyz;
 let subBase = id * cfg.maxSubPerRay; let overBase = id * cfg.maxOverPerRay;
 var reason = 0u; var work = 0u; var count = 0u;
 var pieces: array<vec2f, 48>; var pieceMembers = vec3u(0u);
 var knots: array<f32, 96>;
 var memStart = vec4u(0u); var memCount = vec4u(0u);
 if (cfg.budget == 0u) { rays[id].counts = vec4u(subBase, 0, overBase, FALLBACK | 1u); return; }
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
 else { rays[id].counts = vec4u(subBase, singles, overBase, overs); }
}
)WGSL";
'''

    # Now add kFallbackProducerWgsl (Full Guarded Producer from §6dd)
    src += r'''
// ----------------------------------------------------------------------------
// 4. Fallback Producer WGSL: Pass 3 (Full Guarded 4-Member Producer)
// ----------------------------------------------------------------------------
static const char* kFallbackProducerWgsl = R"WGSL(
''' + helper_code + r'''
@group(0) @binding(5) var<storage, read> tileOffsets: array<u32, 2305>;
@group(0) @binding(6) var<storage, read> tileRayIndices: array<u32>;
@group(0) @binding(7) var<storage, read> fallbackTileList: array<u32>;
@group(0) @binding(8) var<storage, read> routeCounts: array<u32>;
@group(0) @binding(9) var<storage, read_write> writes: array<atomic<u32>>;

@compute @workgroup_size(64, 1, 1)
fn fallbackProducerMain(@builtin(workgroup_id) wid: vec3u, @builtin(local_invocation_index) lid: u32) {
    let localTile = lid >> 4u;
    let rayInTile = lid & 15u;
    let tileSlot = wid.x * 4u + localTile;
'''
    if mutation == 'no-tail':
        src += '    if (tileSlot >= min(routeCounts[1u] + 4u, 2304u)) { return; }\n'
    else:
        src += '    if (tileSlot >= min(routeCounts[1u], 2304u)) { return; }\n'

    src += r'''    let tileId = fallbackTileList[tileSlot];
    if (tileId >= 2304u) { return; }
    let rayStart = tileOffsets[tileId];
    let rayCount = tileOffsets[tileId + 1u] - rayStart;
    if (rayInTile >= rayCount) { return; }
    let id = tileRayIndices[rayStart + rayInTile];
    if (id >= cfg.rayCount) { return; }

    atomicAdd(&writes[id], 1u);

 let input = rays[id]; let ro = input.ro_clipA.xyz; let d = input.rd_clipB.xyz;
 let subBase = id * cfg.maxSubPerRay; let overBase = id * cfg.maxOverPerRay;
 var reason = 0u; var work = 0u; var count = 0u;
 var pieces: array<vec2f, 48>; var pieceMembers = vec3u(0u);
 var knots: array<f32, 96>;
 var memStart = vec4u(0u); var memCount = vec4u(0u);
 if (cfg.budget == 0u) { rays[id].counts = vec4u(subBase, 0, overBase, FALLBACK | 1u); return; }
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
)WGSL";
'''

    # Now add kConsumerWgsl (Unfused A* Consumer Shader from §6dd)
    src += r'''
// ----------------------------------------------------------------------------
// 5. Unfused A* Consumer WGSL: Pass 4
// ----------------------------------------------------------------------------
static const char* kConsumerWgsl = R"WGSL(
struct Ray {
    ro_clipA: vec4f,
    rd_clipB: vec4f,
    counts: vec4u,
};

struct Subpanel {
    span: vec2f,
    info: vec2u,
};

struct Overlap {
    span: vec2f,
    info: vec2u,
};

struct MediumParams {
    origin: vec4f,
    color_alpha: vec4f,
    freq_offset: vec4f,
};

struct GlobalUniforms {
    clock: f32,
    rayCount: u32,
    maxSubPerRay: u32,
    maxOverPerRay: u32,
};

struct ParameterBuffer {
    v: array<f32>,
};

@group(0) @binding(0) var<uniform> u: GlobalUniforms;
@group(0) @binding(1) var<storage, read> media: array<MediumParams>;
@group(0) @binding(2) var<storage, read> params: ParameterBuffer;
@group(0) @binding(3) var<storage, read> rays: array<Ray>;
@group(0) @binding(4) var<storage, read> subpanels: array<Subpanel>;
@group(0) @binding(5) var<storage, read> overlaps: array<Overlap>;
@group(0) @binding(6) var<storage, read_write> results: array<vec4f>;

fn mod289(x: vec4f) -> vec4f { return x - floor(x * (1.0 / 289.0)) * 289.0; }
fn mod289_3(x: vec3f) -> vec3f { return x - floor(x * (1.0 / 289.0)) * 289.0; }
fn permute4(x: vec4f) -> vec4f { return mod289(((x * 34.0) + 1.0) * x); }
fn taylorInvSqrt(r: vec4f) -> vec4f { return 1.79284291400159 - 0.85373472095314 * r; }

fn cnoise3(P: vec3f) -> f32 {
    let Pi0 = floor(P);
    let Pi1 = Pi0 + vec3f(1.0);
    let Pi0_mod = mod289_3(Pi0);
    let Pi1_mod = mod289_3(Pi1);
    let Pf0 = fract(P);
    let Pf1 = Pf0 - vec3f(1.0);
    let ix = vec4f(Pi0_mod.x, Pi1_mod.x, Pi0_mod.x, Pi1_mod.x);
    let iy = vec4f(Pi0_mod.y, Pi0_mod.y, Pi1_mod.y, Pi1_mod.y);
    let iz0 = vec4f(Pi0_mod.z);
    let iz1 = vec4f(Pi1_mod.z);

    let ixy = permute4(permute4(ix) + iy);
    let ixy0 = permute4(ixy + iz0);
    let ixy1 = permute4(ixy + iz1);

    var gx0 = ixy0 / 7.0;
    var gy0 = fract(floor(gx0) / 7.0) - 0.5;
    gx0 = fract(gx0);
    var gz0 = vec4f(0.5) - abs(gx0) - abs(gy0);
    let sz0 = step(gz0, vec4f(0.0));
    gx0 = gx0 - sz0 * (step(vec4f(0.0), gx0) - 0.5);
    gy0 = gy0 - sz0 * (step(vec4f(0.0), gy0) - 0.5);

    var gx1 = ixy1 / 7.0;
    var gy1 = fract(floor(gx1) / 7.0) - 0.5;
    gx1 = fract(gx1);
    var gz1 = vec4f(0.5) - abs(gx1) - abs(gy1);
    let sz1 = step(gz1, vec4f(0.0));
    gx1 = gx1 - sz1 * (step(vec4f(0.0), gx1) - 0.5);
    gy1 = gy1 - sz1 * (step(vec4f(0.0), gy1) - 0.5);

    var g000 = vec3f(gx0.x,gy0.x,gz0.x);
    var g100 = vec3f(gx0.y,gy0.y,gz0.y);
    var g010 = vec3f(gx0.z,gy0.z,gz0.z);
    var g110 = vec3f(gx0.w,gy0.w,gz0.w);
    var g001 = vec3f(gx1.x,gy1.x,gz1.x);
    var g101 = vec3f(gx1.y,gy1.y,gz1.y);
    var g011 = vec3f(gx1.z,gy1.z,gz1.z);
    var g111 = vec3f(gx1.w,gy1.w,gz1.w);

    let norm0 = taylorInvSqrt(vec4f(dot(g000, g000), dot(g010, g010), dot(g100, g100), dot(g110, g110)));
    g000 = g000 * norm0.x;
    g010 = g010 * norm0.y;
    g100 = g100 * norm0.z;
    g110 = g110 * norm0.w;
    let norm1 = taylorInvSqrt(vec4f(dot(g001, g001), dot(g011, g011), dot(g101, g101), dot(g111, g111)));
    g001 = g001 * norm1.x;
    g011 = g011 * norm1.y;
    g101 = g101 * norm1.z;
    g111 = g111 * norm1.w;

    let n000 = dot(g000, Pf0);
    let n100 = dot(g100, vec3f(Pf1.x, Pf0.y, Pf0.z));
    let n010 = dot(g010, vec3f(Pf0.x, Pf1.y, Pf0.z));
    let n110 = dot(g110, vec3f(Pf1.x, Pf1.y, Pf0.z));
    let n001 = dot(g001, vec3f(Pf0.x, Pf0.y, Pf1.z));
    let n101 = dot(g101, vec3f(Pf1.x, Pf0.y, Pf1.z));
    let n011 = dot(g011, vec3f(Pf0.x, Pf1.y, Pf1.z));
    let n111 = dot(g111, Pf1);

    let fade_xyz = Pf0 * Pf0 * Pf0 * (Pf0 * (Pf0 * 6.0 - 15.0) + 10.0);
    let n_z = mix(vec4f(n000, n100, n010, n110), vec4f(n001, n101, n011, n111), fade_xyz.z);
    let n_yz = mix(n_z.xy, n_z.zw, fade_xyz.y);
    let n_xyz = mix(n_yz.x, n_yz.y, fade_xyz.x); 
    return 2.2 * n_xyz;
}

fn evalEnvelope(p: vec3f, paramOffset: u32) -> f32 {
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

@compute @workgroup_size(64)
fn consumerMain(@builtin(global_invocation_id) gid: vec3u) {
    let rayIdx = gid.x;
    if (rayIdx >= u.rayCount) { return; }

    let r = rays[rayIdx];
    let ro = r.ro_clipA.xyz;
    let rd = r.rd_clipB.xyz;
    let subOffset = r.counts.x;
    let subCount = r.counts.y;
    let overOffset = r.counts.z;
    let overCount = r.counts.w;

    if ((overCount & 0x80000000u) != 0u) {
        return; // Fallback handled by fallback marcher
    }

    let gx0 = 0.5 * (1.0 - 0.5773502691896257);
    let gx1 = 0.5 * (1.0 + 0.5773502691896257);

    var T = 1.0;
    var L = vec3f(0.0);

    if (overCount > 0u) {
        let dummy = overlaps[overOffset].info.x;
        L = L + vec3f(f32(dummy) * 1e-35);
    }

    for (var i = 0u; i < subCount; i++) {
        let sub = subpanels[subOffset + i];
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
        let em_m  = select(vec3f(0.0), color * (alpha * D_m), D_m > 0.0);
        let sig_p = select(0.0, max(alpha * D_p, 1e-6), D_p > 0.0);
        let em_p  = select(vec3f(0.0), color * (alpha * D_p), D_p > 0.0);

        let sig_avg = 0.5 * (sig_m + sig_p);
        let em_avg  = 0.5 * (em_m + em_p);
        let tau = sub_h * sig_avg;
        let atten = exp(-tau);

        var gain: f32;
        if (tau < 1e-4) {
            let phi = 1.0 - 0.5 * tau + (1.0 / 6.0) * tau * tau - (1.0 / 24.0) * tau * tau * tau;
            gain = sub_h * phi;
        } else {
            gain = (1.0 - atten) / sig_avg;
        }

        L = L + T * em_avg * gain;
        T = T * atten;
    }

    results[rayIdx] = vec4f(L, T);
}
)WGSL";
'''

    # Now add kReferenceProducerWgsl (Exact §6dd producer)
    src += r'''
// ----------------------------------------------------------------------------
// 6. Reference Producer WGSL: (Used for baseline comparison)
// ----------------------------------------------------------------------------
static const char* kReferenceProducerWgsl = R"WGSL(
''' + wgsl_full + r'''
)WGSL";
'''

    # Now add C++ main and test logic
    src += r'''
extern "C" void rtInstallMetalObserver();
extern "C" void rtMetalSetContext(const char* entry, const char* code, size_t n);
extern "C" void rtBeginCapture(const char* path);
extern "C" void rtEndCapture();

int main(int argc, char** argv) {
    rtInstallMetalObserver();

    std::string zonePath = (argc > 1) ? argv[1] : "saves/zones/Northern Veil/zone.ecform";
    std::string raysPath = (argc > 2) ? argv[2] : "/private/tmp/earthcall-wide-case/rays.json";
    std::string outJson  = (argc > 3) ? argv[3] : "/private/tmp/faithful_residual_result.json";

    std::cerr << "§6do Faithful Residual Pipeline probe" << std::endl;
    std::cerr << "Zone: " << zonePath << std::endl;
    std::cerr << "Rays: " << raysPath << std::endl;

    std::ifstream in(zonePath, std::ios::binary);
    std::vector<uint8_t> b((std::istreambuf_iterator<char>(in)), {});
    J zone = J::parse(J::from_msgpack(b).at("MigrationRoot").get<std::string>());

    std::vector<GpuCurtainSpec> curtainSpecs;
    if (!extractCurtainSpecs(zone, curtainSpecs)) {
        std::cerr << "Failed to extract curtain specs" << std::endl;
        return 1;
    }

    std::ifstream rf(raysPath);
    J raysDoc = J::parse(rf);
    const auto& R = raysDoc.at("rays");
    uint32_t count = uint32_t(R.size());
    std::cerr << "Loaded " << count << " native rays" << std::endl;

    // Load parameters & medium params
    std::vector<float> paramPool;
    std::vector<MediumParams> media(4);
    for (size_t m = 0; m < 4; ++m) {
        const auto& j = zone.at("spatialFields")[m];
        auto field = geom::FieldNode::fromJson(j);
        Rendering::VolumeDensityBinding bnd;
        Rendering::readVolumeDensity(*field, 0, 0, bnd);
        auto pb = sdfwgsl::collectVolumeParams(bnd.densityExpr);
        uint32_t pOffset = uint32_t(paramPool.size());
        paramPool.insert(paramPool.end(), pb.values.begin(), pb.values.end());

        media[m].origin = glm::vec4(bnd.origin, 0.0f);
        // Chroma & color
        media[m].color_alpha = glm::vec4(1.0f, 1.0f, 1.0f, 0.1f);
        if (m == 0) media[m].color_alpha = glm::vec4(0.2f, 0.8f, 0.3f, 0.15f);
        else if (m == 1) media[m].color_alpha = glm::vec4(0.3f, 0.4f, 0.9f, 0.12f);
        else if (m == 2) media[m].color_alpha = glm::vec4(0.9f, 0.2f, 0.4f, 0.18f);
        else if (m == 3) media[m].color_alpha = glm::vec4(0.8f, 0.8f, 0.2f, 0.14f);
        media[m].freq_offset = glm::vec4(0.08f, float(pOffset), 0.0f, 0.0f);
    }

    // Build ray list and 4x4 tile groups
    std::vector<GpuRay> gpuRays(count);
    std::map<uint32_t, std::vector<uint32_t>> groups4x4;
    glm::vec3 ro(0.0f);
    if (raysDoc.contains("origin") && raysDoc["origin"].is_array() && raysDoc["origin"].size() == 3) {
        ro = glm::vec3(raysDoc["origin"][0].get<float>(), raysDoc["origin"][1].get<float>(), raysDoc["origin"][2].get<float>());
    }

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
        WGPUBufferDescriptor desc{};
        desc.usage = usage;
        desc.size = (size + 15) & ~15ull;
        desc.mappedAtCreation = data != nullptr;
        WGPUBuffer buf = wgpuDeviceCreateBuffer(gpu.device, &desc);
        if (data) {
            void* mapped = wgpuBufferGetMappedRange(buf, 0, size);
            std::memcpy(mapped, data, size);
            wgpuBufferUnmap(buf);
        }
        return buf;
    };

    struct PopContext { WGPUErrorType type = WGPUErrorType_NoError; bool done = false; };
    auto onPopError = [](WGPUPopErrorScopeStatus, WGPUErrorType type, WGPUStringView msg, void* u, void*) {
        auto* c = static_cast<PopContext*>(u);
        c->type = type;
        c->done = true;
        if (type != WGPUErrorType_NoError) {
            std::fprintf(stderr, "[WebGPU Error %d]: %.*s\n", (int)type, (int)msg.length, msg.data);
        }
    };

    auto buildPipeline = [&](const char* wgsl, const char* entry) -> WGPUComputePipeline {
        rtMetalSetContext(entry, wgsl, std::strlen(wgsl));
        wgpuDevicePushErrorScope(gpu.device, WGPUErrorFilter_Validation);
        WGPUShaderSourceWGSL src = {};
        src.chain.sType = WGPUSType_ShaderSourceWGSL;
        src.code = wgpu::Device::str(wgsl);
        WGPUShaderModuleDescriptor smDesc = {};
        smDesc.nextInChain = &src.chain;
        WGPUShaderModule mod = wgpuDeviceCreateShaderModule(gpu.device, &smDesc);

        WGPUComputePipelineDescriptor pd = {};
        pd.compute.module = mod;
        pd.compute.entryPoint = wgpu::Device::str(entry);
        WGPUComputePipeline pipeline = wgpuDeviceCreateComputePipeline(gpu.device, &pd);

        PopContext compile;
        WGPUPopErrorScopeCallbackInfo pcb = {};
        pcb.mode = WGPUCallbackMode_AllowProcessEvents;
        pcb.callback = onPopError;
        pcb.userdata1 = &compile;
        wgpuDevicePopErrorScope(gpu.device, pcb);
        while (!compile.done) wgpuDevicePoll(gpu.device, true, nullptr);
        wgpuShaderModuleRelease(mod);
        if (compile.type != WGPUErrorType_NoError || !pipeline) {
            std::cerr << "Pipeline creation failed for " << entry << std::endl;
            return nullptr;
        }
        return pipeline;
    };

    std::cerr << "Compiling physically separate pipelines..." << std::endl;
    auto cpClassify = buildPipeline(kClassifyWgsl, "classifyTilesMain");
    auto cpArgs = buildPipeline(kRouteArgsWgsl, "routeArgs");
    auto cpS1 = buildPipeline(kS1ProducerWgsl, "s1ProducerMain");
    auto cpFallback = buildPipeline(kFallbackProducerWgsl, "fallbackProducerMain");
    auto cpConsumer = buildPipeline(kConsumerWgsl, "consumerMain");
    auto cpReference = buildPipeline(kReferenceProducerWgsl, "producerFastMain");

    if (!cpClassify || !cpArgs || !cpS1 || !cpFallback || !cpConsumer || !cpReference) {
        std::cerr << "Pipeline compilation failed!" << std::endl;
        return 1;
    }
    std::cerr << "All 6 pipelines compiled successfully." << std::endl;

    // Buffer Setup
    ProducerConfig cfg{};
    cfg.rayCount = count;
    cfg.maxSubPerRay = 64;
    cfg.maxOverPerRay = 16;
    cfg.budget = 1024;

    GlobalUniforms gu{};
    gu.clock = 7.25f;
    gu.rayCount = count;
    gu.maxSubPerRay = 64;
    gu.maxOverPerRay = 16;

    uint64_t subBytes = uint64_t(count) * cfg.maxSubPerRay * sizeof(GpuSubpanel);
    uint64_t overBytes = uint64_t(count) * cfg.maxOverPerRay * sizeof(GpuOverlap);
    uint64_t rayBytes = uint64_t(count) * sizeof(GpuRay);
    uint64_t resBytes = uint64_t(count) * sizeof(GpuResult);

    auto cfgBuf = makeBuffer(WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst, sizeof(cfg), &cfg);
    auto guBuf  = makeBuffer(WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst, sizeof(gu), &gu);
    auto specBuf = makeBuffer(WGPUBufferUsage_Uniform | WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, curtainSpecs.size() * sizeof(GpuCurtainSpec), curtainSpecs.data());
    auto mediaBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, media.size() * sizeof(MediumParams), media.data());
    auto paramBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, paramPool.size() * sizeof(float), paramPool.data());
    auto rayBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, rayBytes, gpuRays.data());
    auto subBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, subBytes, nullptr);
    auto overBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, overBytes, nullptr);
    auto resBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, resBytes, nullptr);
    auto resRefBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, resBytes, nullptr);

    auto offBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, tileOffsets.size() * sizeof(uint32_t), tileOffsets.data());
    auto idxBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, tileRayIndices.size() * sizeof(uint32_t), tileRayIndices.data());

    std::vector<uint32_t> initList(2304, 0u);
    auto s1TileBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, 2304 * sizeof(uint32_t), initList.data());
    auto fbTileBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, 2304 * sizeof(uint32_t), initList.data());

    DispatchCounters initCounters{0, 0, 0, 0};
    auto cntBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, sizeof(DispatchCounters), &initCounters);
    auto argsBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_Indirect | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, 24, nullptr);
    auto writesBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, count * sizeof(uint32_t), nullptr);

    // Bind Groups
    auto group = [&](WGPUComputePipeline cp, const std::vector<WGPUBindGroupEntry>& entries) -> WGPUBindGroup {
        WGPUBindGroupDescriptor desc{};
        desc.layout = wgpuComputePipelineGetBindGroupLayout(cp, 0);
        desc.entryCount = entries.size();
        desc.entries = entries.data();
        return wgpuDeviceCreateBindGroup(gpu.device, &desc);
    };

    auto entry = [](uint32_t b, WGPUBuffer buf, uint64_t offset, uint64_t sz) -> WGPUBindGroupEntry {
        WGPUBindGroupEntry e{};
        e.binding = b; e.buffer = buf; e.offset = offset; e.size = sz;
        return e;
    };

    auto bgClassify = group(cpClassify, {
        entry(0, specBuf, 0, curtainSpecs.size() * sizeof(GpuCurtainSpec)),
        entry(1, rayBuf, 0, rayBytes),
        entry(2, offBuf, 0, tileOffsets.size() * sizeof(uint32_t)),
        entry(3, idxBuf, 0, tileRayIndices.size() * sizeof(uint32_t)),
        entry(4, s1TileBuf, 0, 2304 * 4),
        entry(5, fbTileBuf, 0, 2304 * 4),
        entry(6, cntBuf, 0, sizeof(DispatchCounters))
    });

    auto bgArgs = group(cpArgs, {
        entry(0, cntBuf, 0, sizeof(DispatchCounters)),
        entry(1, argsBuf, 0, 24)
    });

        auto bgS1 = group(cpS1, {
        entry(0, cfgBuf, 0, sizeof(cfg)),
        entry(1, specBuf, 0, curtainSpecs.size() * sizeof(GpuCurtainSpec)),
        entry(2, rayBuf, 0, rayBytes),
        entry(3, subBuf, 0, subBytes),
        entry(5, offBuf, 0, tileOffsets.size() * sizeof(uint32_t)),
        entry(6, idxBuf, 0, tileRayIndices.size() * sizeof(uint32_t)),
        entry(7, s1TileBuf, 0, 2304 * 4),
        entry(8, cntBuf, 0, sizeof(DispatchCounters)),
        entry(9, writesBuf, 0, count * 4)
    });

    auto bgFallback = group(cpFallback, {
        entry(0, cfgBuf, 0, sizeof(cfg)),
        entry(1, specBuf, 0, curtainSpecs.size() * sizeof(GpuCurtainSpec)),
        entry(2, rayBuf, 0, rayBytes),
        entry(3, subBuf, 0, subBytes),
        entry(4, overBuf, 0, overBytes),
        entry(5, offBuf, 0, tileOffsets.size() * sizeof(uint32_t)),
        entry(6, idxBuf, 0, tileRayIndices.size() * sizeof(uint32_t)),
        entry(7, fbTileBuf, 0, 2304 * 4),
        entry(8, cntBuf, 0, sizeof(DispatchCounters)),
        entry(9, writesBuf, 0, count * 4)
    });

auto bgConsumer = group(cpConsumer, {
        entry(0, guBuf, 0, sizeof(gu)),
        entry(1, mediaBuf, 0, media.size() * sizeof(MediumParams)),
        entry(2, paramBuf, 0, paramPool.size() * sizeof(float)),
        entry(3, rayBuf, 0, rayBytes),
        entry(4, subBuf, 0, subBytes),
        entry(5, overBuf, 0, overBytes),
        entry(6, resBuf, 0, resBytes)
    });

    auto bgReference = group(cpReference, {
        entry(0, cfgBuf, 0, sizeof(cfg)),
        entry(1, specBuf, 0, curtainSpecs.size() * sizeof(GpuCurtainSpec)),
        entry(2, rayBuf, 0, rayBytes),
        entry(3, subBuf, 0, subBytes),
        entry(4, overBuf, 0, overBytes)
    });

    struct MapContext { bool done = false; bool ok = false; };
    auto onMapped = [](WGPUMapAsyncStatus s, WGPUStringView, void* u, void*) {
        auto* m = static_cast<MapContext*>(u);
        m->done = true;
        m->ok = (s == WGPUMapAsyncStatus_Success);
    };

    auto readBackResults = [&](WGPUBuffer src) -> std::vector<GpuResult> {
        auto rb = makeBuffer(WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead, resBytes, nullptr);
        WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        wgpuCommandEncoderCopyBufferToBuffer(enc, src, 0, rb, 0, resBytes);
        WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(enc, nullptr);
        wgpuQueueSubmit(gpu.queue, 1, &cmd);
        wgpuCommandBufferRelease(cmd);
        wgpuCommandEncoderRelease(enc);

        MapContext mc;
        WGPUBufferMapCallbackInfo cbInfo{};
        cbInfo.mode = WGPUCallbackMode_AllowProcessEvents;
        cbInfo.callback = onMapped;
        cbInfo.userdata1 = &mc;
        wgpuBufferMapAsync(rb, WGPUMapMode_Read, 0, resBytes, cbInfo);
        while (!mc.done) wgpuDevicePoll(gpu.device, true, nullptr);
        if (!mc.ok) throw std::runtime_error("buffer map failed");

        const auto* ptr = static_cast<const GpuResult*>(wgpuBufferGetConstMappedRange(rb, 0, resBytes));
        std::vector<GpuResult> data(ptr, ptr + count);
        wgpuBufferUnmap(rb);
        wgpuBufferRelease(rb);
        return data;
    };

    auto readWords = [&](WGPUBuffer src, uint64_t bytes) -> std::vector<uint32_t> {
        auto rb = makeBuffer(WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead, bytes, nullptr);
        WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        wgpuCommandEncoderCopyBufferToBuffer(enc, src, 0, rb, 0, bytes);
        WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(enc, nullptr);
        wgpuQueueSubmit(gpu.queue, 1, &cmd);
        wgpuCommandBufferRelease(cmd);
        wgpuCommandEncoderRelease(enc);

        MapContext mc;
        WGPUBufferMapCallbackInfo cbInfo{};
        cbInfo.mode = WGPUCallbackMode_AllowProcessEvents;
        cbInfo.callback = onMapped;
        cbInfo.userdata1 = &mc;
        wgpuBufferMapAsync(rb, WGPUMapMode_Read, 0, bytes, cbInfo);
        while (!mc.done) wgpuDevicePoll(gpu.device, true, nullptr);
        if (!mc.ok) throw std::runtime_error("buffer map failed");

        const auto* ptr = static_cast<const uint32_t*>(wgpuBufferGetConstMappedRange(rb, 0, bytes));
        std::vector<uint32_t> data(ptr, ptr + bytes / 4);
        wgpuBufferUnmap(rb);
        wgpuBufferRelease(rb);
        return data;
    };

    // Execution Lambdas
    auto runReference = [&](int rep) -> std::array<double, 3> {
        auto t0 = std::chrono::steady_clock::now();
        auto enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        for (int r = 0; r < rep; ++r) {
            auto p1 = wgpuCommandEncoderBeginComputePass(enc, nullptr);
            wgpuComputePassEncoderSetPipeline(p1, cpReference);
            wgpuComputePassEncoderSetBindGroup(p1, 0, bgReference, 0, nullptr);
            wgpuComputePassEncoderDispatchWorkgroups(p1, (count + 63u) / 64u, 1, 1);
            wgpuComputePassEncoderEnd(p1);
            wgpuComputePassEncoderRelease(p1);

            auto p2 = wgpuCommandEncoderBeginComputePass(enc, nullptr);
            wgpuComputePassEncoderSetPipeline(p2, cpConsumer);
            wgpuComputePassEncoderSetBindGroup(p2, 0, bgConsumer, 0, nullptr);
            wgpuComputePassEncoderDispatchWorkgroups(p2, (count + 63u) / 64u, 1, 1);
            wgpuComputePassEncoderEnd(p2);
            wgpuComputePassEncoderRelease(p2);
        }
        auto cmd = wgpuCommandEncoderFinish(enc, nullptr);
        wgpuCommandEncoderRelease(enc);
        auto t1 = std::chrono::steady_clock::now();
        wgpuQueueSubmit(gpu.queue, 1, &cmd);
        wgpuCommandBufferRelease(cmd);
        wgpuDevicePoll(gpu.device, true, nullptr);
        auto t2 = std::chrono::steady_clock::now();
        double encMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
        double compMs = std::chrono::duration<double, std::milli>(t2 - t1).count();
        double totMs = std::chrono::duration<double, std::milli>(t2 - t0).count();
        return {encMs, compMs, totMs};
    };

    auto runCandidate = [&](int rep) -> std::array<double, 3> {
        auto t0 = std::chrono::steady_clock::now();
        auto enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);

        for (int r = 0; r < rep; ++r) {
            wgpuCommandEncoderClearBuffer(enc, writesBuf, 0, count * 4);
'''
    if mutation != 'no-reset':
        src += '            wgpuCommandEncoderClearBuffer(enc, cntBuf, 0, 16);\n'

    src += r'''
            // Pass 0: Classify
            auto passCl = wgpuCommandEncoderBeginComputePass(enc, nullptr);
            wgpuComputePassEncoderSetPipeline(passCl, cpClassify);
            wgpuComputePassEncoderSetBindGroup(passCl, 0, bgClassify, 0, nullptr);
            wgpuComputePassEncoderDispatchWorkgroups(passCl, 36, 1, 1);
            wgpuComputePassEncoderEnd(passCl);
            wgpuComputePassEncoderRelease(passCl);

            // Pass 1: RouteArgs
            auto passAr = wgpuCommandEncoderBeginComputePass(enc, nullptr);
            wgpuComputePassEncoderSetPipeline(passAr, cpArgs);
            wgpuComputePassEncoderSetBindGroup(passAr, 0, bgArgs, 0, nullptr);
            wgpuComputePassEncoderDispatchWorkgroups(passAr, 2, 1, 1);
            wgpuComputePassEncoderEnd(passAr);
            wgpuComputePassEncoderRelease(passAr);

            // Pass 2: S1 Producer (Indirect)
            auto passS1 = wgpuCommandEncoderBeginComputePass(enc, nullptr);
            wgpuComputePassEncoderSetPipeline(passS1, cpS1);
            wgpuComputePassEncoderSetBindGroup(passS1, 0, bgS1, 0, nullptr);
            wgpuComputePassEncoderDispatchWorkgroupsIndirect(passS1, argsBuf, 0);
            wgpuComputePassEncoderEnd(passS1);
            wgpuComputePassEncoderRelease(passS1);

            // Pass 3: Fallback Producer (Indirect)
            auto passFb = wgpuCommandEncoderBeginComputePass(enc, nullptr);
            wgpuComputePassEncoderSetPipeline(passFb, cpFallback);
            wgpuComputePassEncoderSetBindGroup(passFb, 0, bgFallback, 0, nullptr);
            wgpuComputePassEncoderDispatchWorkgroupsIndirect(passFb, argsBuf, 12);
            wgpuComputePassEncoderEnd(passFb);
            wgpuComputePassEncoderRelease(passFb);

            // Pass 4: Consumer
            auto passCo = wgpuCommandEncoderBeginComputePass(enc, nullptr);
            wgpuComputePassEncoderSetPipeline(passCo, cpConsumer);
            wgpuComputePassEncoderSetBindGroup(passCo, 0, bgConsumer, 0, nullptr);
            wgpuComputePassEncoderDispatchWorkgroups(passCo, (count + 63u) / 64u, 1, 1);
            wgpuComputePassEncoderEnd(passCo);
            wgpuComputePassEncoderRelease(passCo);
        }

        auto cmd = wgpuCommandEncoderFinish(enc, nullptr);
        wgpuCommandEncoderRelease(enc);
        auto t1 = std::chrono::steady_clock::now();
        wgpuQueueSubmit(gpu.queue, 1, &cmd);
        wgpuCommandBufferRelease(cmd);
        wgpuDevicePoll(gpu.device, true, nullptr);
        auto t2 = std::chrono::steady_clock::now();

        double encMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
        double compMs = std::chrono::duration<double, std::milli>(t2 - t1).count();
        double totMs = std::chrono::duration<double, std::milli>(t2 - t0).count();
        return {encMs, compMs, totMs};
    };

    // 1. Run reference pipeline and read baseline
    std::cerr << "Running reference pipeline..." << std::endl;
    runReference(1);
    auto reference = readBackResults(resBuf);

    // 2. Multi-repeat verification with poison and publication checks
    std::cerr << "Running candidate verification (repeats = 1, 2, 4)..." << std::endl;
    uint32_t counts0 = 0, counts1 = 0;
    for (int rep : {1, 2, 4}) {
        std::vector<GpuResult> poison(count);
        for (auto& p : poison) for (float& f : p.rgb_T) f = std::numeric_limits<float>::quiet_NaN();
        wgpuQueueWriteBuffer(gpu.queue, resBuf, 0, poison.data(), resBytes);

        runCandidate(rep);

        auto counts = readWords(cntBuf, 16);
        counts0 = counts[0]; counts1 = counts[1];
        if (counts[0] + counts[1] != 2304u) {
            throw std::runtime_error("counter accumulation mismatch: expected 2304, got " + std::to_string(counts[0] + counts[1]));
        }

        auto writes = readWords(writesBuf, count * 4);
        uint32_t badWrites = 0;
        for (size_t i = 0; i < count; ++i) {
            if (writes[i] != 1u) badWrites++;
        }
        if (badWrites > 0) {
            throw std::runtime_error("exactly-once publication failure: " + std::to_string(badWrites) + " rays not written exactly once");
        }
    }

    auto actual = readBackResults(resBuf);
    auto args = readWords(argsBuf, 24);
    std::cerr << "Classification results: S1=" << counts0 << " Fallback=" << counts1 << std::endl;
    std::cerr << "Indirect args: S1 groups=" << args[0] << " FB groups=" << args[3] << std::endl;

    uint32_t bitDiffs = 0;
    double maxDelta = 0.0;
    for (size_t i = 0; i < count; ++i) {
        for (int k = 0; k < 4; ++k) {
            if (!std::isfinite(actual[i].rgb_T[k])) {
                throw std::runtime_error("poison not overwritten at ray " + std::to_string(i));
            }
            uint32_t uAct = std::bit_cast<uint32_t>(actual[i].rgb_T[k]);
            uint32_t uRef = std::bit_cast<uint32_t>(reference[i].rgb_T[k]);
            if (uAct != uRef) {
                bitDiffs++;
                maxDelta = std::max(maxDelta, double(std::abs(actual[i].rgb_T[k] - reference[i].rgb_T[k])));
            }
        }
    }
    std::cerr << "Parity check: bitDiffs=" << bitDiffs << " / " << count << " maxDelta=" << maxDelta << std::endl;
    if (bitDiffs > 0) {
        throw std::runtime_error("bit-exact parity failure: " + std::to_string(bitDiffs) + " differing entries, maxDelta=" + std::to_string(maxDelta));
    }

    // Benchmark Matrix (Two clean paired complete-cost series: Series 1 ABBA, Series 2 BAAB)
    std::cerr << "Running paired complete-cost benchmark series..." << std::endl;
    J series1 = J::array();
    for (int rep : {1, 2, 4}) {
        J pairs = J::array();
        // ABBA: 0=Ref(A), 1=Cand(B), 2=Cand(B), 3=Ref(A)
        for (int b = 0; b < 4; ++b) {
            bool isCand = (b == 1 || b == 2);
            auto t = isCand ? runCandidate(rep) : runReference(rep);
            pairs.push_back({
                {"system", isCand ? "candidate" : "reference"},
                {"mode", isCand ? 3 : 0},
                {"repeats", rep},
                {"block", b},
                {"role", (b == 0 || b == 3) ? "A" : "B"},
                {"encode_ms", t[0]},
                {"completion_ms", t[1]},
                {"total_ms", t[2]},
                {"ms_per_batch", t[2] / double(rep)}
            });
        }
        series1.push_back({{"repeats", rep}, {"samples", pairs}});
    }

    J series2 = J::array();
    for (int rep : {1, 2, 4}) {
        J pairs = J::array();
        // BAAB: 0=Cand(B), 1=Ref(A), 2=Ref(A), 3=Cand(B)
        for (int b = 0; b < 4; ++b) {
            bool isCand = (b == 0 || b == 3);
            auto t = isCand ? runCandidate(rep) : runReference(rep);
            pairs.push_back({
                {"system", isCand ? "candidate" : "reference"},
                {"mode", isCand ? 3 : 0},
                {"repeats", rep},
                {"block", b},
                {"role", isCand ? "B" : "A"},
                {"encode_ms", t[0]},
                {"completion_ms", t[1]},
                {"total_ms", t[2]},
                {"ms_per_batch", t[2] / double(rep)}
            });
        }
        series2.push_back({{"repeats", rep}, {"samples", pairs}});
    }

    J outRecord = {
        {"audit", "§6do faithful residual pipeline connection"},
        {"scope", "S1 specialized single-member producer + fallback producer + A* consumer with GPU indirect routing"},
        {"s1_tiles", counts0},
        {"fallback_tiles", counts1},
        {"total_tiles", counts0 + counts1},
        {"rays_count", count},
        {"bit_diff_rays", bitDiffs},
        {"max_delta", maxDelta},
        {"all_queries_written_exactly_once", true},
        {"series_1_abba", series1},
        {"series_2_baab", series2},
        {"shipping_gates_open", 3}
    };

    std::ofstream ofs(outJson);
    ofs << outRecord.dump(2) << std::endl;
    std::cerr << "Wrote §6do audit record to " << outJson << std::endl;

    return 0;
}
'''
    return src


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root', type=Path, required=True)
    p.add_argument('--build-root', type=Path, required=True)
    p.add_argument('--work', type=Path, required=True)
    p.add_argument('--mutation', choices=['fixed', 'no-reset', 'no-tail'], default='fixed')
    a = p.parse_args()

    root = a.root.resolve()
    work = a.work.resolve()
    buildRoot = a.build_root.resolve()

    if work.exists() or root == work or root in work.parents:
        raise ValueError('fresh external work required')
    work.mkdir(parents=True, exist_ok=False)

    cpp_src = generate_cpp_source(root, mutation=a.mutation)
    (work / 'probe.cpp').write_text(cpp_src)
    (work / 'source.json').write_text(json.dumps({
        'cpp_sha256': hashlib.sha256(cpp_src.encode()).hexdigest(),
        'mutation': a.mutation
    }, indent=2) + '\n')

    b = buildRoot / 'build'
    db = json.loads((b / 'compile_commands.json').read_text())
    entry = next(x for x in db if x['file'].endswith('/entry.cpp') and 'earthcall_webgpu.dir' in x['command'])
    cmd = shlex.split(entry['command'])
    cmd[cmd.index('-c') + 1] = str(work / 'probe.cpp')
    cmd[cmd.index('-o') + 1] = str(work / 'probe.o')
    cmd += ['-std=c++20', '-O3']
    subprocess.run(cmd, cwd=b, check=True)

    cmd = shlex.split((b / 'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text())
    cmd[next(i for i, x in enumerate(cmd) if x.endswith('/src/entry.cpp.o'))] = str(work / 'probe.o')
    obs_src = root / "scratch/probes/ray_transport_metal_pipeline_observer.mm"
    subprocess.run(["/usr/bin/clang++", "-std=c++20", "-O2", "-fobjc-arc", "-c", str(obs_src), "-o", str(work / "observer.o")], check=True)
    cmd[cmd.index("-o") + 1] = str(work / "probe")
    cmd += [str(work / "observer.o"), "-framework", "Metal", "-framework", "Foundation"]
    subprocess.run(cmd, cwd=b, check=True)
    print('BUILT faithful residual probe in', work)


if __name__ == '__main__':
    main()
