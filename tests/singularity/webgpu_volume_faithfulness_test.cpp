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
#include "Singularity/OntoMath/LineRestriction.hpp"
#include "Singularity/Screen/VolumeDensity.hpp"
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"
#include "support/test_harness.hpp"

#include <webgpu/wgpu.h>
#include <glm/gtc/noise.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
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
    uint32_t media = 0, measured = 0, refused = 0, nonFinite = 0, lessFaithful = 0;
    double sumRatio[2] = {};
    double quadratureWorst = 0.0, quadratureWorst2 = 0.0;
    uint32_t quadratureMeasured = 0;
    // Transmittance by OntoMath's integration office (2026-10-19): the renderer's
    // own sigma integrated by hybridIntegral on segmentLine's pieces, against the
    // 96-sample midpoint sum the lattice computes, both against the continuum.
    double lineWorst96 = 0.0, lineWorstHybrid = 0.0;
    uint32_t lineMedia = 0, lineBetter = 0, lineMisses = 0;
    std::string lineWorstWho;
    // In-scattered light under a unit, unshadowed white light with phase 1: the
    // marcher's (σ_s/σ_t)(T_before − T_after) rule against ∫ σ_s·T ds by line
    // integration, T = e^(−τ) handed to hybridIntegral as a sampled factor.
    double scatterWorst96 = 0.0, scatterWorstLine = 0.0;
    uint32_t scatterMedia = 0, scatterBetter = 0, scatterMisses = 0;
    std::string scatterWorstWho;
    std::string quadratureWho;
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
@group(2) @binding(2) var<storage, read_write> fidelityGap: array<f32>;
@compute @workgroup_size(64)
fn fidelityMain(@builtin(global_invocation_id) id: vec3<u32>) {
    if (2u * id.x + 1u >= arrayLength(&fidelityOut)) { return; }
    g_instIdx = 0u;
    let inst = instances[0u];
    let ro = fidelityRays[2u * id.x].xyz;
    let t0 = fidelityRays[2u * id.x].w;
    let rd = fidelityRays[2u * id.x + 1u].xyz;
    let offset = fidelityRays[2u * id.x + 1u].w;
    // The marcher's world-frame point, and the medium-frame (Rung 3a) one.
    let sampleT = t0 + offset;
    let pWorld = (ro + rd * sampleT) - inst.origin.xyz;
    // Medium frame: once per ray at the camera's distance, then the box's own magnitude.
    let pLocal = ((ro - inst.origin.xyz) + rd * t0) + rd * offset;
)WGSL" + std::string(fill ? "    volumeSharedEval(pWorld);\n" : "") + R"WGSL(
    fidelityOut[2u * id.x] = volumeDensityEval(pWorld);
)WGSL" + std::string(fill ? "    volumeSharedEval(pLocal);\n" : "") + R"WGSL(
    fidelityOut[2u * id.x + 1u] = volumeDensityEval(pLocal);
    let gap = abs(pWorld - pLocal);
    fidelityGap[id.x] = max(gap.x, max(gap.y, gap.z));
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
            const uint64_t outBytes = uint64_t(n) * 2 * sizeof(float);
            WGPUBuffer outBuf = makeBuffer(gpu, outBytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc);
            WGPUBuffer readBuf = makeBuffer(gpu, outBytes, WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead);
            const uint64_t gapBytes = uint64_t(n) * sizeof(float);
            WGPUBuffer gapBuf = makeBuffer(gpu, gapBytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc);
            WGPUBuffer gapRead = makeBuffer(gpu, gapBytes, WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead);
            double gapMax = 0.0;
            uint64_t gapDiffer = 0, gapTotal = 0;

            // [camera set][formula]: set 0 near the box (3 radii out), set 1 far
            // (300 units out); formula 0 the marcher's world frame, 1 the medium's.
            double worstAbs[2][2] = {}, sumSq[2][2] = {}, scale = 0.0;
            uint64_t count[2] = {};
            Ref ref;
            for (int set = 0; set < 2; ++set)
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
                    const double out = set == 0 ? 3.0 * glm::length(half) + 1.0 : 300.0;
                    const glm::vec3 ro = glm::vec3(origin + target - glm::dvec3(rd) * out);
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
                        // The lattice point t0 + (i + ½)·step, exactly, from the f32
                        // t0 and step the marcher holds.
                        const float offset = (float(i) + 0.5f) * step;
                        rays.push_back(glm::vec4(ro, t0));
                        rays.push_back(glm::vec4(rd, offset));
                        const double exactT = double(t0) + (double(i) + 0.5) * double(step);
                        refP.push_back(glm::dvec3(ro) + glm::dvec3(rd) * exactT - glm::dvec3(glm::vec3(origin)));
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
                                                   {1u, {outBuf, outBytes}}, {2u, {gapBuf, gapBytes}}}));
                WGPUComputePassEncoder pass = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                wgpuComputePassEncoderSetPipeline(pass, cp);
                for (uint32_t g = 0; g < 3; ++g) wgpuComputePassEncoderSetBindGroup(pass, g, groups[base + g], 0, nullptr);
                wgpuComputePassEncoderDispatchWorkgroups(pass, (n + 63) / 64, 1, 1);
                wgpuComputePassEncoderEnd(pass);
                wgpuComputePassEncoderRelease(pass);
                wgpuCommandEncoderCopyBufferToBuffer(enc, outBuf, 0, readBuf, 0, outBytes);
                wgpuCommandEncoderCopyBufferToBuffer(enc, gapBuf, 0, gapRead, 0, gapBytes);
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
                const auto* out = static_cast<const float*>(wgpuBufferGetConstMappedRange(readBuf, 0, outBytes));
                for (uint32_t i = 0; i < n; ++i) {
                    ref.p = refP[i];
                    ref.t = t;
                    ref.scalars = {{"x", ref.p.x}, {"y", ref.p.y}, {"z", ref.p.z}, {OntoMath::kTimeVar, t},
                                   {OntoMath::kWiXVar, 0.0}, {OntoMath::kWiYVar, 0.0}, {OntoMath::kWiZVar, 0.0},
                                   {OntoMath::kWoXVar, 0.0}, {OntoMath::kWoYVar, 0.0}, {OntoMath::kWoZVar, 1.0},
                                   {OntoMath::kOmegaXVar, 0.0}, {OntoMath::kOmegaYVar, 0.0}, {OntoMath::kOmegaZVar, 1.0}};
                    const auto exact = ref.density(*b.densityExpr);
                    if (!exact) continue;
                    scale = std::max(scale, std::abs(*exact));
                    ++count[set];
                    for (int f = 0; f < 2; ++f) {
                        const double dev = double(out[2 * i + f]);
                        if (!std::isfinite(dev)) { ++nonFinite; continue; }
                        const double err = std::abs(dev - *exact);
                        worstAbs[set][f] = std::max(worstAbs[set][f], err);
                        sumSq[set][f] += err * err;
                    }
                }
                wgpuBufferUnmap(readBuf);
                {
                    MapR gr;
                    WGPUBufferMapCallbackInfo gci = {};
                    gci.mode = WGPUCallbackMode_AllowProcessEvents;
                    gci.callback = onMap;
                    gci.userdata1 = &gr;
                    wgpuBufferMapAsync(gapRead, WGPUMapMode_Read, 0, gapBytes, gci);
                    while (!gr.done) wgpuInstanceProcessEvents(gpu.instance);
                    const auto* g = static_cast<const float*>(wgpuBufferGetConstMappedRange(gapRead, 0, gapBytes));
                    for (uint32_t i = 0; i < n; ++i) {
                        gapMax = std::max(gapMax, double(g[i]));
                        gapDiffer += g[i] != 0.0f ? 1 : 0;
                        ++gapTotal;
                    }
                    wgpuBufferUnmap(gapRead);
                }
                for (auto* g : groups) wgpuBindGroupRelease(g);
            }
            for (auto* buf : {paramBuf, instBuf, rayBuf, outBuf, readBuf, gapBuf, gapRead}) wgpuBufferRelease(buf);
            wgpuComputePipelineRelease(cp);
            if (prep) wgpuComputePipelineRelease(prep);
            wgpuShaderModuleRelease(sm);
            if (prepModule) wgpuShaderModuleRelease(prepModule);
            if (count[0] == 0 || count[1] == 0) {
                std::printf("    %s: reference refused (%s)\n", b.producerId.c_str(), ref.refusal.c_str());
                ++refused;
                continue;
            }
            ++measured;
            {
                // Quadrature against the continuum (CPU, double): the optical depth the
                // marcher integrates, tau = sum sigma(s_i)·step over 96 midpoints, with
                // sigma = D > 0 ? max(sigma_t, 1e-6) : 0 (sigma_t authored, or 0.5·D),
                // against the converged integral of the same authored sigma.
                auto sigmaAt = [&](const glm::dvec3& q, double t) -> std::optional<double> {
                    ref.p = q;
                    ref.scalars["x"] = q.x; ref.scalars["y"] = q.y; ref.scalars["z"] = q.z;
                    ref.scalars[OntoMath::kTimeVar] = t;
                    const auto D = ref.density(*b.densityExpr);
                    if (!D) return std::nullopt;
                    if (!(*D > 0.0)) return 0.0;
                    if (!b.extinctionExpr || b.extinctionExpr->pieces.empty()) return std::max(0.5 * *D, 1e-6);
                    const auto e = ref.density(*b.extinctionExpr);
                    if (!e) return std::nullopt;
                    return std::max(*e, 1e-6);
                };
                double worstT = 0.0, worstTau = 0.0, worstConv = 0.0;
                bool ok = true;
                std::mt19937 qrng(99u);
                // The same sigma on OntoMath's line algebra: D's sign by segmentLine,
                // sigma_t (authored extinction, or 0.5·D) by hybridIntegral.
                const auto lineD = OntoMath::restrictAlgebraicToLine(*b.densityExpr, false);
                const bool authoredSigma = b.extinctionExpr && !b.extinctionExpr->pieces.empty();
                const auto lineE = authoredSigma ? OntoMath::restrictAlgebraicToLine(*b.extinctionExpr, false)
                                                 : OntoMath::LineAlgebraicRestriction{};
                const bool lineOk = bool(lineD) && (!authoredSigma || bool(lineE));
                double line96 = 0.0, lineHybrid = 0.0, lineEvals = 0.0;
                uint32_t lineRays = 0, lineMiss = 0;
                const bool authoredScatter = b.scatteringExpr && !b.scatteringExpr->pieces.empty();
                const auto lineS = authoredScatter ? OntoMath::restrictAlgebraicToLine(*b.scatteringExpr, false)
                                                   : OntoMath::LineAlgebraicRestriction{};
                // The marcher's no-source branch (no incident source bound): white
                // incident light, phase(p, wi = 0, wo) — 0 when phase reads wi —
                // medium chroma, and emission(p, omega), omega toward the camera.
                auto restrictIf = [](const OntoMath::Piecewise* pw, bool vec) {
                    return pw && !pw->pieces.empty() ? OntoMath::restrictAlgebraicToLine(*pw, vec) : OntoMath::LineAlgebraicRestriction{};
                };
                const bool hasChroma = b.volumeChromaExpr && !b.volumeChromaExpr->pieces.empty();
                const bool hasPhase = b.phaseExpr && !b.phaseExpr->pieces.empty();
                const bool hasEmission = b.emissionExpr && !b.emissionExpr->pieces.empty();
                const auto lineC = restrictIf(b.volumeChromaExpr, true);
                const auto lineP = restrictIf(b.phaseExpr, false);
                const auto lineM = restrictIf(b.emissionExpr, true);
                const bool phaseReadsWi = hasPhase && bool(lineP) &&
                    (lineP.form->reads.count(OntoMath::kWiXVar) || lineP.form->reads.count(OntoMath::kWiYVar) ||
                     lineP.form->reads.count(OntoMath::kWiZVar));
                const bool scatterOk = lineOk && (!authoredScatter || bool(lineS)) && (!hasChroma || bool(lineC)) &&
                                       (!hasPhase || bool(lineP)) && (!hasEmission || bool(lineM));
                if (lineOk && !scatterOk)
                    std::printf("    %s colour by line integration: not admitted (%s)\n", b.producerId.c_str(),
                                authoredScatter && !lineS ? lineS.refusal.c_str() : hasChroma && !lineC ? lineC.refusal.c_str()
                                : hasPhase && !lineP ? lineP.refusal.c_str() : lineM.refusal.c_str());
                double scatter96 = 0.0, scatterLine = 0.0;
                uint32_t scatterRays = 0, scatterMiss = 0;
                for (int r = 0; r < 16 && ok; ++r) {
                    glm::dvec3 dir(u(qrng), u(qrng), u(qrng));
                    if (glm::length(dir) < 1e-3) dir = glm::dvec3(0.3, 0.2, 1.0);
                    dir = glm::normalize(dir);
                    const glm::dvec3 target = half * glm::dvec3(u(qrng), u(qrng), u(qrng));
                    const glm::dvec3 o = target - dir * (3.0 * glm::length(half) + 1.0);   // medium frame
                    double t0 = -1e300, t1 = 1e300;
                    for (int a = 0; a < 3; ++a) {
                        if (std::abs(dir[a]) < 1e-12) continue;
                        double ta = (-half[a] - o[a]) / dir[a], tb = (half[a] - o[a]) / dir[a];
                        if (ta > tb) std::swap(ta, tb);
                        t0 = std::max(t0, ta); t1 = std::min(t1, tb);
                    }
                    if (!(t1 > t0)) continue;
                    auto midpoint = [&](int m) -> std::optional<double> {
                        const double h = (t1 - t0) / m;
                        double sum = 0.0;
                        for (int i = 0; i < m; ++i) {
                            const auto sg = sigmaAt(o + dir * (t0 + (i + 0.5) * h), clocks[1]);
                            if (!sg) return std::nullopt;
                            sum += *sg * h;
                        }
                        return sum;
                    };
                    const auto tau96 = midpoint(kSamples), tauA = midpoint(8192), tauB = midpoint(16384);
                    if (!tau96 || !tauA || !tauB) { ok = false; break; }
                    worstConv = std::max(worstConv, std::abs(std::exp(-*tauA) - std::exp(-*tauB)));
                    worstT = std::max(worstT, std::abs(std::exp(-*tau96) - std::exp(-*tauB)));
                    if (*tauB > 1e-9) worstTau = std::max(worstTau, std::abs(*tau96 - *tauB) / *tauB);
                    if (!lineOk) continue;
                    std::map<std::string, double> bind;
                    for (int a = 0; a < 3; ++a) {
                        bind[OntoMath::kLineOriginVars[a]] = o[a];
                        bind[OntoMath::kLineDirectionVars[a]] = dir[a];
                    }
                    bind[OntoMath::kTimeVar] = clocks[1];
                    for (const char* v : {OntoMath::kWiXVar, OntoMath::kWiYVar, OntoMath::kWiZVar}) bind[v] = 0.0;
                    bind[OntoMath::kWoXVar] = -dir.x; bind[OntoMath::kWoYVar] = -dir.y; bind[OntoMath::kWoZVar] = -dir.z;
                    bind[OntoMath::kOmegaXVar] = -dir.x; bind[OntoMath::kOmegaYVar] = -dir.y; bind[OntoMath::kOmegaZVar] = -dir.z;
                    OntoMath::LineAlgebraicEvaluator evD(*lineD.form, bind, true);
                    std::optional<OntoMath::LineAlgebraicEvaluator> evE;
                    if (authoredSigma) evE.emplace(*lineE.form, bind, true);
                    if (!evD.ok() || (evE && !evE->ok())) continue;
                    const uint32_t rootD = lineD.form->roots[0], rootE = authoredSigma ? lineE.form->roots[0] : 0;
                    auto sigma = [&](double s) {
                        const double D = evD.value(rootD, s).value_or(0.0);
                        if (!(D > 0.0)) return 0.0;
                        const double st = evE ? evE->value(rootE, s).value_or(0.0) : 0.5 * D;
                        return std::max(st, 1e-6);
                    };
                    // The midpoint sum, and how many times D's sign flips between its
                    // samples and the largest sigma it saw (for the reference's bound).
                    int jumps = 0;
                    double sigmaMax = 0.0;
                    auto mid = [&](int m) {
                        const double h = (t1 - t0) / m;
                        double sum = 0.0;
                        bool wasPositive = false;
                        jumps = 0;
                        for (int i = 0; i < m; ++i) {
                            const double sg = sigma(t0 + (i + 0.5) * h);
                            sum += sg * h;
                            if (i > 0 && (sg > 0.0) != wasPositive) ++jumps;
                            wasPositive = sg > 0.0;
                            sigmaMax = std::max(sigmaMax, sg);
                        }
                        return sum;
                    };
                    // An authored sigma_t jumps where D crosses zero (0.16 → 0 for the
                    // Borealis clouds): the midpoint sum is then first order and
                    // Richardson's second-order correction is invalid. Such a reference is
                    // the plain 2^18 sum, carrying (jumps × sup sigma × h) plus its
                    // observed change from 2^17. 0.5·D is continuous there: Richardson.
                    double reference, referenceError;
                    if (authoredSigma) {
                        const double coarse = mid(1 << 17);
                        reference = mid(1 << 18);
                        // + the summation's own rounding, N·ε of the sum.
                        referenceError = jumps * sigmaMax * (t1 - t0) / double(1 << 18) + std::abs(reference - coarse) +
                                         double(1 << 18) * std::numeric_limits<double>::epsilon() * reference;
                    } else {
                        const double fine = mid(32768);
                        reference = fine + (fine - mid(16384)) / 3.0;
                        referenceError = 1e-6 * reference;
                    }
                    const double plain = mid(kSamples);
                    const std::vector<std::pair<std::size_t, uint32_t>> signTarget{{0, rootD}};
                    const auto pieces = OntoMath::segmentLine({&evD}, t0, t1, 1e-7 * (t1 - t0), nullptr, 1u << 16, true, nullptr, &signTarget);
                    double dense = 0.0;
                    for (const auto& pc : pieces)
                        if (pc.definite && pc.targetSigns[0] > 0) dense += pc.s1 - pc.s0;
                    double est = 0.0, rad = 0.0;
                    bool pieceOk = true;
                    const std::vector<int8_t> openE(authoredSigma ? lineE.form->switches.size() : 0, 0);
                    for (const auto& pc : pieces) {
                        if (pc.targetSigns[0] < 0) continue;   // D proved ≤ 0: sigma is 0
                        const double w = pc.s1 - pc.s0;
                        bool undefined = false;
                        const auto rd = evD.range(rootD, pc.s0, pc.s1, pc.signs[0], &undefined);
                        if (undefined) { pieceOk = false; break; }
                        if (!pc.definite) {
                            // A root sliver: 0 ≤ ∫sigma ≤ w·sup sigma.
                            double hi = 0.5 * std::max(0.0, double(rd.hi));
                            if (evE) { bool u2 = false; hi = std::max(0.0, double(evE->range(rootE, pc.s0, pc.s1, openE, &u2).hi)); }
                            hi = std::max(hi, 1e-6) * w;
                            est += 0.5 * hi; rad += 0.5 * hi;
                            continue;
                        }
                        const uint32_t cells = std::max<uint32_t>(1, uint32_t(std::lround(95.0 * w / std::max(dense, 1e-300))));
                        if (!evE) {
                            // sigma = max(0.5·D, 1e-6) with D > 0 proved: 0.5∫D, plus what the
                            // floor adds — exactly nothing where 0.5·D ≥ 1e-6 is proved.
                            const auto h = evD.hybridIntegral(rootD, pc.s0, pc.s1, pc.signs[0], cells);
                            if (!h) { pieceOk = false; break; }
                            const double floorGap = 0.5 * double(rd.lo) >= 1e-6 ? 0.0 : 1e-6 * w;
                            est += 0.5 * h->estimate + 0.5 * floorGap;
                            rad += 0.5 * h->radius + 0.5 * floorGap;
                            lineEvals += h->remainderEvaluations;
                            continue;
                        }
                        // Authored sigma_t: its integral, plus what the 1e-6 floor can add.
                        const auto h = evE->hybridIntegral(rootE, pc.s0, pc.s1, openE, cells);
                        bool u2 = false;
                        const auto re = evE->range(rootE, pc.s0, pc.s1, openE, &u2);
                        if (!h || u2) { pieceOk = false; break; }
                        const double floorGap = std::max(0.0, 1e-6 - double(re.lo)) * w;
                        est += h->estimate + 0.5 * floorGap;
                        rad += h->radius + 0.5 * floorGap;
                        lineEvals += h->remainderEvaluations;
                    }
                    if (!pieceOk) continue;
                    ++lineRays;
                    line96 = std::max(line96, std::abs(std::exp(-plain) - std::exp(-reference)));
                    lineHybrid = std::max(lineHybrid, std::abs(std::exp(-est) - std::exp(-reference)));
                    if (std::abs(est - reference) > rad + referenceError + 1e-12) ++lineMiss;

                    // ---- Colour (RGB): the marcher's no-source model, every authored factor ----
                    if (!scatterOk) continue;
                    using Ev = OntoMath::LineAlgebraicEvaluator;
                    std::optional<Ev> evS, evC, evP, evM;
                    if (authoredScatter) evS.emplace(*lineS.form, bind, true);
                    if (hasChroma) evC.emplace(*lineC.form, bind, true);
                    if (hasPhase) evP.emplace(*lineP.form, bind, true);
                    if (hasEmission) evM.emplace(*lineM.form, bind, true);
                    if ((evS && !evS->ok()) || (evC && !evC->ok()) || (evP && !evP->ok()) || (evM && !evM->ok())) continue;
                    const uint32_t rootS = authoredScatter ? lineS.form->roots[0] : 0, rootP = hasPhase ? lineP.form->roots[0] : 0;
                    auto openOf = [](const std::optional<Ev>& ev) { return std::vector<int8_t>(ev ? ev->form().switches.size() : 0, 0); };
                    const auto openS = openOf(evS), openC = openOf(evC), openP = openOf(evP), openM = openOf(evM);
                    auto sigmaS = [&](double s) {
                        const double D = evD.value(rootD, s).value_or(0.0);
                        if (!(D > 0.0)) return 0.0;
                        return evS ? std::max(evS->value(rootS, s).value_or(0.0), 0.0) : D;
                    };
                    auto chromaAt = [&](int c, double s) { return evC ? evC->value(lineC.form->roots[c], s).value_or(0.0) : 1.0; };
                    auto phaseAt = [&](double s) {
                        if (!evP) return 1.0;
                        if (phaseReadsWi) return 0.0;
                        return std::max(evP->value(rootP, s).value_or(0.0), 0.0);
                    };
                    auto emissionAt = [&](int c, double s) { return evM ? std::max(evM->value(lineM.form->roots[c], s).value_or(0.0), 0.0) : 0.0; };
                    auto marcherColour = [&](int n) {
                        const double hs = (t1 - t0) / n;
                        double T = 1.0;
                        std::array<double, 3> L{0.0, 0.0, 0.0};
                        for (int i = 0; i < n; ++i) {
                            const double sm = t0 + (i + 0.5) * hs, st = sigma(sm);
                            if (!(st > 0.0)) continue;
                            const double Tn = T * std::exp(-st * hs);
                            const double sc = sigmaS(sm) / st * phaseAt(sm) * (T - Tn);
                            for (int c = 0; c < 3; ++c) L[c] += chromaAt(c, sm) * sc + emissionAt(c, sm) * (T - Tn) / st;
                            T = Tn;
                        }
                        return L;
                    };
                    const auto L96 = marcherColour(kSamples), L17 = marcherColour(1 << 17), refL = marcherColour(1 << 18);
                    // ∫ sigma_t over [p, q] inside a piece where D > 0 is proved, with
                    // the floor's contribution in the radius (as for transmittance).
                    auto sigmaTIntegral = [&](const OntoMath::LineSegmentationPiece& pc, double p0, double q0, uint32_t c)
                        -> std::optional<std::pair<double, double>> {
                        const double w2 = q0 - p0;
                        if (!evE) {
                            bool u2 = false;
                            const auto rdp = evD.range(rootD, p0, q0, pc.signs[0], &u2);
                            const auto hh = evD.hybridIntegral(rootD, p0, q0, pc.signs[0], c);
                            if (!hh || u2) return std::nullopt;
                            const double gap = 0.5 * double(rdp.lo) >= 1e-6 ? 0.0 : 1e-6 * w2;
                            return std::make_pair(0.5 * hh->estimate + 0.5 * gap, 0.5 * hh->radius + 0.5 * gap);
                        }
                        bool u2 = false;
                        const auto rep2 = evE->range(rootE, p0, q0, openE, &u2);
                        const auto hh = evE->hybridIntegral(rootE, p0, q0, openE, c);
                        if (!hh || u2) return std::nullopt;
                        const double gap = std::max(0.0, 1e-6 - double(rep2.lo)) * w2;
                        return std::make_pair(hh->estimate + 0.5 * gap, hh->radius + 0.5 * gap);
                    };
                    auto sigmaTSup = [&](const OntoMath::LineSegmentationPiece& pc, double p0, double q0) {
                        bool u2 = false;
                        if (!evE) return std::max(1e-6, 0.5 * double(evD.range(rootD, p0, q0, pc.signs[0], &u2).hi));
                        return std::max(1e-6, double(evE->range(rootE, p0, q0, openE, &u2).hi));
                    };
                    // Proved range and slope of an optional node over [p, q] (the
                    // constant `absent` when not authored; max(·, 0) when clamped).
                    struct Bound { double lo, hi, slope; };
                    auto boundOf = [&](const std::optional<Ev>& ev, const std::vector<int8_t>& open, uint32_t root, double p0, double q0,
                                       double absent, bool clampZero) -> Bound {
                        if (!ev) return {absent, absent, 0.0};
                        bool u2 = false;
                        const auto r = ev->range(root, p0, q0, open, &u2);
                        const auto d = ev->derivativeRange(root, p0, q0, open);
                        const double inf = std::numeric_limits<double>::infinity();
                        Bound bd{u2 ? -inf : double(r.lo), u2 ? inf : double(r.hi),
                                 std::max(std::abs(double(d.lo)), std::abs(double(d.hi)))};
                        if (clampZero) { bd.lo = std::max(bd.lo, 0.0); bd.hi = std::max(bd.hi, 0.0); }
                        return bd;
                    };
                    auto supAbsOf = [](const Bound& bd) { return std::max(std::abs(bd.lo), std::abs(bd.hi)); };
                    std::array<double, 3> Lest{0, 0, 0}, Lrad{0, 0, 0};
                    double tau = 0.0, tauRad = 0.0;
                    bool colourOk = true;
                    for (const auto& pc : pieces) {
                        if (pc.targetSigns[0] < 0) continue;
                        const double w = pc.s1 - pc.s0;
                        bool undefined = false;
                        const auto rd = evD.range(rootD, pc.s0, pc.s1, pc.signs[0], &undefined);
                        if (undefined) { colourOk = false; break; }
                        if (!pc.definite) {
                            // A root sliver: each channel's light in [0, sup(integrand)·T·w].
                            const auto bS = evS ? boundOf(evS, openS, rootS, pc.s0, pc.s1, 0.0, true) : Bound{0.0, std::max(0.0, double(rd.hi)), 0.0};
                            const auto bP = phaseReadsWi ? Bound{0, 0, 0} : boundOf(evP, openP, rootP, pc.s0, pc.s1, 1.0, true);
                            const double supT = sigmaTSup(pc, pc.s0, pc.s1);
                            for (int c = 0; c < 3; ++c) {
                                const auto bC = boundOf(evC, openC, evC ? lineC.form->roots[c] : 0, pc.s0, pc.s1, 1.0, false);
                                const auto bM = boundOf(evM, openM, evM ? lineM.form->roots[c] : 0, pc.s0, pc.s1, 0.0, true);
                                const double light = (bS.hi * supAbsOf(bC) * bP.hi + bM.hi) * std::exp(-tau + tauRad) * w;
                                Lest[c] += 0.5 * light; Lrad[c] += 0.5 * light;
                            }
                            tau += 0.5 * supT * w; tauRad += 0.5 * supT * w;
                            continue;
                        }
                        const uint32_t cells = std::max<uint32_t>(2, uint32_t(std::lround(95.0 * w / std::max(dense, 1e-300))));
                        const double hc = w / cells;
                        auto edgeAt = [&](uint32_t j) { return j == cells ? pc.s1 : pc.s0 + hc * j; };
                        std::vector<double> tauAt(cells + 1), radAt(cells + 1);
                        tauAt[0] = tau; radAt[0] = tauRad;
                        for (uint32_t j = 0; j < cells; ++j) {
                            const auto seg = sigmaTIntegral(pc, edgeAt(j), edgeAt(j + 1), 2);
                            if (!seg) { colourOk = false; break; }
                            tauAt[j + 1] = tauAt[j] + seg->first;
                            radAt[j + 1] = radAt[j] + seg->second;
                        }
                        if (!colourOk) break;
                        auto indexOf = [&](double sv) {
                            return uint32_t(std::clamp<long>(std::lround((sv - pc.s0) / hc), 0L, long(cells)));
                        };
                        auto tRange = [&](double p0, double q0) {
                            return std::make_pair(std::exp(-tauAt[indexOf(q0)] - radAt[indexOf(q0)]), std::exp(-tauAt[indexOf(p0)] + radAt[indexOf(p0)]));
                        };
                        auto tSlope = [&](double p0, double q0) { return sigmaTSup(pc, p0, q0) * tRange(p0, q0).second; };
                        const double drift = std::expm1(radAt[cells]);   // node values of T carry τ's error
                        for (int c = 0; c < 3; ++c) {
                            const uint32_t rootC = evC ? lineC.form->roots[c] : 0, rootM = evM ? lineM.form->roots[c] : 0;
                            // Scattered: ∫ sigma_s · (T · chroma_c · phase) ds.
                            if (!phaseReadsWi) {
                                OntoMath::LineSampledFactor F;
                                F.value = [&, c](double sv) -> std::optional<double> {
                                    return std::exp(-tauAt[indexOf(sv)]) * chromaAt(c, sv) * phaseAt(sv);
                                };
                                F.range = [&, rootC](double p0, double q0) {
                                    const auto [tl, th] = tRange(p0, q0);
                                    const auto bC = boundOf(evC, openC, rootC, p0, q0, 1.0, false);
                                    const auto bP = boundOf(evP, openP, rootP, p0, q0, 1.0, true);
                                    double lo = 1e300, hi = -1e300;
                                    for (double x1 : {tl, th}) for (double x2 : {bC.lo, bC.hi}) for (double x3 : {bP.lo, bP.hi}) {
                                        lo = std::min(lo, x1 * x2 * x3); hi = std::max(hi, x1 * x2 * x3);
                                    }
                                    return std::make_pair(lo, hi);
                                };
                                F.lipschitz = [&, rootC](double p0, double q0) {
                                    const double tSup = tRange(p0, q0).second;
                                    const auto bC = boundOf(evC, openC, rootC, p0, q0, 1.0, false);
                                    const auto bP = boundOf(evP, openP, rootP, p0, q0, 1.0, true);
                                    return tSlope(p0, q0) * supAbsOf(bC) * supAbsOf(bP) + tSup * bC.slope * supAbsOf(bP) +
                                           tSup * supAbsOf(bC) * bP.slope;
                                };
                                const auto hs = evS ? evS->hybridIntegral(rootS, pc.s0, pc.s1, openS, cells, F)
                                                    : evD.hybridIntegral(rootD, pc.s0, pc.s1, pc.signs[0], cells, F);
                                if (!hs) { colourOk = false; break; }
                                Lest[c] += hs->estimate;
                                Lrad[c] += hs->radius + std::abs(hs->estimate) * drift;
                            }
                            // Emitted: ∫ max(e_c, 0) · T ds, when e_c ≥ 0 is proved on the piece.
                            if (evM) {
                                const auto bM = boundOf(evM, openM, rootM, pc.s0, pc.s1, 0.0, false);
                                if (bM.lo < 0.0) { colourOk = false; break; }   // max(e, 0) would cut: not admitted here
                                OntoMath::LineSampledFactor F;
                                F.value = [&](double sv) -> std::optional<double> { return std::exp(-tauAt[indexOf(sv)]); };
                                F.range = tRange;
                                F.lipschitz = tSlope;
                                const auto he = evM->hybridIntegral(rootM, pc.s0, pc.s1, openM, cells, F);
                                if (!he) { colourOk = false; break; }
                                Lest[c] += he->estimate;
                                Lrad[c] += he->radius + std::abs(he->estimate) * drift;
                            }
                        }
                        if (!colourOk) break;
                        tau = tauAt[cells];
                        tauRad = radAt[cells];
                    }
                    if (!colourOk) continue;
                    ++scatterRays;
                    for (int c = 0; c < 3; ++c) {
                        const double refErr = 2.0 * std::abs(refL[c] - L17[c]) + double(1 << 18) * std::numeric_limits<double>::epsilon() * std::abs(refL[c]);
                        scatter96 = std::max(scatter96, std::abs(L96[c] - refL[c]));
                        scatterLine = std::max(scatterLine, std::abs(Lest[c] - refL[c]));
                        if (std::abs(Lest[c] - refL[c]) > Lrad[c] + refErr + 1e-12) ++scatterMiss;
                    }
                }
                if (scatterRays) {
                    std::printf("    %s colour, no-source model (%u rays, RGB): 96-sample max |dL| %.3g (%.3f of 255), "
                                "line-integrated %.3g (%.5f of 255); %u outside the proved radius\n",
                                b.producerId.c_str(), scatterRays, scatter96, 255.0 * scatter96, scatterLine, 255.0 * scatterLine,
                                scatterMiss);
                    ++scatterMedia;
                    scatterBetter += scatterLine <= scatter96 ? 1 : 0;
                    scatterMisses += scatterMiss;
                    scatterWorstLine = std::max(scatterWorstLine, scatterLine);
                    if (scatter96 > scatterWorst96) { scatterWorst96 = scatter96; scatterWorstWho = b.producerId; }
                }
                if (lineRays) {
                    std::printf("    %s transmittance by line integration (%u rays): 96-sample max |dT| %.3g (%.3f of 255 levels), "
                                "hybrid %.3g (%.5f of 255) at %.0f remainder evaluations/ray; %u outside the proved radius\n",
                                b.producerId.c_str(), lineRays, line96, 255.0 * line96, lineHybrid, 255.0 * lineHybrid,
                                lineEvals / lineRays, lineMiss);
                    ++lineMedia;
                    lineBetter += lineHybrid <= line96 ? 1 : 0;
                    lineMisses += lineMiss;
                    lineWorstHybrid = std::max(lineWorstHybrid, lineHybrid);
                    if (line96 > lineWorst96) { lineWorst96 = line96; lineWorstWho = b.producerId; }
                } else if (!lineOk) {
                    std::printf("    %s transmittance by line integration: not admitted (%s)\n", b.producerId.c_str(),
                                !lineD ? lineD.refusal.c_str() : lineE.refusal.c_str());
                }
                if (ok) {
                    std::printf("    %s quadrature: 96-sample transmittance vs the continuum, max |dT| %.3g (%.2f of 255 levels), "
                                "max relative optical-depth error %.3g; reference converged to %.1g\n",
                                b.producerId.c_str(), worstT, 255.0 * worstT, worstTau, worstConv);
                    quadratureWorst = std::max(quadratureWorst, worstT);
                    if (worstT > quadratureWorst2) { quadratureWorst2 = worstT; quadratureWho = b.producerId; }
                    ++quadratureMeasured;
                }
            }
            std::printf("    %s: the two frames' points differ at %.3g%% of samples, by up to %.3g\n", b.producerId.c_str(),
                        100.0 * double(gapDiffer) / double(std::max<uint64_t>(gapTotal, 1)), gapMax);
            const double norm = scale > 0.0 ? scale : 1.0;
            const double normalized = worstAbs[0][0] / norm;
            if (normalized > worstNormalized) { worstNormalized = normalized; worstWho = b.producerId; }
            for (int set = 0; set < 2; ++set) {
                const double rmsW = std::sqrt(sumSq[set][0] / double(count[set]));
                const double rmsL = std::sqrt(sumSq[set][1] / double(count[set]));
                std::printf("    %s %s: max/rms error, world frame %.3g/%.3g, medium frame %.3g/%.3g (of scale %.3g)%s\n",
                            b.producerId.c_str(), set == 0 ? "near" : "far ", worstAbs[set][0] / norm, rmsW / norm,
                            worstAbs[set][1] / norm, rmsL / norm, scale,
                            prep && set == 0 ? "; clock leaves prepared on the device" : "");
                // Never less faithful (a few-ulp allowance for sampling the max).
                const bool ok = worstAbs[set][1] <= worstAbs[set][0] * 1.25 + 1e-9 * norm && rmsL <= rmsW * 1.25 + 1e-10 * norm;
                if (!ok) ++lessFaithful;
                sumRatio[set] += (rmsW > 0.0 ? rmsL / rmsW : 1.0);
            }
        }
    }
    std::printf("    %u media, %u measured against the double-precision authored density, %u refused; worst %.3g of "
                "its density scale (%s); %u non-finite device values\n",
                media, measured, refused, worstNormalized, worstWho.c_str(), nonFinite);
    check(measured > 0 && measured * 10 >= media * 8, "at least 80% of real media are measured against the authored equation");
    check(nonFinite == 0, "the device density is finite at every sampled point");
    std::printf("    medium-frame rms error / world-frame rms error, mean over media: near %.3g, far %.3g\n",
                measured ? sumRatio[0] / measured : 0.0, measured ? sumRatio[1] / measured : 0.0);
    std::printf("    quadrature: %u media; worst 96-sample transmittance error against the continuum %.3g (%.2f of 255 levels, %s)\n",
                quadratureMeasured, quadratureWorst, 255.0 * quadratureWorst, quadratureWho.c_str());
    std::printf("    transmittance by line integration: %u media; worst 96-sample |dT| %.3g (%.2f of 255, %s), worst hybrid %.3g "
                "(%.4f of 255); hybrid at least as faithful on %u; %u rays outside the proved radius\n",
                lineMedia, lineWorst96, 255.0 * lineWorst96, lineWorstWho.c_str(), lineWorstHybrid, 255.0 * lineWorstHybrid,
                lineBetter, lineMisses);
    check(lineMedia > 0 && lineMisses == 0, "line-integrated optical depth lies inside its proved radius on every ray");
    std::printf("    colour by line integration (no-source model, RGB): %u media; worst 96-sample |dL| %.3g (%.2f of 255, %s), "
                "worst line-integrated %.3g (%.4f of 255); at least as faithful on %u; %u rays outside the proved radius\n",
                scatterMedia, scatterWorst96, 255.0 * scatterWorst96, scatterWorstWho.c_str(), scatterWorstLine,
                255.0 * scatterWorstLine, scatterBetter, scatterMisses);
    check(scatterMedia > 0 && scatterMisses == 0, "line-integrated colour (scattering and emission, RGB) lies inside its proved radius on every ray");
    check(lessFaithful == 0, "the medium-frame lattice point is never less faithful than the world-frame one (near and far cameras)");
    std::printf("webgpu_volume_faithfulness_test: %d/%d checks passed\n", g_checks - g_failures, g_checks);
    return g_failures == 0 ? 0 : 1;
}
