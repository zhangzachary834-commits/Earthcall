"""Northern Veil — GPU vs CPU Canonical Producer Equivalence Probe
Directly compares subpanel intervals, overlap intervals, and ray counts produced
by the GPU Producer WGSL shader against the certified CPU canonical reference.

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
from ray_transport_canonical_root_contract import CPP_ROOT_CONTRACT, CPP_FIXTURE

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
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"

using J = nlohmann::json;

ROOT_CONTRACT_PLACEHOLDER

FIXTURE_PLACEHOLDER

struct MapContext { bool done = false; bool ok = false; };
static void onMapped(WGPUMapAsyncStatus s, WGPUStringView, void* u, void*) {
    auto* c = static_cast<MapContext*>(u);
    c->ok = (s == WGPUMapAsyncStatus_Success);
    c->done = true;
}

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

// Full WGSL producer implementing recursive/iterative level set isolation with affine kinks
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

fn evalPhiDeriv(m: CurtainSpec, o: vec3<f32>, d: vec3<f32>, s: f32) -> f32 {
    let px = o.x + s * d.x;
    let dw_dx = m.amps.x * m.freqs.x * cos(m.freqs.x * px + m.phases.x)
              - m.amps.y * m.freqs.y * sin(m.freqs.y * px + m.phases.y)
              + m.amps.z * m.freqs.z * cos(m.freqs.z * px + m.phases.z);
    return d.z + d.x * dw_dx;
}

fn bisectRoot(m: CurtainSpec, o: vec3<f32>, d: vec3<f32>, a_in: f32, b_in: f32, c: f32) -> f32 {
    var a = a_in;
    var b = b_in;
    for (var it = 0u; it < 24u; it += 1u) {
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

    // Buffer to hold candidate active pieces across all curtains: [s0, s1, mediumId, unused]
    var pieces: array<vec4<f32>, 32>;
    var pieceCount = 0u;

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
            if (dk == 0.0) {
                if (abs(ok) > hk) { hit = false; break; }
            } else {
                let x = (-hk - ok) / dk;
                let y = (hk - ok) / dk;
                ta = max(ta, min(x, y));
                tb = min(tb, max(x, y));
            }
        }
        if (!hit || !(tb > ta)) { continue; }

        // 2. Affine support clip (Wx, Wy)
        let Wx = m.limits.x;
        let Wy = m.limits.y;
        if (d.x != 0.0) {
            let x0 = (-Wx - o.x) / d.x;
            let x1 = (Wx - o.x) / d.x;
            ta = max(ta, min(x0, x1));
            tb = min(tb, max(x0, x1));
        } else {
            if (abs(o.x) > Wx) { continue; }
        }

        if (d.y != 0.0) {
            let y0 = (-Wy - o.y) / d.y;
            let y1 = (Wy - o.y) / d.y;
            ta = max(ta, min(y0, y1));
            tb = min(tb, max(y0, y1));
        } else {
            if (abs(o.y) > Wy) { continue; }
        }

        if (!(tb > ta)) { continue; }

        // 3. Level-set isolation and affine kinks
        let wz = m.limits.w;
        let M2_s = m.curv.x * (d.x * d.x);

        // Events list for this curtain
        var evs: array<f32, 32>;
        var numEvs = 0u;
        evs[numEvs] = ta; numEvs += 1u;

        // Add affine kinks
        if (d.x != 0.0) {
            let r = -o.x / d.x;
            if (r > ta && r < tb && numEvs < 31u) { evs[numEvs] = r; numEvs += 1u; }
        }
        if (d.y != 0.0) {
            let r0 = -o.y / d.y;
            if (r0 > ta && r0 < tb && numEvs < 31u) { evs[numEvs] = r0; numEvs += 1u; }
            let rKnee = (m.limits.z - o.y) / d.y;
            if (rKnee > ta && rKnee < tb && numEvs < 31u) { evs[numEvs] = rKnee; numEvs += 1u; }
        }

        // Subdivide into 8 coarse bins to find wave roots for -wz, 0, +wz
        let n_bins = 8u;
        let step = (tb - ta) / 8.0;
        let delta_bow = 0.125 * M2_s * (step * step);

        for (var b = 0u; b < 8u; b += 1u) {
            let bin_a = ta + f32(b) * step;
            let bin_b = ta + f32(b + 1u) * step;

            let va = evalPhi(m, o, d, bin_a);
            let vb = evalPhi(m, o, d, bin_b);
            let lo_bow = min(va, vb) - delta_bow;
            let hi_bow = max(va, vb) + delta_bow;

            // Check level -wz
            if (lo_bow <= -wz && hi_bow >= -wz && numEvs < 30u) {
                if ((va - (-wz)) * (vb - (-wz)) <= 0.0) {
                    evs[numEvs] = bisectRoot(m, o, d, bin_a, bin_b, -wz); numEvs += 1u;
                } else {
                    // Possible extrema turning point inside bin
                    let mid = 0.5 * (bin_a + bin_b);
                    evs[numEvs] = mid; numEvs += 1u;
                }
            }
            // Check level +wz
            if (lo_bow <= wz && hi_bow >= wz && numEvs < 30u) {
                if ((va - wz) * (vb - wz) <= 0.0) {
                    evs[numEvs] = bisectRoot(m, o, d, bin_a, bin_b, wz); numEvs += 1u;
                } else {
                    let mid = 0.5 * (bin_a + bin_b);
                    evs[numEvs] = mid; numEvs += 1u;
                }
            }
            // Check level 0
            if (lo_bow <= 0.0 && hi_bow >= 0.0 && numEvs < 30u) {
                if (va * vb <= 0.0) {
                    evs[numEvs] = bisectRoot(m, o, d, bin_a, bin_b, 0.0); numEvs += 1u;
                }
            }
        }
        evs[numEvs] = tb; numEvs += 1u;

        // Sort events for this curtain
        if (numEvs > 1u) {
            for (var i = 0u; i < numEvs - 1u; i += 1u) {
                for (var j = 0u; j < numEvs - 1u - i; j += 1u) {
                    if (evs[j] > evs[j + 1u]) {
                        let tmp = evs[j];
                        evs[j] = evs[j + 1u];
                        evs[j + 1u] = tmp;
                    }
                }
            }
        }

        // Deduplicate and filter active pieces where abs(phi) <= wz
        for (var i = 0u; i < numEvs - 1u; i += 1u) {
            let sa = evs[i];
            let sb = evs[i + 1u];
            if (sb - sa > 1e-4) {
                let smid = 0.5 * (sa + sb);
                if (abs(evalPhi(m, o, d, smid)) <= wz + 1e-3 && pieceCount < 32u) {
                    pieces[pieceCount] = vec4<f32>(sa, sb, f32(m_idx), 0.0);
                    pieceCount += 1u;
                }
            }
        }
    }

    if (pieceCount == 0u) {
        var r_out = r_in;
        r_out.counts = vec4<u32>(subBase, 0u, overBase, 0u);
        rays[rayIdx] = r_out;
        return;
    }

    // Global event sweep across all pieces
    var allEvents: array<f32, 64>;
    var numAll = 0u;
    for (var i = 0u; i < pieceCount; i += 1u) {
        if (numAll < 63u) { allEvents[numAll] = pieces[i].x; numAll += 1u; }
        if (numAll < 63u) { allEvents[numAll] = pieces[i].y; numAll += 1u; }
    }

    // Sort all events
    if (numAll > 1u) {
        for (var i = 0u; i < numAll - 1u; i += 1u) {
            for (var j = 0u; j < numAll - 1u - i; j += 1u) {
                if (allEvents[j] > allEvents[j + 1u]) {
                    let tmp = allEvents[j];
                    allEvents[j] = allEvents[j + 1u];
                    allEvents[j + 1u] = tmp;
                }
            }
        }
    }

    // Deduplicate
    var dedup: array<f32, 64>;
    var numDedup = 0u;
    for (var i = 0u; i < numAll; i += 1u) {
        if (numDedup == 0u || allEvents[i] > dedup[numDedup - 1u] + 1e-4) {
            dedup[numDedup] = allEvents[i];
            numDedup += 1u;
        }
    }

    // Partition into subpanels and overlaps
    if (numDedup > 1u) {
        for (var i = 0u; i < numDedup - 1u; i += 1u) {
            let s0 = dedup[i];
            let s1 = dedup[i + 1u];
            let h = s1 - s0;
            if (h <= 1e-4) { continue; }
            let smid = 0.5 * (s0 + s1);

            var mask = 0u;
            var singleMedium = 0u;
            var activeCount = 0u;

            for (var j = 0u; j < pieceCount; j += 1u) {
                if (smid >= pieces[j].x && smid <= pieces[j].y) {
                    let mid = u32(pieces[j].z);
                    if ((mask & (1u << mid)) == 0u) {
                        mask = mask | (1u << mid);
                        singleMedium = mid;
                        activeCount += 1u;
                    }
                }
            }

            if (activeCount == 0u) { continue; }

            if (activeCount == 1u) {
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
                if (overCount < cfg.maxOverPerRay) {
                    overlaps[overBase + overCount] = Overlap(vec2<f32>(s0, s1), vec2<u32>(mask, 0u));
                    overCount += 1u;
                }
            }
        }
    }

    var r_out = r_in;
    r_out.counts = vec4<u32>(subBase, subCount, overBase, overCount);
    rays[rayIdx] = r_out;
}
)WGSL";

int main(int argc, char** argv) {
    std::ifstream in("saves/zones/Northern Veil/zone.ecform", std::ios::binary);
    std::vector<uint8_t> b((std::istreambuf_iterator<char>(in)), {});
    const J zone = J::parse(J::from_msgpack(b).at("MigrationRoot").get<std::string>());
    
    std::vector<GpuCurtainSpec> curtainSpecs;
    if (!extractCurtainSpecs(zone, curtainSpecs)) {
        std::cerr << "Failed to extract curtain specs!" << std::endl;
        return 1;
    }

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

    // 1. Compute CPU Canonical Reference Schedule
    std::vector<GpuSubpanel> cpuSubpanels;
    std::vector<GpuOverlap> cpuOverlaps;
    std::vector<GpuCapturedRay> cpuRays = gpuRays;
    uint32_t cpuFallbackRays = 0;

    for (size_t i = 0; i < count; ++i) {
        const auto& r = R[i];
        const std::array<double, 3> origin{ro.x, ro.y, ro.z}, d{r[2], r[3], r[4]};
        auto prep = canonical_fixture::prepare(origin, d, r[5], r[6]);
        if (prep.fallback) {
            cpuFallbackRays++;
            continue;
        }

        std::vector<double> events;
        for (const auto& p : prep.pieces) {
            events.push_back(p.lo);
            events.push_back(p.hi);
        }
        std::sort(events.begin(), events.end());
        events.erase(std::unique(events.begin(), events.end()), events.end());

        uint32_t sStart = cpuSubpanels.size();
        uint32_t oStart = cpuOverlaps.size();

        for (size_t e = 1; e < events.size(); ++e) {
            double a = events[e - 1], b = events[e];
            if (!(b > a)) continue;
            double mid = 0.5 * (a + b);
            uint32_t mask = 0;
            for (const auto& p : prep.pieces) {
                if (p.lo <= mid && mid <= p.hi) mask |= (1u << p.medium);
            }
            if (!mask) continue;
            if (__builtin_popcount(mask) == 1) {
                unsigned id = __builtin_ctz(mask);
                unsigned n = std::max(1, int(std::ceil((b - a) / 4.0)));
                for (unsigned k = 0; k < n; ++k) {
                    float lo = float(a + (b - a) * double(k) / n);
                    float hi = float(a + (b - a) * double(k + 1) / n);
                    cpuSubpanels.push_back(GpuSubpanel{lo, hi, id, 0});
                }
            } else {
                cpuOverlaps.push_back(GpuOverlap{float(a), float(b), mask, 0});
            }
        }

        cpuRays[i].counts = glm::uvec4(sStart, cpuSubpanels.size() - sStart, oStart, cpuOverlaps.size() - oStart);
    }

    std::cout << "CPU Canonical Schedule: " << cpuSubpanels.size() << " subpanels, "
              << cpuOverlaps.size() << " overlaps (fallback rays: " << cpuFallbackRays << ")\n";

    // 2. Initialize WebGPU on Metal
    wgpu::Device gpu;
    if (!gpu.init()) {
        std::cerr << "FAILED to init WebGPU" << std::endl;
        return 1;
    }

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
    WGPUBuffer readSubBuf = makeBuffer(WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead, totalSub * sizeof(GpuSubpanel), nullptr);
    WGPUBuffer readOverBuf = makeBuffer(WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead, totalOver * sizeof(GpuOverlap), nullptr);

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

    // Run GPU producer
    {
        auto enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        auto pass = wgpuCommandEncoderBeginComputePass(enc, nullptr);
        wgpuComputePassEncoderSetPipeline(pass, cpProducer);
        wgpuComputePassEncoderSetBindGroup(pass, 0, bgProducer, 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass, (count + 63) / 64, 1, 1);
        wgpuComputePassEncoderEnd(pass);
        wgpuComputePassEncoderRelease(pass);
        wgpuCommandEncoderCopyBufferToBuffer(enc, rayBuf, 0, readRayBuf, 0, count * sizeof(GpuCapturedRay));
        wgpuCommandEncoderCopyBufferToBuffer(enc, subBuf, 0, readSubBuf, 0, totalSub * sizeof(GpuSubpanel));
        wgpuCommandEncoderCopyBufferToBuffer(enc, overBuf, 0, readOverBuf, 0, totalOver * sizeof(GpuOverlap));
        auto cmd = wgpuCommandEncoderFinish(enc, nullptr);
        wgpuCommandEncoderRelease(enc);
        wgpuQueueSubmit(gpu.queue, 1, &cmd);
        wgpuCommandBufferRelease(cmd);
        wgpuDevicePoll(gpu.device, true, nullptr);
    }

    // Map and read back
    MapContext ctxRays;
    WGPUBufferMapCallbackInfo mciRays = {};
    mciRays.mode = WGPUCallbackMode_AllowProcessEvents;
    mciRays.callback = onMapped;
    mciRays.userdata1 = &ctxRays;
    wgpuBufferMapAsync(readRayBuf, WGPUMapMode_Read, 0, count * sizeof(GpuCapturedRay), mciRays);
    while (!ctxRays.done) wgpuDevicePoll(gpu.device, true, nullptr);
    const auto* mappedRays = static_cast<const GpuCapturedRay*>(wgpuBufferGetConstMappedRange(readRayBuf, 0, count * sizeof(GpuCapturedRay)));

    uint32_t totalGpuSubs = 0, totalGpuOvers = 0;
    for (size_t i = 0; i < count; ++i) {
        totalGpuSubs += mappedRays[i].counts.y;
        totalGpuOvers += mappedRays[i].counts.w;
    }

    std::cout << "GPU Producer Output: " << totalGpuSubs << " subpanels, " << totalGpuOvers << " overlaps\n";

    // Compare ray by ray
    size_t countMatch = 0, countMismatch = 0;
    for (size_t i = 0; i < count; ++i) {
        uint32_t cSub = cpuRays[i].counts.y;
        uint32_t gSub = mappedRays[i].counts.y;
        uint32_t cOver = cpuRays[i].counts.w;
        uint32_t gOver = mappedRays[i].counts.w;
        if (cSub == gSub && cOver == gOver) {
            countMatch++;
        } else {
            countMismatch++;
            if (countMismatch <= 5) {
                std::cout << "  Ray " << i << " mismatch: CPU sub=" << cSub << " over=" << cOver
                          << " vs GPU sub=" << gSub << " over=" << gOver << "\n";
            }
        }
    }
    std::cout << "Ray Schedule Comparison: " << countMatch << " exact match, " << countMismatch << " mismatch out of " << count << " rays.\n";

    wgpuBufferUnmap(readRayBuf);
    return 0;
}
'''

def main():
    root = Path(".").resolve()
    work = Path("/private/tmp/sparkly_gpu_producer_comparison").resolve()
    work.mkdir(parents=True, exist_ok=True)

    src = CPP_SOURCE.replace("ROOT_CONTRACT_PLACEHOLDER", CPP_ROOT_CONTRACT)
    src = src.replace("FIXTURE_PLACEHOLDER", CPP_FIXTURE)
    (work / "probe.cpp").write_text(src)

    build = root / "build"
    db = json.loads((build / "compile_commands.json").read_text())
    c = next(x for x in db if x["file"].endswith("/entry.cpp") and "earthcall_webgpu.dir" in x["command"])
    args = shlex.split(c["command"])
    args[args.index("-c") + 1] = str(work / "probe.cpp")
    args[args.index("-o") + 1] = str(work / "probe.o")
    args += ["-std=c++20", "-O3"]

    subprocess.run(args, cwd=build, check=True)

    link_args = shlex.split((build / "CMakeFiles/earthcall_webgpu.dir/link.txt").read_text())
    link_args[next(i for i, x in enumerate(link_args) if x.endswith("/src/entry.cpp.o"))] = str(work / "probe.o")
    link_args[link_args.index("-o") + 1] = str(work / "probe")

    subprocess.run(link_args, cwd=build, check=True)

    res = subprocess.run([str(work / "probe")], cwd=root, capture_output=True, text=True)
    print("=== PROBE STDOUT ===")
    print(res.stdout)
    if res.stderr:
        print("=== PROBE STDERR ===")
        print(res.stderr)
    print("Exit code:", res.returncode)

if __name__ == "__main__":
    main()
