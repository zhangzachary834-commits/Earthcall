// webgpu_integral_operation_test — the authored Integral op (MathNode::Op::Integral)
// executed on the device, outside the volume path.
//
// Step 2 of the reconciled order in the ray-domain thread (2026-10-19): the same
// authored tree the CPU evaluates is lowered to WGSL and dispatched, and its
// values are read back. Lowering goes through the one antiderivative office
// (Zach: "build it on top of antiderivatives so we dont have two competing
// antiderivative code"): toScalarForm gives F(hi) − F(lo) as a closed form, and
// the emitter writes it like any authored ScalarForm.
//
// The program is compileScreenForm's generic authored-math emitter. Instead of
// drawing, a compute entry calls its screenColor(p) at chosen points. Nothing
// here names a medium, a ray or a camera.
//
// What this does and does not establish (Codex's witness matrix):
//  · references are INDEPENDENT of the office: analytic antiderivatives written
//    out here, or long-double composite Simpson;
//  · the f32 error is MEASURED at the sampled points, not enclosed;
//  · step 2b: the lowering is the office's CENTRED decomposition (centre m and
//    half-width h as exact forms; each moment's tabular or series form chosen at
//    run time), so intervals far from the origin are asserted like the rest;
//  · the line route (SDF(f, q) through segmentation) is not lowered; its
//    refusal is a negative control;
//  · coverage is accounted: every attempted case is admitted and executed, or
//    refused with a reason, and an expected-supported refusal fails.
// Claude Code · Claude Opus 5.5 · session f3af84cf-bfe3-429d-b417-2d533119f169 · 2026-10-19.

#include "Singularity/OntoMath/ScalarForm.hpp"
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"

#include <webgpu/wgpu.h>

#include <cmath>
#include <cstdio>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {
int g_checks = 0, g_failures = 0;
void check(bool condition, const std::string& description) {
    ++g_checks;
    if (!condition) ++g_failures;
    std::cout << (condition ? "  ok: " : "  FAILED: ") << description << "\n";
}

struct MapR { bool done = false; };
void onMap(WGPUMapAsyncStatus, WGPUStringView, void* u, void*) { static_cast<MapR*>(u)->done = true; }
struct PopResult { WGPUErrorType type = WGPUErrorType_NoError; bool done = false; };
void onPopError(WGPUPopErrorScopeStatus, WGPUErrorType type, WGPUStringView msg, void* ud1, void*) {
    auto* r = static_cast<PopResult*>(ud1);
    r->type = type;
    r->done = true;
    if (type != WGPUErrorType_NoError) std::fprintf(stderr, "[WebGPU error %d]: %.*s\n", (int)type, (int)msg.length, msg.data);
}

using OntoMath::MathNode;
using OntoMath::ScalarForm;
using Op = MathNode::Op;
using K = OntoMath::TransFactor::Kind;
using Node = std::unique_ptr<MathNode>;

Node leaf(ScalarForm f) { auto n = std::make_unique<MathNode>(); n->op = Op::ScalarLeaf; n->scalarForm = std::move(f); return n; }
Node constant(double c) { return leaf(ScalarForm::constant(c)); }
Node named(const std::string& v) { auto n = std::make_unique<MathNode>(); n->op = Op::ValueLeaf; n->variableName = v; return n; }
template <class... Kids> Node node(Op op, Kids... kids) {
    auto n = std::make_unique<MathNode>(); n->op = op; (n->children.push_back(std::move(kids)), ...); return n;
}
Node integral(const std::string& v, Node body, Node lo, Node hi) {
    auto n = node(Op::Integral, std::move(body), std::move(lo), std::move(hi)); n->variableName = v; return n;
}
ScalarForm var(const std::string& v) { return ScalarForm::variable(v); }
ScalarForm trans(K k, const std::string& v, double scale, double shift = 0.0) { return ScalarForm::transcendental(k, v, scale, shift); }

// Long-double composite Simpson, independent of the office.
double simpson(const std::function<long double(long double)>& f, double a, double b, int panels = 20000) {
    const long double h = ((long double)b - a) / panels;
    long double sum = f(a) + f(b);
    for (int i = 1; i < panels; ++i) sum += f(a + i * h) * (i % 2 ? 4.0L : 2.0L);
    return double(sum * h / 3.0L);
}

struct Case {
    std::string name;
    Node value;                                    // the scalar shown in the red channel
    std::function<double(double x, double y)> reference;
    bool expectLowered = true;
    bool reportOnly = false;                       // measured and printed, not asserted
    std::vector<std::pair<double, double>> points;
};

std::vector<std::pair<double, double>> grid(double x0, double x1, double y) {
    std::vector<std::pair<double, double>> out;
    for (int i = 0; i <= 20; ++i) out.push_back({x0 + (x1 - x0) * i / 20.0, y + 0.05 * i});
    return out;
}

// Dispatch screenColor at each point; nullopt when the device path fails.
std::optional<std::vector<float>> runOnDevice(wgpu::Device& gpu, const sdfwgsl::Program& prog,
                                              const std::vector<std::pair<double, double>>& points) {
    std::vector<float> params = prog.params;
    params.push_back(0.0f);   // never an empty storage binding
    std::vector<float> in;
    for (const auto& [x, y] : points) { in.push_back(float(x)); in.push_back(float(y)); in.push_back(0.0f); in.push_back(0.0f); }
    const uint64_t inBytes = in.size() * sizeof(float), outBytes = points.size() * sizeof(float);
    const float screenData[8] = {1280.0f, 720.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

    auto buffer = [&](WGPUBufferUsage usage, uint64_t size, const void* data) {
        WGPUBufferDescriptor d = {};
        d.usage = usage;
        d.size = size;
        WGPUBuffer b = wgpuDeviceCreateBuffer(gpu.device, &d);
        if (data) wgpuQueueWriteBuffer(gpu.queue, b, 0, data, size);
        return b;
    };
    WGPUBuffer screenBuf = buffer(WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst, sizeof(screenData), screenData);
    WGPUBuffer paramBuf = buffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, params.size() * sizeof(float), params.data());
    WGPUBuffer inBuf = buffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, inBytes, in.data());
    WGPUBuffer outBuf = buffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc, outBytes, nullptr);
    WGPUBuffer readBuf = buffer(WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead, outBytes, nullptr);

    // Touch both program bindings so the auto layout always carries them.
    const std::string code = prog.wgsl +
        "\n@group(0) @binding(2) var<storage, read> evalPoints: array<vec4<f32>>;\n"
        "@group(0) @binding(3) var<storage, read_write> evalOut: array<f32>;\n"
        "@compute @workgroup_size(1)\n"
        "fn evalMain(@builtin(global_invocation_id) gid: vec3<u32>) {\n"
        "    let i = gid.x;\n"
        "    if (i >= arrayLength(&evalOut)) { return; }\n"
        "    let q = evalPoints[i].xyz;\n"
        "    screenPoint = q;\n"
        "    evalOut[i] = screenColor(q).x + 0.0 * (screen.size.x + P.v[0]);\n"
        "}\n";
    wgpuDevicePushErrorScope(gpu.device, WGPUErrorFilter_Validation);
    WGPUShaderSourceWGSL src = {};
    src.chain.sType = WGPUSType_ShaderSourceWGSL;
    src.code = wgpu::Device::str(code.c_str());
    WGPUShaderModuleDescriptor smd = {};
    smd.nextInChain = &src.chain;
    WGPUShaderModule sm = wgpuDeviceCreateShaderModule(gpu.device, &smd);
    WGPUComputePipelineDescriptor cpd = {};
    cpd.compute.module = sm;
    cpd.compute.entryPoint = wgpu::Device::str("evalMain");
    WGPUComputePipeline cp = wgpuDeviceCreateComputePipeline(gpu.device, &cpd);
    WGPUBindGroupLayout bgl = wgpuComputePipelineGetBindGroupLayout(cp, 0);
    WGPUBindGroupEntry entries[4] = {};
    entries[0].binding = 0; entries[0].buffer = screenBuf; entries[0].size = sizeof(screenData);
    entries[1].binding = 1; entries[1].buffer = paramBuf; entries[1].size = params.size() * sizeof(float);
    entries[2].binding = 2; entries[2].buffer = inBuf; entries[2].size = inBytes;
    entries[3].binding = 3; entries[3].buffer = outBuf; entries[3].size = outBytes;
    WGPUBindGroupDescriptor bgd = {};
    bgd.layout = bgl;
    bgd.entryCount = 4;
    bgd.entries = entries;
    WGPUBindGroup bg = wgpuDeviceCreateBindGroup(gpu.device, &bgd);

    WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
    WGPUComputePassEncoder pass = wgpuCommandEncoderBeginComputePass(enc, nullptr);
    wgpuComputePassEncoderSetPipeline(pass, cp);
    wgpuComputePassEncoderSetBindGroup(pass, 0, bg, 0, nullptr);
    wgpuComputePassEncoderDispatchWorkgroups(pass, uint32_t(points.size()), 1, 1);
    wgpuComputePassEncoderEnd(pass);
    wgpuComputePassEncoderRelease(pass);
    wgpuCommandEncoderCopyBufferToBuffer(enc, outBuf, 0, readBuf, 0, outBytes);
    WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(enc, nullptr);
    wgpuQueueSubmit(gpu.queue, 1, &cmd);
    wgpuCommandBufferRelease(cmd);
    wgpuCommandEncoderRelease(enc);

    PopResult pr;
    WGPUPopErrorScopeCallbackInfo pcb = {};
    pcb.mode = WGPUCallbackMode_AllowProcessEvents;
    pcb.callback = onPopError;
    pcb.userdata1 = &pr;
    wgpuDevicePopErrorScope(gpu.device, pcb);
    while (!pr.done) wgpuDevicePoll(gpu.device, true, nullptr);

    std::optional<std::vector<float>> out;
    if (pr.type == WGPUErrorType_NoError) {
        MapR mr;
        WGPUBufferMapCallbackInfo mci = {};
        mci.mode = WGPUCallbackMode_AllowProcessEvents;
        mci.callback = onMap;
        mci.userdata1 = &mr;
        wgpuBufferMapAsync(readBuf, WGPUMapMode_Read, 0, outBytes, mci);
        while (!mr.done) wgpuDevicePoll(gpu.device, true, nullptr);
        const auto* values = static_cast<const float*>(wgpuBufferGetConstMappedRange(readBuf, 0, outBytes));
        out = std::vector<float>(values, values + points.size());
        wgpuBufferUnmap(readBuf);
    }
    wgpuBindGroupRelease(bg);
    wgpuBindGroupLayoutRelease(bgl);
    wgpuComputePipelineRelease(cp);
    wgpuShaderModuleRelease(sm);
    for (WGPUBuffer b : {screenBuf, paramBuf, inBuf, outBuf, readBuf}) wgpuBufferRelease(b);
    return out;
}
} // namespace

int main() {
    std::cout << "Starting webgpu_integral_operation_test...\n";
    wgpu::Device gpu;
    if (!gpu.init()) {
        std::cout << "UNAVAILABLE: no WebGPU device; this is an environment result, not a pass\n";
        return 77;
    }

    std::vector<Case> cases;
    cases.push_back({"∫_0^x cos 3u du", integral("u", leaf(trans(K::Cos, "u", 3.0)), constant(0.0), named("x")),
                     [](double x, double) { return std::sin(3.0 * x) / 3.0; }, true, false, grid(-2.5, 2.5, -1.0)});
    cases.push_back({"∫_0^x u·e^(−u/2)·sin 2u du (exp·sin, tabular)",
                     integral("u", leaf(var("u").times(trans(K::Exp, "u", -0.5)).times(trans(K::Sin, "u", 2.0))), constant(0.0), named("x")),
                     [](double x, double) {
                         return simpson([](long double u) { return u * std::exp(-0.5L * u) * std::sin(2.0L * u); }, 0.0, x);
                     },
                     true, false, grid(-2.5, 2.5, -1.0)});
    cases.push_back({"∫_0^x ∫_0^s (u² + 1) du ds (cumulative)",
                     integral("s", integral("u", leaf(var("u").times(var("u")).plus(ScalarForm::constant(1.0))), constant(0.0), named("s")),
                              constant(0.0), named("x")),
                     [](double x, double) { return x * x * x * x / 12.0 + x * x / 2.0; }, true, false, grid(-2.5, 2.5, -1.0)});
    cases.push_back({"∫_y^x sin 2u·cos 5u du (product-to-sum, two coordinates)",
                     integral("u", leaf(trans(K::Sin, "u", 2.0).times(trans(K::Cos, "u", 5.0))), named("y"), named("x")),
                     [](double x, double y) {
                         auto F = [](double u) { return 0.5 * (-std::cos(7.0 * u) / 7.0 + std::cos(3.0 * u) / 3.0); };
                         return F(x) - F(y);
                     },
                     true, false, grid(-2.5, 2.5, -1.0)});
    cases.push_back({"far from the origin: ∫_x^(x+½) u·e^(0.001u)·cos(0.002u + 0.1) du, x ≈ 1000",
                     integral("u", leaf(var("u").times(trans(K::Exp, "u", 0.001)).times(trans(K::Cos, "u", 0.002, 0.1))),
                              named("x"), leaf(var("x").plus(ScalarForm::constant(0.5)))),
                     [](double x, double) {
                         return simpson([](long double u) { return u * std::exp(0.001L * u) * std::cos(0.002L * u + 0.1L); }, x, x + 0.5);
                     },
                     true, false, grid(999.0, 1001.0, 0.0)});
    cases.push_back({"a fast wave far from the origin: ∫_x^(x+3) u·cos 5u du, x ≈ 1000",
                     integral("u", leaf(var("u").times(trans(K::Cos, "u", 5.0))), named("x"), leaf(var("x").plus(ScalarForm::constant(3.0)))),
                     [](double x, double) {
                         return simpson([](long double u) { return u * std::cos(5.0L * u); }, x, x + 3.0);
                     },
                     true, false, grid(999.0, 1001.0, 0.0)});
    // Negative controls: admitted on the CPU (the line route) or not at all, never lowered.
    cases.push_back({"a field along a line (SDF route): CPU only", integral("s", node(Op::SDF, leaf(var("x").times(var("y"))),
                         node(Op::VectorConstruct, named("s"), constant(1.0), constant(0.0))), constant(0.0), named("x")),
                     nullptr, false, false, {}});
    {
        // A tolerance asks for an f32 execution bound the device path cannot give yet.
        auto tolerant = node(Op::Integral, leaf(trans(K::Cos, "u", 3.0)), constant(0.0), named("x"), constant(1e-3));
        tolerant->variableName = "u";
        cases.push_back({"a tolerance-bearing integral: no device execution bound yet", std::move(tolerant), nullptr, false, false, {}});
    }
    cases.push_back({"noise: no closed form", integral("s", node(Op::Noise, node(Op::VectorConstruct, named("s"), constant(0.0), constant(0.0))),
                                                       constant(0.0), named("x")),
                     nullptr, false, false, {}});

    int attempted = 0, admitted = 0, refused = 0, executed = 0, expectedRefusedWrongly = 0;
    for (auto& c : cases) {
        ++attempted;
        auto color = OntoMath::Piecewise::continuous(node(Op::VectorConstruct, std::move(c.value), constant(0.0), constant(0.0)));
        const auto prog = sdfwgsl::compileScreenForm(color);
        if (!prog.ok) {
            ++refused;
            if (c.expectLowered) ++expectedRefusedWrongly;
            std::cout << "    " << c.name << ": refused to lower (" << prog.error << ")\n";
            check(!c.expectLowered && prog.error.find("Integral does not lower") != std::string::npos,
                  c.name + " is refused by the emitter with the Integral's reason");
            continue;
        }
        ++admitted;
        if (!c.expectLowered) { check(false, c.name + " lowered although it has no closed form"); continue; }
        const auto device = runOnDevice(gpu, prog, c.points);
        check(device.has_value(), c.name + ": the device ran the emitted program without validation errors");
        if (!device) continue;
        ++executed;
        double worstGpu = 0.0, worstCpu = 0.0, scale = 1.0;
        bool finite = true;
        for (std::size_t i = 0; i < c.points.size(); ++i) {
            const auto [x, y] = c.points[i];
            const double ref = c.reference(x, y);
            scale = std::max(scale, std::abs(ref));
            const double g = (*device)[i];
            finite = finite && std::isfinite(g);
            worstGpu = std::max(worstGpu, std::abs(g - ref));
            // The CPU evaluates the same authored tree through MathNode::evaluate.
            const auto cpu = color.evaluate({{"x", PropertyValue(x)}, {"y", PropertyValue(y)}, {"z", PropertyValue(0.0)},
                                             {"p", PropertyValue(glm::vec3(float(x), float(y), 0.0f))}});
            double cx = NAN;
            if (cpu && std::holds_alternative<glm::vec3>(*cpu)) cx = std::get<glm::vec3>(*cpu).x;
            worstCpu = std::max(worstCpu, std::isfinite(cx) ? std::abs(cx - ref) : INFINITY);
        }
        std::printf("    %s: %zu points, |values| up to %.4g; worst GPU f32 error %.3g (%.3g of scale), worst CPU %.3g\n",
                    c.name.c_str(), c.points.size(), scale, worstGpu, worstGpu / scale, worstCpu);
        // The CPU check is limited by the float vec3 colour channel it reads through.
        check(finite && worstCpu <= 1e-6 * scale, c.name + ": CPU MathNode::evaluate agrees with the independent reference");
        if (c.reportOnly) {
            check(finite, c.name + ": finite on the device (accuracy reported, not asserted: f32 F(hi) − F(lo) cancels far from the origin)");
        } else {
            // Measured, not proved: a few f32 ulps of the case's scale.
            check(finite && worstGpu <= 1e-5 * scale, c.name + ": GPU within 1e-5 of the case's scale (measured f32 error)");
        }
    }
    std::printf("    coverage: %d attempted, %d lowered, %d refused, %d executed on the device\n", attempted, admitted, refused, executed);
    check(expectedRefusedWrongly == 0 && executed == 6 && refused == 3,
          "every expected-supported case lowered and ran on the device; all three negative controls refused");

    std::cout << "webgpu_integral_operation_test: " << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
