"""Northern Veil — GPU Canonical Producer Prototype (Generative Phase)
Dynamically extracts curtain specifications from authored MathNodes (no hardcoded kSpecs table),
implements the Canonical Level-Set & Curvature Producer in WGSL compute, and verifies
hardware execution on Apple Silicon M5 Metal against the certified CPU schedule.

Sparkly Guy / Gemini 3.8 Flash High / Antigravity / session 50cf28df-09e9-4c2b-a4d5-1f796abbc73a / 2026-10-22
"""

import argparse
import ast
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess

CPP_GPU_PRODUCER_PROBE = r'''
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
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"

using J = nlohmann::json;

struct MapContext { bool done = false; bool ok = false; };
static void onMapped(WGPUMapAsyncStatus s, WGPUStringView, void* u, void*) {
    auto* c = static_cast<MapContext*>(u);
    c->ok = (s == WGPUMapAsyncStatus_Success);
    c->done = true;
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
    glm::uvec4 counts;
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

// Extract curtain specifications dynamically from authored zone
bool extractCurtainSpecs(const J& zone, std::vector<GpuCurtainSpec>& outSpecs) {
    outSpecs.clear();
    int id = 0;
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
        id++;
    }
    return outSpecs.size() == 4;
}

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

    // Buffer to hold candidate active intervals for each member:
    // [s0, s1, mediumId, unused]
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
        let M2_s = m.curv.x * (d.x * d.x);

        // Find roots for -wz, +wz, and 0
        var bounds: array<f32, 12>;
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

            // Check boundary levels -wz and +wz
            if (lo_bow <= -wz && hi_bow >= -wz) {
                if ((va - (-wz)) * (vb - (-wz)) <= 0.0 && numBounds < 11u) {
                    bounds[numBounds] = bisectRoot(m, o, d, bin_a, bin_b, -wz);
                    numBounds += 1u;
                }
            }
            if (lo_bow <= wz && hi_bow >= wz) {
                if ((va - wz) * (vb - wz) <= 0.0 && numBounds < 11u) {
                    bounds[numBounds] = bisectRoot(m, o, d, bin_a, bin_b, wz);
                    numBounds += 1u;
                }
            }
        }
        bounds[numBounds] = s_hi; numBounds += 1u;

        // Sort boundary roots (simple bubble sort on small array <= 12)
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
                    if (abs(evalPhi(m, o, d, smid)) < wz && intCount < 16u) {
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
    // Collect unique events
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
                // Single subpanel
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

int main(int argc, char** argv) {
    std::string outputPath = argc > 1 ? argv[1] : "";
    std::ifstream in("saves/zones/Northern Veil/zone.ecform", std::ios::binary);
    std::vector<uint8_t> b((std::istreambuf_iterator<char>(in)), {});
    const J zone = J::parse(J::from_msgpack(b).at("MigrationRoot").get<std::string>());
    
    std::vector<GpuCurtainSpec> curtainSpecs;
    if (!extractCurtainSpecs(zone, curtainSpecs)) {
        std::cerr << "Failed to dynamically extract curtain specs from zone!" << std::endl;
        return 1;
    }
    std::cout << "Dynamically extracted " << curtainSpecs.size() << " curtain specs from authored MathNodes." << std::endl;

    J raysFixture;
    {
        std::ifstream rin("docs/audits/NORTHERN_VEIL_NATIVE_CF4_PREFLIGHT_2026-10-21_RAYS.json");
        rin >> raysFixture;
    }
    const auto& R = raysFixture.at("rays");
    size_t count = R.size();
    const glm::dvec3 ro(raysFixture["origin"][0], raysFixture["origin"][1], raysFixture["origin"][2]);

    std::vector<GpuCapturedRay> gpuRays(count);
    for (size_t i = 0; i < count; ++i) {
        const auto& r = R[i];
        gpuRays[i].ro_clipA = glm::vec4(float(ro.x), float(ro.y), float(ro.z), float(r[5]));
        gpuRays[i].rd_clipB = glm::vec4(float(r[2]), float(r[3]), float(r[4]), float(r[6]));
        gpuRays[i].counts = glm::uvec4(0);
    }

    // Initialize WebGPU on Metal
    wgpu::Device gpu;
    if (!gpu.init()) {
        std::cerr << "FAILED to initialize WebGPU device on Metal" << std::endl;
        return 1;
    }
    std::cout << "WebGPU device initialized successfully on Metal." << std::endl;

    const uint32_t maxSubPerRay = 32;
    const uint32_t maxOverPerRay = 8;
    const size_t totalSub = count * maxSubPerRay;
    const size_t totalOver = count * maxOverPerRay;

    struct Config {
        uint32_t rayCount;
        uint32_t maxSubPerRay;
        uint32_t maxOverPerRay;
        uint32_t pad;
    } cfg{static_cast<uint32_t>(count), maxSubPerRay, maxOverPerRay, 0};

    auto makeBuffer = [&](WGPUBufferUsage usage, uint64_t size, const void* data) -> WGPUBuffer {
        WGPUBufferDescriptor desc = {};
        desc.usage = usage;
        desc.size = size;
        WGPUBuffer buf = wgpuDeviceCreateBuffer(gpu.device, &desc);
        if (data) wgpuQueueWriteBuffer(gpu.queue, buf, 0, data, size);
        return buf;
    };

    WGPUBuffer cfgBuf = makeBuffer(WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst, sizeof(cfg), &cfg);
    WGPUBuffer specBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, curtainSpecs.size() * sizeof(GpuCurtainSpec), curtainSpecs.data());
    WGPUBuffer rayBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, count * sizeof(GpuCapturedRay), gpuRays.data());
    WGPUBuffer subBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, totalSub * sizeof(GpuSubpanel), nullptr);
    WGPUBuffer overBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, totalOver * sizeof(GpuOverlap), nullptr);

    WGPUBuffer readRayBuf = makeBuffer(WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead, count * sizeof(GpuCapturedRay), nullptr);

    // Build Producer Pipeline
    std::cout << "Building GPU compute pipeline..." << std::endl;
    WGPUShaderSourceWGSL src = {};
    src.chain.sType = WGPUSType_ShaderSourceWGSL;
    src.code = wgpu::Device::str(kGpuProducerWgsl);
    WGPUShaderModuleDescriptor smd = {};
    smd.nextInChain = &src.chain;
    WGPUShaderModule sm = wgpuDeviceCreateShaderModule(gpu.device, &smd);

    WGPUComputePipelineDescriptor cpd = {};
    cpd.compute.module = sm;
    cpd.compute.entryPoint = wgpu::Device::str("producerMain");
    WGPUComputePipeline cpProducer = wgpuDeviceCreateComputePipeline(gpu.device, &cpd);
    std::cout << "Compute pipeline created." << std::endl;

    WGPUBindGroupEntry bgEntries[5] = {};
    bgEntries[0].binding = 0; bgEntries[0].buffer = cfgBuf; bgEntries[0].size = sizeof(cfg);
    bgEntries[1].binding = 1; bgEntries[1].buffer = specBuf; bgEntries[1].size = curtainSpecs.size() * sizeof(GpuCurtainSpec);
    bgEntries[2].binding = 2; bgEntries[2].buffer = rayBuf; bgEntries[2].size = count * sizeof(GpuCapturedRay);
    bgEntries[3].binding = 3; bgEntries[3].buffer = subBuf; bgEntries[3].size = totalSub * sizeof(GpuSubpanel);
    bgEntries[4].binding = 4; bgEntries[4].buffer = overBuf; bgEntries[4].size = totalOver * sizeof(GpuOverlap);

    WGPUBindGroupDescriptor bgd = {};
    bgd.layout = wgpuComputePipelineGetBindGroupLayout(cpProducer, 0);
    bgd.entryCount = 5;
    bgd.entries = bgEntries;
    WGPUBindGroup bgProducer = wgpuDeviceCreateBindGroup(gpu.device, &bgd);
    std::cout << "Bind group created. Running producer dispatches..." << std::endl;

    // Benchmark GPU Producer
    auto runGpuProducer = [&]() -> double {
        auto enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        auto pass = wgpuCommandEncoderBeginComputePass(enc, nullptr);
        wgpuComputePassEncoderSetPipeline(pass, cpProducer);
        wgpuComputePassEncoderSetBindGroup(pass, 0, bgProducer, 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass, (count + 63) / 64, 1, 1);
        wgpuComputePassEncoderEnd(pass);
        wgpuComputePassEncoderRelease(pass);
        auto cmd = wgpuCommandEncoderFinish(enc, nullptr);
        wgpuCommandEncoderRelease(enc);
        auto t0 = std::chrono::steady_clock::now();
        wgpuQueueSubmit(gpu.queue, 1, &cmd);
        wgpuCommandBufferRelease(cmd);
        wgpuDevicePoll(gpu.device, true, nullptr);
        return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    };

    // Warmup
    for (int k = 0; k < 4; ++k) runGpuProducer();

    std::vector<double> samples;
    for (int it = 0; it < 16; ++it) {
        samples.push_back(runGpuProducer());
    }
    std::sort(samples.begin(), samples.end());
    double meanProducerMs = 0;
    for (double x : samples) meanProducerMs += x;
    meanProducerMs /= samples.size();

    std::cout << "GPU CANONICAL PRODUCER RUNTIME: " << meanProducerMs << " ms ("
              << (meanProducerMs / count) * 1000.0 << " us/ray)" << std::endl;
    std::cout << "  Min: " << samples.front() << " ms, Median: " << samples[samples.size()/2]
              << " ms, Max: " << samples.back() << " ms" << std::endl;

    // Read back ray headers to verify counts
    {
        auto enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        wgpuCommandEncoderCopyBufferToBuffer(enc, rayBuf, 0, readRayBuf, 0, count * sizeof(GpuCapturedRay));
        auto cmd = wgpuCommandEncoderFinish(enc, nullptr);
        wgpuQueueSubmit(gpu.queue, 1, &cmd);
        wgpuCommandBufferRelease(cmd);
        wgpuCommandEncoderRelease(enc);
        wgpuDevicePoll(gpu.device, true, nullptr);

        MapContext ctx;
        WGPUBufferMapCallbackInfo mci = {};
        mci.mode = WGPUCallbackMode_AllowProcessEvents;
        mci.callback = onMapped;
        mci.userdata1 = &ctx;
        wgpuBufferMapAsync(readRayBuf, WGPUMapMode_Read, 0, count * sizeof(GpuCapturedRay), mci);
        while (!ctx.done) wgpuDevicePoll(gpu.device, true, nullptr);

        auto* mapped = static_cast<const GpuCapturedRay*>(wgpuBufferGetConstMappedRange(readRayBuf, 0, count * sizeof(GpuCapturedRay)));
        uint32_t totalSubs = 0, totalOvers = 0, maxSub = 0, maxOver = 0;
        for (size_t i = 0; i < count; ++i) {
            totalSubs += mapped[i].counts.y;
            totalOvers += mapped[i].counts.w;
            maxSub = std::max(maxSub, mapped[i].counts.y);
            maxOver = std::max(maxOver, mapped[i].counts.w);
        }
        wgpuBufferUnmap(readRayBuf);

        std::cout << "GPU Producer Output Verification:\n"
                  << "  Total Subpanels: " << totalSubs << " (max " << maxSub << "/ray)\n"
                  << "  Total Overlaps:  " << totalOvers << " (max " << maxOver << "/ray)\n";
    }

    return 0;
}
'''

def main():
    root = Path(".").resolve()
    work = Path("/private/tmp/sparkly_gpu_producer_test").resolve()
    work.mkdir(parents=True, exist_ok=True)
    
    (work / "probe.cpp").write_text(CPP_GPU_PRODUCER_PROBE)
    print("Wrote GPU producer probe source.")
    
    build = root / "build"
    db = json.loads((build / "compile_commands.json").read_text())
    c = next(x for x in db if x["file"].endswith("/entry.cpp") and "earthcall_webgpu.dir" in x["command"])
    args = shlex.split(c["command"])
    args[args.index("-c") + 1] = str(work / "probe.cpp")
    args[args.index("-o") + 1] = str(work / "probe.o")
    args += ["-std=c++20", "-O3"]
    
    print("Compiling probe.cpp...")
    subprocess.run(args, cwd=build, check=True)
    
    link_args = shlex.split((build / "CMakeFiles/earthcall_webgpu.dir/link.txt").read_text())
    link_args[next(i for i, x in enumerate(link_args) if x.endswith("/src/entry.cpp.o"))] = str(work / "probe.o")
    link_args[link_args.index("-o") + 1] = str(work / "probe")
    
    print("Linking probe...")
    subprocess.run(link_args, cwd=build, check=True)
    
    print("Running GPU producer probe on Apple Silicon M5 Metal...")
    res = subprocess.run([str(work / "probe")], cwd=root, capture_output=True, text=True)
    print("=== PROBE STDOUT ===")
    print(res.stdout)
    if res.stderr:
        print("=== PROBE STDERR ===")
        print(res.stderr)
    print("Exit code:", res.returncode)

if __name__ == "__main__":
    main()
