// Native witness for the execution model every certificate skip rests on
// (Rendering::kVolumeExecutionModel, declared, not proved: 2⁻¹⁶ of magnitude per
// operation/literal/coordinate read, 2⁻¹⁰ per transcendental or noise). A killer
// certifies where sign·g − δ > 0 with δ = τ + 2·slack(g), so the claim needs the
// device's g to stay within slack(g) of the exact g. Pixel parity shows nothing
// changed on the views tried; this measures how much of slack(g) the device
// actually uses, for every certified killer of every real medium, at random box
// points and clocks inside the derived clock bucket.
//
// Method: each killer's own production program (sdfwgsl::composeVolumeSet with
// only that switch compiled) gains one compute entry point that calls its
// volumeSupportReach at given points. With the header {±1, δ = 0, 1/L = 1, offset}
// the function returns sign·g itself, multiplied by 1 and offset by 0, both
// exact in f32, so g comes back with no added arithmetic; sign −1 recovers the
// points where g ≤ −1 (the reach clamps below at −1). The reference evaluates the
// authored operands on the CPU at the same f32 points (scalars in double, vector
// operations in f32: under 2⁻⁸ of slack(g) for these magnitudes).
//
// Evidence class: one adapter, sampled points. A device margin, not an
// enclosure proof (docs/audits/VOLUME_SUPPORT_NATIVE_PROFILE_2026-10-12.md).
//
// Claude Code · Claude Opus 5.5 · session f3af84cf-bfe3-429d-b417-2d533119f169 ·
// 2026-10-18. Zach: "KEEP GOING"; harness pattern webgpu_perlin_exact_gradient_test.
#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "Singularity/Screen/VolumeDensity.hpp"
#include "Singularity/Screen/VolumeRayTransport.hpp"
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"
#include "support/test_harness.hpp"

#include <webgpu/wgpu.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <random>
#include <string>
#include <vector>

namespace {

int g_checks = 0;
int g_failures = 0;
void check(bool condition, const std::string& description) {
    ++g_checks;
    if (!condition) ++g_failures;
    std::printf("  %s: %s\n", condition ? "ok" : "FAILED", description.c_str());
}

struct MapR { bool done = false; bool ok = false; };
void onMap(WGPUMapAsyncStatus s, WGPUStringView, void* u, void*) {
    auto* r = static_cast<MapR*>(u);
    r->ok = s == WGPUMapAsyncStatus_Success;
    r->done = true;
}
struct PopResult { WGPUErrorType type = WGPUErrorType_NoError; bool done = false; std::string message; };
void onPopError(WGPUPopErrorScopeStatus, WGPUErrorType type, WGPUStringView msg, void* ud1, void*) {
    auto* r = static_cast<PopResult*>(ud1);
    r->type = type;
    r->done = true;
    if (type != WGPUErrorType_NoError) r->message.assign(msg.data, msg.length);
}

std::map<std::string, PropertyValue> varsAt(const glm::vec3& p, double t) {
    return {{"p", PropertyValue(p)}, {"x", PropertyValue(double(p.x))}, {"y", PropertyValue(double(p.y))},
            {"z", PropertyValue(double(p.z))}, {OntoMath::kTimeVar, PropertyValue(t)},
            {OntoMath::kWiXVar, PropertyValue(0.0)}, {OntoMath::kWiYVar, PropertyValue(0.0)},
            {OntoMath::kWiZVar, PropertyValue(0.0)}, {OntoMath::kWoXVar, PropertyValue(0.0)},
            {OntoMath::kWoYVar, PropertyValue(0.0)}, {OntoMath::kWoZVar, PropertyValue(1.0)},
            {OntoMath::kOmegaXVar, PropertyValue(0.0)}, {OntoMath::kOmegaYVar, PropertyValue(0.0)},
            {OntoMath::kOmegaZVar, PropertyValue(1.0)}};
}

// The 80-byte VolumeInstanceData: origin, halfExtent, time (vec4 each), 8 u32.
constexpr uint64_t kInstanceBytes = 80;

struct Gpu {
    wgpu::Device& dev;
    WGPUBuffer make(uint64_t bytes, WGPUBufferUsage usage) {
        WGPUBufferDescriptor d = {};
        d.size = std::max<uint64_t>(16, (bytes + 15) & ~uint64_t(15));
        d.usage = usage;
        return wgpuDeviceCreateBuffer(dev.device, &d);
    }
};

} // namespace

int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    std::printf("Starting webgpu_volume_support_enclosure_test...\n");
    wgpu::Device gpu;
    if (!gpu.init()) { std::printf("SKIP: no WebGPU device\n"); return 0; }
    Gpu g{gpu};

    namespace fs = std::filesystem;
    fs::path root;
    {
        std::size_t best = 0;
        for (const char* candidate : {"saves/zones", "../saves/zones"}) {
            std::error_code ec;
            if (!fs::is_directory(candidate, ec)) continue;
            std::size_t zones = 0;
            for (const auto& entry : fs::directory_iterator(candidate, ec)) zones += entry.is_directory() ? 1u : 0u;
            if (zones > best) { best = zones; root = candidate; }
        }
        if (best == 0) root = TestSupport::resolveRealWorldPath("saves/zones");
    }
    if (!fs::is_directory(root)) { std::printf("SKIP: no saves/zones\n"); return 0; }

    constexpr uint32_t kPoints = 4096;
    // Two clock buckets: the first, and one where f32 time rounding inside
    // sin(k·t) dominates (slack is derived per bucket, |t| <= bucket).
    const double buckets[] = {1.0, 1024.0};
    const double clockFractions[] = {0.0, 0.375, -0.75, 1.0};
    std::mt19937 rng(20261018u);
    std::uniform_real_distribution<float> u(-1.0f, 1.0f);

    uint32_t killers = 0, compiled = 0, overSlack = 0, overMargin = 0, refusedCpu = 0;
    double worstRatio = 0.0;
    std::string worstWho;
    std::vector<double> ratios;

    for (const auto& zoneDir : fs::directory_iterator(root)) {
        nlohmann::json zone;
        try {
            if (fs::exists(zoneDir.path() / "zone.json")) {
                std::ifstream in(zoneDir.path() / "zone.json");
                zone = nlohmann::json::parse(in);
            } else if (fs::exists(zoneDir.path() / "zone.ecform")) {
                std::ifstream in(zoneDir.path() / "zone.ecform", std::ios::binary);
                const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
                zone = nlohmann::json::parse(nlohmann::json::from_msgpack(bytes).at("MigrationRoot").get<std::string>());
            } else continue;
        } catch (...) { continue; }
        if (!zone.is_object() || !zone.contains("spatialFields")) continue;
        for (const auto& fj : zone.at("spatialFields")) {
            auto field = geom::FieldNode::fromJson(fj);
            Rendering::VolumeDensityBinding b;
            if (!field || !Rendering::readVolumeDensity(*field, 0.0, 0.0, b) || !b.densityExpr) continue;
            Rendering::VolumeRayFormCache cache;
            const auto& forms = cache.formsFor(b);
            if (!forms.density.algebraicAdmitted()) continue;
            const glm::dvec3 half = glm::abs(glm::dvec3(b.scale));
            for (const double kClock : buckets) {
            const auto certs = Rendering::deriveVolumeSupportCertificates(forms, half, kClock);
            for (const auto& c : certs.killers) {
                ++killers;
                const std::vector<uint32_t> only{static_cast<uint32_t>(c.switchIndex)};
                const auto lowered = sdfwgsl::lowerVolumeSupportKillers(b.densityExpr, &only);
                if (!lowered.ok || lowered.switches != only) continue;
                sdfwgsl::VolumeProgramInput input;
                input.densityExpr = b.densityExpr;
                const auto program = sdfwgsl::composeVolumeSet({input}, {}, false, {true}, {only});
                if (!program.ok || program.wgsl.find("fn volumeSupportReach(") == std::string::npos) continue;
                const std::string wgsl = program.wgsl + R"WGSL(
@group(2) @binding(0) var<storage, read> enclosurePoints: array<vec4<f32>>;
@group(2) @binding(1) var<storage, read_write> enclosureOut: array<f32>;
@compute @workgroup_size(64)
fn enclosureMain(@builtin(global_invocation_id) id: vec3<u32>) {
    if (id.x >= arrayLength(&enclosureOut)) { return; }
    g_instIdx = 0u;
    enclosureOut[id.x] = volumeSupportReach(enclosurePoints[id.x].xyz, 0u);
    // Never taken: keeps P and instances in the auto layout for clock-free killers.
    if (id.x == 0xffffffffu) { enclosureOut[0] = instances[0].time.x + P.v[0]; }
}
)WGSL";
                wgpuDevicePushErrorScope(gpu.device, WGPUErrorFilter_Validation);
                WGPUShaderSourceWGSL src = {};
                src.chain.sType = WGPUSType_ShaderSourceWGSL;
                src.code = wgpu::Device::str(wgsl.c_str());
                WGPUShaderModuleDescriptor smd = {};
                smd.nextInChain = &src.chain;
                WGPUShaderModule sm = wgpuDeviceCreateShaderModule(gpu.device, &smd);
                WGPUComputePipelineDescriptor cpd = {};
                cpd.compute.module = sm;
                cpd.compute.entryPoint = wgpu::Device::str("enclosureMain");
                WGPUComputePipeline cp = sm ? wgpuDeviceCreateComputePipeline(gpu.device, &cpd) : nullptr;
                PopResult pr;
                WGPUPopErrorScopeCallbackInfo pcb = {};
                pcb.mode = WGPUCallbackMode_AllowProcessEvents;
                pcb.callback = onPopError;
                pcb.userdata1 = &pr;
                wgpuDevicePopErrorScope(gpu.device, pcb);
                while (!pr.done) wgpuInstanceProcessEvents(gpu.instance);
                if (!cp || pr.type != WGPUErrorType_NoError) {
                    std::printf("    %s switch %zu: compute pipeline refused: %s\n", b.producerId.c_str(),
                                c.switchIndex, pr.message.substr(0, 160).c_str());
                    if (cp) wgpuComputePipelineRelease(cp);
                    if (sm) wgpuShaderModuleRelease(sm);
                    continue;
                }
                ++compiled;

                std::vector<glm::vec4> points(kPoints);
                for (auto& p : points) p = glm::vec4(glm::vec3(half) * glm::vec3(u(rng), u(rng), u(rng)), 0.0f);
                WGPUBuffer pointBuf = g.make(kPoints * sizeof(glm::vec4), WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
                wgpuQueueWriteBuffer(gpu.queue, pointBuf, 0, points.data(), kPoints * sizeof(glm::vec4));
                const uint64_t outBytes = kPoints * sizeof(float);
                WGPUBuffer outBuf = g.make(outBytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc);
                WGPUBuffer readBuf = g.make(outBytes, WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead);
                std::vector<float> header{1.0f, 0.0f, 1.0f, static_cast<float>(c.offset)};
                std::vector<float> params = header;
                params.insert(params.end(), lowered.params.begin(), lowered.params.end());
                const uint64_t paramBytes = params.size() * sizeof(float);
                WGPUBuffer paramBuf = g.make(paramBytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
                WGPUBuffer instBuf = g.make(kInstanceBytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);

                auto run = [&](float sign, double t, std::vector<float>& out) {
                    params[0] = sign;
                    wgpuQueueWriteBuffer(gpu.queue, paramBuf, 0, params.data(), paramBytes);
                    float inst[20] = {};
                    inst[4] = float(half.x); inst[5] = float(half.y); inst[6] = float(half.z);
                    inst[8] = static_cast<float>(t);
                    wgpuQueueWriteBuffer(gpu.queue, instBuf, 0, inst, sizeof(inst));
                    WGPUBindGroupEntry e0 = {}; e0.binding = 1; e0.buffer = paramBuf; e0.size = (paramBytes + 15) & ~uint64_t(15);
                    WGPUBindGroupEntry e1 = {}; e1.binding = 0; e1.buffer = instBuf; e1.size = kInstanceBytes;
                    WGPUBindGroupEntry e2[2] = {};
                    e2[0].binding = 0; e2[0].buffer = pointBuf; e2[0].size = kPoints * sizeof(glm::vec4);
                    e2[1].binding = 1; e2[1].buffer = outBuf; e2[1].size = outBytes;
                    WGPUBindGroupLayout l0 = wgpuComputePipelineGetBindGroupLayout(cp, 0);
                    WGPUBindGroupLayout l1 = wgpuComputePipelineGetBindGroupLayout(cp, 1);
                    WGPUBindGroupLayout l2 = wgpuComputePipelineGetBindGroupLayout(cp, 2);
                    WGPUBindGroupDescriptor d0 = {}; d0.layout = l0; d0.entryCount = 1; d0.entries = &e0;
                    WGPUBindGroupDescriptor d1 = {}; d1.layout = l1; d1.entryCount = 1; d1.entries = &e1;
                    WGPUBindGroupDescriptor d2 = {}; d2.layout = l2; d2.entryCount = 2; d2.entries = e2;
                    WGPUBindGroup b0 = wgpuDeviceCreateBindGroup(gpu.device, &d0);
                    WGPUBindGroup b1 = wgpuDeviceCreateBindGroup(gpu.device, &d1);
                    WGPUBindGroup b2 = wgpuDeviceCreateBindGroup(gpu.device, &d2);
                    WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
                    WGPUComputePassEncoder pass = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                    wgpuComputePassEncoderSetPipeline(pass, cp);
                    wgpuComputePassEncoderSetBindGroup(pass, 0, b0, 0, nullptr);
                    wgpuComputePassEncoderSetBindGroup(pass, 1, b1, 0, nullptr);
                    wgpuComputePassEncoderSetBindGroup(pass, 2, b2, 0, nullptr);
                    wgpuComputePassEncoderDispatchWorkgroups(pass, (kPoints + 63) / 64, 1, 1);
                    wgpuComputePassEncoderEnd(pass);
                    wgpuComputePassEncoderRelease(pass);
                    wgpuCommandEncoderCopyBufferToBuffer(enc, outBuf, 0, readBuf, 0, outBytes);
                    WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(enc, nullptr);
                    wgpuQueueSubmit(gpu.queue, 1, &cmd);
                    wgpuCommandBufferRelease(cmd);
                    wgpuCommandEncoderRelease(enc);
                    MapR mr;
                    WGPUBufferMapCallbackInfo mci = {};
                    mci.mode = WGPUCallbackMode_AllowProcessEvents;
                    mci.callback = onMap;
                    mci.userdata1 = &mr;
                    wgpuBufferMapAsync(readBuf, WGPUMapMode_Read, 0, outBytes, mci);
                    while (!mr.done) wgpuInstanceProcessEvents(gpu.instance);
                    const auto* v = static_cast<const float*>(wgpuBufferGetConstMappedRange(readBuf, 0, outBytes));
                    out.assign(v, v + kPoints);
                    wgpuBufferUnmap(readBuf);
                    for (auto* bg : {b0, b1, b2}) wgpuBindGroupRelease(bg);
                    for (auto* l : {l0, l1, l2}) wgpuBindGroupLayoutRelease(l);
                };

                const double slack = std::max((c.deviceSlack - Rendering::kVolumeSupportDeviceTolerance) / 2.0, 0.0);
                double killerWorst = 0.0, killerWorstErr = 0.0;
                for (const double fraction : clockFractions) {
                    const double t = fraction * kClock;
                    std::vector<float> plus, minus;
                    run(1.0f, t, plus);
                    run(-1.0f, t, minus);
                    for (uint32_t i = 0; i < kPoints; ++i) {
                        const double gDev = plus[i] > -1.0f ? double(plus[i]) : -double(minus[i]);
                        const auto vars = varsAt(glm::vec3(points[i]), t);
                        double a = 0.0, bv = 0.0;
                        const auto av = c.sourceA ? c.sourceA->evaluate(vars) : std::nullopt;
                        const auto bvv = c.sourceB ? c.sourceB->evaluate(vars) : std::nullopt;
                        if (!av || !propertyValueToNumber(*av, a) ||
                            (c.role == 1 && (!bvv || !propertyValueToNumber(*bvv, bv)))) { ++refusedCpu; continue; }
                        const double gExact = c.role == 1 ? a - bv : (c.role == 2 || c.role == 3) ? a - c.offset : a;
                        const double err = std::abs(gDev - gExact);
                        if (!std::isfinite(err)) { ++overSlack; continue; }
                        killerWorstErr = std::max(killerWorstErr, err);
                        // A claim's whole margin: sign·g must clear δ, and execution slack
                        // eats into it twice (the killer's g and the density's own switch).
                        if (err > c.deviceSlack) ++overMargin;
                        const double ratio = slack > 0.0 ? err / slack : (err > 0.0 ? 1e30 : 0.0);
                        killerWorst = std::max(killerWorst, ratio);
                    }
                }
                ratios.push_back(killerWorst);
                if (killerWorst > 1.0) ++overSlack;
                if (killerWorst > worstRatio) {
                    worstRatio = killerWorst;
                    worstWho = b.producerId + " switch " + std::to_string(c.switchIndex) + " at |t|<=" +
                               std::to_string(int(kClock));
                }
                std::printf("    |t|<=%g %s switch %zu (role %u, sign %+d%s): max |g_dev - g| %.3g, slack(g) %.3g, delta %.3g -> %.4f of slack\n",
                            kClock, b.producerId.c_str(), c.switchIndex, c.role, int(c.sign),
                            c.closedForms.empty() ? "" : ", closed form", killerWorstErr, slack, c.deviceSlack, killerWorst);
                for (auto* buf : {pointBuf, outBuf, readBuf, paramBuf, instBuf}) wgpuBufferRelease(buf);
                wgpuComputePipelineRelease(cp);
                wgpuShaderModuleRelease(sm);
            }
            }
        }
    }

    std::sort(ratios.begin(), ratios.end());
    const double median = ratios.empty() ? 0.0 : ratios[ratios.size() / 2];
    std::printf("    %u certified killer-buckets, %u measured on the device (%u points x %zu clocks each, both signs);"
                " worst %.4f of slack(g) (%s), median %.4f; %u CPU-undefined points\n",
                killers, compiled, kPoints, std::size(clockFractions), worstRatio, worstWho.c_str(), median, refusedCpu);
    check(compiled > 0, "the corpus's certified killers run on the device");
    check(compiled * 10 >= killers * 9, "at least 90% of certified killers compile into the witness");
    check(overMargin == 0, "no device evaluation of a killer's g strays beyond its certificate margin delta");
    check(overSlack == 0, "every killer's device g stays within its declared execution slack (the model holds here)");
    std::printf("webgpu_volume_support_enclosure_test: %d/%d checks passed\n", g_checks - g_failures, g_checks);
    return g_failures == 0 ? 0 : 1;
}
