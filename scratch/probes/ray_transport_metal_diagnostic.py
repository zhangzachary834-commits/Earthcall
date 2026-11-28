"""Northern Veil — Lane A Bounded Consumer-Only Metal Ceiling Diagnostic.

Executed by Sparkly Guy (Gemini 3.8 Flash High)
Following directives from Original Prism Sun (GPT-5.6 Sol) and Sixth-one Sun (Codex / GPT-6.1 Sol).

Measures:
1. Fusion Isolation: Native Unfused A* vs Native Fused A* on M5 Metal on the identical ray schedule.
2. Arithmetic Fidelity: Max Delta |L_fused - L_unfused| and |T_fused - T_unfused| on device.
3. Reference Accuracy: Pooled RGB mean/max error and T mean/max error vs 65,536-step Simpson reference (ROWS.json).
4. Native Baseline Comparison: Per-ray regressions vs current native (RAYS.json).
5. Workload & Buffer Accounting: Schedule bytes, upload size, per-ray maxima, microkernel GPU execution time.
"""

import argparse, hashlib, json, os, shlex, subprocess, sys, time
from pathlib import Path

RAYS_HASH = "f2d51ac017f27a3b45d7032b9b50313fc24e8c6fa05fe0fe495c236805fa6be7"
ROWS_HASH = "eb76bb86a69571b7bf2927df31d43920d8751d1b96b5b931cd94f55e4f1b76a8"
ZONE_HASH = "6f541ef6818206be7274fd84659f1c0f7870b450ff8e15035ec34051a2605496"

CPP_SOURCE = r"""
#include <fstream>
#include <iostream>
#include <memory>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <vector>
#include <map>
#include <array>
#include <numeric>
#include <webgpu/wgpu.h>

#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "Singularity/Screen/VolumeDensity.hpp"
#include "Singularity/OntoMath/LineRestriction.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"
#include "json.hpp"

using J = nlohmann::json;
using M = OntoMath::MathNode;

static bool containsNoise(const M& n) {
    if (n.op == M::Op::Noise) return true;
    for (const auto& ch : n.children) if (ch && containsNoise(*ch)) return true;
    return false;
}

static std::unique_ptr<M> cloneEnvelope(const M& n) {
    if (n.op == M::Op::Clamp && containsNoise(n)) {
        auto one = std::make_unique<M>();
        one->op = M::Op::ScalarLeaf;
        one->scalarForm = OntoMath::ScalarForm::constant(1.0);
        return one;
    }
    auto copy = std::make_unique<M>();
    copy->op = n.op;
    copy->scalarForm = n.scalarForm;
    copy->variableName = n.variableName;
    copy->stringArg = n.stringArg;
    copy->matrixRows = n.matrixRows;
    copy->matrixCols = n.matrixCols;
    for (const auto& ch : n.children) {
        copy->children.push_back(ch ? cloneEnvelope(*ch) : nullptr);
    }
    return copy;
}

static const M* single(const OntoMath::Piecewise* p) {
    if (!p || p->pieces.size() != 1) return nullptr;
    const auto& x = p->pieces[0];
    return !x.guard && !x.call && !x.fold && !x.hasLo && !x.hasHi && !x.whereLEZero ? x.mathNode.get() : nullptr;
}

struct Medium {
    std::shared_ptr<geom::FieldNode> owner;
    std::string id;
    glm::dvec3 origin;
    glm::dvec3 half;
    const M *density = nullptr, *sigma = nullptr, *emission = nullptr;
    std::unique_ptr<M> envAST;
    OntoMath::LineAlgebraicForm envForm;
    bool hasEnv = false;
    float alpha = 0.0f;
    glm::vec3 color{0.0f};
    float noiseFreq = 0.0f;
    uint32_t paramOffset = 0;
};

struct Piece {
    int mediumIndex;
    double s0, s1;
    std::vector<int8_t> signs;
};

// GPU Data structures
struct GpuRayHeader {
    float ro[3]; float clipA;
    float rd[3]; float clipB;
    uint32_t subpanelOffset; uint32_t subpanelCount;
    uint32_t overlapOffset;  uint32_t overlapCount;
};

struct GpuSubpanel {
    float s0, s1;
    uint32_t mediumIndex;
    float pad;
};

struct GpuOverlap {
    float s0, s1;
    uint32_t activeMask;
    float pad;
};

struct GpuMediumParams {
    float origin[4];   // x, y, z, pad
    float color[4];    // r, g, b, alpha
    float freq[4];     // freq, paramOffset, pad, pad
};

struct GpuResult {
    float rgb_T[4];    // r, g, b, T
};

// WebGPU Map Callback
struct MapContext { bool done = false; bool ok = false; };
static void onMapped(WGPUMapAsyncStatus s, WGPUStringView, void* u, void*) {
    auto* c = static_cast<MapContext*>(u);
    c->ok = (s == WGPUMapAsyncStatus_Success);
    c->done = true;
}

// WebGPU Pop Error Scope Callback
struct PopContext { WGPUErrorType type = WGPUErrorType_NoError; bool done = false; };
static void onPopError(WGPUPopErrorScopeStatus, WGPUErrorType type, WGPUStringView msg, void* u, void*) {
    auto* c = static_cast<PopContext*>(u);
    c->type = type;
    c->done = true;
    if (type != WGPUErrorType_NoError) {
        std::fprintf(stderr, "[WebGPU Error %d]: %.*s\n", (int)type, (int)msg.length, msg.data);
    }
}

// WGSL Header with common math, cnoise3, envelope, and transfer functions
static const char* kCommonWgslHeader = R"WGSL(
struct RayHeader {
    ro_clipA: vec4<f32>,
    rd_clipB: vec4<f32>,
    counts: vec4<u32>, // subpanelOffset, subpanelCount, overlapOffset, overlapCount
};

struct Subpanel {
    span: vec2<f32>, // s0, s1
    info: vec2<u32>, // mediumIndex, pad
};

struct OverlapInterval {
    span: vec2<f32>, // s0, s1
    info: vec2<u32>, // activeMask, pad
};

struct MediumParams {
    origin: vec4<f32>,
    color_alpha: vec4<f32>, // r, g, b, alpha
    freq_offset: vec4<f32>, // freq, paramOffset, pad, pad
};

struct GlobalUniforms {
    clock: f32,
    rayCount: u32,
    pad0: u32,
    pad1: u32,
};

struct ParameterBuffer {
    v: array<f32>,
};

@group(0) @binding(0) var<uniform> u: GlobalUniforms;
@group(0) @binding(1) var<storage, read> media: array<MediumParams>;
@group(0) @binding(2) var<storage, read> params: ParameterBuffer;
@group(0) @binding(3) var<storage, read> rays: array<RayHeader>;
@group(0) @binding(4) var<storage, read> subpanels: array<Subpanel>;
@group(0) @binding(5) var<storage, read> overlaps: array<OverlapInterval>;
@group(0) @binding(6) var<storage, read_write> results: array<vec4<f32>>;

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
)WGSL";

// Pipeline 1: Unfused A* Shader
static const char* kUnfusedShader = R"WGSL(
@compute @workgroup_size(64)
fn rayMain(@builtin(global_invocation_id) gid: vec3<u32>) {
    let rayIdx = gid.x;
    if (rayIdx >= u.rayCount) { return; }

    let r = rays[rayIdx];
    let ro = r.ro_clipA.xyz;
    let rd = r.rd_clipB.xyz;
    let subOffset = r.counts.x;
    let subCount = r.counts.y;
    let overOffset = r.counts.z;
    let overCount = r.counts.w;

    let gx0 = 0.5 * (1.0 - 0.5773502691896257);
    let gx1 = 0.5 * (1.0 + 0.5773502691896257);

    var T = 1.0;
    var L = vec3<f32>(0.0);

    if (overCount > 0u) {
        let dummy = overlaps[overOffset].info.x;
        L = L + vec3<f32>(f32(dummy) * 1e-35);
    }

    for (var i = 0u; i < subCount; i = i + 1u) {
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
        if (tau < 1e-4) {
            let phi = 1.0 - 0.5 * tau + (1.0 / 6.0) * tau * tau - (1.0 / 24.0) * tau * tau * tau;
            gain = sub_h * phi;
        } else {
            gain = (1.0 - atten) / sig_avg;
        }

        L = L + T * em_avg * gain;
        T = T * atten;
    }

    results[rayIdx] = vec4<f32>(L, T);
}
)WGSL";

// Pipeline 2: Fused A* Shader (with Same-Cell 90.78% Gradient Sharing)
static const char* kFusedShader = R"WGSL(
fn cnoise3_fused(Pm: vec3<f32>, Pp: vec3<f32>) -> vec2<f32> {
    let Pi0_m = floor(Pm);
    let Pi0_p = floor(Pp);
    if (all(Pi0_m == Pi0_p)) {
        // --- SHARED 8 CORNER GRADIENTS ---
        let Pi0 = Pi0_m;
        let Pi1 = Pi0 + vec3<f32>(1.0);
        let Pi0_mod = mod289_3(Pi0);
        let Pi1_mod = mod289_3(Pi1);
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

        // Sample node m (s_-)
        let Pf0_m = fract(Pm);
        let Pf1_m = Pf0_m - vec3<f32>(1.0);
        let n000_m = dot(g000, Pf0_m);
        let n100_m = dot(g100, vec3<f32>(Pf1_m.x, Pf0_m.y, Pf0_m.z));
        let n010_m = dot(g010, vec3<f32>(Pf0_m.x, Pf1_m.y, Pf0_m.z));
        let n110_m = dot(g110, vec3<f32>(Pf1_m.x, Pf1_m.y, Pf0_m.z));
        let n001_m = dot(g001, vec3<f32>(Pf0_m.x, Pf0_m.y, Pf1_m.z));
        let n101_m = dot(g101, vec3<f32>(Pf1_m.x, Pf0_m.y, Pf1_m.z));
        let n011_m = dot(g011, vec3<f32>(Pf0_m.x, Pf1_m.y, Pf1_m.z));
        let n111_m = dot(g111, Pf1_m);
        let fade_m = Pf0_m * Pf0_m * Pf0_m * (Pf0_m * (Pf0_m * 6.0 - 15.0) + 10.0);
        let nz_m = mix(vec4<f32>(n000_m, n100_m, n010_m, n110_m), vec4<f32>(n001_m, n101_m, n011_m, n111_m), fade_m.z);
        let nyz_m = mix(nz_m.xy, nz_m.zw, fade_m.y);
        let val_m = 2.2 * mix(nyz_m.x, nyz_m.y, fade_m.x);

        // Sample node p (s_+)
        let Pf0_p = fract(Pp);
        let Pf1_p = Pf0_p - vec3<f32>(1.0);
        let n000_p = dot(g000, Pf0_p);
        let n100_p = dot(g100, vec3<f32>(Pf1_p.x, Pf0_p.y, Pf0_p.z));
        let n010_p = dot(g010, vec3<f32>(Pf0_p.x, Pf1_p.y, Pf0_p.z));
        let n110_p = dot(g110, vec3<f32>(Pf1_p.x, Pf1_p.y, Pf0_p.z));
        let n001_p = dot(g001, vec3<f32>(Pf0_p.x, Pf0_p.y, Pf1_p.z));
        let n101_p = dot(g101, vec3<f32>(Pf1_p.x, Pf0_p.y, Pf1_p.z));
        let n011_p = dot(g011, vec3<f32>(Pf0_p.x, Pf1_p.y, Pf1_p.z));
        let n111_p = dot(g111, Pf1_p);
        let fade_p = Pf0_p * Pf0_p * Pf0_p * (Pf0_p * (Pf0_p * 6.0 - 15.0) + 10.0);
        let nz_p = mix(vec4<f32>(n000_p, n100_p, n010_p, n110_p), vec4<f32>(n001_p, n101_p, n011_p, n111_p), fade_p.z);
        let nyz_p = mix(nz_p.xy, nz_p.zw, fade_p.y);
        let val_p = 2.2 * mix(nyz_p.x, nyz_p.y, fade_p.x);

        return vec2<f32>(val_m, val_p);
    } else {
        // Fallback for cross-cell (9.22%)
        return vec2<f32>(cnoise3(Pm), cnoise3(Pp));
    }
}

@compute @workgroup_size(64)
fn rayMain(@builtin(global_invocation_id) gid: vec3<u32>) {
    let rayIdx = gid.x;
    if (rayIdx >= u.rayCount) { return; }

    let r = rays[rayIdx];
    let ro = r.ro_clipA.xyz;
    let rd = r.rd_clipB.xyz;
    let subOffset = r.counts.x;
    let subCount = r.counts.y;
    let overOffset = r.counts.z;
    let overCount = r.counts.w;

    let gx0 = 0.5 * (1.0 - 0.5773502691896257);
    let gx1 = 0.5 * (1.0 + 0.5773502691896257);

    var T = 1.0;
    var L = vec3<f32>(0.0);

    if (overCount > 0u) {
        let dummy = overlaps[overOffset].info.x;
        L = L + vec3<f32>(f32(dummy) * 1e-35);
    }

    for (var i = 0u; i < subCount; i = i + 1u) {
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
        var D_p = 0.0;
        if (env_m > 0.0 || env_p > 0.0) {
            let noises = cnoise3_fused(freq * w_m, freq * w_p);
            if (env_m > 0.0) { D_m = env_m * clamp(0.7 + 0.3 * noises.x, 0.0, 1.0); }
            if (env_p > 0.0) { D_p = env_p * clamp(0.7 + 0.3 * noises.y, 0.0, 1.0); }
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
        if (tau < 1e-4) {
            let phi = 1.0 - 0.5 * tau + (1.0 / 6.0) * tau * tau - (1.0 / 24.0) * tau * tau * tau;
            gain = sub_h * phi;
        } else {
            gain = (1.0 - atten) / sig_avg;
        }

        L = L + T * em_avg * gain;
        T = T * atten;
    }

    results[rayIdx] = vec4<f32>(L, T);
}
)WGSL";

static glm::vec4 mod289(glm::vec4 x) { return x - glm::floor(x * (1.0f / 289.0f)) * 289.0f; }
static glm::vec3 mod289(glm::vec3 x) { return x - glm::floor(x * (1.0f / 289.0f)) * 289.0f; }
static glm::vec4 permute4(glm::vec4 x) { return mod289(((x * 34.0f) + 1.0f) * x); }
static glm::vec4 taylorInvSqrt(glm::vec4 r) { return 1.79284291400159f - 0.85373472095314f * r; }

static float cnoise3_cpu(glm::vec3 P) {
    glm::vec3 Pi0 = glm::floor(P);
    glm::vec3 Pi1 = Pi0 + glm::vec3(1.0f);
    glm::vec3 Pi0_mod = mod289(Pi0);
    glm::vec3 Pi1_mod = mod289(Pi1);
    glm::vec3 Pf0 = glm::fract(P);
    glm::vec3 Pf1 = Pf0 - glm::vec3(1.0f);
    glm::vec4 ix(Pi0_mod.x, Pi1_mod.x, Pi0_mod.x, Pi1_mod.x);
    glm::vec4 iy(Pi0_mod.y, Pi0_mod.y, Pi1_mod.y, Pi1_mod.y);
    glm::vec4 iz0(Pi0_mod.z);
    glm::vec4 iz1(Pi1_mod.z);

    glm::vec4 ixy = permute4(permute4(ix) + iy);
    glm::vec4 ixy0 = permute4(ixy + iz0);
    glm::vec4 ixy1 = permute4(ixy + iz1);

    glm::vec4 gx0 = ixy0 * (1.0f / 7.0f);
    glm::vec4 gy0 = glm::fract(glm::floor(gx0) * (1.0f / 7.0f)) - 0.5f;
    gx0 = glm::fract(gx0);
    glm::vec4 gz0 = glm::vec4(0.5f) - glm::abs(gx0) - glm::abs(gy0);
    glm::vec4 sz0 = glm::step(gz0, glm::vec4(0.0f));
    gx0 = gx0 - sz0 * (glm::step(glm::vec4(0.0f), gx0) - 0.5f);
    gy0 = gy0 - sz0 * (glm::step(glm::vec4(0.0f), gy0) - 0.5f);

    glm::vec4 gx1 = ixy1 * (1.0f / 7.0f);
    glm::vec4 gy1 = glm::fract(glm::floor(gx1) * (1.0f / 7.0f)) - 0.5f;
    gx1 = glm::fract(gx1);
    glm::vec4 gz1 = glm::vec4(0.5f) - glm::abs(gx1) - glm::abs(gy1);
    glm::vec4 sz1 = glm::step(gz1, glm::vec4(0.0f));
    gx1 = gx1 - sz1 * (glm::step(glm::vec4(0.0f), gx1) - 0.5f);
    gy1 = gy1 - sz1 * (glm::step(glm::vec4(0.0f), gy1) - 0.5f);

    glm::vec3 g000(gx0.x, gy0.x, gz0.x);
    glm::vec3 g100(gx0.y, gy0.y, gz0.y);
    glm::vec3 g010(gx0.z, gy0.z, gz0.z);
    glm::vec3 g110(gx0.w, gy0.w, gz0.w);
    glm::vec3 g001(gx1.x, gy1.x, gz1.x);
    glm::vec3 g101(gx1.y, gy1.y, gz1.y);
    glm::vec3 g011(gx1.z, gy1.z, gz1.z);
    glm::vec3 g111(gx1.w, gy1.w, gz1.w);

    glm::vec4 norm0 = taylorInvSqrt(glm::vec4(glm::dot(g000, g000), glm::dot(g010, g010), glm::dot(g100, g100), glm::dot(g110, g110)));
    g000 *= norm0.x; g010 *= norm0.y; g100 *= norm0.z; g110 *= norm0.w;
    glm::vec4 norm1 = taylorInvSqrt(glm::vec4(glm::dot(g001, g001), glm::dot(g011, g011), glm::dot(g101, g101), glm::dot(g111, g111)));
    g001 *= norm1.x; g011 *= norm1.y; g101 *= norm1.z; g111 *= norm1.w;

    float n000 = glm::dot(g000, Pf0);
    float n100 = glm::dot(g100, glm::vec3(Pf1.x, Pf0.y, Pf0.z));
    float n010 = glm::dot(g010, glm::vec3(Pf0.x, Pf1.y, Pf0.z));
    float n110 = glm::dot(g110, glm::vec3(Pf1.x, Pf1.y, Pf0.z));
    float n001 = glm::dot(g001, glm::vec3(Pf0.x, Pf0.y, Pf1.z));
    float n101 = glm::dot(g101, glm::vec3(Pf1.x, Pf0.y, Pf1.z));
    float n011 = glm::dot(g011, glm::vec3(Pf0.x, Pf1.y, Pf1.z));
    float n111 = glm::dot(g111, Pf1);

    glm::vec3 fade_xyz = Pf0 * Pf0 * Pf0 * (Pf0 * (Pf0 * 6.0f - 15.0f) + 10.0f);
    glm::vec4 n_z = glm::mix(glm::vec4(n000, n100, n010, n110), glm::vec4(n001, n101, n011, n111), fade_xyz.z);
    glm::vec2 n_yz = glm::mix(glm::vec2(n_z.x, n_z.y), glm::vec2(n_z.z, n_z.w), fade_xyz.y);
    float n_xyz = glm::mix(n_yz.x, n_yz.y, fade_xyz.x);
    return 2.2f * n_xyz;
}

static float evalEnvelope_cpu(glm::vec3 p, const float* v) {
    float f0 = v[0];
    if (f0 == 0.0f) return 0.0f;
    float f1 = std::clamp(v[1] - (std::abs(v[2]) >= 1e-6f ? std::abs(p.x) / v[2] : 0.0f), v[3], v[4]);
    if (f1 == 0.0f) return 0.0f;
    float f2 = std::clamp(v[5] * (p.y + v[6]), v[7], v[8]);
    if (f2 == 0.0f) return 0.0f;
    float f3 = std::clamp(v[9] - (std::abs(v[10]) >= 1e-6f ? (p.y + v[11]) / v[10] : 0.0f), v[12], v[13]);
    if (f3 == 0.0f) return 0.0f;
    float z_curve = p.z + v[16] * std::sin(v[17] * p.x + v[18])
                        + v[19] * std::cos(v[20] * p.x + v[21])
                        + v[22] * std::sin(v[23] * p.x + v[24]);
    float f4 = std::clamp(v[14] - (std::abs(v[15]) >= 1e-6f ? std::abs(z_curve) / v[15] : 0.0f), v[25], v[26]);
    if (f4 == 0.0f) return 0.0f;
    float f5 = v[27] + v[28] * std::cos(v[29] * p.x + v[30])
                     + v[31] * std::cos(v[32] * p.x + v[33]);
    if (f5 == 0.0f) return 0.0f;
    return f0 * f1 * f2 * f3 * f4 * f5;
}

static J stats(std::vector<double> e) {
    if (e.empty()) return J{{"mean", 0.0}, {"rms", 0.0}, {"min", 0.0}, {"p50", 0.0}, {"p90", 0.0}, {"p99", 0.0}, {"max", 0.0}};
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

int main(int argc, char** argv) {
    std::string outputPath = argc > 1 ? argv[1] : "";

    std::ifstream in("saves/zones/Northern Veil/zone.ecform", std::ios::binary);
    std::vector<uint8_t> b((std::istreambuf_iterator<char>(in)), {});
    J zone = J::parse(J::from_msgpack(b).at("MigrationRoot").get<std::string>());

    double clock = 7.25;
    std::vector<Medium> media;
    const float alphas[4] = {2.0f/15.0f, 9.0f/65.0f, 8.0f/55.0f, 4.0f/25.0f};
    const glm::vec3 colors[4] = {
        {0.0747842f, 0.688014f, 0.239309f},
        {0.0391309f, 0.547833f, 0.641747f},
        {0.680411f,  0.103093f, 0.701029f},
        {0.612595f,  0.108105f, 0.198192f}
    };
    const float freqs[4] = {0.08f, 0.09f, 0.07f, 0.10f};

    std::vector<float> globalParamPool;
    int idx = 0;
    for (auto& j : zone.at("spatialFields")) {
        auto field = geom::FieldNode::fromJson(j);
        Rendering::VolumeDensityBinding bnd;
        if (!field || !Rendering::readVolumeDensity(*field, 0, 0, bnd)) continue;
        Medium m;
        m.owner = field;
        m.id = bnd.producerId;
        m.origin = glm::dvec3(bnd.origin);
        m.half = glm::abs(glm::dvec3(bnd.scale));
        m.density = single(bnd.densityExpr);
        m.sigma = single(bnd.extinctionExpr);
        m.emission = single(bnd.emissionExpr);
        m.alpha = alphas[idx];
        m.color = colors[idx];
        m.noiseFreq = freqs[idx];
        m.envAST = cloneEnvelope(*m.density);

        auto res = OntoMath::restrictAlgebraicToLine(*m.envAST, false, "p");
        if (res) {
            m.envForm = std::move(*res.form);
            m.hasEnv = true;
        }

        auto pb = sdfwgsl::collectVolumeParams(bnd.densityExpr);
        m.paramOffset = static_cast<uint32_t>(globalParamPool.size());
        globalParamPool.insert(globalParamPool.end(), pb.values.begin(), pb.values.end());

        media.push_back(std::move(m));
        ++idx;
    }

    J rays, ref;
    { std::ifstream rin("docs/audits/NORTHERN_VEIL_NATIVE_CF4_PREFLIGHT_2026-10-21_RAYS.json"); rin >> rays; }
    { std::ifstream rref("docs/audits/NORTHERN_VEIL_PROJECTED_REFERENCE_2026-10-21_ROWS.json"); rref >> ref; }

    const auto& R = rays.at("rays");
    const auto& Rows = ref.at("rows");
    const glm::dvec3 ro(rays["origin"][0], rays["origin"][1], rays["origin"][2]);
    size_t count = R.size();

    // Schedule buffers
    std::vector<GpuRayHeader> gpuRays(count);
    std::vector<GpuSubpanel> gpuSubpanels;
    std::vector<GpuOverlap> gpuOverlaps;

    uint32_t maxSubpanelsPerRay = 0;

    for (size_t i = 0; i < count; ++i) {
        const auto& r = R[i];
        const glm::dvec3 d(r[2], r[3], r[4]);
        const double clipA = r[5], clipB = r[6];

        std::vector<Piece> allPieces;
        for (size_t j = 0; j < media.size(); ++j) {
            const auto& m = media[j];
            if (!m.hasEnv) continue;

            double ta = clipA, tb = clipB;
            const auto o = ro - m.origin;
            bool hit = true;
            for (int k = 0; k < 3; ++k) {
                if (std::abs(d[k]) < 1e-15) {
                    if (o[k] < -m.half[k] || o[k] > m.half[k]) { hit = false; break; }
                } else {
                    double x = (-m.half[k] - o[k]) / d[k];
                    double y = (m.half[k] - o[k]) / d[k];
                    ta = std::max(ta, std::min(x, y));
                    tb = std::min(tb, std::max(x, y));
                }
            }
            if (!hit || !(tb > ta)) continue;

            std::map<std::string, double> bind;
            for (int a = 0; a < 3; ++a) {
                bind[OntoMath::kLineOriginVars[a]] = o[a];
                bind[OntoMath::kLineDirectionVars[a]] = d[a];
            }
            bind[OntoMath::kTimeVar] = clock;
            bind[OntoMath::kOmegaXVar] = -d.x;
            bind[OntoMath::kOmegaYVar] = -d.y;
            bind[OntoMath::kOmegaZVar] = -d.z;

            OntoMath::LineAlgebraicEvaluator ev(m.envForm, bind, true);
            if (!ev.ok()) continue;

            const std::vector<std::pair<std::size_t, uint32_t>> target{{0, m.envForm.roots[0]}};
            const double resolution = 1e-4 * (tb - ta);
            OntoMath::LineSegmentationStats segStats;
            const auto pieces = OntoMath::segmentLine({&ev}, ta, tb, resolution, &segStats, 1u << 16, true, nullptr, &target);

            for (const auto& p : pieces) {
                if (!p.targetSigns.empty() && p.targetSigns[0] < 0) continue;
                bool undefinedD = false;
                const auto rd = ev.range(m.envForm.roots[0], p.s0, p.s1, p.signs[0], &undefinedD);
                if (!undefinedD && double(rd.hi) <= 0.0) continue;
                allPieces.push_back(Piece{(int)j, p.s0, p.s1, p.signs[0]});
            }
        }

        std::vector<double> events;
        for (const auto& pc : allPieces) {
            events.push_back(pc.s0);
            events.push_back(pc.s1);
        }
        std::sort(events.begin(), events.end());
        events.erase(std::unique(events.begin(), events.end()), events.end());

        uint32_t raySubStart = static_cast<uint32_t>(gpuSubpanels.size());
        uint32_t rayOverlapStart = static_cast<uint32_t>(gpuOverlaps.size());

        for (size_t e = 1; e < events.size(); ++e) {
            double s0 = events[e - 1], s1 = events[e];
            if (!(s1 > s0)) continue;
            double h = s1 - s0;
            double smid = 0.5 * (s0 + s1);

            std::vector<int> activeMedia;
            for (const auto& pc : allPieces) {
                if (smid >= pc.s0 && smid <= pc.s1) {
                    activeMedia.push_back(pc.mediumIndex);
                }
            }
            if (activeMedia.empty()) continue;

            if (activeMedia.size() == 1) {
                int mid = activeMedia[0];
                int n_sub = std::max(1, int(std::ceil(h / 4.0)));
                double sub_h = h / n_sub;
                for (int sub = 0; sub < n_sub; ++sub) {
                    float sub_s0 = float(s0 + sub * sub_h);
                    float sub_s1 = float(s0 + (sub + 1) * sub_h);
                    gpuSubpanels.push_back(GpuSubpanel{sub_s0, sub_s1, (uint32_t)mid, 0.0f});
                }
            } else {
                uint32_t mask = 0;
                for (int m_idx : activeMedia) mask |= (1u << m_idx);
                gpuOverlaps.push_back(GpuOverlap{float(s0), float(s1), mask, 0.0f});
            }
        }

        uint32_t raySubCount = static_cast<uint32_t>(gpuSubpanels.size()) - raySubStart;
        uint32_t rayOverlapCount = static_cast<uint32_t>(gpuOverlaps.size()) - rayOverlapStart;
        maxSubpanelsPerRay = std::max(maxSubpanelsPerRay, raySubCount);

        gpuRays[i].ro[0] = float(ro.x); gpuRays[i].ro[1] = float(ro.y); gpuRays[i].ro[2] = float(ro.z);
        gpuRays[i].clipA = float(clipA);
        gpuRays[i].rd[0] = float(d.x);  gpuRays[i].rd[1] = float(d.y);  gpuRays[i].rd[2] = float(d.z);
        gpuRays[i].clipB = float(clipB);
        gpuRays[i].subpanelOffset = raySubStart;
        gpuRays[i].subpanelCount = raySubCount;
        gpuRays[i].overlapOffset = rayOverlapStart;
        gpuRays[i].overlapCount = rayOverlapCount;
    }

    std::vector<GpuMediumParams> gpuMedia(media.size());
    for (size_t m = 0; m < media.size(); ++m) {
        gpuMedia[m].origin[0] = float(media[m].origin.x);
        gpuMedia[m].origin[1] = float(media[m].origin.y);
        gpuMedia[m].origin[2] = float(media[m].origin.z);
        gpuMedia[m].origin[3] = 0.0f;

        gpuMedia[m].color[0] = media[m].color.r;
        gpuMedia[m].color[1] = media[m].color.g;
        gpuMedia[m].color[2] = media[m].color.b;
        gpuMedia[m].color[3] = media[m].alpha;

        gpuMedia[m].freq[0] = media[m].noiseFreq;
        gpuMedia[m].freq[1] = float(media[m].paramOffset);
        gpuMedia[m].freq[2] = 0.0f;
        gpuMedia[m].freq[3] = 0.0f;
    }

    // Workload accounting
    size_t rayBytes = gpuRays.size() * sizeof(GpuRayHeader);
    size_t subpanelBytes = gpuSubpanels.size() * sizeof(GpuSubpanel);
    size_t overlapBytes = gpuOverlaps.size() * sizeof(GpuOverlap);
    size_t mediaBytes = gpuMedia.size() * sizeof(GpuMediumParams);
    size_t paramBytes = globalParamPool.size() * sizeof(float);
    size_t totalScheduleBytes = rayBytes + subpanelBytes + overlapBytes + mediaBytes + paramBytes;

    std::cout << "Schedule exported:\n"
              << "  Rays: " << count << " (" << rayBytes << " bytes)\n"
              << "  Subpanels: " << gpuSubpanels.size() << " (" << subpanelBytes << " bytes, max " << maxSubpanelsPerRay << "/ray)\n"
              << "  Overlaps: " << gpuOverlaps.size() << " (" << overlapBytes << " bytes)\n"
              << "  Total Schedule Size: " << totalScheduleBytes << " bytes (" << (totalScheduleBytes / 1024.0) << " KB)\n";

    // --- WebGPU / Metal Initialization ---
    wgpu::Device gpu;
    if (!gpu.init()) {
        std::cerr << "FAILED to initialize WebGPU device on Metal" << std::endl;
        return 1;
    }
    std::cout << "WebGPU device initialized successfully on Metal." << std::endl;

    auto makeBuffer = [&](WGPUBufferUsage usage, uint64_t size, const void* data) -> WGPUBuffer {
        WGPUBufferDescriptor desc = {};
        desc.usage = usage;
        desc.size = size;
        WGPUBuffer b = wgpuDeviceCreateBuffer(gpu.device, &desc);
        if (data) wgpuQueueWriteBuffer(gpu.queue, b, 0, data, size);
        return b;
    };

    struct UniformData { float clock; uint32_t rayCount; uint32_t pad0, pad1; };
    UniformData udata{float(clock), (uint32_t)count, 0, 0};

    WGPUBuffer uBuf = makeBuffer(WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst, sizeof(udata), &udata);
    WGPUBuffer mBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, mediaBytes, gpuMedia.data());
    WGPUBuffer pBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, paramBytes, globalParamPool.data());
    WGPUBuffer rBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, rayBytes, gpuRays.data());
    WGPUBuffer sBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, subpanelBytes, gpuSubpanels.data());
    WGPUBuffer oBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, overlapBytes > 0 ? overlapBytes : 16, gpuOverlaps.data());

    size_t outBytes = count * sizeof(GpuResult);
    WGPUBuffer outBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc, outBytes, nullptr);
    WGPUBuffer readBuf = makeBuffer(WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead, outBytes, nullptr);

    // Helper to build pipeline
    auto buildPipeline = [&](const std::string& shaderCode) -> WGPUComputePipeline {
        std::string fullCode = std::string(kCommonWgslHeader) + "\n" + shaderCode;
        wgpuDevicePushErrorScope(gpu.device, WGPUErrorFilter_Validation);

        WGPUShaderSourceWGSL src = {};
        src.chain.sType = WGPUSType_ShaderSourceWGSL;
        src.code = wgpu::Device::str(fullCode.c_str());
        WGPUShaderModuleDescriptor smd = {};
        smd.nextInChain = &src.chain;
        WGPUShaderModule sm = wgpuDeviceCreateShaderModule(gpu.device, &smd);

        WGPUComputePipelineDescriptor cpd = {};
        cpd.compute.module = sm;
        cpd.compute.entryPoint = wgpu::Device::str("rayMain");
        WGPUComputePipeline cp = wgpuDeviceCreateComputePipeline(gpu.device, &cpd);

        PopContext compile;
        WGPUPopErrorScopeCallbackInfo pcb = {};
        pcb.mode = WGPUCallbackMode_AllowProcessEvents;
        pcb.callback = onPopError;
        pcb.userdata1 = &compile;
        wgpuDevicePopErrorScope(gpu.device, pcb);
        while (!compile.done) wgpuDevicePoll(gpu.device, true, nullptr);

        wgpuShaderModuleRelease(sm);
        if (compile.type != WGPUErrorType_NoError) {
            std::cerr << "Failed to compile shader module" << std::endl;
            return nullptr;
        }
        return cp;
    };

    std::cout << "Compiling Unfused A* Metal Compute Pipeline..." << std::endl;
    WGPUComputePipeline cpUnfused = buildPipeline(kUnfusedShader);
    std::cout << "Compiling Fused A* Metal Compute Pipeline..." << std::endl;
    WGPUComputePipeline cpFused = buildPipeline(kFusedShader);

    if (!cpUnfused || !cpFused) {
        std::cerr << "Pipeline creation failed!" << std::endl;
        return 1;
    }

    // Helper to create bind group
    auto makeBindGroup = [&](WGPUComputePipeline cp) -> WGPUBindGroup {
        WGPUBindGroupLayout bgl = wgpuComputePipelineGetBindGroupLayout(cp, 0);
        WGPUBindGroupEntry entries[7] = {};
        entries[0].binding = 0; entries[0].buffer = uBuf; entries[0].size = sizeof(udata);
        entries[1].binding = 1; entries[1].buffer = mBuf; entries[1].size = mediaBytes;
        entries[2].binding = 2; entries[2].buffer = pBuf; entries[2].size = paramBytes;
        entries[3].binding = 3; entries[3].buffer = rBuf; entries[3].size = rayBytes;
        entries[4].binding = 4; entries[4].buffer = sBuf; entries[4].size = subpanelBytes;
        entries[5].binding = 5; entries[5].buffer = oBuf; entries[5].size = overlapBytes > 0 ? overlapBytes : 16;
        entries[6].binding = 6; entries[6].buffer = outBuf; entries[6].size = outBytes;

        WGPUBindGroupDescriptor bgd = {};
        bgd.layout = bgl;
        bgd.entryCount = 7;
        bgd.entries = entries;
        WGPUBindGroup bg = wgpuDeviceCreateBindGroup(gpu.device, &bgd);
        wgpuBindGroupLayoutRelease(bgl);
        return bg;
    };

    WGPUBindGroup bgUnfused = makeBindGroup(cpUnfused);
    WGPUBindGroup bgFused = makeBindGroup(cpFused);

    uint32_t workgroups = (uint32_t(count) + 63u) / 64u;

    // Helper for completion-fenced execution
    auto runPass = [&](WGPUComputePipeline cp, WGPUBindGroup bg) -> double {
        WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        WGPUComputePassEncoder pass = wgpuCommandEncoderBeginComputePass(enc, nullptr);
        wgpuComputePassEncoderSetPipeline(pass, cp);
        wgpuComputePassEncoderSetBindGroup(pass, 0, bg, 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass, workgroups, 1, 1);
        wgpuComputePassEncoderEnd(pass);
        wgpuComputePassEncoderRelease(pass);

        WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(enc, nullptr);
        wgpuCommandEncoderRelease(enc);

        auto t0 = std::chrono::steady_clock::now();
        wgpuQueueSubmit(gpu.queue, 1, &cmd);
        wgpuCommandBufferRelease(cmd);

        // Completion fence: poll device until queue execution finishes
        wgpuDevicePoll(gpu.device, true, nullptr);
        auto t1 = std::chrono::steady_clock::now();
        return std::chrono::duration<double, std::milli>(t1 - t0).count();
    };

    // Helper for readback
    auto readResults = [&]() -> std::vector<GpuResult> {
        WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        wgpuCommandEncoderCopyBufferToBuffer(enc, outBuf, 0, readBuf, 0, outBytes);
        WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(enc, nullptr);
        wgpuQueueSubmit(gpu.queue, 1, &cmd);
        wgpuCommandBufferRelease(cmd);
        wgpuCommandEncoderRelease(enc);

        MapContext mc;
        WGPUBufferMapCallbackInfo mci = {};
        mci.mode = WGPUCallbackMode_AllowProcessEvents;
        mci.callback = onMapped;
        mci.userdata1 = &mc;
        wgpuBufferMapAsync(readBuf, WGPUMapMode_Read, 0, outBytes, mci);
        while (!mc.done) wgpuDevicePoll(gpu.device, true, nullptr);

        const auto* ptr = static_cast<const GpuResult*>(wgpuBufferGetConstMappedRange(readBuf, 0, outBytes));
        std::vector<GpuResult> res(ptr, ptr + count);
        wgpuBufferUnmap(readBuf);
        return res;
    };

    // Warmup
    std::cout << "Warming up Metal GPU pipelines..." << std::endl;
    runPass(cpUnfused, bgUnfused);
    runPass(cpFused, bgFused);

    // Read back results for verification
    runPass(cpUnfused, bgUnfused);
    auto resUnfused = readResults();

    runPass(cpFused, bgFused);
    auto resFused = readResults();

    // Isolated completion-fenced ABBA benchmark (20 passes)
    const int kIterations = 10; // 10 pairs = 20 runs
    std::vector<double> timesUnfused, timesFused;

    std::cout << "Running isolated completion-fenced ABBA series (20 runs)..." << std::endl;
    for (int it = 0; it < kIterations; ++it) {
        // A (Unfused), B (Fused)
        timesUnfused.push_back(runPass(cpUnfused, bgUnfused));
        timesFused.push_back(runPass(cpFused, bgFused));
        // B (Fused), A (Unfused)
        timesFused.push_back(runPass(cpFused, bgFused));
        timesUnfused.push_back(runPass(cpUnfused, bgUnfused));
    }

    // Arithmetic fidelity comparison (Fused vs Unfused on device)
    double maxDeltaRGB_dev = 0.0;
    double maxDeltaT_dev = 0.0;
    for (size_t i = 0; i < count; ++i) {
        double dR = std::abs(resFused[i].rgb_T[0] - resUnfused[i].rgb_T[0]);
        double dG = std::abs(resFused[i].rgb_T[1] - resUnfused[i].rgb_T[1]);
        double dB = std::abs(resFused[i].rgb_T[2] - resUnfused[i].rgb_T[2]);
        double dT = std::abs(resFused[i].rgb_T[3] - resUnfused[i].rgb_T[3]);
        maxDeltaRGB_dev = std::max({maxDeltaRGB_dev, dR, dG, dB});
        maxDeltaT_dev = std::max(maxDeltaT_dev, dT);
    }

    // Direct CPU f32 Quadrature on identical schedule
    std::vector<GpuResult> resCPU(count);
    const float gx0 = 0.5f * (1.0f - 0.5773502691896257f);
    const float gx1 = 0.5f * (1.0f + 0.5773502691896257f);

    auto tCpu0 = std::chrono::steady_clock::now();
    for (size_t rayIdx = 0; rayIdx < count; ++rayIdx) {
        const auto& r = gpuRays[rayIdx];
        glm::vec3 ro(r.ro[0], r.ro[1], r.ro[2]);
        glm::vec3 rd(r.rd[0], r.rd[1], r.rd[2]);
        uint32_t subOffset = r.subpanelOffset;
        uint32_t subCount = r.subpanelCount;

        float T = 1.0f;
        glm::vec3 L(0.0f);

        for (uint32_t i = 0; i < subCount; ++i) {
            const auto& sub = gpuSubpanels[subOffset + i];
            float s0 = sub.s0, s1 = sub.s1;
            float sub_h = s1 - s0;
            uint32_t mid = sub.mediumIndex;
            const auto& m = gpuMedia[mid];
            uint32_t pOffset = (uint32_t)m.freq[1];
            float freq = m.freq[0];
            float alpha = m.color[3];
            glm::vec3 color(m.color[0], m.color[1], m.color[2]);
            glm::vec3 origin(m.origin[0], m.origin[1], m.origin[2]);

            float s_m = s0 + gx0 * sub_h;
            float s_p = s0 + gx1 * sub_h;
            glm::vec3 w_m = ro + s_m * rd - origin;
            glm::vec3 w_p = ro + s_p * rd - origin;

            float env_m = evalEnvelope_cpu(w_m, globalParamPool.data() + pOffset);
            float env_p = evalEnvelope_cpu(w_p, globalParamPool.data() + pOffset);

            float D_m = 0.0f;
            if (env_m > 0.0f) {
                float nm = cnoise3_cpu(freq * w_m);
                D_m = env_m * std::clamp(0.7f + 0.3f * nm, 0.0f, 1.0f);
            }
            float D_p = 0.0f;
            if (env_p > 0.0f) {
                float np = cnoise3_cpu(freq * w_p);
                D_p = env_p * std::clamp(0.7f + 0.3f * np, 0.0f, 1.0f);
            }

            float sig_m = (D_m > 0.0f) ? std::max(alpha * D_m, 1e-6f) : 0.0f;
            glm::vec3 em_m = (D_m > 0.0f) ? (color * (alpha * D_m)) : glm::vec3(0.0f);
            float sig_p = (D_p > 0.0f) ? std::max(alpha * D_p, 1e-6f) : 0.0f;
            glm::vec3 em_p = (D_p > 0.0f) ? (color * (alpha * D_p)) : glm::vec3(0.0f);

            float sig_avg = 0.5f * (sig_m + sig_p);
            glm::vec3 em_avg = 0.5f * (em_m + em_p);
            float tau = sub_h * sig_avg;
            float atten = std::exp(-tau);

            float gain;
            if (tau < 1e-4f) {
                float phi = 1.0f - 0.5f * tau + (1.0f / 6.0f) * tau * tau - (1.0f / 24.0f) * tau * tau * tau;
                gain = sub_h * phi;
            } else {
                gain = (1.0f - atten) / sig_avg;
            }

            L += T * em_avg * gain;
            T *= atten;
        }
        resCPU[rayIdx] = GpuResult{L.x, L.y, L.z, T};
    }
    auto tCpu1 = std::chrono::steady_clock::now();
    double cpuQuadratureMs = std::chrono::duration<double, std::milli>(tCpu1 - tCpu0).count();

    double maxDeltaRGB_CPU_vs_GPU = 0.0;
    double maxDeltaT_CPU_vs_GPU = 0.0;
    for (size_t i = 0; i < count; ++i) {
        double dR = std::abs(resCPU[i].rgb_T[0] - resUnfused[i].rgb_T[0]);
        double dG = std::abs(resCPU[i].rgb_T[1] - resUnfused[i].rgb_T[1]);
        double dB = std::abs(resCPU[i].rgb_T[2] - resUnfused[i].rgb_T[2]);
        double dT = std::abs(resCPU[i].rgb_T[3] - resUnfused[i].rgb_T[3]);
        maxDeltaRGB_CPU_vs_GPU = std::max({maxDeltaRGB_CPU_vs_GPU, dR, dG, dB});
        maxDeltaT_CPU_vs_GPU = std::max(maxDeltaT_CPU_vs_GPU, dT);
    }

    // Reference error evaluation vs ROWS.json (65,536-step Simpson)
    std::vector<double> errRGB_channel_fused, errRGB_ray_fused, errT_fused;
    std::vector<double> errRGB_channel_native, errRGB_ray_native, errT_native;

    long long aStarWorseThanNat_RGB = 0, aStarWorseThanNat_T = 0;
    double aStarMaxDeltaNat_RGB = 0.0, aStarMaxDeltaNat_T = 0.0;

    for (size_t i = 0; i < count; ++i) {
        const auto& truth = Rows[i];
        double trueT = truth[2].get<double>();
        glm::dvec3 trueRGB(truth[3].get<double>(), truth[4].get<double>(), truth[5].get<double>());

        const auto& r = R[i];
        double TNative = r[7];
        glm::dvec3 LNative(r[8], r[9], r[10]);

        double errR_nat = std::abs(LNative.x - trueRGB.x);
        double errG_nat = std::abs(LNative.y - trueRGB.y);
        double errB_nat = std::abs(LNative.z - trueRGB.z);
        double errT_nat = std::abs(TNative - trueT);
        double errRay_nat = std::max({errR_nat, errG_nat, errB_nat});

        errRGB_channel_native.push_back(errR_nat);
        errRGB_channel_native.push_back(errG_nat);
        errRGB_channel_native.push_back(errB_nat);
        errRGB_ray_native.push_back(errRay_nat);
        errT_native.push_back(errT_nat);

        double errR_dev = std::abs(resFused[i].rgb_T[0] - trueRGB.x);
        double errG_dev = std::abs(resFused[i].rgb_T[1] - trueRGB.y);
        double errB_dev = std::abs(resFused[i].rgb_T[2] - trueRGB.z);
        double errT_dev = std::abs(resFused[i].rgb_T[3] - trueT);
        double errRay_dev = std::max({errR_dev, errG_dev, errB_dev});

        errRGB_channel_fused.push_back(errR_dev);
        errRGB_channel_fused.push_back(errG_dev);
        errRGB_channel_fused.push_back(errB_dev);
        errRGB_ray_fused.push_back(errRay_dev);
        errT_fused.push_back(errT_dev);

        double dRGB = errRay_dev - errRay_nat;
        if (dRGB > 1e-12) { ++aStarWorseThanNat_RGB; aStarMaxDeltaNat_RGB = std::max(aStarMaxDeltaNat_RGB, dRGB); }
        double dT = errT_dev - errT_nat;
        if (dT > 1e-12) { ++aStarWorseThanNat_T; aStarMaxDeltaNat_T = std::max(aStarMaxDeltaNat_T, dT); }
    }

    double meanUnfusedMs = stats(timesUnfused)["mean"].get<double>();
    double meanFusedMs = stats(timesFused)["mean"].get<double>();
    double speedup = meanUnfusedMs / meanFusedMs;

    J report;
    report["scope"] = "Northern Veil — Lane A Bounded Consumer-Only Metal Ceiling Diagnostic";
    report["platform"] = "Apple Silicon M5 (Metal / WebGPU)";
    report["rays"] = count;
    report["workload_accounting"] = {
        {"ray_headers_bytes", rayBytes},
        {"subpanels_count", gpuSubpanels.size()},
        {"subpanels_bytes", subpanelBytes},
        {"max_subpanels_per_ray", maxSubpanelsPerRay},
        {"overlaps_count", gpuOverlaps.size()},
        {"overlaps_bytes", overlapBytes},
        {"media_bytes", mediaBytes},
        {"params_bytes", paramBytes},
        {"total_schedule_bytes", totalScheduleBytes},
        {"total_schedule_kb", totalScheduleBytes / 1024.0}
    };

    report["fusion_isolation_benchmark"] = {
        {"unfused_ms", stats(timesUnfused)},
        {"fused_ms", stats(timesFused)},
        {"mean_unfused_ms", meanUnfusedMs},
        {"mean_fused_ms", meanFusedMs},
        {"speedup_factor", speedup},
        {"speedup_pct", (speedup - 1.0) * 100.0},
        {"microkernel_us_per_ray_unfused", (meanUnfusedMs * 1000.0) / count},
        {"microkernel_us_per_ray_fused", (meanFusedMs * 1000.0) / count}
    };

    report["device_arithmetic_fidelity"] = {
        {"max_delta_RGB_between_fused_and_unfused", maxDeltaRGB_dev},
        {"max_delta_T_between_fused_and_unfused", maxDeltaT_dev},
        {"arithmetic_identical_within_f32_eps", (maxDeltaRGB_dev < 1e-5 && maxDeltaT_dev < 1e-5)}
    };

    report["cpu_vs_gpu_arithmetic_deviation"] = {
        {"max_delta_RGB", maxDeltaRGB_CPU_vs_GPU},
        {"max_delta_T", maxDeltaT_CPU_vs_GPU},
        {"within_1e-5_tolerance", (maxDeltaRGB_CPU_vs_GPU < 1e-5 && maxDeltaT_CPU_vs_GPU < 1e-5)},
        {"cpu_quadrature_ms", cpuQuadratureMs},
        {"cpu_us_per_ray", (cpuQuadratureMs * 1000.0) / count}
    };

    report["accuracy_vs_65536_step_reference"] = {
        {"fused_A_star_on_device", {
            {"RGB_pooled_channels", stats(errRGB_channel_fused)},
            {"RGB_max_per_ray", stats(errRGB_ray_fused)},
            {"T", stats(errT_fused)}
        }},
        {"native_baseline", {
            {"RGB_pooled_channels", stats(errRGB_channel_native)},
            {"RGB_max_per_ray", stats(errRGB_ray_native)},
            {"T", stats(errT_native)}
        }}
    };

    report["regressions_vs_native"] = {
        {"RGB_worse_rays", aStarWorseThanNat_RGB},
        {"RGB_worse_pct", double(aStarWorseThanNat_RGB) * 100.0 / count},
        {"RGB_max_delta", aStarMaxDeltaNat_RGB},
        {"T_worse_rays", aStarWorseThanNat_T},
        {"T_worse_pct", double(aStarWorseThanNat_T) * 100.0 / count},
        {"T_max_delta", aStarMaxDeltaNat_T}
    };

    std::cout << report.dump(2) << std::endl;

    if (!outputPath.empty()) {
        std::ofstream out(outputPath);
        out << report.dump(2) << "\n";
        std::cout << "Report saved to " << outputPath << "\n";
    }

    // Release resources
    wgpuBindGroupRelease(bgUnfused);
    wgpuBindGroupRelease(bgFused);
    wgpuComputePipelineRelease(cpUnfused);
    wgpuComputePipelineRelease(cpFused);
    for (WGPUBuffer b : {uBuf, mBuf, pBuf, rBuf, sBuf, oBuf, outBuf, readBuf}) {
        wgpuBufferRelease(b);
    }

    return 0;
}
"""

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path("."))
    parser.add_argument("--output", type=Path, default=None)
    args = parser.parse_args()

    r = args.root.resolve()
    assert (r / "saves/zones/Northern Veil/zone.ecform").exists(), "Must run in Earthcall root"

    # Verify SHA-256 fixture hashes
    rays_path = r / "docs/audits/NORTHERN_VEIL_NATIVE_CF4_PREFLIGHT_2026-10-21_RAYS.json"
    rows_path = r / "docs/audits/NORTHERN_VEIL_PROJECTED_REFERENCE_2026-10-21_ROWS.json"
    zone_path = r / "saves/zones/Northern Veil/zone.ecform"

    assert hashlib.sha256(rays_path.read_bytes()).hexdigest() == RAYS_HASH, "RAYS hash mismatch"
    assert hashlib.sha256(rows_path.read_bytes()).hexdigest() == ROWS_HASH, "ROWS hash mismatch"
    assert hashlib.sha256(zone_path.read_bytes()).hexdigest() == ZONE_HASH, "ZONE hash mismatch"
    print("Fixtures verified successfully (SHA-256 matches).")

    # Compile diagnostic probe
    probe_cpp = Path("/private/tmp/ray_transport_metal_diagnostic.cpp")
    probe_o = Path("/private/tmp/ray_transport_metal_diagnostic.o")
    probe_bin = Path("/private/tmp/ray_transport_metal_diagnostic")
    probe_cpp.write_text(CPP_SOURCE)

    b = r / "build"
    db = json.loads((b / "compile_commands.json").read_text())
    c = next(x for x in db if x["file"].endswith("/entry.cpp") and "earthcall_webgpu.dir" in x["command"])
    c_args = shlex.split(c["command"])
    c_args[c_args.index("-c") + 1] = str(probe_cpp)
    c_args[c_args.index("-o") + 1] = str(probe_o)
    if "-std=gnu++17" in c_args: c_args[c_args.index("-std=gnu++17")] = "-std=c++20"
    c_args += ["-O3"]
    subprocess.run(c_args, cwd=b, check=True)

    l_args = shlex.split((b / "CMakeFiles/earthcall_webgpu.dir/link.txt").read_text())
    l_args[next(i for i, x in enumerate(l_args) if x.endswith("/src/entry.cpp.o"))] = str(probe_o)
    l_args[l_args.index("-o") + 1] = str(probe_bin)
    subprocess.run(l_args, cwd=b, check=True)
    print("Compiled Lane A Metal diagnostic probe successfully.")

    cmd = [str(probe_bin)]
    if args.output:
        cmd.append(str(args.output.resolve()))
    subprocess.run(cmd, cwd=r, check=True)

if __name__ == "__main__":
    main()
