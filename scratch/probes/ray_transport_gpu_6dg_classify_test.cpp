#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
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

struct DispatchCounters {
    uint32_t s1Count;
    uint32_t s2Count;
    uint32_t fallbackCount;
    uint32_t pad;
};

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

        for (var i = 0u; i < rayCount; i++) {
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
            let bow = 0.125 * cur.curv.z * h_ux * h_ux;
            let thresh = bow + cur.limits.w + 0.001;

            var enter = 1e9f;
            var exit = -1e9f;
            var curActive = false;

            for (var i = 0u; i < rayCount; i++) {
                let rIdx = tileRayIndices[rayStart + i];
                let ray = rays[rIdx];
                let oRel = ray.ro_clipA.xyz - cur.origin.xyz;
                let dx = ray.rd_clipB.x;
                if (abs(dx) < 1e-6) { continue; }

                var a = ray.ro_clipA.w;
                var b = ray.rd_clipB.w;
                for (var axis = 0; axis < 3; axis++) {
                    var half = cur.half[axis];
                    if (axis == 0) { half = min(half, cur.limits.x); }
                    if (axis == 1) { half = min(half, cur.limits.y); }
                    let d = ray.rd_clipB[axis];
                    let o = oRel[axis];
                    if (d != 0.0) {
                        let q0 = (-half - cur.curv.y - o) / d;
                        let q1 = (half + cur.curv.y - o) / d;
                        a = max(a, min(q0, q1));
                        b = min(b, max(q0, q1));
                    }
                }
                if (b <= a) { continue; }

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

    std::ifstream in(zonePath, std::ios::binary);
    std::vector<uint8_t> b((std::istreambuf_iterator<char>(in)), {});
    J zone = J::parse(J::from_msgpack(b).at("MigrationRoot").get<std::string>());

    std::vector<GpuCurtainSpec> curtains;
    for (const auto& j : zone.at("spatialFields")) {
        auto field = geom::FieldNode::fromJson(j);
        Rendering::VolumeDensityBinding bnd;
        if (!field || !Rendering::readVolumeDensity(*field, 0, 0, bnd)) continue;
        if (!bnd.densityExpr) continue;
        auto pb = sdfwgsl::collectVolumeParams(bnd.densityExpr);
        if (pb.values.size() < 34) continue;
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
        std::cerr << "Failed to init WebGPU" << std::endl;
        return 1;
    }

    auto makeBuffer = [&](WGPUBufferUsage usage, uint64_t size, const void* data) -> WGPUBuffer {
        WGPUBufferDescriptor desc = {};
        desc.usage = usage;
        desc.size = size;
        WGPUBuffer b = wgpuDeviceCreateBuffer(gpu.device, &desc);
        if (data) wgpuQueueWriteBuffer(gpu.queue, b, 0, data, size);
        return b;
    };

    WGPUBuffer curBuf = makeBuffer(WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst, curtains.size() * sizeof(GpuCurtainSpec), curtains.data());
    WGPUBuffer rayBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, gpuRays.size() * sizeof(GpuRay), gpuRays.data());
    WGPUBuffer offBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, tileOffsets.size() * sizeof(uint32_t), tileOffsets.data());
    WGPUBuffer idxBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, tileRayIndices.size() * sizeof(uint32_t), tileRayIndices.data());

    DispatchCounters initCounters{0, 0, 0, 0};
    WGPUBuffer cntBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, sizeof(DispatchCounters), &initCounters);
    WGPUBuffer s1Buf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc, 2304 * sizeof(uint32_t), nullptr);
    WGPUBuffer s2Buf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc, 2304 * sizeof(uint32_t), nullptr);
    WGPUBuffer fbBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc, 2304 * sizeof(uint32_t), nullptr);
    WGPUBuffer progBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc, 2304 * sizeof(glm::uvec2), nullptr);

    WGPUBuffer readCntBuf = makeBuffer(WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead, sizeof(DispatchCounters), nullptr);

    // Build compute pipeline
    wgpuDevicePushErrorScope(gpu.device, WGPUErrorFilter_Validation);
    WGPUShaderSourceWGSL src = {};
    src.chain.sType = WGPUSType_ShaderSourceWGSL;
    src.code = wgpu::Device::str(kClassifyWgsl);
    WGPUShaderModuleDescriptor smd = {};
    smd.nextInChain = &src.chain;
    WGPUShaderModule sm = wgpuDeviceCreateShaderModule(gpu.device, &smd);

    WGPUComputePipelineDescriptor cpd = {};
    cpd.compute.module = sm;
    cpd.compute.entryPoint = wgpu::Device::str("classifyTilesMain");
    WGPUComputePipeline cp = wgpuDeviceCreateComputePipeline(gpu.device, &cpd);

    PopContext compile;
    WGPUPopErrorScopeCallbackInfo pcb = {};
    pcb.mode = WGPUCallbackMode_AllowProcessEvents;
    pcb.callback = onPopError;
    pcb.userdata1 = &compile;
    wgpuDevicePopErrorScope(gpu.device, pcb);
    while (!compile.done) wgpuDevicePoll(gpu.device, true, nullptr);
    wgpuShaderModuleRelease(sm);

    if (compile.type != WGPUErrorType_NoError || !cp) {
        std::cerr << "Classifier compilation failed!" << std::endl;
        return 1;
    }

    WGPUBindGroupLayout bglInputs = wgpuComputePipelineGetBindGroupLayout(cp, 0);
    WGPUBindGroupEntry entriesIn[4] = {};
    entriesIn[0].binding = 0; entriesIn[0].buffer = curBuf; entriesIn[0].size = curtains.size() * sizeof(GpuCurtainSpec);
    entriesIn[1].binding = 1; entriesIn[1].buffer = rayBuf; entriesIn[1].size = gpuRays.size() * sizeof(GpuRay);
    entriesIn[2].binding = 2; entriesIn[2].buffer = offBuf; entriesIn[2].size = tileOffsets.size() * sizeof(uint32_t);
    entriesIn[3].binding = 3; entriesIn[3].buffer = idxBuf; entriesIn[3].size = tileRayIndices.size() * sizeof(uint32_t);

    WGPUBindGroupDescriptor bgdIn = {};
    bgdIn.layout = bglInputs;
    bgdIn.entryCount = 4;
    bgdIn.entries = entriesIn;
    WGPUBindGroup bgInputs = wgpuDeviceCreateBindGroup(gpu.device, &bgdIn);

    WGPUBindGroupLayout bglOutputs = wgpuComputePipelineGetBindGroupLayout(cp, 1);
    WGPUBindGroupEntry entriesOut[5] = {};
    entriesOut[0].binding = 0; entriesOut[0].buffer = cntBuf; entriesOut[0].size = sizeof(DispatchCounters);
    entriesOut[1].binding = 1; entriesOut[1].buffer = s1Buf; entriesOut[1].size = 2304 * sizeof(uint32_t);
    entriesOut[2].binding = 2; entriesOut[2].buffer = s2Buf; entriesOut[2].size = 2304 * sizeof(uint32_t);
    entriesOut[3].binding = 3; entriesOut[3].buffer = fbBuf; entriesOut[3].size = 2304 * sizeof(uint32_t);
    entriesOut[4].binding = 4; entriesOut[4].buffer = progBuf; entriesOut[4].size = 2304 * sizeof(glm::uvec2);

    WGPUBindGroupDescriptor bgdOut = {};
    bgdOut.layout = bglOutputs;
    bgdOut.entryCount = 5;
    bgdOut.entries = entriesOut;
    WGPUBindGroup bgOutputs = wgpuDeviceCreateBindGroup(gpu.device, &bgdOut);

    // Run classify pass
    auto runClassify = [&]() -> double {
        wgpuQueueWriteBuffer(gpu.queue, cntBuf, 0, &initCounters, sizeof(DispatchCounters));

        WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        WGPUComputePassEncoder pass = wgpuCommandEncoderBeginComputePass(enc, nullptr);
        wgpuComputePassEncoderSetPipeline(pass, cp);
        wgpuComputePassEncoderSetBindGroup(pass, 0, bgInputs, 0, nullptr);
        wgpuComputePassEncoderSetBindGroup(pass, 1, bgOutputs, 0, nullptr);
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

    // Warmup
    runClassify();

    // Measurement
    double totalMs = 0.0;
    int reps = 10;
    for (int r = 0; r < reps; ++r) {
        totalMs += runClassify();
    }
    double avgMs = totalMs / reps;

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
    DispatchCounters res = *ptr;
    wgpuBufferUnmap(readCntBuf);

    std::cout << "=== Native Metal Pass 0: classifyTilesMain Results ===" << std::endl;
    std::cout << "Execution Time (avg of " << reps << " reps): " << std::fixed << std::setprecision(4) << avgMs << " ms" << std::endl;
    std::cout << "S1 Tile Count: " << res.s1Count << " / 2304 (" << (100.0 * res.s1Count / 2304.0) << "%)" << std::endl;
    std::cout << "S2 Tile Count: " << res.s2Count << " / 2304 (" << (100.0 * res.s2Count / 2304.0) << "%)" << std::endl;
    std::cout << "Fallback Tile Count: " << res.fallbackCount << " / 2304 (" << (100.0 * res.fallbackCount / 2304.0) << "%)" << std::endl;
    std::cout << "Total Classified: " << (res.s1Count + res.s2Count + res.fallbackCount) << " / 2304" << std::endl;

    return 0;
}
