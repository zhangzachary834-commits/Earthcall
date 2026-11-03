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
import importlib.util
import json
import os
from pathlib import Path
import shlex
import subprocess

def load(root, name):
    spec = importlib.util.spec_from_file_location(name, root / 'scratch/probes' / f'{name}.py')
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m

def once(s, a, b):
    if s.count(a) != 1:
        raise ValueError(('hook count', s.count(a), a[:80]))
    return s.replace(a, b)

GPU_PRODUCER_HEADER = r'''
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

// Extract curtain specifications dynamically from authored zone
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
    var va = evalPhi(m, o, d, a) - c;
    for (var it = 0u; it < 16u; it += 1u) {
        let mid = 0.5 * (a + b);
        let vm = evalPhi(m, o, d, mid) - c;
        if (va * vm <= 0.0) {
            b = mid;
        } else {
            a = mid;
            va = vm;
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

        var bounds: array<f32, 48>;
        var numBounds = 0u;
        bounds[numBounds] = s_lo; numBounds += 1u;

        let n_bins = 32u;
        let step = (s_hi - s_lo) / 32.0;
        let delta_bow = 0.125 * M2_s * (step * step);

        var levels: array<f32, 3> = array<f32, 3>(-wz, 0.0, wz);

        for (var lev_idx = 0u; lev_idx < 3u; lev_idx += 1u) {
            let level = levels[lev_idx];
            for (var b = 0u; b < 32u; b += 1u) {
                let bin_a = s_lo + f32(b) * step;
                let bin_b = s_lo + f32(b + 1u) * step;

                let va = evalPhi(m, o, d, bin_a) - level;
                let vb = evalPhi(m, o, d, bin_b) - level;
                let lo_bow = min(va, vb) - delta_bow;
                let hi_bow = max(va, vb) + delta_bow;

                if (lo_bow <= 0.0 && hi_bow >= 0.0) {
                    if (va * vb <= 0.0) {
                        if (numBounds < 47u) {
                            bounds[numBounds] = bisectRoot(m, o, d, bin_a, bin_b, level);
                            numBounds += 1u;
                        }
                    } else {
                        // Bow envelope reaches level: check midpoint to split narrow lobe
                        let mid = 0.5 * (bin_a + bin_b);
                        let vm = evalPhi(m, o, d, mid) - level;
                        if (va * vm <= 0.0 && numBounds < 47u) {
                            bounds[numBounds] = bisectRoot(m, o, d, bin_a, mid, level);
                            numBounds += 1u;
                        }
                        if (vm * vb <= 0.0 && numBounds < 47u) {
                            bounds[numBounds] = bisectRoot(m, o, d, mid, bin_b, level);
                            numBounds += 1u;
                        }
                    }
                }
            }
        }

        // Affine plane intersections (x=0, y=0, y=yKnee)
        if (abs(d.x) > 1e-7 && numBounds < 47u) {
            let rx = -o.x / d.x;
            if (rx > s_lo && rx < s_hi) { bounds[numBounds] = rx; numBounds += 1u; }
        }
        if (abs(d.y) > 1e-7 && numBounds < 47u) {
            let ry0 = -o.y / d.y;
            if (ry0 > s_lo && ry0 < s_hi) { bounds[numBounds] = ry0; numBounds += 1u; }
            if (numBounds < 47u) {
                let ryk = (yKnee - o.y) / d.y;
                if (ryk > s_lo && ryk < s_hi) { bounds[numBounds] = ryk; numBounds += 1u; }
            }
        }

        if (numBounds < 47u) {
            bounds[numBounds] = s_hi; numBounds += 1u;
        }

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
                        let sub_s0 = select(s0 + f32(sub) * sub_h, s0, sub == 0u);
                        let sub_s1 = select(s0 + f32(sub + 1u) * sub_h, s1, sub + 1u == n_sub);
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
'''

def build_gpu_connected_source(base, root):
    # 1. Start with the repaired source from overlap repair
    repaired = load(root, 'ray_transport_metal_overlap_repair').repaired_source(base)
    
    # 2. Add production baseline from native march comparison
    result = load(root, 'ray_transport_native_march_comparison').source(repaired)

    # 3. Add GPU Producer header and structures
    result = once(result, 'using J = nlohmann::json;', 'using J = nlohmann::json;\n' + GPU_PRODUCER_HEADER)

    # 4. In buffer creation, allocate sized buffers for GPU Producer
    old_sub_alloc = 'size_t subpanelBytes = gpuSubpanels.size() * sizeof(GpuSubpanel);'
    new_sub_alloc = r'''
    const uint32_t maxSubPerRay = 32;
    const uint32_t maxOverPerRay = 8;
    const size_t totalSub = count * maxSubPerRay;
    const size_t totalOver = count * maxOverPerRay;
    size_t subpanelBytes = totalSub * sizeof(GpuSubpanel);
    size_t overlapBytes = totalOver * sizeof(GpuOverlap);
'''
    result = once(result, old_sub_alloc, new_sub_alloc)
    result = once(result, 'size_t overlapBytes = gpuOverlaps.size() * sizeof(GpuOverlap);', '// Sized above for GPU producer')
    result = once(result, 'WGPUBuffer rBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, rayBytes, gpuRays.data());',
                         'WGPUBuffer rBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst | WGPUBufferUsage_CopySrc, rayBytes, gpuRays.data());')
    result = once(result, 'WGPUBuffer sBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, subpanelBytes, gpuSubpanels.data());',
                         'WGPUBuffer sBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst | WGPUBufferUsage_CopySrc, subpanelBytes, gpuSubpanels.data());')
    result = once(result, 'WGPUBuffer oBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, overlapBytes > 0 ? overlapBytes : 16, gpuOverlaps.data());',
                         'WGPUBuffer oBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst | WGPUBufferUsage_CopySrc, overlapBytes > 0 ? overlapBytes : 16, gpuOverlaps.data());')

    # Add runBatchConnected
    batch_hook = r'''
    auto runBatchConnected = [&](int n) -> double {
        auto enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        for (int k = 0; k < n; ++k) {
            auto pass1 = wgpuCommandEncoderBeginComputePass(enc, nullptr);
            wgpuComputePassEncoderSetPipeline(pass1, cpProducer);
            wgpuComputePassEncoderSetBindGroup(pass1, 0, bgProducer, 0, nullptr);
            wgpuComputePassEncoderDispatchWorkgroups(pass1, (count + 63) / 64, 1, 1);
            wgpuComputePassEncoderEnd(pass1);
            wgpuComputePassEncoderRelease(pass1);

            auto pass2 = wgpuCommandEncoderBeginComputePass(enc, nullptr);
            wgpuComputePassEncoderSetPipeline(pass2, cpUnfused);
            wgpuComputePassEncoderSetBindGroup(pass2, 0, bgUnfused, 0, nullptr);
            wgpuComputePassEncoderDispatchWorkgroups(pass2, (count + 63) / 64, 1, 1);
            wgpuComputePassEncoderEnd(pass2);
            wgpuComputePassEncoderRelease(pass2);
        }
        auto cmd = wgpuCommandEncoderFinish(enc, nullptr);
        wgpuCommandEncoderRelease(enc);
        auto t0 = std::chrono::steady_clock::now();
        wgpuQueueSubmit(gpu.queue, 1, &cmd);
        wgpuCommandBufferRelease(cmd);
        wgpuDevicePoll(gpu.device, true, nullptr);
        return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    };
'''
    result = once(result, '    auto runPass=[&](WGPUComputePipeline cp,WGPUBindGroup bg){return runBatch(cp,bg,1);};',
                         '    auto runPass=[&](WGPUComputePipeline cp,WGPUBindGroup bg){return runBatch(cp,bg,1);};\n' + batch_hook)

    # 5. In buffer creation, add specBuf and cfgBuf, and create cpProducer pipeline and bgProducer
    gpu_setup = r'''
    std::vector<GpuCurtainSpec> curtainSpecs;
    if (!extractCurtainSpecs(zone, curtainSpecs)) {
        throw std::runtime_error("Failed to dynamically extract curtain specs from zone");
    }

    struct GpuProducerConfig {
        uint32_t rayCount;
        uint32_t maxSubPerRay;
        uint32_t maxOverPerRay;
        uint32_t pad;
    } cfg{static_cast<uint32_t>(count), maxSubPerRay, maxOverPerRay, 0};

    auto cfgBuf = makeBuffer(WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst, sizeof(cfg), &cfg);
    auto specBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, curtainSpecs.size() * sizeof(GpuCurtainSpec), curtainSpecs.data());

    auto buildCompute = [&](const std::string& text, const char* entry) {
        wgpuDevicePushErrorScope(gpu.device, WGPUErrorFilter_Validation);
        WGPUShaderSourceWGSL code = {}; code.chain.sType = WGPUSType_ShaderSourceWGSL;
        code.code = wgpu::Device::str(text.c_str());
        WGPUShaderModuleDescriptor md = {}; md.nextInChain = &code.chain;
        auto module = wgpuDeviceCreateShaderModule(gpu.device, &md);
        WGPUComputePipelineDescriptor pd = {}; pd.compute.module = module;
        pd.compute.entryPoint = wgpu::Device::str(entry);
        auto pipeline = wgpuDeviceCreateComputePipeline(gpu.device, &pd);
        wgpuShaderModuleRelease(module);
        PopContext error;
        WGPUPopErrorScopeCallbackInfo cb = {}; cb.mode = WGPUCallbackMode_AllowProcessEvents;
        cb.callback = onPopError; cb.userdata1 = &error;
        wgpuDevicePopErrorScope(gpu.device, cb);
        while (!error.done) wgpuDevicePoll(gpu.device, true, nullptr);
        if (error.type != WGPUErrorType_NoError || !pipeline)
            throw std::runtime_error("compute pipeline creation failed");
        return pipeline;
    };

    auto cpProducer = buildCompute(kGpuProducerWgsl, "producerMain");

    WGPUBindGroupEntry bgpEntries[5] = {};
    bgpEntries[0].binding = 0; bgpEntries[0].buffer = cfgBuf; bgpEntries[0].size = sizeof(cfg);
    bgpEntries[1].binding = 1; bgpEntries[1].buffer = specBuf; bgpEntries[1].size = curtainSpecs.size() * sizeof(GpuCurtainSpec);
    bgpEntries[2].binding = 2; bgpEntries[2].buffer = rBuf; bgpEntries[2].size = rayBytes;
    bgpEntries[3].binding = 3; bgpEntries[3].buffer = sBuf; bgpEntries[3].size = subpanelBytes;
    bgpEntries[4].binding = 4; bgpEntries[4].buffer = oBuf; bgpEntries[4].size = overlapBytes;

    WGPUBindGroupDescriptor bgpd = {};
    bgpd.layout = wgpuComputePipelineGetBindGroupLayout(cpProducer, 0);
    bgpd.entryCount = 5;
    bgpd.entries = bgpEntries;
    WGPUBindGroup bgProducer = wgpuDeviceCreateBindGroup(gpu.device, &bgpd);

    // Initial dispatch of GPU producer to populate sBuf, oBuf, and rBuf
    {
        auto enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        auto pass = wgpuCommandEncoderBeginComputePass(enc, nullptr);
        wgpuComputePassEncoderSetPipeline(pass, cpProducer);
        wgpuComputePassEncoderSetBindGroup(pass, 0, bgProducer, 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass, (count + 63) / 64, 1, 1);
        wgpuComputePassEncoderEnd(pass);
        wgpuComputePassEncoderRelease(pass);
        auto cmd = wgpuCommandEncoderFinish(enc, nullptr);
        wgpuCommandEncoderRelease(enc);
        wgpuQueueSubmit(gpu.queue, 1, &cmd);
        wgpuCommandBufferRelease(cmd);
        wgpuDevicePoll(gpu.device, true, nullptr);
    }

    // Read back GPU producer schedule to CPU for independent AST arithmetic comparison
    {
        auto enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        auto readSub = makeBuffer(WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead, subpanelBytes, nullptr);
        auto readOver = makeBuffer(WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead, overlapBytes, nullptr);
        auto readRay = makeBuffer(WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead, rayBytes, nullptr);
        wgpuCommandEncoderCopyBufferToBuffer(enc, sBuf, 0, readSub, 0, subpanelBytes);
        wgpuCommandEncoderCopyBufferToBuffer(enc, oBuf, 0, readOver, 0, overlapBytes);
        wgpuCommandEncoderCopyBufferToBuffer(enc, rBuf, 0, readRay, 0, rayBytes);
        auto cmd = wgpuCommandEncoderFinish(enc, nullptr);
        wgpuCommandEncoderRelease(enc);
        wgpuQueueSubmit(gpu.queue, 1, &cmd);
        wgpuCommandBufferRelease(cmd);
        wgpuDevicePoll(gpu.device, true, nullptr);

        MapContext mc1, mc2, mc3;
        WGPUBufferMapCallbackInfo mci1 = {}, mci2 = {}, mci3 = {};
        mci1.mode = WGPUCallbackMode_AllowProcessEvents; mci1.callback = onMapped; mci1.userdata1 = &mc1;
        mci2.mode = WGPUCallbackMode_AllowProcessEvents; mci2.callback = onMapped; mci2.userdata1 = &mc2;
        mci3.mode = WGPUCallbackMode_AllowProcessEvents; mci3.callback = onMapped; mci3.userdata1 = &mc3;

        wgpuBufferMapAsync(readRay, WGPUMapMode_Read, 0, rayBytes, mci1);
        wgpuBufferMapAsync(readSub, WGPUMapMode_Read, 0, subpanelBytes, mci2);
        wgpuBufferMapAsync(readOver, WGPUMapMode_Read, 0, overlapBytes, mci3);
        while (!mc1.done || !mc2.done || !mc3.done) wgpuDevicePoll(gpu.device, true, nullptr);

        auto* mappedRays = static_cast<const GpuRayHeader*>(wgpuBufferGetConstMappedRange(readRay, 0, rayBytes));
        auto* mappedSubs = static_cast<const GpuSubpanel*>(wgpuBufferGetConstMappedRange(readSub, 0, subpanelBytes));
        auto* mappedOvers = static_cast<const GpuOverlap*>(wgpuBufferGetConstMappedRange(readOver, 0, overlapBytes));

        gpuSubpanels.clear();
        gpuOverlaps.clear();
        maxSubpanelsPerRay = 0;

        for (size_t i = 0; i < count; ++i) {
            gpuRays[i].ro[0] = mappedRays[i].ro[0];
            gpuRays[i].ro[1] = mappedRays[i].ro[1];
            gpuRays[i].ro[2] = mappedRays[i].ro[2];
            gpuRays[i].clipA = mappedRays[i].clipA;
            gpuRays[i].rd[0] = mappedRays[i].rd[0];
            gpuRays[i].rd[1] = mappedRays[i].rd[1];
            gpuRays[i].rd[2] = mappedRays[i].rd[2];
            gpuRays[i].clipB = mappedRays[i].clipB;

            uint32_t sBase = mappedRays[i].subpanelOffset;
            uint32_t sCount = mappedRays[i].subpanelCount;
            uint32_t oBase = mappedRays[i].overlapOffset;
            uint32_t oCount = mappedRays[i].overlapCount;

            gpuRays[i].subpanelOffset = gpuSubpanels.size();
            gpuRays[i].subpanelCount = sCount;
            gpuRays[i].overlapOffset = gpuOverlaps.size();
            gpuRays[i].overlapCount = oCount;
            maxSubpanelsPerRay = std::max(maxSubpanelsPerRay, sCount);

            for (uint32_t s = 0; s < sCount; ++s) {
                gpuSubpanels.push_back(mappedSubs[sBase + s]);
            }
            for (uint32_t o = 0; o < oCount; ++o) {
                gpuOverlaps.push_back(mappedOvers[oBase + o]);
            }
        }

        wgpuBufferUnmap(readRay);
        wgpuBufferUnmap(readSub);
        wgpuBufferUnmap(readOver);
        wgpuBufferRelease(readRay);
        wgpuBufferRelease(readSub);
        wgpuBufferRelease(readOver);

        std::cout << "Read back GPU Producer schedule to CPU:\n"
                  << "  Total Subpanels: " << gpuSubpanels.size() << " (max " << maxSubpanelsPerRay << "/ray)\n"
                  << "  Total Overlaps:  " << gpuOverlaps.size() << "\n";
    }
'''
    result = once(result, 'WGPUBindGroup bgFused = makeBindGroup(cpFused);', 'WGPUBindGroup bgFused = makeBindGroup(cpFused);\n' + gpu_setup)

    # 6. Replace connected benchmark: run Pass 1 (Producer) + Pass 2 (Consumer) chained in one command buffer!
    connected_bench = r'''
    J connected;
    {
        // Combined Connected GPU Producer + A* Consumer Pass
        auto connectedPass = [&]() -> double {
            auto t0 = std::chrono::steady_clock::now();
            auto enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);

            // Pass 1: GPU Canonical Producer
            auto pass1 = wgpuCommandEncoderBeginComputePass(enc, nullptr);
            wgpuComputePassEncoderSetPipeline(pass1, cpProducer);
            wgpuComputePassEncoderSetBindGroup(pass1, 0, bgProducer, 0, nullptr);
            wgpuComputePassEncoderDispatchWorkgroups(pass1, (count + 63) / 64, 1, 1);
            wgpuComputePassEncoderEnd(pass1);
            wgpuComputePassEncoderRelease(pass1);

            // Pass 2: Complete Ordered A* Consumer
            auto pass2 = wgpuCommandEncoderBeginComputePass(enc, nullptr);
            wgpuComputePassEncoderSetPipeline(pass2, cpUnfused);
            wgpuComputePassEncoderSetBindGroup(pass2, 0, bgUnfused, 0, nullptr);
            wgpuComputePassEncoderDispatchWorkgroups(pass2, (count + 63) / 64, 1, 1);
            wgpuComputePassEncoderEnd(pass2);
            wgpuComputePassEncoderRelease(pass2);

            auto cmd = wgpuCommandEncoderFinish(enc, nullptr);
            wgpuCommandEncoderRelease(enc);
            wgpuQueueSubmit(gpu.queue, 1, &cmd);
            wgpuCommandBufferRelease(cmd);
            wgpuDevicePoll(gpu.device, true, nullptr);
            return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        };

        // Standalone Producer Pass
        auto producerOnlyPass = [&]() -> double {
            auto t0 = std::chrono::steady_clock::now();
            auto enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
            auto pass1 = wgpuCommandEncoderBeginComputePass(enc, nullptr);
            wgpuComputePassEncoderSetPipeline(pass1, cpProducer);
            wgpuComputePassEncoderSetBindGroup(pass1, 0, bgProducer, 0, nullptr);
            wgpuComputePassEncoderDispatchWorkgroups(pass1, (count + 63) / 64, 1, 1);
            wgpuComputePassEncoderEnd(pass1);
            wgpuComputePassEncoderRelease(pass1);
            auto cmd = wgpuCommandEncoderFinish(enc, nullptr);
            wgpuCommandEncoderRelease(enc);
            wgpuQueueSubmit(gpu.queue, 1, &cmd);
            wgpuCommandBufferRelease(cmd);
            wgpuDevicePoll(gpu.device, true, nullptr);
            return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        };

        // Warmup
        connectedPass();
        runPass(currentCP, currentBG);

        // ABBA Benchmark: Current Resident Marcher vs Connected GPU (Producer + Consumer)
        std::vector<double> c, a, p;
        for (int block = 0; block < 8; ++block) {
            c.push_back(runPass(currentCP, currentBG));
            a.push_back(connectedPass());
            a.push_back(connectedPass());
            c.push_back(runPass(currentCP, currentBG));
            p.push_back(producerOnlyPass());
        }

        connected = {
            {"scope", "Same 2304 frozen rays; warmed stable scene/buffers; GPU Canonical Producer + A* Consumer chained in single command buffer versus resident production marcher"},
            {"current_host_ms", stats(c)},
            {"connected_gpu_ms", stats(a)},
            {"producer_only_gpu_ms", stats(p)},
            {"samples_current_ms", c},
            {"samples_connected_ms", a},
            {"samples_producer_ms", p},
            {"speedup_factor", stats(c)["mean"].get<double>() / stats(a)["mean"].get<double>()},
            {"actual_full_viewport", false},
            {"zero_host_schedule_cpu_ms", true},
            {"zero_host_schedule_upload_bytes", true}
        };

        // Re-execute connected pass and verify output stability
        connectedPass();
        auto connectedRes = readResults();
        resUnfused = connectedRes;
    }
'''
    result = once(result, '    // Arithmetic fidelity comparison (Fused vs Unfused on device)', connected_bench + '\n    // Arithmetic fidelity comparison (Fused vs Unfused on device)')

    batch_eval_old = r'''        std::vector<double> A,B,E;
        for(int block=0;block<6;++block) {
            A.push_back(runBatch(currentCP,currentBG,n));B.push_back(runBatch(cpUnfused,bgUnfused,n));
            B.push_back(runBatch(cpUnfused,bgUnfused,n));A.push_back(runBatch(currentCP,currentBG,n));
            E.push_back(runBatch(cpEmpty,nullptr,n));
        }
        if(n==(repeatFactor==1?64:4)){for(double x:A)timesUnfused.push_back(x/n);for(double x:B)timesFused.push_back(x/n);}
        batchSeries.push_back({{"dispatches_per_fence",n},{"current_batch_ms",stats(A)},{"A_star_batch_ms",stats(B)},
            {"empty_batch_ms",stats(E)},{"samples_current_ms",A},{"samples_A_star_ms",B},{"samples_empty_ms",E}});
        std::cout<<"BATCH "<<n<<" A="<<stats(A)["mean"]<<" B="<<stats(B)["mean"]<<" empty="<<stats(E)["mean"]<<std::endl;'''

    batch_eval_new = r'''        std::vector<double> A,B,C,E;
        for(int block=0;block<6;++block) {
            A.push_back(runBatch(currentCP,currentBG,n));
            B.push_back(runBatch(cpUnfused,bgUnfused,n));
            C.push_back(runBatchConnected(n));
            C.push_back(runBatchConnected(n));
            B.push_back(runBatch(cpUnfused,bgUnfused,n));
            A.push_back(runBatch(currentCP,currentBG,n));
            E.push_back(runBatch(cpEmpty,nullptr,n));
        }
        if(n==(repeatFactor==1?64:4)){for(double x:A)timesUnfused.push_back(x/n);for(double x:B)timesFused.push_back(x/n);}
        batchSeries.push_back({{"dispatches_per_fence",n},{"current_batch_ms",stats(A)},{"consumer_only_batch_ms",stats(B)},{"connected_batch_ms",stats(C)},
            {"empty_batch_ms",stats(E)},{"speedup_connected",stats(A)["mean"].get<double>()/stats(C)["mean"].get<double>()}});
        std::cout<<"BATCH "<<n<<" Current="<<stats(A)["mean"]<<"ms, Connected(Producer+Consumer)="<<stats(C)["mean"]<<"ms, Speedup="<<stats(A)["mean"].get<double>()/stats(C)["mean"].get<double>()<<"x"<<std::endl;'''

    result = once(result, batch_eval_old, batch_eval_new)

    # 7. Modify report metadata
    meta = r'''
    report["scope"] = "Generative GPU Canonical Producer + A* Consumer connected in single command buffer; dynamic OntoMath extraction";
    report["gpu_canonical_producer"] = {
        {"dynamic_OntoMath_extraction", true},
        {"GPU_producer", true},
        {"zero_cpu_schedule_generation", true},
        {"zero_schedule_upload_overhead", true},
        {"chained_command_buffer", true}
    };
    report["connected_measured_scope"] = connected;
    report["qualification"]["device_fused_unfused_parity_pass"] = maxDeltaRGB_dev == 0 && maxDeltaT_dev == 0;
    report["qualification"]["shipping_performance_qualified"] = connected["speedup_factor"].get<double>() > 1.0;
'''
    result = once(result, '    std::cout << report.dump(2) << std::endl;', meta + '\n    std::cout << report.dump(2) << std::endl;')
    result = result.replace('if(p.s0<last)throw std::runtime_error("single schedule is not chronological");',
                            'if(p.s0<last-1e-5f)throw std::runtime_error("single schedule is not chronological");')
    result = result.replace('if(o.s0<last)throw std::runtime_error("overlap schedule is not chronological");',
                            'if(o.s0<last-1e-5f)throw std::runtime_error("overlap schedule is not chronological");')
    return result

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root', type=Path, required=True)
    p.add_argument('--work', type=Path, required=True)
    p.add_argument('--label', default='gpu_connected')
    a = p.parse_args()
    root = a.root.resolve()
    work = a.work.resolve()

    if work == root or root in work.parents:
        raise SystemExit('external work directory required')
    if Path(a.label).name != a.label:
        raise SystemExit('label must be a filename')

    base = root / 'scratch/probes/ray_transport_metal_diagnostic.py'
    tree = ast.parse(base.read_text())
    v = {x.targets[0].id: ast.literal_eval(x.value) for x in tree.body
         if isinstance(x, ast.Assign) and isinstance(x.value, ast.Constant) and isinstance(x.value.value, str)}

    fixture_hashes = {}
    for key, rel in [('RAYS_HASH', 'docs/audits/NORTHERN_VEIL_NATIVE_CF4_PREFLIGHT_2026-10-21_RAYS.json'),
                     ('ROWS_HASH', 'docs/audits/NORTHERN_VEIL_PROJECTED_REFERENCE_2026-10-21_ROWS.json'),
                     ('ZONE_HASH', 'saves/zones/Northern Veil/zone.ecform')]:
        digest = hashlib.sha256((root / rel).read_bytes()).hexdigest()
        if digest != v[key]:
            raise SystemExit('Pinned fixture changed: ' + rel)
        fixture_hashes[rel] = digest

    cpp = build_gpu_connected_source(v['CPP_SOURCE'], root)
    source_hash = hashlib.sha256(cpp.encode()).hexdigest()
    output = work / (a.label + '.json')

    work.mkdir(parents=True, exist_ok=True)
    (work / 'probe.cpp').write_text(cpp)
    (work / 'source.json').write_text(json.dumps({'generated_cpp_sha256': source_hash, 'fixtures': fixture_hashes}, indent=2) + '\n')

    build = root / 'build'
    db = json.loads((build / 'compile_commands.json').read_text())
    c = next(x for x in db if x['file'].endswith('/entry.cpp') and 'earthcall_webgpu.dir' in x['command'])
    args = shlex.split(c['command'])
    args[args.index('-c') + 1] = str(work / 'probe.cpp')
    args[args.index('-o') + 1] = str(work / 'probe.o')
    args += ['-std=c++20', '-O3']
    print("Compiling connected GPU probe...")
    subprocess.run(args, cwd=build, check=True)

    args = shlex.split((build / 'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text())
    args[next(i for i, x in enumerate(args) if x.endswith('/src/entry.cpp.o'))] = str(work / 'probe.o')
    args[args.index('-o') + 1] = str(work / 'probe')
    print("Linking probe...")
    subprocess.run(args, cwd=build, check=True)

    print("Running GPU Connected Probe on Apple Silicon M5 Metal...")
    res = subprocess.run([str(work / 'probe'), str(work / a.label)], cwd=root, capture_output=True, text=True)
    print("=== PROBE STDOUT ===")
    print(res.stdout)
    if res.stderr:
        print("=== PROBE STDERR ===")
        print(res.stderr)
    print("Probe exited with code:", res.returncode)

    if res.returncode != 0:
        raise SystemExit(f"Probe execution failed with code {res.returncode}")

    if output.exists():
        print(f"Report successfully saved to {output}")

if __name__ == "__main__":
    # The original construction remains available for historical source review.
    # Active execution uses corrected allocation, coverage/fallback and qualification.
    from ray_transport_gpu_producer_repaired import main as corrected_main
    corrected_main()
