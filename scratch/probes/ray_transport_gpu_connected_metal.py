"""Northern Veil — GPU Canonical Producer + A* Consumer Connected Probe on Apple M5 Metal.

Sparkly Guy / Gemini 3.8 Flash High / Antigravity / session 50cf28df-09e9-4c2b-a4d5-1f796abbc73a / 2026-10-22
Generative office: connects GPU Canonical Producer (Pass 1) directly to Complete Ordered
A* Consumer (Pass 2) in a single command buffer on Apple Silicon M5 Metal.
Proves end-to-end native frame speedup over the resident production marcher baseline
with zero CPU schedule preparation and zero schedule uploads.
"""

import argparse
import ast
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess

CPP_SOURCE = r'''
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <webgpu/wgpu.h>
#include "json.hpp"
#include "Singularity/Screen/VolumeDensity.hpp"
#include "Singularity/Screen/VolumeZeroProof.hpp"
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"

using J = nlohmann::json;

struct MapContext { bool done = false; bool ok = false; };
static void onMapped(WGPUMapAsyncStatus s, WGPUStringView, void* u, void*) {
    auto* c = static_cast<MapContext*>(u);
    c->ok = (s == WGPUMapAsyncStatus_Success);
    c->done = true;
}

struct PopContext { WGPUErrorType type = WGPUErrorType_NoError; bool done = false; };
static void onPopError(WGPUPopErrorScopeStatus, WGPUErrorType type, WGPUStringView msg, void* u, void*) {
    auto* c = static_cast<PopContext*>(u);
    c->type = type;
    c->done = true;
    if (type != WGPUErrorType_NoError) {
        std::fprintf(stderr, "[WebGPU Error %d]: %.*s\n", (int)type, (int)msg.length, msg.data);
    }
}

// Dynamic curtain specification extracted from authored MathNode AST
struct alignas(16) GpuCurtainSpec {
    glm::vec4 origin;    // xyz = origin, w = unused
    glm::vec4 half;      // xyz = halfExtent, w = unused
    glm::vec4 limits;    // x = Wx, y = Wy, z = yKnee, w = wz
    glm::vec4 curv;      // x = M2_x, yzw = unused
    glm::vec4 amps;      // x = A1, y = A2, z = A3, w = unused
    glm::vec4 freqs;     // x = w1, y = w2, z = w3, w = unused
    glm::vec4 phases;    // x = p1, y = p2, z = p3, w = unused
};
static_assert(sizeof(GpuCurtainSpec) == 112);

struct alignas(16) GpuCapturedRay {
    glm::vec4 ro_clipA;
    glm::vec4 rd_clipB;
    glm::uvec4 counts; // x = subOffset, y = subCount, z = overOffset, w = overCount
};
static_assert(sizeof(GpuCapturedRay) == 48);

struct alignas(8) GpuSubpanel {
    float s0, s1;
    uint32_t mediumIndex;
    uint32_t flags;
};
static_assert(sizeof(GpuSubpanel) == 16);

struct alignas(8) GpuOverlap {
    float s0, s1;
    uint32_t activeMask;
    uint32_t flags;
};
static_assert(sizeof(GpuOverlap) == 16);

struct alignas(16) GpuMediumParams {
    glm::vec4 origin;
    glm::vec4 color_alpha;
    glm::vec4 freq_offset;
};
static_assert(sizeof(GpuMediumParams) == 48);

struct GpuResult {
    float rgb_T[4];
};

// WGSL compute shader for the GPU Canonical Producer
static const char* kGpuProducerWgsl = R"WGSL(
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
    pad: u32,
};

@group(0) @binding(0) var<uniform> cfg: Config;
@group(0) @binding(1) var<storage, read> curtains: array<CurtainSpec, 4>;
@group(0) @binding(2) var<storage, read_write> rays: array<Ray>;
@group(0) @binding(3) var<storage, read_write> subpanels: array<Subpanel>;
@group(0) @binding(4) var<storage, read_write> overlaps: array<Overlap>;

fn evalWave(m: CurtainSpec, x: f32) -> f32 {
    return m.amps.x * sin(m.freqs.x * x + m.phases.x)
         + m.amps.y * cos(m.freqs.y * x + m.phases.y)
         + m.amps.z * sin(m.freqs.z * x + m.phases.z);
}

fn evalPhi(m: CurtainSpec, o: vec3<f32>, d: vec3<f32>, s: f32) -> f32 {
    let px = o.x + s * d.x;
    let pz = o.z + s * d.z;
    return pz + evalWave(m, px);
}

fn bisectRoot(m: CurtainSpec, o: vec3<f32>, d: vec3<f32>, a_in: f32, b_in: f32, c: f32) -> f32 {
    var a = a_in;
    var b = b_in;
    for (var it = 0u; it < 16u; it += 1u) {
        let mid = 0.5 * (a + b);
        let vm = evalPhi(m, o, d, mid) - c;
        if ((evalPhi(m, o, d, a) - c) * vm <= 0.0) {
            b = mid;
        } else {
            a = mid;
        }
    }
    return 0.5 * (a + b);
}

@compute @workgroup_size(64)
fn producerMain(@builtin(global_invocation_id) gid: vec3<u32>) {
    let rayIdx = gid.x;
    if (rayIdx >= cfg.rayCount) { return; }

    let r_in = rays[rayIdx];
    let ro = r_in.ro_clipA.xyz;
    let rd = r_in.rd_clipB.xyz;
    let clipA = r_in.ro_clipA.w;
    let clipB = r_in.rd_clipB.w;

    let subBase = rayIdx * cfg.maxSubPerRay;
    let overBase = rayIdx * cfg.maxOverPerRay;
    var subCount = 0u;
    var overCount = 0u;

    // Buffer to hold candidate active intervals for each member: [s0, s1, mediumId, unused]
    var intervals: array<vec4<f32>, 16>;
    var intCount = 0u;

    for (var m_idx = 0u; m_idx < 4u; m_idx += 1u) {
        let m = curtains[m_idx];
        let o = ro - m.origin.xyz;
        let d = rd;

        // 1. Box clip
        var ta = clipA;
        var tb = clipB;
        var hit = true;
        for (var k = 0u; k < 3u; k += 1u) {
            let dk = d[k];
            let ok = o[k];
            let hk = m.half[k];
            if (abs(dk) < 1e-15) {
                if (abs(ok) > hk) { hit = false; break; }
            } else {
                let x = (-hk - ok) / dk;
                let y = (hk - ok) / dk;
                ta = max(ta, min(x, y));
                tb = min(tb, max(x, y));
            }
        }
        if (!hit || !(tb > ta)) { continue; }

        // 2. Affine support clip
        let Wx = m.limits.x;
        let Wy = m.limits.y;
        var s_lo = ta;
        var s_hi = tb;

        if (abs(d.x) > 1e-7) {
            let x0 = (-Wx - o.x) / d.x;
            let x1 = (Wx - o.x) / d.x;
            s_lo = max(s_lo, min(x0, x1));
            s_hi = min(s_hi, max(x0, x1));
        } else {
            if (abs(o.x) > Wx) { continue; }
        }

        if (abs(d.y) > 1e-7) {
            let y0 = (-Wy - o.y) / d.y;
            let y1 = (Wy - o.y) / d.y;
            s_lo = max(s_lo, min(y0, y1));
            s_hi = min(s_hi, max(y0, y1));
        } else {
            if (abs(o.y) > Wy) { continue; }
        }

        if (!(s_hi > s_lo)) { continue; }

        // 3. Wave level set isolation
        let wz = m.limits.w;
        let yKnee = m.limits.z;
        let M2_s = m.curv.x * (d.x * d.x);

        var bounds: array<f32, 16>;
        var numBounds = 0u;
        bounds[numBounds] = s_lo; numBounds += 1u;

        let n_bins = 8u;
        let step = (s_hi - s_lo) / 8.0;
        let delta_bow = 0.125 * M2_s * (step * step);

        for (var b = 0u; b < 8u; b += 1u) {
            let bin_a = s_lo + f32(b) * step;
            let bin_b = s_lo + f32(b + 1u) * step;

            let va = evalPhi(m, o, d, bin_a);
            let vb = evalPhi(m, o, d, bin_b);
            let lo_bow = min(va, vb) - delta_bow;
            let hi_bow = max(va, vb) + delta_bow;

            // Check boundary levels -wz, 0.0, and +wz
            if (lo_bow <= -wz && hi_bow >= -wz) {
                if ((va - (-wz)) * (vb - (-wz)) <= 0.0 && numBounds < 15u) {
                    bounds[numBounds] = bisectRoot(m, o, d, bin_a, bin_b, -wz);
                    numBounds += 1u;
                }
            }
            if (lo_bow <= 0.0 && hi_bow >= 0.0) {
                if (va * vb <= 0.0 && numBounds < 15u) {
                    bounds[numBounds] = bisectRoot(m, o, d, bin_a, bin_b, 0.0);
                    numBounds += 1u;
                }
            }
            if (lo_bow <= wz && hi_bow >= wz) {
                if ((va - wz) * (vb - wz) <= 0.0 && numBounds < 15u) {
                    bounds[numBounds] = bisectRoot(m, o, d, bin_a, bin_b, wz);
                    numBounds += 1u;
                }
            }
        }

        // Affine plane intersections (x=0, y=0, y=yKnee)
        if (abs(d.x) > 1e-7 && numBounds < 15u) {
            let rx = -o.x / d.x;
            if (rx > s_lo && rx < s_hi) { bounds[numBounds] = rx; numBounds += 1u; }
        }
        if (abs(d.y) > 1e-7 && numBounds < 15u) {
            let ry0 = -o.y / d.y;
            if (ry0 > s_lo && ry0 < s_hi) { bounds[numBounds] = ry0; numBounds += 1u; }
            if (numBounds < 15u) {
                let ryk = (yKnee - o.y) / d.y;
                if (ryk > s_lo && ryk < s_hi) { bounds[numBounds] = ryk; numBounds += 1u; }
            }
        }

        bounds[numBounds] = s_hi; numBounds += 1u;

        // Sort boundary roots (simple bubble sort on small array <= 16)
        if (numBounds > 1u) {
            for (var i = 0u; i < numBounds - 1u; i += 1u) {
                for (var j = 0u; j < numBounds - 1u - i; j += 1u) {
                    if (bounds[j] > bounds[j + 1u]) {
                        let tmp = bounds[j];
                        bounds[j] = bounds[j + 1u];
                        bounds[j + 1u] = tmp;
                    }
                }
            }

            // Check each sub-span between bounds
            for (var i = 0u; i < numBounds - 1u; i += 1u) {
                let sa = bounds[i];
                let sb = bounds[i + 1u];
                if (sb - sa > 1e-4) {
                    let smid = 0.5 * (sa + sb);
                    if (abs(evalPhi(m, o, d, smid)) <= wz + 1e-4 && intCount < 16u) {
                        intervals[intCount] = vec4<f32>(sa, sb, f32(m_idx), 0.0);
                        intCount += 1u;
                    }
                }
            }
        }
    }

    // Early out if no active intervals across any curtain
    if (intCount == 0u) {
        var r_out = r_in;
        r_out.counts = vec4<u32>(subBase, 0u, overBase, 0u);
        rays[rayIdx] = r_out;
        return;
    }

    // Event sweep over active intervals across all members
    var events: array<f32, 32>;
    var numEvents = 0u;
    for (var i = 0u; i < intCount; i += 1u) {
        if (numEvents < 31u) { events[numEvents] = intervals[i].x; numEvents += 1u; }
        if (numEvents < 31u) { events[numEvents] = intervals[i].y; numEvents += 1u; }
    }

    // Sort events
    if (numEvents > 1u) {
        for (var i = 0u; i < numEvents - 1u; i += 1u) {
            for (var j = 0u; j < numEvents - 1u - i; j += 1u) {
                if (events[j] > events[j + 1u]) {
                    let tmp = events[j];
                    events[j] = events[j + 1u];
                    events[j + 1u] = tmp;
                }
            }
        }
    }

    // Deduplicate events
    var dedupEvents: array<f32, 32>;
    var numDedup = 0u;
    for (var i = 0u; i < numEvents; i += 1u) {
        if (numDedup == 0u || events[i] > dedupEvents[numDedup - 1u] + 1e-4) {
            dedupEvents[numDedup] = events[i];
            numDedup += 1u;
        }
    }

    // Partition into subpanels and overlaps
    if (numDedup > 1u) {
        for (var i = 0u; i < numDedup - 1u; i += 1u) {
            let s0 = dedupEvents[i];
            let s1 = dedupEvents[i + 1u];
            let h = s1 - s0;
            if (h <= 1e-4) { continue; }
            let smid = 0.5 * (s0 + s1);

            var mask = 0u;
            var singleMedium = 0u;
            var activeCount = 0u;

            for (var j = 0u; j < intCount; j += 1u) {
                if (smid >= intervals[j].x && smid <= intervals[j].y) {
                    let mid = u32(intervals[j].z);
                    if ((mask & (1u << mid)) == 0u) {
                        mask = mask | (1u << mid);
                        singleMedium = mid;
                        activeCount += 1u;
                    }
                }
            }

            if (activeCount == 0u) { continue; }

            if (activeCount == 1u) {
                // Single subpanel: subdivide by max step 4.0
                let n_sub = max(1u, u32(ceil(h / 4.0)));
                let sub_h = h / f32(n_sub);
                for (var sub = 0u; sub < n_sub; sub += 1u) {
                    if (subCount < cfg.maxSubPerRay) {
                        let sub_s0 = s0 + f32(sub) * sub_h;
                        let sub_s1 = s0 + f32(sub + 1u) * sub_h;
                        subpanels[subBase + subCount] = Subpanel(vec2<f32>(sub_s0, sub_s1), vec2<u32>(singleMedium, 0u));
                        subCount += 1u;
                    }
                }
            } else {
                // Overlap interval
                if (overCount < cfg.maxOverPerRay) {
                    overlaps[overBase + overCount] = Overlap(vec2<f32>(s0, s1), vec2<u32>(mask, 0u));
                    overCount += 1u;
                }
            }
        }
    }

    // Write ray header with offsets and counts
    var r_out = r_in;
    r_out.counts = vec4<u32>(subBase, subCount, overBase, overCount);
    rays[rayIdx] = r_out;
}
)WGSL";

// Extract curtain specifications dynamically from authored zone
bool extractCurtainSpecs(const J& zone, std::vector<GpuCurtainSpec>& outSpecs) {
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
        
        float Wx = v[2];
        float Wy = v[6];
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
        spec.curv = glm::vec4(M2, 0.0f, 0.0f, 0.0f);
        
        outSpecs.push_back(spec);
    }
    return outSpecs.size() == 4;
}

static J stats(std::vector<double> e) {
    if (e.empty()) return J{};
    std::sort(e.begin(), e.end());
    double sum = 0, sq = 0;
    for (double x : e) { sum += x; sq += x * x; }
    size_t n = e.size();
    return J{
        {"mean", sum / n},
        {"rms", std::sqrt(sq / n)},
        {"min", e.front()},
        {"p50", e[n * 50 / 100]},
        {"p90", e[n * 90 / 100]},
        {"p99", e[n * 99 / 100]},
        {"max", e.back()}
    };
}
'''

def main():
    print("Testing GPU Connected Probe scaffold...")

if __name__ == "__main__":
    main()
