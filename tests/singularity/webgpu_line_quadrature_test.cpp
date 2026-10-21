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

#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "Singularity/Screen/VolumeDensity.hpp"
#include "Singularity/OntoMath/LineRestriction.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"
#include "Singularity/Screen/VolumeRayTransport.hpp"
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"

#include <webgpu/wgpu.h>

#include "json.hpp"
#include <filesystem>
#include <fstream>
#include <cmath>
#include <chrono>
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

struct Ray { double o[3], d[3], lo, hi, tol; double clock = 0.0; double omega[3] = {0.0, 0.0, 1.0}; double tTolerance = 0.0; };

// One lineIntegral per ray; nullopt when the device path fails.
std::optional<std::vector<float>> runOnDevice(wgpu::Device& gpu, const sdfwgsl::Program& prog, const std::vector<Ray>& rays,
                                              const std::string& function = "lineIntegral") {
    std::vector<float> params = prog.params;
    params.push_back(0.0f);
    std::vector<float> in;
    for (const auto& r : rays) {
        for (int i = 0; i < 3; ++i) in.push_back(float(r.o[i]));
        in.push_back(float(r.lo));
        for (int i = 0; i < 3; ++i) in.push_back(float(r.d[i]));
        in.push_back(float(r.hi));
        in.push_back(float(r.tol)); in.push_back(float(r.tTolerance>0?r.tTolerance:r.tol)); in.push_back(0.0f); in.push_back(0.0f);
        for (int i = 0; i < 3; ++i) in.push_back(float(r.omega[i]));
        in.push_back(float(r.clock));
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
    std::string invocation;
    if(function=="scheduledL" || function=="scheduledT" || function=="scheduledWork") {
        invocation="    let r=lineScheduledTransportAt(a.xyz,b.xyz,a.w,b.w,c.x,c.y,context.w,context.xyz);\n";
        if(function=="scheduledL") invocation+="    results[i]=vec4<f32>(r.radiance,r.status,r.panels,r.radianceError);\n";
        else if(function=="scheduledT") invocation+="    results[i]=vec4<f32>(r.transmittance,r.status,r.panels,r.transmittanceError);\n";
        else invocation+="    results[i]=vec4<f32>(r.samples,r.panels,r.status);\n";
    } else if(function=="reducedRGB" || function=="reducedErrors" || function=="reducedWork") {
        invocation="    let r=lineReducedIntegralAt(a.xyz,b.xyz,a.w,b.w,c.x,c.y,context.w,context.xyz);\n";
        if(function=="reducedRGB")invocation+="    results[i]=vec4<f32>(r.value,r.transmittance);\n";
        else if(function=="reducedErrors")invocation+="    results[i]=vec4<f32>(r.error,r.transmittanceError);\n";
        else invocation+="    results[i]=vec4<f32>(r.samples,r.panels,r.status);\n";
    } else invocation="    results[i] = " + function + "(a.xyz, b.xyz, a.w, b.w, c.x" +
        (function == "lineTransportAt" ? ", context.w, context.xyz" : "") + ");\n";
    const std::string code = prog.wgsl +
        "\n@group(0) @binding(2) var<storage, read> rays: array<vec4<f32>>;\n"
        "@group(0) @binding(3) var<storage, read_write> results: array<vec4<f32>>;\n"
        "@compute @workgroup_size(1)\n"
        "fn rayMain(@builtin(global_invocation_id) gid: vec3<u32>) {\n"
        "    let i = gid.x;\n"
        "    if (i >= arrayLength(&results)) { return; }\n"
        "    let a = rays[4u * i];\n"
        "    let b = rays[4u * i + 1u];\n"
        "    let c = rays[4u * i + 2u];\n"
        "    let context = rays[4u * i + 3u];\n"
        "    screenPoint = a.xyz;\n"
        + invocation + "    results[i] = results[i] + vec4<f32>(0.0 * (screen.size.x + P.v[0]));\n"
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
    // ---- Transport on the device (plan §6aq): L = ∫ exp(−∫σ)·ε ds, one march ----
    {
        std::cout << "  -- transport: L = ∫ exp(−∫ σ) ε ds, σ = 0.25·noise(p) + 1, ε = 0.25·noise(p + 5) + 1 --\n";
        auto scaled = [](Node inner) { return node(Op::Add, node(Op::Scale, constant(0.25), std::move(inner)), constant(1.0)); };
        auto sigma = scaled(node(Op::Noise, named("p")));
        auto emission = scaled(node(Op::Noise, node(Op::Add, named("p"), node(Op::VectorConstruct, constant(5.0), constant(5.0), constant(5.0)))));
        std::string why;
        const auto sc = OntoMath::certifyLineQuadrature(*sigma, Rendering::kVolumeExecutionModel, 20, &why);
        const auto ec = sc ? OntoMath::certifyLineQuadrature(*emission, Rendering::kVolumeExecutionModel, 20, &why) : std::nullopt;
        check(sc && ec && sc->bands[2].low >= 0.0, "σ and ε are certified, σ proved nonnegative (band low " +
                                                   std::to_string(sc ? sc->bands[2].low : NAN) + ")" + (ec ? "" : " — " + why));
        if (sc && ec) {
            const auto prog = sdfwgsl::compileLineTransport(*sigma, *emission, *sc, *ec, 4096);
            check(prog.ok, "the transport program compiles" + (prog.ok ? "" : " — " + prog.error));
            std::mt19937_64 rng(77);
            std::uniform_real_distribution<double> uo(-2.5, 2.5), ud(-1.0, 1.0), ul(0.5, 2.5);
            std::vector<Ray> rays;
            for (int i = 0; i < 48; ++i) {
                Ray r{};
                double n2 = 0.0;
                for (int k = 0; k < 3; ++k) { r.o[k] = uo(rng); r.d[k] = ud(rng); n2 += r.d[k] * r.d[k]; }
                for (int k = 0; k < 3; ++k) r.d[k] /= std::sqrt(n2);
                // ±0.1: at ±0.05 one 2.3-unit ray refuses on the DEVICE MODEL's
                // execution floor (2^-10 per noise and per exp, compounding
                // through the depth: 0.073 at 8192 cells), not on its radius.
                r.lo = 0.0; r.hi = ul(rng); r.tol = 1e-1;
                rays.push_back(r);
            }
            Ray backwards = rays[0];
            std::swap(backwards.lo, backwards.hi);   // hi < lo: transport is not antisymmetric
            rays.push_back(backwards);
            const auto out = prog.ok ? runOnDevice(gpu, prog, rays, "lineTransport") : std::nullopt;
            check(out.has_value(), "the device ran the transport program without validation errors");
            auto at = [&](const MathNode& f, const Ray& r, long double s) {
                const glm::vec3 p(float(r.o[0] + s * r.d[0]), float(r.o[1] + s * r.d[1]), float(r.o[2] + s * r.d[2]));
                const auto v = f.evaluate({{"p", PropertyValue(p)}});
                double dd = NAN; if (v) propertyValueToNumber(*v, dd); return (long double)dd;
            };
            // Independent reference: cumulative long-double Simpson (fine), then L.
            auto referenceL = [&](const Ray& r) {
                const int M = 4000;
                const long double h = ((long double)r.hi - r.lo) / M;
                std::vector<long double> tau(M + 1, 0.0L);
                for (int i = 2; i <= M; i += 2)
                    tau[i] = tau[i - 2] + h / 3.0L * (at(*sigma, r, r.lo + (i - 2) * h) + 4.0L * at(*sigma, r, r.lo + (i - 1) * h) + at(*sigma, r, r.lo + i * h));
                for (int i = 1; i < M; i += 2)
                    tau[i] = tau[i - 1] + h * (5.0L * at(*sigma, r, r.lo + (i - 1) * h) + 8.0L * at(*sigma, r, r.lo + i * h) - at(*sigma, r, r.lo + (i + 1) * h)) / 12.0L;
                long double sum = 0.0L;
                for (int i = 0; i <= M; ++i) sum += (i == 0 || i == M ? 1.0L : (i % 2 ? 4.0L : 2.0L)) * std::exp(-tau[i]) * at(*emission, r, r.lo + i * h);
                return double(sum * h / 3.0L);
            };
            // The device's march, in double, with the plan's cells: its distance from the
            // reference isolates the MATHEMATICAL part (quadrature of depth and of L).
            auto marchL = [&](const Ray& r, int n) {
                const double h = (r.hi - r.lo) / n;
                double tauA = 0.0, sigA = double(at(*sigma, r, r.lo)), gA = double(at(*emission, r, r.lo)), L = 0.0;
                for (int j = 0; j < n; j += 2) {
                    const double sm = r.lo + (j + 1) * h, sb = r.lo + (j + 2) * h;
                    const double sigM = double(at(*sigma, r, sm)), sigB = double(at(*sigma, r, sb));
                    const double tauM = tauA + h * (5.0 * sigA + 8.0 * sigM - sigB) / 12.0, tauB = tauA + h / 3.0 * (sigA + 4.0 * sigM + sigB);
                    const double gM = std::exp(-tauM) * double(at(*emission, r, sm)), gB = std::exp(-tauB) * double(at(*emission, r, sb));
                    L += h / 3.0 * (gA + 4.0 * gM + gB);
                    tauA = tauB; sigA = sigB; gA = gB;
                }
                return L;
            };
            if (out) {
                int admitted = 0, inside = 0, agree = 0, mathInside = 0;
                double worst = 0.0, worstRatio = 0.0, meanCells = 0.0;
                for (std::size_t i = 0; i + 1 < rays.size(); ++i) {
                    const float L = (*out)[4 * i], status = (*out)[4 * i + 1], cells = (*out)[4 * i + 2], total = (*out)[4 * i + 3];
                    std::string planWhy;
                    const auto plan = OntoMath::planLineTransport(*sc, *ec, rays[i].o, rays[i].d, rays[i].lo, rays[i].hi, rays[i].tol, 4096, &planWhy);
                    if (status != 1.0f || !plan) {
                        continue;
                    }
                    ++admitted;
                    meanCells += cells;
                    agree += double(cells) == plan->cells;
                    const double ref = referenceL(rays[i]);
                    const double err = std::abs(double(L) - ref);
                    worst = std::max(worst, err);
                    worstRatio = std::max(worstRatio, err / double(total));
                    inside += err <= double(total) && double(total) <= rays[i].tol;
                    mathInside += std::abs(marchL(rays[i], int(plan->cells)) - ref) <= plan->mathematical;
                }
                std::printf("    %d/48 admitted, mean %.0f cells (ONE march: depth accumulates); worst |device − reference| %.3g, worst error/total %.4f\n",
                            admitted, admitted ? meanCells / admitted : 0.0, worst, worstRatio);
                check(admitted == 48, "every in-domain ray's transport is admitted at ±0.1");
                check(agree == admitted, "the device's cell count equals the CPU mirror's on every ray");
                check(inside == admitted, "every admitted ray's true transport error lies inside its device-reported total");
                check(mathInside == admitted, "the double march with the plan's cells lies inside the mathematical part alone (depth and L quadrature)");
                check((*out)[4 * 48 + 1] == 0.0f, "transport with hi < lo refuses (light runs one way; not a reversed integral)");
            }
            // An extinction that may go negative is priced, not refused outright: e^−τ ≤
            // T_max = e^(max(0, −σ_low)·S) scales every factor (plan §6ar). Bare noise
            // over one unit plans; noise − 10 over two units (T_max ≈ e^27.6) refuses.
            const double o[3] = {0.1, 0.2, 0.3}, dd[3] = {1.0, 0.0, 0.0};
            auto signedSigma = node(Op::Noise, named("p"));
            const auto nc = OntoMath::certifyLineQuadrature(*signedSigma, Rendering::kVolumeExecutionModel, 20, &why);
            check(nc && OntoMath::planLineTransport(*nc, *ec, o, dd, 0.0, 1.0, 1.0, 4096, &why).has_value(),
                  "a signed extinction (bare noise) plans, its bound scaled by T_max");
            auto negativeSigma = node(Op::Add, node(Op::Noise, named("p")), constant(-10.0));
            const auto mc = OntoMath::certifyLineQuadrature(*negativeSigma, Rendering::kVolumeExecutionModel, 20, &why);
            check(mc && !OntoMath::planLineTransport(*mc, *ec, o, dd, 0.0, 2.0, 1.0, 4096, &why) && why.find("unbounded") != std::string::npos,
                  "an extinction negative enough that e^-tau is unbounded refuses: " + why);
        }
    }
    // Real curtains read independent clocks and emission directions. A proved
    // interval is not a value binding: exercise both and the qualification gate.
    {
        const std::map<std::string, std::pair<double, double>> symbols{
            {OntoMath::kTimeVar, {-1.0, 1.0}}, {OntoMath::kOmegaZVar, {-1.0, 1.0}}};
        auto sigma = node(Op::Add, constant(0.5), node(Op::Scale, constant(0.125), named(OntoMath::kTimeVar)));
        auto epsilon = node(Op::Add, constant(1.0), named(OntoMath::kOmegaZVar));
        const auto sc = OntoMath::certifyLineQuadrature(*sigma, Rendering::kVolumeExecutionModel, 4, nullptr, symbols);
        const auto ec = OntoMath::certifyLineQuadrature(*epsilon, Rendering::kVolumeExecutionModel, 4, nullptr, symbols);
        check(sc && ec && sc->symbols == symbols && ec->symbols == symbols,
              "quadrature certificates retain the exact clock/angular qualification domain");
        if (sc && ec) {
            const auto prog = sdfwgsl::compileLineTransport(*sigma, *epsilon, *sc, *ec, 4096);
            check(prog.ok, "clock- and direction-dependent transport lowers with explicit runtime bindings" +
                           (prog.ok ? std::string() : " — " + prog.error));
            std::vector<Ray> rays;
            for (double clock : {-0.5, 0.5}) for (double z : {-1.0, 0.0, 0.5}) {
                Ray r{{0, 0, 0}, {1, 0, 0}, 0, 1, 0.02}; r.clock = clock; r.omega[2] = z; rays.push_back(r);
            }
            auto invalid = rays.back(); invalid.clock = 2.0; rays.push_back(invalid);
            invalid = rays.back(); invalid.clock = 0.0; invalid.omega[2] = -1.25; rays.push_back(invalid);
            invalid = rays.front(); invalid.clock = NAN; rays.push_back(invalid);
            const auto out = prog.ok ? runOnDevice(gpu, prog, rays, "lineTransportAt") : std::nullopt;
            check(out.has_value(), "the device runs explicit clock/angular transport without validation errors");
            if (out) {
                int contained = 0, refused = 0;
                double values[6]{};
                for (int i = 0; i < 6; ++i) {
                    const auto& r = rays[i];
                    const double s = 0.5 + 0.125 * r.clock;
                    const double reference = (1.0 + r.omega[2]) * -std::expm1(-s) / s;
                    values[i] = (*out)[4*i];
                    contained += (*out)[4*i+1] == 1.f && std::abs(values[i] - reference) <= (*out)[4*i+3] &&
                                 (*out)[4*i+3] <= r.tol;
                }
                for (int i = 6; i < 9; ++i) refused += (*out)[4*i+1] == 0.f;
                check(contained == 6, "all six bound clock/direction results contain their analytic transport");
                check(values[0] == 0 && values[3] == 0 && values[2] > values[1] && values[1] > values[4],
                      "clock and angular changes alter radiance as authored, including exact dark direction");
                check(refused == 3, "out-of-domain clock, direction and NaN refuse before the march");
            }
            auto missing = *sc; missing.symbols.clear();
            check(!sdfwgsl::compileLineTransport(*sigma, *epsilon, missing, *ec, 4096).ok,
                  "one field cannot borrow the other field's clock certificate");
            auto narrow = *sc; narrow.symbols[OntoMath::kTimeVar] = {0.1, 0.1};
            check(!sdfwgsl::compileLineTransport(*sigma, *epsilon, narrow, *ec, 4096).ok,
                  "a point domain with no exact f32 representation refuses instead of widening");
        }
    }

    // Actual device execution of a nonuniform, complete-ray schedule. Analytic
    // controls are independent of the plan's range/derivative construction.
    {
        auto tent = [](double c,double h) {
            return node(Op::Clamp,node(Op::Sub,constant(1),node(Op::Div,
                node(Op::Abs,node(Op::Sub,named("x"),constant(c))),constant(h))),constant(0),constant(1));
        };
        const double o[3]={0,0,0},d[3]={1,0,0};
        for(int fixture=0;fixture<2;++fixture) {
            auto sigma=fixture==0?constant(0):tent(2,1);
            auto epsilon=fixture==0?tent(5,0.125):tent(8,1);
            const double end=fixture==0?64:16, tolerance=fixture==0?0.001:0.01;
            OntoMath::RegionCertificate sc(*sigma,Rendering::kVolumeExecutionModel,0.25),ec(*epsilon,Rendering::kVolumeExecutionModel,0.25);
            auto f=[](double x,double c,double h){return std::max(0.0,1.0-std::abs(x-c)/h);};
            std::string why;
            const auto plan=OntoMath::planRegionalLineTransport(sc,ec,
                [&](double x){return fixture==0?0.0:f(x,2,1);},
                [&](double x){return fixture==0?f(x,5,0.125):f(x,8,1);},
                o,d,0,end,tolerance*0.9,0.009,2048,&why);
            check(plan && plan->admitted,"analytic schedule admits before device execution: "+why);
            if(!plan || !plan->admitted)continue;
            const auto program=sdfwgsl::compileScheduledLineTransport(*sigma,*epsilon,*plan);
            check(program.ok,"nonuniform fields lower through the shared emitter: "+program.error);
            if(!program.ok)continue;
            Ray r{{0,0,0},{1,0,0},0,end,tolerance};r.tTolerance=0.01;
            std::vector<Ray> rays{r};
            auto moved=r;moved.o[0]=0.25;rays.push_back(moved);
            auto cropped=r;cropped.hi=end-1;rays.push_back(cropped);
            auto tight=r;tight.tol=1e-7;rays.push_back(tight);
            auto tTight=r;tTight.tTolerance=1e-7;rays.push_back(tTight);
            const auto radiance=runOnDevice(gpu,program,rays,"scheduledL");
            const auto transmittance=runOnDevice(gpu,program,{r},"scheduledT");
            const auto work=runOnDevice(gpu,program,{r},"scheduledWork");
            check(radiance && transmittance && work,"the nonuniform L/T kernel and work counters execute without validation errors");
            if(radiance && transmittance && work) {
                const double wantL=fixture==0?0.125:std::exp(-1.0),wantT=fixture==0?1.0:std::exp(-1.0);
                std::printf("    schedule device fixture %d: L %.8g +/- %.6g, T %.8g +/- %.6g, sigma/emission calls %.0f/%.0f, %zu panels\n",
                    fixture,(*radiance)[0],(*radiance)[3],(*transmittance)[0],(*transmittance)[3],(*work)[0],(*work)[1],plan->panels.size());
                check((*radiance)[1]==1 && std::abs((*radiance)[0]-wantL)<=(*radiance)[3] && (*radiance)[3]<=tolerance,
                      "device radiance contains the independent analytic result within the requested tolerance");
                check((*transmittance)[1]==1 && std::abs((*transmittance)[0]-wantT)<=(*transmittance)[3] && (*transmittance)[3]<=0.01,
                      "device final transmittance contains the same complete absorbing prefix");
                check((*work)[0]==plan->sigmaSamples && (*work)[1]==plan->emissionSamples,
                      "executed field calls equal the nonuniform schedule's promised work");
                bool refused=true;for(int i=1;i<5;++i)refused &= (*radiance)[4*i+1]==0.f;
                check(refused,"a different ray, clipped interval and either tighter output budget refuse safely");
                if(fixture==0)check((*work)[0]==0 && (*work)[1]<1024,"the GPU actually bypasses zero density and avoids thousands of empty field sites");
                else check((*radiance)[0]<0.5 && (*work)[0]>0,"an exact dark region still executes the absorption needed by later emission");
            }
            auto other=constant(3);
            check(!sdfwgsl::compileScheduledLineTransport(*sigma,*other,*plan).ok,
                  "an edited source cannot execute under a stale schedule proof");
            auto gap=*plan;gap.panels.front().lo+=0.125;
            check(!sdfwgsl::compileScheduledLineTransport(*sigma,*epsilon,gap).ok,
                  "an incomplete panel schedule is refused before shader creation");
        }
    }
    // A real authored curtain, read-only, at the same seeded occupied rays as
    // the corrected CPU census. This does NOT install a native renderer lane.
    {
        namespace fs=std::filesystem;
        fs::path dir;
        for(const char* p:{"saves/zones/Northern Veil","../saves/zones/Northern Veil"})
            if(fs::exists(fs::path(p)/"zone.ecform") || fs::exists(fs::path(p)/"zone.json")){dir=p;break;}
        check(!dir.empty(),"the real Northern Veil fixture is available for a device transport witness");
        if(!dir.empty()) {
            nlohmann::json zone;
            if(fs::exists(dir/"zone.json")){std::ifstream in(dir/"zone.json");zone=nlohmann::json::parse(in);}
            else{std::ifstream in(dir/"zone.ecform",std::ios::binary);std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)),{});
                zone=nlohmann::json::parse(nlohmann::json::from_msgpack(bytes).at("MigrationRoot").get<std::string>());}
            std::shared_ptr<geom::FieldNode> field;
            Rendering::VolumeDensityBinding binding;
            for(const auto& j:zone.at("spatialFields")) {
                auto candidate=geom::FieldNode::fromJson(j);
                Rendering::VolumeDensityBinding b;
                if(candidate && Rendering::readVolumeDensity(*candidate,0,0,b) && b.densityExpr){field=std::move(candidate);binding=b;break;}
            }
            auto single=[](const OntoMath::Piecewise* p)->const MathNode*{
                if(!p || p->pieces.size()!=1)return nullptr;const auto& x=p->pieces[0];
                return !x.guard && !x.call && !x.fold && !x.hasLo && !x.hasHi && !x.whereLEZero?x.mathNode.get():nullptr;
            };
            const auto* extinction=single(binding.extinctionExpr);const auto* density=single(binding.densityExpr);const auto* emission=single(binding.emissionExpr);
            check((extinction || density) && emission,"the actual curtain channels have their supported single-piece mathematical form");
            if((extinction || density) && emission) {
                auto clone=[](const MathNode& x){return MathNode::fromJson(x.toJson());};
                auto sigma=extinction?clone(*extinction):node(Op::Scale,constant(0.5),clone(*density));
                auto component=node(Op::Component,clone(*emission));component->stringArg="z";
                auto epsilon=node(Op::Intersection,std::move(component),constant(0));
                const std::map<std::string,std::pair<double,double>> symbols{{OntoMath::kTimeVar,{0,0}},
                    {OntoMath::kOmegaXVar,{-1,1}},{OntoMath::kOmegaYVar,{-1,1}},{OntoMath::kOmegaZVar,{-1,1}}};
                OntoMath::RegionCertificate sc(*sigma,Rendering::kVolumeExecutionModel,4,symbols),ec(*epsilon,Rendering::kVolumeExecutionModel,4,symbols);
                std::mt19937_64 rng(1002);std::uniform_real_distribution<double> un(-1,1);
                std::string firstCode;int admitted=0,contained=0,nonzero=0,sameProgram=0;
                for(int index=0;index<12;++index) {
                    Ray r{};double norm=0;const double half[3]={std::abs(binding.scale.x),std::abs(binding.scale.y),std::abs(binding.scale.z)};
                    for(int k=0;k<3;++k){r.o[k]=half[k]*0.9*un(rng);r.d[k]=un(rng);norm+=r.d[k]*r.d[k];}
                    for(double& x:r.d)x/=std::sqrt(norm);
                    r.lo=-1e300;r.hi=1e300;
                    for(int k=0;k<3;++k){const double a=(-half[k]-r.o[k])/r.d[k],b=(half[k]-r.o[k])/r.d[k];r.lo=std::max(r.lo,std::min(a,b));r.hi=std::min(r.hi,std::max(a,b));}
                    for(int k=0;k<3;++k){r.o[k]=float(r.o[k]);r.d[k]=float(r.d[k]);r.omega[k]=-r.d[k];}r.lo=float(r.lo);r.hi=float(r.hi);r.tol=r.tTolerance=0.01;
                    if(index!=3 && index!=4 && index!=10 && index!=11)continue;
                    auto at=[&](const MathNode& f,long double t){glm::vec3 p(float(r.o[0]+t*r.d[0]),float(r.o[1]+t*r.d[1]),float(r.o[2]+t*r.d[2]));
                        auto v=f.evaluate({{"p",PropertyValue(p)},{"x",PropertyValue(double(p.x))},{"y",PropertyValue(double(p.y))},{"z",PropertyValue(double(p.z))},
                            {OntoMath::kTimeVar,PropertyValue(0.0)},
                            {OntoMath::kOmegaXVar,PropertyValue(r.omega[0])},{OntoMath::kOmegaYVar,PropertyValue(r.omega[1])},{OntoMath::kOmegaZVar,PropertyValue(r.omega[2])}});
                        double value=NAN;if(v)propertyValueToNumber(*v,value);return (long double)value;};
                    std::string why;
                    // Host preparation is tighter than final admission; the
                    // GPU reevaluates its own bound with outward f32 arithmetic.
                    const auto plan=OntoMath::planRegionalLineTransport(sc,ec,[&](double x){return double(at(*sigma,x));},
                        [&](double x){return double(at(*epsilon,x));},r.o,r.d,r.lo,r.hi,0.009,0.009,2048,&why);
                    check(plan && plan->admitted,"real occupied ray "+std::to_string(index)+" obtains a complete L/T schedule: "+why);
                    if(!plan || !plan->admitted)continue;
                    const auto program=sdfwgsl::compileScheduledLineTransport(*sigma,*epsilon,*plan);
                    check(program.ok,"the real authored clock/noise/clamp channels compile into the scheduled device kernel: "+program.error);
                    if(!program.ok)continue;
                    if(firstCode.empty())firstCode=program.wgsl;
                    sameProgram+=firstCode==program.wgsl;
                    const auto L=runOnDevice(gpu,program,{r},"scheduledL"),T=runOnDevice(gpu,program,{r},"scheduledT"),W=runOnDevice(gpu,program,{r},"scheduledWork");
                    if(!(L && T && W)){check(false,"real curtain GPU execution has no validation error");continue;}
                    auto reference=[&](int cells){
                        const long double h=((long double)r.hi-r.lo)/cells;long double tau=0,acc=0;
                        long double sa=at(*sigma,r.lo),ea=at(*epsilon,r.lo),ga=ea;
                        for(int j=0;j<cells;j+=2){const auto sm=r.lo+(j+1)*h,sb=r.lo+(j+2)*h;
                            const auto sM=at(*sigma,sm),sB=at(*sigma,sb);
                            const auto tm=tau+h*(5*sa+8*sM-sB)/12,tb=tau+h*(sa+4*sM+sB)/3;
                            const auto gm=std::exp(-tm)*at(*epsilon,sm),gb=std::exp(-tb)*at(*epsilon,sb);
                            acc+=h*(ga+4*gm+gb)/3;tau=tb;sa=sB;ga=gb;}
                        return std::pair<double,double>{double(acc),double(std::exp(-tau))};};
                    const auto ref=reference(4000),finer=reference(8000);
                    nonzero+=finer.first>0 && finer.second<0.99999;
                    const bool fits=(*L)[1]==1 && (*T)[1]==1;admitted+=fits;
                    contained+=fits && std::abs((*L)[0]-finer.first)<=(*L)[3] && std::abs((*T)[0]-finer.second)<=(*T)[3] && (*L)[3]<=0.01 && (*T)[3]<=0.01;
                    check(std::abs(ref.first-finer.first)<1e-5 && std::abs(ref.second-finer.second)<1e-5,
                          "two fine real-field numerical references agree well below the requested budget");
                    check((*W)[0]==plan->sigmaSamples && (*W)[1]==plan->emissionSamples,"real device field calls match the executable schedule");
                    std::printf("    REAL emerald z ray %d: device L %.8g T %.8g, errors %.5g / %.5g, calls %.0f/%.0f, %zu panels\n",index,
                        (*L)[0],(*T)[0],(*L)[3],(*T)[3],(*W)[0],(*W)[1],plan->panels.size());
                    auto outOfTime=r;outOfTime.clock=1;
                    const auto refused=runOnDevice(gpu,program,{outOfTime},"scheduledL");
                    check(refused && (*refused)[1]==0,"a real curtain cannot reuse its t=0 proof at a different clock");
                }
                check(nonzero==4,"all four device witnesses contain actual source and attenuation (no empty-ray admission)");
                check(admitted==4 && contained==4,"all four real-curtain L/T results admit and contain the fine numerical reference");
                check(sameProgram==4,"different ray schedules change numeric payload only, preserving identical shader text");
            }
        }
    }
    // The same mathematical coefficient derivation, now consumed alongside
    // one bounded device extinction integral for all vector components and T.
    {
        auto sigma=leaf(OntoMath::ScalarForm::variable("x").plus(OntoMath::ScalarForm::constant(1.0)));
        auto zero=constant(0);
        auto pulse=leaf(OntoMath::ScalarForm::constant(0.85).plus(
            OntoMath::ScalarForm::transcendental(OntoMath::TransFactor::Kind::Cos,"t",0.32,0,0.15)));
        auto vector=node(Op::Scale,node(Op::VectorConstruct,constant(0.25),constant(-0.5),constant(2)),std::move(pulse));
        auto source=node(Op::Scale,std::make_unique<MathNode>(*sigma),std::move(vector));
        const std::map<std::string,std::pair<double,double>> symbols{{OntoMath::kTimeVar,{0,3}}};
        OntoMath::RegionCertificate sc(*sigma,Rendering::kVolumeExecutionModel,4,symbols),zc(*zero,Rendering::kVolumeExecutionModel,4,symbols);
        Ray ray{{0,0,0},{1,0,0},0,1,0.01};ray.tTolerance=0.006;
        std::string why;
        auto plan=OntoMath::planRegionalLineTransport(sc,zc,[](double x){return 1+x;},[](double){return 0;},
            ray.o,ray.d,ray.lo,ray.hi,0.009,0.004,2048,&why);
        check(plan && plan->admitted,"an extinction-only schedule admits for the shared complement reduction: "+why);
        if(plan) {
            const auto program=sdfwgsl::compileScheduledProportionalIntegral(*sigma,*source,*plan);
            check(program.ok,"the general exact vector reduction compiles: "+program.error);
            if(program.ok) {
                check(program.wgsl.find("lineEmission(")==std::string::npos && program.wgsl.find("rayProofSample(")==std::string::npos,
                      "the endpoint program structurally omits source sampling and outer-integration code");
                auto split=*plan;
                const auto p=split.panels.front();const double mid=float((p.lo+p.hi)*0.5);
                if(mid>p.lo && mid<p.hi){auto left=p,right=p;left.hi=mid;right.lo=mid;split.panels.erase(split.panels.begin());split.panels.insert(split.panels.begin(),right);split.panels.insert(split.panels.begin(),left);}
                const auto other=sdfwgsl::compileScheduledProportionalIntegral(*sigma,*source,split);
                check(other.ok && other.wgsl==program.wgsl && other.params.size()!=program.params.size(),
                      "coefficient payload follows a dynamic panel tail, so a changed panel count preserves shader identity");
                const auto splitValues=other.ok?runOnDevice(gpu,other,{ray},"reducedRGB"):std::nullopt;
                check(splitValues && std::abs((*splitValues)[2]-2*(1-std::exp(-1.5)))<1e-5,
                      "the same program actually executes a different-sized numeric panel payload");
                auto edited=std::make_unique<MathNode>(*source);
                edited->children[1]->children[0]->children[2]=constant(1.5);
                const auto edit=sdfwgsl::compileScheduledProportionalIntegral(*sigma,*edited,*plan);
                const auto editedValues=edit.ok?runOnDevice(gpu,edit,{ray},"reducedRGB"):std::nullopt;
                check(edit.ok && edit.wgsl==program.wgsl && editedValues && std::abs((*editedValues)[2]-1.5*(1-std::exp(-1.5)))<1e-5,
                      "a numeric coefficient edit reuses shader text and changes the executed value");
                auto later=ray;later.clock=3;
                const auto values=runOnDevice(gpu,program,{ray,later},"reducedRGB");
                const auto errors=runOnDevice(gpu,program,{ray,later},"reducedErrors");
                const auto work=runOnDevice(gpu,program,{ray,later},"reducedWork");
                bool fits=values && errors && work;
                if(fits)for(int i=0;i<2;++i){
                    const double pulse=0.85+0.15*std::cos(0.32*(i?3:0));const double T=std::exp(-1.5);
                    for(int c=0;c<4;++c){const double k[3]={0.25,-0.5,2};const double reference=c==3?T:k[c]*pulse*(1-T);
                        fits=fits && std::abs((*values)[4*i+c]-reference)<=(*errors)[4*i+c] && (*errors)[4*i+c]<=(c==3?0.006:0.01);}
                    fits=fits && (*work)[4*i+3]==1 && (*work)[4*i]==plan->sigmaSamples && (*work)[4*i+1]==0;
                }
                check(fits,"signed RGB and T contain independent analytic values at both qualified clocks, with sigma calls only");
                check(errors && (*errors)[2]>=2*Rendering::kVolumeExecutionModel.exponentialAbsolute(),
                      "the absolute exp allowance scales with the source coefficient rather than disappearing");
                auto strict=ray;strict.tol=1e-6;
                auto outside=ray;outside.clock=4;
                auto wrongRay=ray;wrongRay.o[1]=1;
                const auto refused=runOnDevice(gpu,program,{strict,outside,wrongRay},"reducedWork");
                check(refused && (*refused)[3]==0 && (*refused)[7]==0 && (*refused)[11]==0,
                      "whole RGB budget, clock-domain and fixed-ray violations refuse the reduced result");
                auto huge=node(Op::Scale,std::make_unique<MathNode>(*sigma),node(Op::VectorConstruct,constant(0.25),constant(-0.5),constant(20000)));
                const auto amplified=sdfwgsl::compileScheduledProportionalIntegral(*sigma,*huge,*plan);
                const auto amplifiedWork=amplified.ok?runOnDevice(gpu,amplified,{ray},"reducedWork"):std::nullopt;
                check(amplifiedWork && (*amplifiedWork)[3]==0,"a large source coefficient amplifies T uncertainty and cannot borrow the unscaled budget");
            }
            auto missingClock=*plan;missingClock.sigmaSymbols.erase(OntoMath::kTimeVar);missingClock.emissionSymbols.erase(OntoMath::kTimeVar);
            check(!sdfwgsl::compileScheduledProportionalIntegral(*sigma,*source,missingClock).ok,
                  "a coefficient cannot invent a clock absent from its explicit qualification domain");
            auto changed=std::make_unique<MathNode>(*sigma);changed->scalarForm=OntoMath::ScalarForm::constant(2);
            check(!sdfwgsl::compileScheduledProportionalIntegral(*changed,*source,*plan).ok,"a stale extinction schedule cannot acquire a fresh coefficient");
            auto unrelated=node(Op::Add,std::make_unique<MathNode>(*source),node(Op::VectorConstruct,constant(0.1),constant(0),constant(0)));
            check(!sdfwgsl::compileScheduledProportionalIntegral(*sigma,*unrelated,*plan).ok,"an additional nonproportional source component declines the exact reduction");
            auto tiny=node(Op::Scale,std::make_unique<MathNode>(*sigma),node(Op::VectorConstruct,constant(1e-40),constant(1),constant(1)));
            check(!sdfwgsl::compileScheduledProportionalIntegral(*sigma,*tiny,*plan).ok,"unqualified subnormal coefficient amplification declines compilation");
        }
    }
    // Controlled transverse-crease comparison: identical shader, ray, samples,
    // field/execution/position evidence and tolerance; only the qualified
    // directional derivatives differ. The independent integral is 5/4.
    {
        auto sigma=node(Op::Scale,node(Op::Sub,constant(1),node(Op::Abs,leaf(ScalarForm::variable("x")))),
            leaf(ScalarForm::constant(1).plus(ScalarForm::variable("z").scaled(0.5))));
        auto source=node(Op::Scale,std::make_unique<MathNode>(*sigma),node(Op::VectorConstruct,constant(0.25),constant(-0.5),constant(2)));
        auto zero=constant(0);
        OntoMath::RegionCertificate sc(*sigma,Rendering::kVolumeExecutionModel,4),zc(*zero,Rendering::kVolumeExecutionModel,4);
        Ray ray{{0,0,0},{0,0,1},0,1,0.01};ray.tTolerance=0.006;std::string why;
        const auto plan=OntoMath::planRegionalLineTransport(sc,zc,[](double z){return 1+0.5*z;},[](double){return 0;},
            ray.o,ray.d,ray.lo,ray.hi,0.009,0.004,64,&why);
        check(plan && plan->admitted && plan->panels.size()==1 && plan->sigmaSamples==3,
              "the transverse-crease analytic case needs one proved panel and three sigma evaluations: "+why);
        if(plan) {
            auto isotropic=*plan;for(auto& p:isotropic.panels){p.sigmaAlong.reset();p.emissionAlong.reset();}
            const auto a=sdfwgsl::compileScheduledProportionalIntegral(*sigma,*source,*plan);
            const auto b=sdfwgsl::compileScheduledProportionalIntegral(*sigma,*source,isotropic);
            check(a.ok && b.ok && a.wgsl==b.wgsl,"directional proof changes only payload, preserving the same shader and integration algorithm");
            if(a.ok && b.ok){
                const auto da=runOnDevice(gpu,a,{ray},"scheduledT"),db=runOnDevice(gpu,b,{ray},"scheduledT");
                check(da && db && (*da)[0]==(*db)[0] && (*da)[1]==1 && (*db)[1]==0 && (*da)[3]<=0.006 && (*db)[3]>0.006,
                      "directional evidence admits the identical depth value that isotropic evidence cannot justify at the same work and budget");
                const auto rgb=runOnDevice(gpu,a,{ray},"reducedRGB"),errors=runOnDevice(gpu,a,{ray},"reducedErrors");
                bool contains=rgb && errors;const double k[3]={0.25,-0.5,2};
                if(contains)for(int c=0;c<4;++c){const double ref=c==3?std::exp(-1.25):k[c]*(1-std::exp(-1.25));contains=contains && std::abs((*rgb)[c]-ref)<=(*errors)[c];}
                check(contains,"the reduced crease-aligned RGB/T contains its independent analytic integral");
            }
            auto rotated=*plan;rotated.direction={1,0,0};
            check(!sdfwgsl::compileScheduledProportionalIntegral(*sigma,*source,rotated).ok,
                  "a rotated ray cannot reuse another direction's panel evidence");
            bool keepsSpatial=true;
            for(const auto& p:plan->panels)keepsSpatial=keepsSpatial && !std::isfinite(p.sigma.hessian) && p.sigma.lipschitz>1 && p.sigma.execution>0;
            check(keepsSpatial,"finite directional curvature never removes the full spatial crease, position or original-execution evidence");
        }
    }
    // One prepared packet and one shader serve many origins, all three direction
    // components and clocks. No host evaluator or per-ray plan is involved.
    {
        auto sigma=leaf(ScalarForm::constant(0.5).plus(ScalarForm::variable("x").scaled(0.125))
            .plus(ScalarForm::variable("y").scaled(0.0625)).plus(ScalarForm::variable("z").scaled(0.25))
            .plus(ScalarForm::variable(OntoMath::kTimeVar).scaled(0.0625)));
        auto source=node(Op::Scale,std::make_unique<MathNode>(*sigma),node(Op::VectorConstruct,constant(0.25),constant(-0.5),constant(2)));
        auto zero=constant(0);
        const std::map<std::string,std::pair<double,double>> symbols{{OntoMath::kTimeVar,{0,3}}};
        OntoMath::RegionCertificate sc(*sigma,Rendering::kVolumeExecutionModel,1,symbols),zc(*zero,Rendering::kVolumeExecutionModel,1,symbols);
        const OntoMath::LineRayDomain domain{{-0.125,-0.125,-0.125},{0.125,0.125,0.125},{-0.125,-0.125,0.875},{0.125,0.125,1.125}};
        std::string why;
        const auto plan=OntoMath::prepareRegionalLineTransport(sc,zc,domain,-0.5,1,16,&why);
        check(plan && !plan->admitted && plan->sigmaPreparations==0,"a ray-family packet contains proof data without a CPU integral or host admission: "+why);
        if(plan){
            const auto program=sdfwgsl::compileScheduledProportionalIntegral(*sigma,*source,*plan);
            check(program.ok,"one proportional-integral program accepts a qualified ray family: "+program.error);
            std::vector<Ray> rays;
            for(int o=0;o<8;++o)for(int d=0;d<4;++d)for(double clock:{0.0,3.0}){
                Ray r{{0,0,0},{0,0,1},-0.5,1,0.01};r.tTolerance=0.006;r.clock=clock;
                for(int i=0;i<3;++i){r.o[i]=(o&(1<<i))?domain.originHi[i]:domain.originLo[i];
                    r.d[i]=((d^(d<<1))&(1<<i))?domain.directionHi[i]:domain.directionLo[i];}
                rays.push_back(r);
            }
            const auto values=program.ok?runOnDevice(gpu,program,rays,"reducedRGB"):std::nullopt;
            const auto errors=program.ok?runOnDevice(gpu,program,rays,"reducedErrors"):std::nullopt;
            const auto work=program.ok?runOnDevice(gpu,program,rays,"reducedWork"):std::nullopt;
            bool contained=values && errors && work;
            const double k[3]={0.25,-0.5,2};
            if(contained)for(std::size_t j=0;j<rays.size();++j){const auto& r=rays[j];
                const double a=0.5+0.125*r.o[0]+0.0625*r.o[1]+0.25*r.o[2]+0.0625*r.clock;
                const double b=0.125*r.d[0]+0.0625*r.d[1]+0.25*r.d[2];
                const double T=std::exp(-(a*(r.hi-r.lo)+b*(r.hi*r.hi-r.lo*r.lo)*0.5));
                contained=contained && (*work)[4*j+3]==1 && (*work)[4*j+1]==0;
                for(int c=0;c<4;++c)contained=contained && std::abs((*values)[4*j+c]-(c==3?T:k[c]*(1-T)))<=(*errors)[4*j+c] && (*errors)[4*j+c]<=(c==3?0.006:0.01);
            }
            check(contained,"all 64 varying-origin/direction/clock RGB/T records contain independent analytic integrals through one unchanged proof payload");
            std::vector<Ray> invalid;
            for(int i=0;i<3;++i)for(bool high:{false,true}){
                auto r=rays[0];r.o[i]=std::nextafter(float(high?domain.originHi[i]:domain.originLo[i]),high?INFINITY:-INFINITY);invalid.push_back(r);
                r=rays[0];r.d[i]=std::nextafter(float(high?domain.directionHi[i]:domain.directionLo[i]),high?INFINITY:-INFINITY);invalid.push_back(r);
            }
            auto r=rays[0];r.o[1]=NAN;invalid.push_back(r);r=rays[0];r.d[2]=NAN;invalid.push_back(r);
            r=rays[0];r.clock=4;invalid.push_back(r);r=rays[0];r.hi=1.125;invalid.push_back(r);
            r=rays[0];r.tTolerance=1e-8;invalid.push_back(r);
            const auto refused=program.ok?runOnDevice(gpu,program,invalid,"reducedWork"):std::nullopt;
            bool declines=refused.has_value();if(refused)for(std::size_t i=0;i<invalid.size();++i)declines=declines && (*refused)[4*i+3]==0;
            check(declines,"ray-family runtime guards refuse one-ulp origin/direction escapes, NaNs, clock/interval escape and unattainable budgets");
            auto expanded=*plan;expanded.rayDomain->directionHi[0]=0.25;
            check(!sdfwgsl::compileScheduledProportionalIntegral(*sigma,*source,expanded).ok,"expanded direction families require fresh directional proof evidence");
            auto edited=node(Op::Add,std::make_unique<MathNode>(*sigma),constant(0.125));
            check(!sdfwgsl::compileScheduledProportionalIntegral(*edited,*source,*plan).ok,"a field edit cannot reuse a previous ray-family proof snapshot");
            auto movedDomain=domain;movedDomain.originLo[0]+=0.5;movedDomain.originHi[0]+=0.5;
            const auto moved=OntoMath::prepareRegionalLineTransport(sc,zc,movedDomain,-0.5,1,16,&why);
            const auto mp=moved?sdfwgsl::compileScheduledProportionalIntegral(*sigma,*source,*moved):sdfwgsl::Program{};
            auto movedRay=rays[0];movedRay.o[0]+=0.5;
            const auto mv=moved && mp.ok?runOnDevice(gpu,mp,{movedRay},"reducedWork"):std::nullopt;
            const auto stale=program.ok?runOnDevice(gpu,program,{movedRay},"reducedWork"):std::nullopt;
            const auto mrgb=moved && mp.ok?runOnDevice(gpu,mp,{movedRay},"reducedRGB"):std::nullopt;
            const auto merr=moved && mp.ok?runOnDevice(gpu,mp,{movedRay},"reducedErrors"):std::nullopt;
            const double ma=0.5+0.125*movedRay.o[0]+0.0625*movedRay.o[1]+0.25*movedRay.o[2]+0.0625*movedRay.clock;
            const double mb=0.125*movedRay.d[0]+0.0625*movedRay.d[1]+0.25*movedRay.d[2],mt=std::exp(-(ma*1.5+mb*0.375));
            bool movedFits=mrgb && merr;if(movedFits)for(int c=0;c<4;++c)movedFits=movedFits && std::abs((*mrgb)[c]-(c==3?mt:k[c]*(1-mt)))<=(*merr)[c];
            check(moved && mp.ok && program.ok && mp.wgsl==program.wgsl && mv && stale && (*mv)[3]==1 && (*stale)[3]==0 && movedFits,
                "a moved origin family executes inside analytic bounds through fresh parameters and identical shader text, while the old payload refuses it");
            const auto general=OntoMath::prepareRegionalLineTransport(sc,sc,domain,-0.5,1,64,&why);
            const auto gp=general?sdfwgsl::compileScheduledLineTransport(*sigma,*sigma,*general):sdfwgsl::Program{};
            const auto gv=general && gp.ok?runOnDevice(gpu,gp,rays,"scheduledL"):std::nullopt;
            bool gfits=gv.has_value();if(gv)for(std::size_t i=0;i<rays.size();++i){const auto& r=rays[i];
                const double a=0.5+0.125*r.o[0]+0.0625*r.o[1]+0.25*r.o[2]+0.0625*r.clock,b=0.125*r.d[0]+0.0625*r.d[1]+0.25*r.d[2];
                const double ref=-std::expm1(-(a*1.5+b*0.375));
                gfits=gfits && (*gv)[4*i+1]==1 && std::abs((*gv)[4*i]-ref)<=(*gv)[4*i+3] && (*gv)[4*i+3]<=0.01;
            }
            check(gfits,"the general nested kernel also executes the same ray family with independent analytic radiance checks");
            const auto gi=general && gp.ok?runOnDevice(gpu,gp,invalid,"scheduledWork"):std::nullopt;
            bool gdeclines=gi.has_value();if(gi)for(std::size_t i=0;i<invalid.size();++i)gdeclines=gdeclines && (*gi)[4*i+3]==0;
            check(gdeclines,"the general nested kernel also refuses escaped family inputs and unattainable output budgets");
        }
    }
    // Actual raw authored curtain equations: all RGB components and T from one
    // device depth. Native floor/gate/scattering admission is deliberately not
    // inferred by this mathematical execution witness.
    {
        namespace fs = std::filesystem;
        fs::path dir;
        for(const char* p:{"saves/zones/Northern Veil","../saves/zones/Northern Veil"})
            if(fs::exists(fs::path(p)/"zone.ecform") || fs::exists(fs::path(p)/"zone.json")){dir=p;break;}
        check(!dir.empty(),"the four-curtain authored fixture is available for shared RGB/T execution");
        if(!dir.empty()) {
            nlohmann::json zone;
            if(fs::exists(dir/"zone.json")){std::ifstream in(dir/"zone.json");zone=nlohmann::json::parse(in);}
            else{std::ifstream in(dir/"zone.ecform",std::ios::binary);std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)),{});
                zone=nlohmann::json::parse(nlohmann::json::from_msgpack(bytes).at("MigrationRoot").get<std::string>());}
            auto single=[](const OntoMath::Piecewise* p)->const MathNode*{
                if(!p || p->pieces.size()!=1)return nullptr;const auto& x=p->pieces[0];
                return !x.guard && !x.call && !x.fold && !x.hasLo && !x.hasHi && !x.whereLEZero?x.mathNode.get():nullptr;
            };
            int media=0,contained=0,occupied=0;
            for(const auto& json:zone.at("spatialFields")) {
                auto field=geom::FieldNode::fromJson(json);Rendering::VolumeDensityBinding b;
                if(!field || !Rendering::readVolumeDensity(*field,0,0,b) || !b.densityExpr)continue;
                const auto* sigma=single(b.extinctionExpr);const auto* source=single(b.emissionExpr);
                check(sigma && source,"raw authored vector source and extinction are available: "+b.producerId);
                if(!sigma || !source)continue;
                // One fixed packet is reused across occupied rays and clocks,
                // including a spatial box straddling x=0. This family keeps dx=dy=0;
                // nonzero transverse directions must not borrow its constancy.
                {
                    auto zero=constant(0);
                    const std::map<std::string,std::pair<double,double>> symbols{{OntoMath::kTimeVar,{0,3}},
                        {OntoMath::kOmegaXVar,{-1,1}},{OntoMath::kOmegaYVar,{-1,1}},{OntoMath::kOmegaZVar,{-1,1}}};
                    OntoMath::RegionCertificate sc(*sigma,Rendering::kVolumeExecutionModel,4,symbols),zc(*zero,Rendering::kVolumeExecutionModel,4,symbols);
                    const OntoMath::LineRayDomain domain{{-0.015625,-0.015625,-0.015625},{0.015625,0.015625,0.015625},{0,0,0.99609375},{0,0,1.00390625}};
                    const double lo=double(float(-std::abs(b.scale.z))),hi=double(float(std::abs(b.scale.z)));
                    std::string why;
                    const auto begin=std::chrono::steady_clock::now();
                    const auto family=OntoMath::prepareRegionalLineTransport(sc,zc,domain,lo,hi,128,&why);
                    const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();
                    const auto program=family?sdfwgsl::compileScheduledProportionalIntegral(*sigma,*source,*family):sdfwgsl::Program{};
                    check(family && program.ok && !family->admitted && family->sigmaPreparations==0,
                        "one shared real-curtain packet prepares without host field evaluation: "+b.producerId+": "+why+program.error);
                    if(family && program.ok){
                        std::vector<Ray> rays;
                        for(int j=0;j<4;++j)for(double clock:{0.0,3.0}){
                            Ray r{{0,0,0},{0,0,1},lo,hi,0.01};r.clock=clock;r.tTolerance=0.006;r.omega[2]=-1;
                            for(int i=0;i<3;++i)r.o[i]=(j&(1<<(i%2)))?domain.originHi[i]:domain.originLo[i];
                            r.d[2]=(j&1)?domain.directionHi[2]:domain.directionLo[2];rays.push_back(r);
                        }
                        const auto values=runOnDevice(gpu,program,rays,"reducedRGB"),errors=runOnDevice(gpu,program,rays,"reducedErrors"),work=runOnDevice(gpu,program,rays,"reducedWork");
                        const auto depth=runOnDevice(gpu,program,rays,"scheduledT");
                        if(depth){double maximum=0;for(std::size_t j=0;j<rays.size();++j)maximum=std::max(maximum,double((*depth)[4*j+3]));
                            std::printf("    FAMILY-DEPTH %s: max T bound %.8g; first status %.0f\n",b.producerId.c_str(),maximum,(*depth)[1]);}
                        auto reference=[&](const Ray& r,int n){
                            auto fields=[&](double t){const glm::vec3 p(float(r.o[0]+t*r.d[0]),float(r.o[1]+t*r.d[1]),float(r.o[2]+t*r.d[2]));
                                const std::map<std::string,PropertyValue> vars{{"p",PropertyValue(p)},{"x",PropertyValue(double(p.x))},{"y",PropertyValue(double(p.y))},{"z",PropertyValue(double(p.z))},
                                    {OntoMath::kTimeVar,PropertyValue(r.clock)},{OntoMath::kOmegaXVar,PropertyValue(r.omega[0])},{OntoMath::kOmegaYVar,PropertyValue(r.omega[1])},{OntoMath::kOmegaZVar,PropertyValue(r.omega[2])}};
                                const auto sv=sigma->evaluate(vars),ev=source->evaluate(vars);double v=NAN;if(sv)propertyValueToNumber(*sv,v);
                                return std::make_pair(v,ev && std::holds_alternative<glm::vec3>(*ev)?std::get<glm::vec3>(*ev):glm::vec3(NAN));};
                            const long double h=((long double)r.hi-r.lo)/n;long double tau=0;std::array<long double,3> L{};
                            auto a=fields(r.lo);std::array<long double,3> ga{a.second.x,a.second.y,a.second.z};
                            for(int j=0;j<n;j+=2){const auto m=fields(double(r.lo+(j+1)*h)),b=fields(double(r.lo+(j+2)*h));
                                const long double tm=tau+h*(5*a.first+8*m.first-b.first)/12,tb=tau+h*(a.first+4*m.first+b.first)/3;
                                for(int c=0;c<3;++c){const long double gm=std::exp(-tm)*m.second[c],gb=std::exp(-tb)*b.second[c];L[c]+=h*(ga[c]+4*gm+gb)/3;ga[c]=gb;}
                                tau=tb;a=b;}
                            return std::array<double,4>{double(L[0]),double(L[1]),double(L[2]),double(std::exp(-tau))};};
                        bool fits=values && errors && work,converged=true,occupied=true;
                        for(std::size_t j=0;j<rays.size();++j){const auto coarse=reference(rays[j],4000),fine=reference(rays[j],8000);
                            occupied=occupied && fine[0]>0 && fine[1]>0 && fine[2]>0 && fine[3]<0.999;
                            for(int c=0;c<4;++c){converged=converged && std::abs(coarse[c]-fine[c])<1e-5;
                                if(values && errors && work)fits=fits && (*work)[4*j+3]==1 && (*work)[4*j+1]==0 &&
                                    std::abs((*values)[4*j+c]-fine[c])<=(*errors)[4*j+c] && (*errors)[4*j+c]<=(c==3?0.006:0.01);}
                        }
                        check(fits && converged && occupied,"one unchanged real-curtain packet admits eight occupied ray/clock RGB/T records inside independent cumulative references: "+b.producerId);
                        if(work && errors)std::printf("    FAMILY %s: %.3f ms shared preparation; %zu panels; %.0f sigma sites; first T bound %.6g; eight records; zero host field samples; admitted %.0f\n",
                            b.producerId.c_str(),ms,family->panels.size(),(*work)[0],(*errors)[3],(*work)[3]);
                        auto escape=rays[0];escape.d[0]=0x1p-20;
                        const auto refused=runOnDevice(gpu,program,{escape},"reducedWork");
                        check(refused && (*refused)[3]==0,"a real-curtain family refuses a tiny nonzero transverse direction outside its proof");
                    }
                }
                for(double transverse:{0.0,2.0}) {
                auto zero=constant(0);
                const std::map<std::string,std::pair<double,double>> symbols{{OntoMath::kTimeVar,{0,3}},
                    {OntoMath::kOmegaXVar,{-1,1}},{OntoMath::kOmegaYVar,{-1,1}},{OntoMath::kOmegaZVar,{-1,1}}};
                OntoMath::RegionCertificate sc(*sigma,Rendering::kVolumeExecutionModel,4,symbols),zc(*zero,Rendering::kVolumeExecutionModel,4,symbols);
                Ray ray{{transverse,0,0},{0,0,1},double(float(-std::abs(b.scale.z))),double(float(std::abs(b.scale.z))),0.01};
                ray.omega[2]=-1;ray.tTolerance=0.006;
                auto scope=[&](double s,double clock){const glm::vec3 p(float(transverse),0,float(s));return std::map<std::string,PropertyValue>{
                    {"p",PropertyValue(p)},{"x",PropertyValue(transverse)},{"y",PropertyValue(0.0)},{"z",PropertyValue(double(p.z))},
                    {OntoMath::kTimeVar,PropertyValue(clock)},{OntoMath::kOmegaXVar,PropertyValue(0.0)},
                    {OntoMath::kOmegaYVar,PropertyValue(0.0)},{OntoMath::kOmegaZVar,PropertyValue(-1.0)}};};
                auto extinction=[&](double s){const auto v=sigma->evaluate(scope(s,0));double x=NAN;if(v)propertyValueToNumber(*v,x);return x;};
                std::string why;
                const auto preparedAt=std::chrono::steady_clock::now();
                const auto plan=OntoMath::planRegionalLineTransport(sc,zc,extinction,[](double){return 0;},ray.o,ray.d,ray.lo,ray.hi,0.009,0.004,2048,&why);
                const double preparationMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-preparedAt).count();
                check(plan && plan->admitted,"the real curtain obtains one extinction-only schedule: "+b.producerId+": "+why);
                if(!plan || !plan->admitted)continue;
                std::printf("    PREPARE %s x %.0f: %.3f ms, %zu unique host sigma evaluations, %zu final device sites\n",
                    b.producerId.c_str(),transverse,preparationMs,plan->sigmaPreparations,plan->sigmaSamples);
                const auto program=sdfwgsl::compileScheduledProportionalIntegral(*sigma,*source,*plan);
                check(program.ok,"all three actual authored source components reduce to the shared depth: "+b.producerId+": "+program.error);
                if(!program.ok)continue;
                auto later=ray;later.clock=3;
                const auto values=runOnDevice(gpu,program,{ray,later},"reducedRGB");
                const auto errors=runOnDevice(gpu,program,{ray,later},"reducedErrors");
                const auto work=runOnDevice(gpu,program,{ray,later},"reducedWork");
                bool fits=values && errors && work,real=true;
                auto reference=[&](int cells,double clock){
                    const long double h=((long double)ray.hi-ray.lo)/cells;
                    long double tau=0;std::array<long double,3> L{};
                    auto emission=[&](double s){auto v=source->evaluate(scope(s,clock));return v && std::holds_alternative<glm::vec3>(*v)?std::get<glm::vec3>(*v):glm::vec3(NAN);};
                    long double sa=extinction(ray.lo);auto ea=emission(ray.lo);std::array<long double,3> ga{ea.x,ea.y,ea.z};
                    for(int j=0;j<cells;j+=2){
                        const double sm=double(ray.lo+(j+1)*h),sb=double(ray.lo+(j+2)*h);const long double sM=extinction(sm),sB=extinction(sb);
                        const long double tm=tau+h*(5*sa+8*sM-sB)/12,tb=tau+h*(sa+4*sM+sB)/3;
                        const auto em=emission(sm),eb=emission(sb);
                        for(int c=0;c<3;++c){const long double gm=std::exp(-tm)*em[c],gb=std::exp(-tb)*eb[c];L[c]+=h*(ga[c]+4*gm+gb)/3;ga[c]=gb;}
                        tau=tb;sa=sB;
                    }
                    return std::array<double,4>{double(L[0]),double(L[1]),double(L[2]),double(std::exp(-tau))};
                };
                for(int i=0;i<2;++i){
                    const auto coarse=reference(4000,i?3:0),fine=reference(8000,i?3:0);
                    bool converged=true;
                    for(int c=0;c<4;++c){converged=converged && std::abs(coarse[c]-fine[c])<1e-5;
                        if(fits)fits=std::abs((*values)[4*i+c]-fine[c])<=(*errors)[4*i+c] && (*errors)[4*i+c]<=(c==3?0.006:0.01);}
                    real=real && fine[0]>0 && fine[1]>0 && fine[2]>0 && fine[3]<0.999;
                    check(converged,"independent cumulative RGB integration agrees at two fine resolutions: "+b.producerId);
                    if(work)fits=fits && (*work)[4*i+3]==1 && (*work)[4*i]==plan->sigmaSamples && (*work)[4*i+1]==0;
                    if(values && errors && work)std::printf("    REDUCED %s x %.0f clock %d: RGB %.8g %.8g %.8g T %.8g; bounds %.5g %.5g %.5g %.5g; sigma/epsilon %.0f/%.0f; %zu panels\n",
                        b.producerId.c_str(),transverse,i?3:0,(*values)[4*i],(*values)[4*i+1],(*values)[4*i+2],(*values)[4*i+3],
                        (*errors)[4*i],(*errors)[4*i+1],(*errors)[4*i+2],(*errors)[4*i+3],(*work)[4*i],(*work)[4*i+1],plan->panels.size());
                }
                ++media;occupied+=real;contained+=fits;
                check(fits,"all RGB/T outputs admit and contain independent references at both clocks, with zero source-field calls: "+b.producerId);
                auto outside=ray;outside.clock=4;
                const auto refused=runOnDevice(gpu,program,{outside},"reducedWork");
                check(refused && (*refused)[3]==0,"the complete real-curtain reduction cannot reuse a proof outside its clock domain");
                }
            }
            check(media==8 && occupied==8 && contained==8,"all four real curtains have two occupied rays and shared RGB/T device witnesses at two clocks");
        }
    }
    std::cout << "webgpu_line_quadrature_test: " << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
