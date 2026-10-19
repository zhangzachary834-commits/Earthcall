// Faithfulness to the authored equation (Zach, 2026-10-18: "Faithfulness to
// exact math should be the rule"). Byte parity with today's shader is no longer
// the test for rewriting volume arithmetic (Rung 3a, the Firmament's Milestone
// 2): agreement with the authored mathematics is. This measures how far the
// production density, as the device executes it, sits from the authored
// equation evaluated in double precision at the same ray samples — the baseline
// any rewrite must match or beat.
//
// Device side: each real medium's own production program (sdfwgsl::composeVolumeSet,
// every channel, the shared-noise fill and the GPU clock preparation pass) plus a
// compute entry that forms each sample exactly as the marcher does
// (worldP = ro + rd·t; p = worldP − origin, f32) and returns volumeDensityEval(p).
// Reference: an independent double-precision interpreter of the authored MathNode
// tree with the CPU evaluator's semantics (guards, clamp, first-match Piecewise;
// Noise is glm::perlin instantiated in double), at p formed in double from the
// same f32 ray, origin and sample t. Double carries ~9 more digits than f32, so
// it is a sound reference for f32 error (not an 80-digit or interval theorem).
//
// Evidence class: one adapter, sampled rays. Report-only until a rewrite exists
// to compare: the checks are coverage and finiteness.
//
// Claude Code · Claude Opus 5.5 · session f3af84cf-bfe3-429d-b417-2d533119f169 ·
// 2026-10-18. Harness pattern: webgpu_volume_support_enclosure_test.
#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "Singularity/Screen/VolumeDensity.hpp"
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"
#include "support/test_harness.hpp"

#include <webgpu/wgpu.h>
#include <glm/gtc/noise.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <optional>
#include <random>
#include <string>
#include <variant>
#include <vector>

namespace {

int g_checks = 0;
int g_failures = 0;
void check(bool condition, const std::string& description) {
    ++g_checks;
    if (!condition) ++g_failures;
    std::printf("  %s: %s\n", condition ? "ok" : "FAILED", description.c_str());
}

struct MapR { bool done = false; };
void onMap(WGPUMapAsyncStatus, WGPUStringView, void* u, void*) { static_cast<MapR*>(u)->done = true; }
struct PopResult { WGPUErrorType type = WGPUErrorType_NoError; bool done = false; std::string message; };
void onPopError(WGPUPopErrorScopeStatus, WGPUErrorType type, WGPUStringView msg, void* ud1, void*) {
    auto* r = static_cast<PopResult*>(ud1);
    r->type = type;
    r->done = true;
    if (type != WGPUErrorType_NoError) r->message.assign(msg.data, msg.length);
}

// ---- The double-precision reference -----------------------------------------
// Authored Noise is glm::perlin as Earthcall's CPU evaluator runs it, on floats.
// Its lattice hash (mod289/permute, the ×1/7 gradient fold, taylorInvSqrt) is a
// DISCRETE selection whose result depends on f32 rounding: glm::perlin<double>
// picks different gradients in some cells and differs by up to 1.2 (found
// 2026-10-18; e.g. (3.1985, −3.4835, 2.0853): 0.5759 vs −0.6281). So the hash is
// kept exactly as its f32 definition, from the exact lattice cell, and only the
// continuous part (fractional position, dots, fade, blends) runs in double.
double perlinReference(const glm::dvec3& P) {
    using glm::vec3; using glm::vec4;
    const glm::dvec3 cell = glm::floor(P);
    auto mod289 = [](vec4 x) { return x - glm::floor(x * (1.0f / 289.0f)) * 289.0f; };
    auto mod289v3 = [](vec3 x) { return x - glm::floor(x * (1.0f / 289.0f)) * 289.0f; };
    auto permute = [&](vec4 x) { return mod289(((x * 34.0f) + 1.0f) * x); };
    auto taylorInvSqrt = [](vec4 r) { return 1.79284291400159f - 0.85373472095314f * r; };
    vec3 Pi0 = vec3(cell), Pi1 = Pi0 + 1.0f;
    Pi0 = mod289v3(Pi0);
    Pi1 = mod289v3(Pi1);
    const vec4 ix(Pi0.x, Pi1.x, Pi0.x, Pi1.x), iy(Pi0.y, Pi0.y, Pi1.y, Pi1.y);
    const vec4 iz0(Pi0.z), iz1(Pi1.z);
    const vec4 ixy = permute(permute(ix) + iy);
    const vec4 ixy0 = permute(ixy + iz0), ixy1 = permute(ixy + iz1);
    auto grads = [](vec4 ixyk, vec4& gx, vec4& gy, vec4& gz) {
        gx = ixyk * float(1.0 / 7.0);
        gy = glm::fract(glm::floor(gx) * float(1.0 / 7.0)) - 0.5f;
        gx = glm::fract(gx);
        gz = vec4(0.5f) - glm::abs(gx) - glm::abs(gy);
        const vec4 sz = glm::step(gz, vec4(0.0f));
        gx -= sz * (glm::step(vec4(0.0f), gx) - 0.5f);
        gy -= sz * (glm::step(vec4(0.0f), gy) - 0.5f);
    };
    vec4 gx0, gy0, gz0, gx1, gy1, gz1;
    grads(ixy0, gx0, gy0, gz0);
    grads(ixy1, gx1, gy1, gz1);
    vec3 g000(gx0.x, gy0.x, gz0.x), g100(gx0.y, gy0.y, gz0.y), g010(gx0.z, gy0.z, gz0.z), g110(gx0.w, gy0.w, gz0.w);
    vec3 g001(gx1.x, gy1.x, gz1.x), g101(gx1.y, gy1.y, gz1.y), g011(gx1.z, gy1.z, gz1.z), g111(gx1.w, gy1.w, gz1.w);
    const vec4 n0 = taylorInvSqrt(vec4(glm::dot(g000, g000), glm::dot(g010, g010), glm::dot(g100, g100), glm::dot(g110, g110)));
    g000 *= n0.x; g010 *= n0.y; g100 *= n0.z; g110 *= n0.w;
    const vec4 n1 = taylorInvSqrt(vec4(glm::dot(g001, g001), glm::dot(g011, g011), glm::dot(g101, g101), glm::dot(g111, g111)));
    g001 *= n1.x; g011 *= n1.y; g101 *= n1.z; g111 *= n1.w;
    // Continuous part, exact (double).
    using D = glm::dvec3;
    const D f0 = P - cell, f1 = f0 - 1.0;
    const double a000 = glm::dot(D(g000), f0), a100 = glm::dot(D(g100), D(f1.x, f0.y, f0.z));
    const double a010 = glm::dot(D(g010), D(f0.x, f1.y, f0.z)), a110 = glm::dot(D(g110), D(f1.x, f1.y, f0.z));
    const double a001 = glm::dot(D(g001), D(f0.x, f0.y, f1.z)), a101 = glm::dot(D(g101), D(f1.x, f0.y, f1.z));
    const double a011 = glm::dot(D(g011), D(f0.x, f1.y, f1.z)), a111 = glm::dot(D(g111), f1);
    const D fade = f0 * f0 * f0 * (f0 * (f0 * 6.0 - 15.0) + 10.0);
    auto mix = [](double a, double b, double t) { return a * (1.0 - t) + b * t; };
    const double z0 = mix(a000, a001, fade.z), z1 = mix(a100, a101, fade.z);
    const double z2 = mix(a010, a011, fade.z), z3 = mix(a110, a111, fade.z);
    const double y0 = mix(z0, z2, fade.y), y1 = mix(z1, z3, fade.y);
    return 2.2 * mix(y0, y1, fade.x);
}

using Val = std::variant<double, glm::dvec3>;
struct Ref {
    glm::dvec3 p;
    double t;
    std::map<std::string, double> scalars;
    std::string refusal;
    std::optional<Val> fail(const std::string& why) { if (refusal.empty()) refusal = why; return std::nullopt; }
    static bool vec(const Val& v) { return std::holds_alternative<glm::dvec3>(v); }
    static double s(const Val& v) { return std::get<double>(v); }
    static glm::dvec3 v3(const Val& v) { return std::get<glm::dvec3>(v); }

    std::optional<Val> eval(const OntoMath::MathNode& n) {
        using Op = OntoMath::MathNode::Op;
        auto arg = [&](std::size_t i) -> std::optional<Val> {
            if (i >= n.children.size() || !n.children[i]) return fail("missing child");
            return eval(*n.children[i]);
        };
        switch (n.op) {
            case Op::ScalarLeaf: {
                const auto v = n.scalarForm.evaluate(scalars);
                if (!v) return fail("scalar leaf unbound");
                return Val(*v);
            }
            case Op::ValueLeaf: {
                if (n.variableName == OntoMath::kAmbientPointVar) return Val(p);
                const auto it = scalars.find(n.variableName);
                if (it == scalars.end()) return fail("value leaf " + n.variableName);
                return Val(it->second);
            }
            case Op::VectorConstruct: {
                auto a = arg(0), b = arg(1), c = arg(2);
                if (!a || !b || !c || vec(*a) || vec(*b) || vec(*c)) return fail("vector construct");
                return Val(glm::dvec3(s(*a), s(*b), s(*c)));
            }
            case Op::Component: {
                auto a = arg(0);
                if (!a || !vec(*a)) return fail("component");
                const int i = n.stringArg == "x" ? 0 : n.stringArg == "y" ? 1 : n.stringArg == "z" ? 2 : -1;
                if (i < 0) return fail("component axis");
                return Val(v3(*a)[i]);
            }
            case Op::Add: case Op::Sub: {
                auto a = arg(0), b = arg(1);
                if (!a || !b || vec(*a) != vec(*b)) return fail("add/sub kinds");
                const double k = n.op == Op::Add ? 1.0 : -1.0;
                if (vec(*a)) return Val(v3(*a) + k * v3(*b));
                return Val(s(*a) + k * s(*b));
            }
            case Op::Scale: {
                auto a = arg(0), b = arg(1);
                if (!a || !b || (vec(*a) && vec(*b))) return fail("scale kinds");
                if (vec(*a)) return Val(v3(*a) * s(*b));
                if (vec(*b)) return Val(s(*a) * v3(*b));
                return Val(s(*a) * s(*b));
            }
            case Op::Dot: case Op::Cross: case Op::Hadamard: case Op::Distance: {
                auto a = arg(0), b = arg(1);
                if (!a || !b || !vec(*a) || !vec(*b)) return fail("vector binary");
                if (n.op == Op::Dot) return Val(glm::dot(v3(*a), v3(*b)));
                if (n.op == Op::Cross) return Val(glm::cross(v3(*a), v3(*b)));
                if (n.op == Op::Hadamard) return Val(v3(*a) * v3(*b));
                return Val(glm::distance(v3(*a), v3(*b)));
            }
            case Op::Normalize: {
                auto a = arg(0);
                if (!a || !vec(*a)) return fail("normalize");
                const double l = glm::length(v3(*a));
                return Val(l < 1e-6 ? glm::dvec3(0.0) : v3(*a) / l);
            }
            case Op::Length: {
                auto a = arg(0);
                if (!a || !vec(*a)) return fail("length");
                return Val(glm::length(v3(*a)));
            }
            case Op::Union: case Op::Intersection: {
                auto a = arg(0), b = arg(1);
                if (!a || !b) return fail("union");
                const bool mn = n.op == Op::Union;
                if (!vec(*a) && !vec(*b)) return Val(mn ? std::min(s(*a), s(*b)) : std::max(s(*a), s(*b)));
                const glm::dvec3 va = vec(*a) ? v3(*a) : glm::dvec3(s(*a)), vb = vec(*b) ? v3(*b) : glm::dvec3(s(*b));
                return Val(mn ? glm::min(va, vb) : glm::max(va, vb));
            }
            case Op::Difference: {
                auto a = arg(0), b = arg(1);
                if (!a || !b || vec(*a) || vec(*b)) return fail("difference");
                return Val(std::max(s(*a), -s(*b)));
            }
            case Op::Div: {
                auto a = arg(0), b = arg(1);
                if (!a || !b) return fail("div");
                const double guard = OntoMath::kDegenerateDivisor;
                if (vec(*a) && vec(*b)) {
                    glm::dvec3 r;
                    for (int i = 0; i < 3; ++i) r[i] = std::abs(v3(*b)[i]) < guard ? 0.0 : v3(*a)[i] / v3(*b)[i];
                    return Val(r);
                }
                if (vec(*a)) return Val(std::abs(s(*b)) < guard ? glm::dvec3(0.0) : v3(*a) / s(*b));
                if (vec(*b)) return fail("scalar / vector");
                return Val(std::abs(s(*b)) < guard ? 0.0 : s(*a) / s(*b));
            }
            case Op::Pow: {
                auto a = arg(0), b = arg(1);
                if (!a || !b || vec(*b)) return fail("pow");
                const double e = s(*b);
                auto pw = [&](double x) { return (x < 0.0 && std::floor(e) != e) ? 0.0 : std::pow(x, e); };
                if (vec(*a)) return Val(glm::dvec3(pw(v3(*a).x), pw(v3(*a).y), pw(v3(*a).z)));
                return Val(pw(s(*a)));
            }
            case Op::Abs: {
                auto a = arg(0);
                if (!a) return fail("abs");
                if (vec(*a)) return Val(glm::abs(v3(*a)));
                return Val(std::abs(s(*a)));
            }
            case Op::Sqrt: {
                auto a = arg(0);
                if (!a) return fail("sqrt");
                if (vec(*a)) return Val(glm::sqrt(glm::max(v3(*a), glm::dvec3(0.0))));
                return Val(std::sqrt(std::max(0.0, s(*a))));
            }
            case Op::Clamp: {
                auto v = arg(0), lo = arg(1), hi = arg(2);
                if (!v || !lo || !hi) return fail("clamp");
                if (vec(*v)) {
                    const glm::dvec3 l = vec(*lo) ? v3(*lo) : glm::dvec3(s(*lo)), h = vec(*hi) ? v3(*hi) : glm::dvec3(s(*hi));
                    return Val(glm::clamp(v3(*v), l, h));
                }
                if (vec(*lo) || vec(*hi)) return fail("clamp kinds");
                return Val(std::clamp(s(*v), s(*lo), s(*hi)));
            }
            case Op::Tan: {
                auto a = arg(0);
                if (!a || vec(*a)) return fail("tan");
                return Val(std::tan(s(*a)));
            }
            case Op::Noise: {
                auto a = arg(0);
                if (!a || !vec(*a)) return fail("noise");
                return Val(perlinReference(v3(*a)));
            }
            case Op::Map: {
                auto a = arg(0);
                if (!a || !vec(*a)) return fail("map");
                const glm::dvec3 v = v3(*a);
                if (n.stringArg == "Round") return Val(glm::dvec3(std::round(v.x), std::round(v.y), std::round(v.z)));
                if (n.stringArg == "Floor") return Val(glm::floor(v));
                return fail("map " + n.stringArg);
            }
            default:
                return fail(std::string("op ") + OntoMath::mathOpName(n.op));
        }
    }
    // The emitter's Piecewise: first piece whose >=/<= bounds hold; no match is 0.
    std::optional<double> density(const OntoMath::Piecewise& f) {
        for (const auto& piece : f.pieces) {
            if (!piece.mathNode) continue;
            if (piece.hasLo || piece.hasHi) {
                const auto it = scalars.find(f.inputVariable);
                if (it == scalars.end()) { fail("piecewise input"); return std::nullopt; }
                const double x = it->second;
                if ((piece.hasLo && !(x >= piece.lo)) || (piece.hasHi && !(x <= piece.hi))) continue;
            }
            const auto v = eval(*piece.mathNode);
            if (!v || vec(*v)) { fail("density is not scalar"); return std::nullopt; }
            return s(*v);
        }
        return 0.0;
    }
};

WGPUBuffer makeBuffer(wgpu::Device& gpu, uint64_t bytes, WGPUBufferUsage usage) {
    WGPUBufferDescriptor d = {};
    d.size = std::max<uint64_t>(16, (bytes + 15) & ~uint64_t(15));
    d.usage = usage;
    return wgpuDeviceCreateBuffer(gpu.device, &d);
}

WGPUComputePipeline makePipeline(wgpu::Device& gpu, const std::string& wgsl, const char* entry, std::string& error,
                                 WGPUShaderModule& module) {
    wgpuDevicePushErrorScope(gpu.device, WGPUErrorFilter_Validation);
    WGPUShaderSourceWGSL src = {};
    src.chain.sType = WGPUSType_ShaderSourceWGSL;
    src.code = wgpu::Device::str(wgsl.c_str());
    WGPUShaderModuleDescriptor smd = {};
    smd.nextInChain = &src.chain;
    module = wgpuDeviceCreateShaderModule(gpu.device, &smd);
    WGPUComputePipelineDescriptor cpd = {};
    cpd.compute.module = module;
    cpd.compute.entryPoint = wgpu::Device::str(entry);
    WGPUComputePipeline cp = module ? wgpuDeviceCreateComputePipeline(gpu.device, &cpd) : nullptr;
    PopResult pr;
    WGPUPopErrorScopeCallbackInfo pcb = {};
    pcb.mode = WGPUCallbackMode_AllowProcessEvents;
    pcb.callback = onPopError;
    pcb.userdata1 = &pr;
    wgpuDevicePopErrorScope(gpu.device, pcb);
    while (!pr.done) wgpuInstanceProcessEvents(gpu.instance);
    if (pr.type != WGPUErrorType_NoError) {
        error = pr.message;
        if (cp) wgpuComputePipelineRelease(cp);
        return nullptr;
    }
    return cp;
}

WGPUBindGroup bind(wgpu::Device& gpu, WGPUComputePipeline cp, uint32_t group,
                   const std::vector<std::pair<uint32_t, std::pair<WGPUBuffer, uint64_t>>>& entries) {
    std::vector<WGPUBindGroupEntry> e(entries.size());
    for (std::size_t i = 0; i < entries.size(); ++i) {
        e[i] = {};
        e[i].binding = entries[i].first;
        e[i].buffer = entries[i].second.first;
        e[i].size = entries[i].second.second;
    }
    WGPUBindGroupLayout layout = wgpuComputePipelineGetBindGroupLayout(cp, group);
    WGPUBindGroupDescriptor d = {};
    d.layout = layout;
    d.entryCount = e.size();
    d.entries = e.data();
    WGPUBindGroup bg = wgpuDeviceCreateBindGroup(gpu.device, &d);
    wgpuBindGroupLayoutRelease(layout);
    return bg;
}

} // namespace

int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    std::printf("Starting webgpu_volume_faithfulness_test...\n");
    {
        // The reference noise is the authored one: against float glm::perlin it
        // may differ only by the continuous part's f32 rounding.
        std::mt19937 r(7u);
        std::uniform_real_distribution<float> w(-40.0f, 40.0f);
        double worst = 0.0;
        for (int i = 0; i < 100000; ++i) {
            const glm::vec3 q(w(r), w(r), w(r));
            worst = std::max(worst, std::abs(double(glm::perlin(q)) - perlinReference(glm::dvec3(q))));
        }
        std::printf("    reference noise vs float glm::perlin: max difference %.3g over 100000 points\n", worst);
        check(worst < 1e-5, "the reference noise is the authored glm::perlin up to f32 rounding of its continuous part");
    }
    wgpu::Device gpu;
    if (!gpu.init()) { std::printf("SKIP: no WebGPU device\n"); return 0; }

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

    constexpr int kRays = 64, kSamples = 96;
    const double clocks[] = {0.0, 0.75};
    std::mt19937 rng(20261018u);
    std::uniform_real_distribution<double> u(-1.0, 1.0);
    uint32_t media = 0, measured = 0, refused = 0, nonFinite = 0;
    double worstNormalized = 0.0;
    std::string worstWho;

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
            ++media;
            sdfwgsl::VolumeProgramInput input;
            input.densityExpr = b.densityExpr;
            input.extinctionExpr = b.extinctionExpr;
            input.scatteringExpr = b.scatteringExpr;
            input.volumeChromaExpr = b.volumeChromaExpr;
            input.phaseExpr = b.phaseExpr;
            input.emissionExpr = b.emissionExpr;
            const auto program = sdfwgsl::composeVolumeSet({input}, {}, false);
            if (!program.ok || program.wgsl.find("fn volumeDensityEval(") == std::string::npos) {
                std::printf("    %s: no single-medium program (%s)\n", b.producerId.c_str(), program.error.substr(0, 100).c_str());
                ++refused;
                continue;
            }
            const auto prelude = sdfwgsl::inspectVolumeSharedPrelude(b.densityExpr, b.extinctionExpr, b.scatteringExpr,
                                                                     b.volumeChromaExpr, b.phaseExpr, b.emissionExpr);
            const bool fill = prelude.densityUses && program.wgsl.find("fn volumeSharedEval(") != std::string::npos;
            const std::string wgsl = program.wgsl + R"WGSL(
@group(2) @binding(0) var<storage, read> fidelityRays: array<vec4<f32>>;
@group(2) @binding(1) var<storage, read_write> fidelityOut: array<f32>;
@compute @workgroup_size(64)
fn fidelityMain(@builtin(global_invocation_id) id: vec3<u32>) {
    if (id.x >= arrayLength(&fidelityOut)) { return; }
    g_instIdx = 0u;
    let inst = instances[0u];
    let ro = fidelityRays[2u * id.x].xyz;
    let sampleT = fidelityRays[2u * id.x].w;
    let rd = fidelityRays[2u * id.x + 1u].xyz;
    let worldP = ro + rd * sampleT;
    let p = worldP - inst.origin.xyz;
)WGSL" + std::string(fill ? "    volumeSharedEval(p);\n" : "") + R"WGSL(
    fidelityOut[id.x] = volumeDensityEval(p);
    if (id.x == 0xffffffffu) { fidelityOut[0] = P.v[0] + inst.time.x; }
}
)WGSL";
            std::string error;
            WGPUShaderModule sm = nullptr, prepModule = nullptr;
            WGPUComputePipeline cp = makePipeline(gpu, wgsl, "fidelityMain", error, sm);
            WGPUComputePipeline prep = nullptr;
            if (cp && !program.kernelVolumePreparationWgsl.empty())
                prep = makePipeline(gpu, program.kernelVolumePreparationWgsl, "volumeTimePrepare", error, prepModule);
            if (!cp || (!program.kernelVolumePreparationWgsl.empty() && !prep)) {
                std::printf("    %s: pipeline refused: %s\n", b.producerId.c_str(), error.substr(0, 160).c_str());
                ++refused;
                if (cp) wgpuComputePipelineRelease(cp);
                if (sm) wgpuShaderModuleRelease(sm);
                if (prepModule) wgpuShaderModuleRelease(prepModule);
                continue;
            }

            const glm::dvec3 origin(b.origin), half = glm::abs(glm::dvec3(b.scale));
            const uint32_t n = kRays * kSamples;
            const uint64_t paramBytes = std::max<std::size_t>(program.params.size(), 4) * sizeof(float);
            WGPUBuffer paramBuf = makeBuffer(gpu, paramBytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
            WGPUBuffer instBuf = makeBuffer(gpu, 80, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
            WGPUBuffer rayBuf = makeBuffer(gpu, uint64_t(n) * 2 * sizeof(glm::vec4), WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
            WGPUBuffer outBuf = makeBuffer(gpu, uint64_t(n) * sizeof(float), WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc);
            WGPUBuffer readBuf = makeBuffer(gpu, uint64_t(n) * sizeof(float), WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead);

            double worstAbs = 0.0, scale = 0.0, sumSq = 0.0;
            uint64_t count = 0;
            Ref ref;
            for (const double t : clocks) {
                std::vector<float> params = program.params;
                params.resize(std::max<std::size_t>(params.size(), 4), 0.0f);
                wgpuQueueWriteBuffer(gpu.queue, paramBuf, 0, params.data(), params.size() * sizeof(float));
                float inst[20] = {};
                inst[0] = float(origin.x); inst[1] = float(origin.y); inst[2] = float(origin.z);
                inst[4] = float(half.x); inst[5] = float(half.y); inst[6] = float(half.z);
                inst[8] = float(t);
                wgpuQueueWriteBuffer(gpu.queue, instBuf, 0, inst, sizeof(inst));
                // Rays from outside the box through random interior points, at the
                // medium's real world origin; samples on the marcher's own lattice.
                std::vector<glm::vec4> rays;
                rays.reserve(n * 2);
                std::vector<glm::dvec3> refP;
                refP.reserve(n);
                for (int r = 0; r < kRays; ++r) {
                    glm::dvec3 dir(u(rng), u(rng), u(rng));
                    if (glm::length(dir) < 1e-3) dir = glm::dvec3(0.3, 0.2, 1.0);
                    const glm::vec3 rd = glm::normalize(glm::vec3(dir));
                    const glm::dvec3 target = half * glm::dvec3(u(rng), u(rng), u(rng));
                    const glm::vec3 ro = glm::vec3(origin + target - glm::dvec3(rd) * (3.0 * glm::length(half) + 1.0));
                    const glm::vec3 lo = glm::vec3(origin - half), hi = glm::vec3(origin + half);
                    float t0 = -3.0e38f, t1 = 3.0e38f;
                    for (int a = 0; a < 3; ++a) {
                        const float inv = 1.0f / (std::abs(rd[a]) < 1e-8f ? 1e-8f : rd[a]);
                        float ta = (lo[a] - ro[a]) * inv, tb = (hi[a] - ro[a]) * inv;
                        if (ta > tb) std::swap(ta, tb);
                        t0 = std::max(t0, ta); t1 = std::min(t1, tb);
                    }
                    t0 = std::max(t0, 0.0f);
                    const float step = (t1 - t0) / float(kSamples);
                    for (int i = 0; i < kSamples; ++i) {
                        const float sampleT = t0 + (float(i) + 0.5f) * step;
                        rays.push_back(glm::vec4(ro, sampleT));
                        rays.push_back(glm::vec4(rd, 0.0f));
                        refP.push_back(glm::dvec3(ro) + glm::dvec3(rd) * double(sampleT) - glm::dvec3(glm::vec3(origin)));
                    }
                }
                wgpuQueueWriteBuffer(gpu.queue, rayBuf, 0, rays.data(), rays.size() * sizeof(glm::vec4));

                WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
                std::vector<WGPUBindGroup> groups;
                if (prep) {
                    groups.push_back(bind(gpu, prep, 0, {{1u, {paramBuf, paramBytes}}}));
                    groups.push_back(bind(gpu, prep, 1, {{0u, {instBuf, 80}}}));
                    WGPUComputePassEncoder pass = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                    wgpuComputePassEncoderSetPipeline(pass, prep);
                    wgpuComputePassEncoderSetBindGroup(pass, 0, groups[0], 0, nullptr);
                    wgpuComputePassEncoderSetBindGroup(pass, 1, groups[1], 0, nullptr);
                    wgpuComputePassEncoderDispatchWorkgroups(pass, 1, 1, 1);
                    wgpuComputePassEncoderEnd(pass);
                    wgpuComputePassEncoderRelease(pass);
                }
                const std::size_t base = groups.size();
                groups.push_back(bind(gpu, cp, 0, {{1u, {paramBuf, paramBytes}}}));
                groups.push_back(bind(gpu, cp, 1, {{0u, {instBuf, 80}}}));
                groups.push_back(bind(gpu, cp, 2, {{0u, {rayBuf, uint64_t(n) * 2 * sizeof(glm::vec4)}},
                                                   {1u, {outBuf, uint64_t(n) * sizeof(float)}}}));
                WGPUComputePassEncoder pass = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                wgpuComputePassEncoderSetPipeline(pass, cp);
                for (uint32_t g = 0; g < 3; ++g) wgpuComputePassEncoderSetBindGroup(pass, g, groups[base + g], 0, nullptr);
                wgpuComputePassEncoderDispatchWorkgroups(pass, (n + 63) / 64, 1, 1);
                wgpuComputePassEncoderEnd(pass);
                wgpuComputePassEncoderRelease(pass);
                wgpuCommandEncoderCopyBufferToBuffer(enc, outBuf, 0, readBuf, 0, uint64_t(n) * sizeof(float));
                WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(enc, nullptr);
                wgpuQueueSubmit(gpu.queue, 1, &cmd);
                wgpuCommandBufferRelease(cmd);
                wgpuCommandEncoderRelease(enc);
                MapR mr;
                WGPUBufferMapCallbackInfo mci = {};
                mci.mode = WGPUCallbackMode_AllowProcessEvents;
                mci.callback = onMap;
                mci.userdata1 = &mr;
                wgpuBufferMapAsync(readBuf, WGPUMapMode_Read, 0, uint64_t(n) * sizeof(float), mci);
                while (!mr.done) wgpuInstanceProcessEvents(gpu.instance);
                const auto* out = static_cast<const float*>(wgpuBufferGetConstMappedRange(readBuf, 0, uint64_t(n) * sizeof(float)));
                for (uint32_t i = 0; i < n; ++i) {
                    ref.p = refP[i];
                    ref.t = t;
                    ref.scalars = {{"x", ref.p.x}, {"y", ref.p.y}, {"z", ref.p.z}, {OntoMath::kTimeVar, t},
                                   {OntoMath::kWiXVar, 0.0}, {OntoMath::kWiYVar, 0.0}, {OntoMath::kWiZVar, 0.0},
                                   {OntoMath::kWoXVar, 0.0}, {OntoMath::kWoYVar, 0.0}, {OntoMath::kWoZVar, 1.0},
                                   {OntoMath::kOmegaXVar, 0.0}, {OntoMath::kOmegaYVar, 0.0}, {OntoMath::kOmegaZVar, 1.0}};
                    const auto exact = ref.density(*b.densityExpr);
                    if (!exact) continue;
                    const double dev = double(out[i]);
                    if (!std::isfinite(dev)) { ++nonFinite; continue; }
                    const double err = std::abs(dev - *exact);
                    worstAbs = std::max(worstAbs, err);
                    scale = std::max(scale, std::abs(*exact));
                    sumSq += err * err;
                    ++count;
                }
                wgpuBufferUnmap(readBuf);
                for (auto* g : groups) wgpuBindGroupRelease(g);
            }
            for (auto* buf : {paramBuf, instBuf, rayBuf, outBuf, readBuf}) wgpuBufferRelease(buf);
            wgpuComputePipelineRelease(cp);
            if (prep) wgpuComputePipelineRelease(prep);
            wgpuShaderModuleRelease(sm);
            if (prepModule) wgpuShaderModuleRelease(prepModule);
            if (count == 0) {
                std::printf("    %s: reference refused (%s)\n", b.producerId.c_str(), ref.refusal.c_str());
                ++refused;
                continue;
            }
            ++measured;
            const double normalized = scale > 0.0 ? worstAbs / scale : worstAbs;
            if (normalized > worstNormalized) { worstNormalized = normalized; worstWho = b.producerId; }
            std::printf("    %s: %llu samples, max |D_device - D_exact| %.3g (rms %.3g), max |D| %.3g -> %.3g of the density scale%s\n",
                        b.producerId.c_str(), (unsigned long long)count, worstAbs, std::sqrt(sumSq / double(count)),
                        scale, normalized, prep ? "; clock leaves prepared on the device" : "");
        }
    }
    std::printf("    %u media, %u measured against the double-precision authored density, %u refused; worst %.3g of "
                "its density scale (%s); %u non-finite device values\n",
                media, measured, refused, worstNormalized, worstWho.c_str(), nonFinite);
    check(measured > 0 && measured * 10 >= media * 8, "at least 80% of real media are measured against the authored equation");
    check(nonFinite == 0, "the device density is finite at every sampled point");
    std::printf("webgpu_volume_faithfulness_test: %d/%d checks passed\n", g_checks - g_failures, g_checks);
    return g_failures == 0 ? 0 : 1;
}
