// webgpu_line_quadrature_test — step 2c, first slice (plan §6ap): an authored
// scalar field integrated along lines ON THE DEVICE, certified by the host.
//
// The host proves, per magnitude band [−2^j, 2^j]³, the field's Lipschitz
// constant, range and one f32 device evaluation's error under the device's own
// declared model (kVolumeExecutionModel) — OntoMath::certifyLineQuadrature. The
// device (sdfwgsl::compileLineQuadrature) picks the band and cell count per ray,
// runs Simpson in f32, and reports (value, status, cells, proved total). For
// every ray this test asserts against an independent long-double reference:
//   · the device ran the program without validation errors;
//   · its cell count matches the CPU mirror (planLineQuadrature) to rounding;
//   · |device − reference| ≤ the device-reported total, every admitted ray;
//   · a ray beyond every band, and a tolerance below the fixed error, refuse.
// The proof is relative to the declared device model (Metal fast-math's full
// enclosure is open, docs/audits/VOLUME_SUPPORT_NATIVE_PROFILE_2026-10-12.md),
// so this witnesses the model on this device; it does not certify the device.
// Claude Code · Claude Opus 5.5 · session f3af84cf-bfe3-429d-b417-2d533119f169 · 2026-10-20.

#include "Singularity/OntoMath/LineRestriction.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"
#include "Singularity/Screen/VolumeRayTransport.hpp"
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"

#include <webgpu/wgpu.h>

#include <cmath>
#include <cstdio>
#include <iostream>
#include <memory>
#include <random>
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

struct Ray { double o[3], d[3], lo, hi, tol; };

// One lineIntegral per ray; nullopt when the device path fails.
std::optional<std::vector<float>> runOnDevice(wgpu::Device& gpu, const sdfwgsl::Program& prog, const std::vector<Ray>& rays) {
    std::vector<float> params = prog.params;
    params.push_back(0.0f);
    std::vector<float> in;
    for (const auto& r : rays) {
        for (int i = 0; i < 3; ++i) in.push_back(float(r.o[i]));
        in.push_back(float(r.lo));
        for (int i = 0; i < 3; ++i) in.push_back(float(r.d[i]));
        in.push_back(float(r.hi));
        in.push_back(float(r.tol)); in.push_back(0.0f); in.push_back(0.0f); in.push_back(0.0f);
    }
    const uint64_t inBytes = in.size() * sizeof(float), outBytes = rays.size() * 4 * sizeof(float);
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
    const std::string code = prog.wgsl +
        "\n@group(0) @binding(2) var<storage, read> rays: array<vec4<f32>>;\n"
        "@group(0) @binding(3) var<storage, read_write> results: array<vec4<f32>>;\n"
        "@compute @workgroup_size(1)\n"
        "fn rayMain(@builtin(global_invocation_id) gid: vec3<u32>) {\n"
        "    let i = gid.x;\n"
        "    if (i >= arrayLength(&results)) { return; }\n"
        "    let a = rays[3u * i];\n"
        "    let b = rays[3u * i + 1u];\n"
        "    let c = rays[3u * i + 2u];\n"
        "    screenPoint = a.xyz;\n"
        "    results[i] = lineIntegral(a.xyz, b.xyz, a.w, b.w, c.x) + vec4<f32>(0.0 * (screen.size.x + P.v[0]));\n"
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
    cpd.compute.entryPoint = wgpu::Device::str("rayMain");
    WGPUComputePipeline cp = wgpuDeviceCreateComputePipeline(gpu.device, &cpd);
    PopResult compile;
    {
        WGPUPopErrorScopeCallbackInfo pcb = {};
        pcb.mode = WGPUCallbackMode_AllowProcessEvents;
        pcb.callback = onPopError;
        pcb.userdata1 = &compile;
        wgpuDevicePopErrorScope(gpu.device, pcb);
        while (!compile.done) wgpuDevicePoll(gpu.device, true, nullptr);
    }
    std::optional<std::vector<float>> out;
    if (compile.type == WGPUErrorType_NoError) {
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
        wgpuComputePassEncoderDispatchWorkgroups(pass, uint32_t(rays.size()), 1, 1);
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
        while (!mr.done) wgpuDevicePoll(gpu.device, true, nullptr);
        const auto* values = static_cast<const float*>(wgpuBufferGetConstMappedRange(readBuf, 0, outBytes));
        out = std::vector<float>(values, values + rays.size() * 4);
        wgpuBufferUnmap(readBuf);
        wgpuBindGroupRelease(bg);
        wgpuBindGroupLayoutRelease(bgl);
    }
    wgpuComputePipelineRelease(cp);
    wgpuShaderModuleRelease(sm);
    for (WGPUBuffer b : {screenBuf, paramBuf, inBuf, outBuf, readBuf}) wgpuBufferRelease(b);
    return out;
}

// The same Simpson the device runs, with the plan's cell count, in double: its
// distance from the reference isolates the MATHEMATICAL radius from the device
// model's allowance (which dominates the device totals and so cannot test it).
double simpsonCells(const MathNode& field, const Ray& r, int cells) {
    auto f = [&](double s) {
        const glm::vec3 p(float(r.o[0] + s * r.d[0]), float(r.o[1] + s * r.d[1]), float(r.o[2] + s * r.d[2]));
        const auto v = field.evaluate({{"p", PropertyValue(p)}, {"x", PropertyValue(double(p.x))},
                                       {"y", PropertyValue(double(p.y))}, {"z", PropertyValue(double(p.z))}});
        double d = NAN;
        if (v) propertyValueToNumber(*v, d);
        return d;
    };
    const double h = (r.hi - r.lo) / cells;
    double sum = 0.0;
    for (int j = 0; j < cells; j += 2)
        sum += h / 3.0 * (f(r.lo + j * h) + 4.0 * f(r.lo + (j + 1) * h) + f(r.lo + (j + 2) * h));
    return sum;
}

// Independent reference: long-double Simpson of MathNode::evaluate along the ray.
double reference(const MathNode& field, const Ray& r, int panels = 20000) {
    auto f = [&](long double s) {
        const glm::vec3 p(float(r.o[0] + s * r.d[0]), float(r.o[1] + s * r.d[1]), float(r.o[2] + s * r.d[2]));
        const auto v = field.evaluate({{"p", PropertyValue(p)}, {"x", PropertyValue(double(p.x))},
                                       {"y", PropertyValue(double(p.y))}, {"z", PropertyValue(double(p.z))}});
        double d = NAN;
        if (v) propertyValueToNumber(*v, d);
        return (long double)d;
    };
    const long double a = r.lo, b = r.hi, h = (b - a) / panels;
    long double sum = f(a) + f(b);
    for (int i = 1; i < panels; ++i) sum += f(a + i * h) * (i % 2 ? 4.0L : 2.0L);
    return double(sum * h / 3.0L);
}
} // namespace

int main() {
    std::cout << "Starting webgpu_line_quadrature_test...\n";
    wgpu::Device gpu;
    if (!gpu.init()) {
        std::cout << "UNAVAILABLE: no WebGPU device; this is an environment result, not a pass\n";
        return 77;
    }
    struct Field { std::string name; Node node; double tolerance; };
    std::vector<Field> fields;
    // A Northern-Veil-like density: noise + 4 (nonnegative; |noise| ≤ 3.81).
    fields.push_back({"noise(p) + 4", node(Op::Add, node(Op::Noise, named("p")), constant(4.0)), 5e-2});
    // A smooth analytic field with polynomial growth (bands matter).
    fields.push_back({"sin(2x)·cos(y) + z²",
                      leaf(ScalarForm::transcendental(K::Sin, "x", 2.0, 0.0).times(ScalarForm::transcendental(K::Cos, "y", 1.0, 0.0))
                               .plus(ScalarForm::variable("z", 2.0))),
                      5e-2});
    const double maxCells = 4096.0;
    for (auto& F : fields) {
        std::cout << "  -- " << F.name << " --\n";
        std::string why;
        const auto cert = OntoMath::certifyLineQuadrature(*F.node, Rendering::kVolumeExecutionModel, 20, &why);
        check(bool(cert), F.name + ": the host certifies it over magnitude bands" + (cert ? "" : " — " + why));
        if (!cert) continue;
        std::printf("    %zu bands; band 2 (|p| ≤ %g): L %.4g, width %.4g, device evaluation error %.3g\n", cert->bands.size(),
                    cert->bands.size() > 2 ? cert->bands[2].cap : 0.0, cert->bands.size() > 2 ? cert->bands[2].lipschitz : 0.0,
                    cert->bands.size() > 2 ? cert->bands[2].width : 0.0, cert->bands.size() > 2 ? cert->bands[2].execution : 0.0);
        const auto prog = sdfwgsl::compileLineQuadrature(*F.node, *cert, uint32_t(maxCells));
        check(prog.ok, F.name + ": the device program compiles" + (prog.ok ? "" : " — " + prog.error));
        if (!prog.ok) continue;
        std::mt19937_64 rng(2026);
        std::uniform_real_distribution<double> uo(-2.5, 2.5), ud(-1.0, 1.0), ul(0.5, 2.5);
        std::vector<Ray> rays;
        for (int i = 0; i < 48; ++i) {
            Ray r{};
            double n2 = 0.0;
            for (int k = 0; k < 3; ++k) { r.o[k] = uo(rng); r.d[k] = ud(rng); n2 += r.d[k] * r.d[k]; }
            for (int k = 0; k < 3; ++k) r.d[k] /= std::sqrt(n2);
            r.lo = 0.0; r.hi = ul(rng); r.tol = F.tolerance;
            rays.push_back(r);
        }
        Ray beyond{{3.0e6, 0.0, 0.0}, {1.0, 0.0, 0.0}, 0.0, 1.0, F.tolerance};   // past every band (2^20)
        Ray tight = rays[0];
        tight.tol = 1e-9;                                                       // below the fixed error
        rays.push_back(beyond);
        rays.push_back(tight);
        const auto out = runOnDevice(gpu, prog, rays);
        check(out.has_value(), F.name + ": the device ran the program without validation errors");
        if (!out) continue;
        int admitted = 0, inside = 0, cellsAgree = 0, radiusHolds = 0;
        double worstRatio = 0.0, worstError = 0.0, meanCells = 0.0, worstRadiusRatio = 0.0;
        for (std::size_t i = 0; i + 2 < rays.size(); ++i) {
            const float value = (*out)[4 * i], status = (*out)[4 * i + 1], cells = (*out)[4 * i + 2], total = (*out)[4 * i + 3];
            const auto plan = OntoMath::planLineQuadrature(*cert, rays[i].o, rays[i].d, rays[i].lo, rays[i].hi, rays[i].tol, maxCells);
            if (status != 1.0f || !plan) continue;
            ++admitted;
            meanCells += cells;
            cellsAgree += std::abs(double(cells) - plan->cells) <= 2.0;
            const double ref = reference(*F.node, rays[i]);
            const double quadratureError = std::abs(simpsonCells(*F.node, rays[i], int(plan->cells)) - ref);
            worstRadiusRatio = std::max(worstRadiusRatio, quadratureError / plan->mathematical);
            radiusHolds += quadratureError <= plan->mathematical;
            const double err = std::abs(double(value) - ref);
            worstError = std::max(worstError, err);
            worstRatio = std::max(worstRatio, err / double(total));
            inside += err <= double(total) && double(total) <= rays[i].tol * 1.001;
        }
        const std::size_t in = rays.size() - 2;
        std::printf("    %d/%zu admitted, mean %.0f cells; worst |device − reference| %.3g, worst error/total %.4f; "
                    "quadrature error / mathematical radius at most %.4f\n",
                    admitted, in, admitted ? meanCells / admitted : 0.0, worstError, worstRatio, worstRadiusRatio);
        check(radiusHolds == admitted, F.name + ": with the plan's cell count, Simpson's own error (double) lies inside the mathematical radius alone");
        check(admitted == int(in), F.name + ": every in-domain ray is admitted at ±" + std::to_string(F.tolerance));
        check(cellsAgree == admitted, F.name + ": the device's cell count matches the CPU mirror (to rounding) on every ray");
        check(inside == admitted, F.name + ": every admitted ray's true error lies inside its device-reported total, which fits the tolerance");
        check((*out)[4 * in + 1] == 0.0f, F.name + ": a ray beyond every magnitude band refuses");
        check((*out)[4 * (in + 1) + 1] == 0.0f, F.name + ": a tolerance below the fixed error refuses");
    }
    std::cout << "webgpu_line_quadrature_test: " << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
