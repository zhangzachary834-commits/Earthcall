// Rung 1 witness for Zach's Ray Transport Canon: one V5 segment executing
// WITHOUT the general sample evaluator, for the exactly-constant class.
//
// Reference: a CPU mirror of the accepted lattice, written line-for-line from
// the fused V5 marcher in SdfWgsl.cpp (and its single-medium special case),
// evaluating every channel through OntoMath's own Piecewise::evaluate — the
// exact evaluator, not the restricted forms under test. The closed form must
// agree with it as REAL-NUMBER mathematics; byte identity is not claimed and
// the f32 deviation is characterized separately.
//
// The production emitter is consulted too (CPU codegen only, no device): every
// medium handed to the planner must compile, and the planner's mirror of
// VOLUME_PHASE_READS_WI / VOLUME_EMISSION_READS_OMEGA is checked against the
// WGSL the emitter actually generates.
//
// NOT exercised here (no native GPU in this environment): WebGPU pixels. No
// renderer consumes this plan yet; see the plan doc §7.
//
// Claude Code · Claude Opus 5.5 · session_01HvZSR64QghrdyCpq2M7qvX · 2026-10-16.
#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "Singularity/Screen/VolumeDensity.hpp"
#include "Singularity/Screen/VolumeRayTransport.hpp"
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"

#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <random>
#include <set>
#include <algorithm>

namespace {

int g_checks = 0;
int g_failures = 0;

void check(bool condition, const std::string& description) {
    ++g_checks;
    if (!condition) {
        ++g_failures;
        std::cout << "  FAILED: " << description << std::endl;
        return;
    }
    std::cout << "  ok: " << description << std::endl;
}

using OntoMath::Piecewise;
using Rendering::VolumeDensityBinding;
using Rendering::VolumeSegmentStrategy;

std::shared_ptr<Piecewise> pw(const std::string& json) {
    return std::make_shared<Piecewise>(Piecewise::fromJson(nlohmann::json::parse(json)));
}
// One unconditional piece holding a signomial, e.g. {"c":0.8,"factors":{}}.
std::shared_ptr<Piecewise> scalarPw(const std::string& termsJson) {
    return pw(R"({"input":"x","pieces":[{"mathNode":{"op":0,"scalarForm":{"terms":[)" + termsJson + "]}}}]}");
}
std::string leafJson(const std::string& termsJson) {
    return R"({"op":0,"scalarForm":{"terms":[)" + termsJson + "]}}";
}
// vec3 from three signomials: VectorConstruct (op 2).
std::shared_ptr<Piecewise> vectorPw(const std::string& a, const std::string& b, const std::string& c) {
    return pw(R"({"input":"x","pieces":[{"mathNode":{"op":2,"children":[)" + leafJson(a) + "," +
              leafJson(b) + "," + leafJson(c) + "]}}]}");
}

struct Medium {
    std::shared_ptr<Piecewise> density, extinction, scattering, chroma, phase, emission;
    VolumeDensityBinding binding(const std::string& id, uint64_t revisionBase) const {
        VolumeDensityBinding b;
        b.producerId = id;
        b.densityExpr = density.get(); b.densityRevision = density ? revisionBase + 1 : 0;
        b.extinctionExpr = extinction.get(); b.extinctionRevision = extinction ? revisionBase + 2 : 0;
        b.scatteringExpr = scattering.get(); b.scatteringRevision = scattering ? revisionBase + 3 : 0;
        b.volumeChromaExpr = chroma.get(); b.volumeChromaRevision = chroma ? revisionBase + 4 : 0;
        b.phaseExpr = phase.get(); b.phaseRevision = phase ? revisionBase + 5 : 0;
        b.emissionExpr = emission.get(); b.emissionRevision = emission ? revisionBase + 6 : 0;
        return b;
    }
};

// ---- Production emitter consultation (CPU codegen, no device) -------------
sdfwgsl::Program emitted(const VolumeDensityBinding& b) {
    return sdfwgsl::compileVolume(b.densityExpr, b.extinctionExpr, b.scatteringExpr,
                                  b.volumeChromaExpr, b.phaseExpr, b.emissionExpr);
}
bool emitterFlag(const sdfwgsl::Program& p, const std::string& name) {
    return p.wgsl.find("const " + name + ": bool = true;") != std::string::npos;
}

// ---- The accepted lattice, mirrored from SdfWgsl.cpp (fused V5 loop) -------
struct MirrorMember { const VolumeDensityBinding* b; glm::dvec3 origin; double time; bool readsWi; bool readsOmega; };
struct MirrorResult { double T; glm::dvec3 L; uint32_t consumed; bool cut; bool ok; };

std::map<std::string, PropertyValue> varsAt(const glm::dvec3& p, double t, const glm::dvec3& wi,
                                            const glm::dvec3& wo) {
    return {{"p", PropertyValue(glm::vec3(p))}, {"x", PropertyValue(p.x)}, {"y", PropertyValue(p.y)},
            {"z", PropertyValue(p.z)}, {OntoMath::kTimeVar, PropertyValue(t)},
            {OntoMath::kWiXVar, PropertyValue(wi.x)}, {OntoMath::kWiYVar, PropertyValue(wi.y)},
            {OntoMath::kWiZVar, PropertyValue(wi.z)}, {OntoMath::kWoXVar, PropertyValue(wo.x)},
            {OntoMath::kWoYVar, PropertyValue(wo.y)}, {OntoMath::kWoZVar, PropertyValue(wo.z)},
            {OntoMath::kOmegaXVar, PropertyValue(wo.x)}, {OntoMath::kOmegaYVar, PropertyValue(wo.y)},
            {OntoMath::kOmegaZVar, PropertyValue(wo.z)}};
}
bool scalarAt(const Piecewise& f, const std::map<std::string, PropertyValue>& v, double& out) {
    const auto r = f.evaluate(v);
    return r && propertyValueToNumber(*r, out);
}
bool vectorAt(const Piecewise& f, const std::map<std::string, PropertyValue>& v, glm::dvec3& out) {
    const auto r = f.evaluate(v);
    if (!r || !std::holds_alternative<glm::vec3>(*r)) return false;
    out = glm::dvec3(std::get<glm::vec3>(*r));
    return true;
}

template <typename Real>
MirrorResult lattice(const std::vector<MirrorMember>& members, const glm::dvec3& ro, const glm::dvec3& rd,
                     const Rendering::VolumeLatticeSpan& span, double tIn, bool budgetZero) {
    Real T = Real(tIn);
    glm::dvec3 L(0.0);
    MirrorResult r{tIn, L, 0, false, true};
    for (uint32_t i = 0; i < span.count; ++i) {
        const double s = span.firstSample + double(i) * span.step;
        const glm::dvec3 worldP = ro + rd * s;
        Real totalExtinction = 0;
        glm::dvec3 totalSource(0.0);
        for (const auto& m : members) {
            const glm::dvec3 p = worldP - m.origin;
            const glm::dvec3 woDelta = ro - worldP;
            const double woLen = glm::length(woDelta);
            const glm::dvec3 wo = woLen > 1e-8 ? woDelta / std::max(woLen, 1e-8) : glm::dvec3(0.0);
            const auto v = varsAt(p, m.time, glm::dvec3(0.0), wo);   // no source: wi = 0
            double D = 0;
            if (!scalarAt(*m.b->densityExpr, v, D)) { r.ok = false; return r; }
            D = std::max(D, 0.0);
            if (!(D > 0.0)) continue;
            double ext = 0.5 * D;
            if (m.b->extinctionExpr && !scalarAt(*m.b->extinctionExpr, v, ext)) { r.ok = false; return r; }
            ext = std::max(ext, 1e-6);
            totalExtinction += Real(ext);
            glm::dvec3 emittedV(0.0);
            if (m.b->emissionExpr && (!m.readsOmega || woLen > 1e-8)) {
                if (!vectorAt(*m.b->emissionExpr, v, emittedV)) { r.ok = false; return r; }
                emittedV = glm::max(emittedV, glm::dvec3(0.0));
            }
            // incidentLi = 1 without a source, so the scatter branch always runs.
            double sc = D;
            if (m.b->scatteringExpr) { if (!scalarAt(*m.b->scatteringExpr, v, sc)) { r.ok = false; return r; } sc = std::max(sc, 0.0); }
            glm::dvec3 chroma(1.0);
            if (m.b->volumeChromaExpr && !vectorAt(*m.b->volumeChromaExpr, v, chroma)) { r.ok = false; return r; }
            double phase = 1.0;
            if (m.b->phaseExpr) {
                phase = 0.0;
                if (!m.readsWi && woLen > 1e-8) {
                    if (!scalarAt(*m.b->phaseExpr, v, phase)) { r.ok = false; return r; }
                    phase = std::max(phase, 0.0);
                }
            }
            totalSource += chroma * sc * phase + emittedV;
        }
        const Real oldT = T;
        if (totalExtinction > Real(0)) {
            T *= std::exp(-totalExtinction * Real(span.step));
            L += totalSource * double((oldT - T) / totalExtinction);
        } else {
            L += double(oldT) * totalSource * span.step;
        }
        ++r.consumed;
        if (budgetZero && T < Real(0.01)) { r.cut = true; break; }
    }
    r.T = double(T);
    r.L = L;
    return r;
}

double relErr(double a, double b) { return std::abs(a - b) / std::max({std::abs(a), std::abs(b), 1e-30}); }
double relErr(const glm::dvec3& a, const glm::dvec3& b) {
    return std::max({relErr(a.x, b.x), relErr(a.y, b.y), relErr(a.z, b.z)});
}

struct Scenario {
    std::vector<Rendering::VolumeSegmentMember> members;
    std::vector<MirrorMember> mirror;
};

Scenario scenario(Rendering::VolumeRayFormCache& cache,
                  const std::vector<std::tuple<const VolumeDensityBinding*, glm::dvec3, double>>& ms) {
    Scenario sc;
    for (const auto& [b, origin, time] : ms) {
        const auto& forms = cache.formsFor(*b);
        sc.members.push_back({&forms, origin, time});
        const auto prog = emitted(*b);
        sc.mirror.push_back({b, origin, time, emitterFlag(prog, "VOLUME_PHASE_READS_WI"),
                             emitterFlag(prog, "VOLUME_EMISSION_READS_OMEGA")});
    }
    return sc;
}

// Run plan + closed form vs the mirror; returns the plan strategy.
VolumeSegmentStrategy compare(const std::string& label, Rendering::VolumeRayFormCache& cache,
                              const std::vector<std::tuple<const VolumeDensityBinding*, glm::dvec3, double>>& ms,
                              const glm::dvec3& ro, const glm::dvec3& rd, const Rendering::VolumeLatticeSpan& span,
                              bool budgetZero, double tolerance, double tIn = 1.0) {
    // Fresh member forms per call: formsFor returns references into the cache.
    const auto sc = scenario(cache, ms);
    const auto plan = Rendering::planVolumeRaySegment(sc.members, {ro, rd, false}, span);
    const auto ref = lattice<double>(sc.mirror, ro, rd, span, tIn, budgetZero);
    if (plan.strategy == VolumeSegmentStrategy::GeneralLattice) {
        std::cout << "    " << label << ": general lattice — " << plan.reason << "\n";
        return plan.strategy;
    }
    Rendering::VolumeTransportState st;
    st.transmittance = tIn;
    Rendering::executeVolumeSegment(plan, span, budgetZero, st);
    const double eT = relErr(st.transmittance, ref.T);
    const double eL = (glm::length(ref.L) == 0 && glm::length(st.radiance) == 0) ? 0.0 : relErr(st.radiance, ref.L);
    const auto f32 = lattice<float>(sc.mirror, ro, rd, span, tIn, budgetZero);
    std::cout << "    " << label << ": " << Rendering::volumeSegmentStrategyName(plan.strategy)
              << "  T=" << st.transmittance << " (lattice " << ref.T << ", rel " << eT << ")"
              << "  L rel " << eL << "  consumed " << st.samplesConsumed << "/" << ref.consumed
              << "  [f32-lattice T rel " << relErr(st.transmittance, f32.T) << "]\n";
    check(ref.ok && eT <= tolerance && eL <= tolerance && st.samplesConsumed == ref.consumed &&
              st.terminated == ref.cut,
          label + ": closed form equals the accepted lattice (real-number tolerance " +
              std::to_string(tolerance) + ")");
    return plan.strategy;
}


// Census helper: every structure in the answering tree that Rung 0 cannot hold,
// not merely the first refusal, so the next rung is chosen from evidence.
void nonPolynomial(const OntoMath::MathNode& n, std::set<std::string>& out) {
    using Op = OntoMath::MathNode::Op;
    switch (n.op) {
        case Op::ScalarLeaf: case Op::ValueLeaf: case Op::VectorConstruct: case Op::Component:
        case Op::Add: case Op::Sub: case Op::Scale: case Op::Hadamard: case Op::Dot: case Op::Cross:
        case Op::Div: case Op::Pow: break;
        default: out.insert(OntoMath::mathOpName(n.op));
    }
    if (n.op == Op::ScalarLeaf) {
        for (const auto& t : n.scalarForm.terms) {
            for (const auto& tf : t.trans)
                if (tf.variable == "x" || tf.variable == "y" || tf.variable == "z") out.insert("trans(" + tf.print().substr(0, 3) + " of space)");
            for (const auto& [v, e] : t.factors)
                if ((v == "x" || v == "y" || v == "z") && (e < 0 || std::floor(e) != e)) out.insert("non-integer power of space");
        }
    }
    for (const auto& c : n.children) if (c) nonPolynomial(*c, out);
}

} // namespace

int main() {
    std::cout << "volume_ray_segment_transport_test\n";
    Rendering::VolumeRayFormCache cache;
    const glm::dvec3 eye(0.0, 1.0, -30.0);
    const glm::dvec3 forward(0.0, 0.0, 1.0);
    const glm::dvec3 tilted = glm::normalize(glm::dvec3(0.2, 0.3, 1.0));
    const Rendering::VolumeLatticeSpan span{22.0 + 0.5 * (16.0 / 96.0), 16.0 / 96.0, 96};

    // ---- Homogeneous medium: every channel authored, all constant ---------
    Medium homogeneous;
    homogeneous.density = scalarPw(R"({"c":0.8,"factors":{}})");
    homogeneous.extinction = scalarPw(R"({"c":0.35,"factors":{}})");
    homogeneous.scattering = scalarPw(R"({"c":0.6,"factors":{}})");
    homogeneous.chroma = vectorPw(R"({"c":0.9,"factors":{}})", R"({"c":0.7,"factors":{}})", R"({"c":1.0,"factors":{}})");
    homogeneous.phase = scalarPw(R"({"c":0.5,"factors":{}})");
    homogeneous.emission = vectorPw(R"({"c":0.2,"factors":{}})", R"({"c":0.05,"factors":{}})", R"({"c":0.1,"factors":{}})");
    const auto hb = homogeneous.binding("medium.homogeneous", 100);
    check(emitted(hb).ok, "the production emitter compiles the homogeneous medium (planner precondition)");
    const glm::dvec3 hOrigin(0.0, 1.0, 0.0);
    // Vector channels arrive from Piecewise::evaluate as glm::vec3 (f32), so
    // the mirror's constants carry f32 rounding: tolerance 1e-6 relative.
    check(compare("homogeneous, forward, budget>0", cache, {{&hb, hOrigin, 0.0}}, eye, forward, span, false, 1e-6) ==
              VolumeSegmentStrategy::ConstantClosedForm, "homogeneous segment executes without the sample loop");
    check(compare("homogeneous, tilted, budget>0", cache, {{&hb, hOrigin, 0.0}}, eye, tilted, span, false, 1e-6) ==
              VolumeSegmentStrategy::ConstantClosedForm, "constancy holds on every ray direction for a homogeneous medium");
    compare("homogeneous, budget 0 (legacy cutoff live)", cache, {{&hb, hOrigin, 0.0}}, eye, forward, span, true, 1e-6);

    // ---- Height fog: constant on horizontal rays, affine on tilted ones ---
    Medium fog;
    fog.density = scalarPw(R"({"c":2.0,"factors":{}},{"c":-0.1,"factors":{"y":1.0}})");   // D = 2 − 0.1y
    const auto fb = fog.binding("medium.heightfog", 200);
    check(emitted(fb).ok, "the production emitter compiles the height fog");
    const glm::dvec3 fOrigin(0.0, -2.0, 0.0);
    check(compare("height fog, horizontal ray", cache, {{&fb, fOrigin, 0.0}}, eye, forward, span, false, 1e-12) ==
              VolumeSegmentStrategy::ConstantClosedForm,
          "a horizontal ray through height fog is exactly constant (dy = 0 proves it)");
    check(compare("height fog, tilted ray", cache, {{&fb, fOrigin, 0.0}}, eye, tilted, span, false, 1e-12) ==
              VolumeSegmentStrategy::GeneralLattice, "a tilted ray through height fog refuses the constant class");
    {
        // Affine transmittance rung: the midpoint lattice sums affine σ exactly.
        const auto sc = scenario(cache, {{&fb, fOrigin, 0.0}});
        std::string why;
        const auto T = Rendering::affineSegmentTransmittance(sc.members, {eye, tilted, false}, span, 1.0, false, &why);
        const auto ref = lattice<double>(sc.mirror, eye, tilted, span, 1.0, false);
        check(T && relErr(*T, ref.T) < 1e-12,
              "affine extinction: one exp(−n·Δ·σ(s_mid)) equals the lattice transmittance (rel " +
                  std::to_string(T ? relErr(*T, ref.T) : -1.0) + ")");
        const auto Tcut = Rendering::affineSegmentTransmittance(sc.members, {eye, tilted, false}, span, 1.0, true, &why);
        std::cout << "    affine under legacy cutoff: " << (Tcut ? "admitted" : "refused — " + why) << "\n";
        check(!Tcut || *Tcut >= 0.01, "the affine rung never executes a span the legacy cutoff could truncate");
        // Steep fog: density changes sign inside the span → refuse, never clamp-guess.
        Medium steep;
        // y runs ~7.3 → ~11.9 across the tilted span, so D = 9.6 − y changes sign mid-span.
        steep.density = scalarPw(R"({"c":9.6,"factors":{}},{"c":-1.0,"factors":{"y":1.0}})");
        const auto sb = steep.binding("medium.steep", 300);
        const auto ssc = scenario(cache, {{&sb, glm::dvec3(0.0, 0.0, 0.0), 0.0}});
        check(!Rendering::affineSegmentTransmittance(ssc.members, {eye, tilted, false}, span, 1.0, false, &why),
              "a density root inside the span refuses the affine rung (" + why + ")");
    }

    // ---- Exact emptiness: the one case that is byte-identical -------------
    Medium hollow;
    hollow.density = scalarPw(R"({"c":-0.25,"factors":{}})");
    hollow.emission = vectorPw(R"({"c":5.0,"factors":{}})", R"({"c":5.0,"factors":{}})", R"({"c":5.0,"factors":{}})");
    const auto ob = hollow.binding("medium.hollow", 400);
    {
        const auto sc = scenario(cache, {{&ob, glm::dvec3(0.0), 0.0}});
        const auto plan = Rendering::planVolumeRaySegment(sc.members, {eye, tilted, true}, span);
        Rendering::VolumeTransportState st;
        Rendering::executeVolumeSegment(plan, span, false, st);
        const auto ref = lattice<float>(sc.mirror, eye, tilted, span, 1.0, false);
        check(plan.strategy == VolumeSegmentStrategy::ExactEmpty && st.transmittance == 1.0 &&
                  st.radiance == glm::dvec3(0.0) && ref.T == 1.0 && ref.L == glm::dvec3(0.0),
              "D ≤ 0 constant: exact-empty, bit-identical to the f32 lattice (T=1, L=0), even with a source");
    }

    // ---- Overlap with independent clocks ---------------------------------
    Medium pulseA, pulseB;
    pulseA.density = scalarPw(R"({"c":0.4,"factors":{}},{"c":0.1,"factors":{"t":1.0}})");    // 0.4 + 0.1t
    pulseA.emission = vectorPw(R"({"c":0.3,"factors":{"t":1.0}})", R"({"c":0.0,"factors":{}})", R"({"c":0.1,"factors":{}})");
    pulseB.density = scalarPw(R"({"c":0.2,"factors":{"t":2.0}})");                           // 0.2t²
    pulseB.chroma = vectorPw(R"({"c":0.1,"factors":{}})", R"({"c":0.8,"factors":{}})", R"({"c":0.4,"factors":{}})");
    const auto ab = pulseA.binding("medium.pulseA", 500);
    const auto bb = pulseB.binding("medium.pulseB", 600);
    check(compare("overlap A+B, clocks 1.5 / 3.0", cache, {{&ab, glm::dvec3(0, 1, 2), 1.5}, {&bb, glm::dvec3(1, 0, 0), 3.0}},
                  eye, tilted, span, false, 1e-6) == VolumeSegmentStrategy::ConstantClosedForm,
          "an overlap segment of two ray-constant media sums into one closed form");
    {
        // Each member binds ITS clock: swapping the clocks changes the answer.
        const auto s1 = scenario(cache, {{&ab, glm::dvec3(0), 1.5}, {&bb, glm::dvec3(0), 3.0}});
        const auto s2 = scenario(cache, {{&ab, glm::dvec3(0), 3.0}, {&bb, glm::dvec3(0), 1.5}});
        const auto p1 = Rendering::planVolumeRaySegment(s1.members, {eye, tilted, false}, span);
        const auto p2 = Rendering::planVolumeRaySegment(s2.members, {eye, tilted, false}, span);
        check(p1.totalExtinction != p2.totalExtinction, "member clocks are bound per medium, never shared");
    }

    // ---- Direction-reading channels mirror the emitter's flags -----------
    {
        // (a) An UNREACHABLE second piece reads wi.x: emitPiecewise returns after
        //     the unconditional first piece, so the flag stays false and phase runs.
        // (b) The answering piece itself reads wi.x: flag true, phase = 0 without a source.
        Medium unreachableWi, reachableWi;
        unreachableWi.density = scalarPw(R"({"c":0.5,"factors":{}})");
        unreachableWi.phase = pw(R"({"input":"x","pieces":[{"mathNode":)" + leafJson(R"({"c":0.7,"factors":{}})") +
                                 R"(},{"mathNode":)" + leafJson(R"({"c":1.0,"factors":{"wi.x":1.0}})") + "}]}");
        reachableWi.density = scalarPw(R"({"c":0.5,"factors":{}})");
        reachableWi.phase = scalarPw(R"({"c":0.7,"factors":{}},{"c":1.0,"factors":{"wi.x":1.0}})");
        Medium omegaLit;
        omegaLit.density = scalarPw(R"({"c":0.5,"factors":{}})");
        omegaLit.emission = vectorPw(R"({"c":1.0,"factors":{"omega.z":2.0}})", R"({"c":0.1,"factors":{}})",
                                     R"({"c":0.2,"factors":{"omega.x":1.0}})");
        const auto ub = unreachableWi.binding("medium.unreachable-wi", 700);
        const auto rb = reachableWi.binding("medium.reachable-wi", 750);
        const auto eb = omegaLit.binding("medium.omega", 800);
        const auto uprog = emitted(ub), rprog = emitted(rb), eprog = emitted(eb);
        check(uprog.ok && !emitterFlag(uprog, "VOLUME_PHASE_READS_WI") && !cache.formsFor(ub).phase.reads("wi."),
              "READS_WI mirror: an unreachable wi piece is not emitted, so neither side flags it");
        check(rprog.ok && emitterFlag(rprog, "VOLUME_PHASE_READS_WI") && cache.formsFor(rb).phase.reads("wi."),
              "READS_WI mirror: a reachable wi read is flagged by the emitter and the planner alike");
        check(eprog.ok && emitterFlag(eprog, "VOLUME_EMISSION_READS_OMEGA") && cache.formsFor(eb).emission.reads("omega."),
              "the planner's READS_OMEGA agrees with the production emitter");
        check(compare("phase with unreachable wi piece (phase runs)", cache, {{&ub, glm::dvec3(0), 0.0}}, eye, tilted, span, false, 1e-12) ==
                  VolumeSegmentStrategy::ConstantClosedForm, "unreachable-wi phase stays closed-form and equals the lattice");
        check(compare("phase reads wi (no source → phase 0)", cache, {{&rb, glm::dvec3(0), 0.0}}, eye, tilted, span, false, 1e-12) ==
                  VolumeSegmentStrategy::ConstantClosedForm, "wi-reading phase without a source contributes no in-scatter, as in the lattice");
        check(compare("emission reads omega = −d", cache, {{&eb, glm::dvec3(0), 0.0}}, eye, tilted, span, false, 1e-6) ==
                  VolumeSegmentStrategy::ConstantClosedForm, "ω = −d/|d| is ray-constant, so ω-reading emission stays closed-form");
        // Too close to the eye: the per-sample direction gate is not provably open.
        const Rendering::VolumeLatticeSpan nearEye{1e-9, 1e-3, 8};
        const auto sc = scenario(cache, {{&eb, glm::dvec3(0), 0.0}});
        check(Rendering::planVolumeRaySegment(sc.members, {eye, tilted, false}, nearEye).strategy ==
                  VolumeSegmentStrategy::GeneralLattice, "a span starting at the eye keeps the lattice (direction gate)");
    }

    // ---- Refusals keep the lattice ----------------------------------------
    {
        const auto sc = scenario(cache, {{&hb, hOrigin, 0.0}});
        check(Rendering::planVolumeRaySegment(sc.members, {eye, forward, true}, span).strategy ==
                  VolumeSegmentStrategy::GeneralLattice,
              "an active incident source keeps the lattice (Li varies with source distance/visibility)");
        Medium noisy;
        noisy.density = pw(R"({"input":"x","pieces":[{"mathNode":{"op":29,"children":[{"op":1,"var":"p"}]}}]})");
        const auto nb = noisy.binding("medium.noisy", 900);
        const auto nsc = scenario(cache, {{&nb, glm::dvec3(0), 0.0}});
        const auto plan = Rendering::planVolumeRaySegment(nsc.members, {eye, forward, false}, span);
        check(plan.strategy == VolumeSegmentStrategy::GeneralLattice, "Perlin density keeps the lattice: " + plan.reason);
        const auto mixed = scenario(cache, {{&hb, hOrigin, 0.0}, {&nb, glm::dvec3(0), 0.0}});
        check(Rendering::planVolumeRaySegment(mixed.members, {eye, forward, false}, span).strategy ==
                  VolumeSegmentStrategy::GeneralLattice, "one unsupported member keeps the whole overlap segment on the lattice");
        check(Rendering::planVolumeRaySegment(sc.members, {eye, glm::dvec3(0.0), false}, span).strategy ==
                  VolumeSegmentStrategy::GeneralLattice, "a zero direction refuses");
        check(Rendering::planVolumeRaySegment(sc.members, {eye, forward, false}, {1.0, 0.0, 8}).strategy ==
                  VolumeSegmentStrategy::GeneralLattice, "a zero step refuses");
        Rendering::VolumeTransportState untouched;
        check(!Rendering::executeVolumeSegment(Rendering::VolumeSegmentPlan{}, span, false, untouched) &&
                  untouched.transmittance == 1.0 && untouched.samplesConsumed == 0,
              "executing a general-lattice plan does nothing and says so");
    }

    // ---- Legacy cutoff: step-count agreement across a sweep ---------------
    {
        int cases = 0, mismatches = 0, nearTies = 0;
        for (int i = 0; i < 400; ++i) {
            const double c = 0.05 + 0.0125 * i;   // density 0.05 .. 5.04
            Medium m;
            m.density = scalarPw(R"({"c":)" + std::to_string(c) + R"(,"factors":{}})");
            const auto b = m.binding("medium.sweep" + std::to_string(i), 10000 + 10 * i);
            const auto sc = scenario(cache, {{&b, glm::dvec3(0), 0.0}});
            const auto plan = Rendering::planVolumeRaySegment(sc.members, {eye, forward, false}, span);
            Rendering::VolumeTransportState st;
            Rendering::executeVolumeSegment(plan, span, true, st);
            const auto ref = lattice<double>(sc.mirror, eye, forward, span, 1.0, true);
            ++cases;
            if (st.samplesConsumed != ref.consumed) {
                ++mismatches;
                // A tie: the lattice's T at the boundary step sits within rounding of 0.01.
                const double sigma = std::max(0.5 * c, 1e-6);
                const double tk = std::exp(-sigma * span.step * std::min(st.samplesConsumed, ref.consumed));
                if (std::abs(tk - 0.01) < 1e-12) ++nearTies;
            }
        }
        std::cout << "    cutoff sweep: " << cases << " densities, " << mismatches << " step-count mismatches ("
                  << nearTies << " at exact-tie rounding)\n";
        check(mismatches == nearTies, "every cutoff step-count mismatch is an exact-tie rounding case");
    }

    // ---- Authored-revision cache: lifetimes and invalidation --------------
    {
        Rendering::VolumeRayFormCache c;
        auto homogeneousB = hb;
        c.formsFor(homogeneousB);
        const auto cold = c.stats().channelLowerings;
        check(cold == 6, "first request lowers all six channels once");
        for (int frame = 0; frame < 120; ++frame) c.formsFor(homogeneousB);   // camera/clock motion never reaches here
        check(c.stats().channelLowerings == cold, "120 warm frames perform zero lowering");
        // Origin, extent, time are runtime values: the binding changes, the forms do not.
        homogeneousB.origin = glm::vec3(50, 0, 0);
        homogeneousB.scale = glm::vec3(9);
        homogeneousB.temporalCoordinate = 77.0;
        c.formsFor(homogeneousB);
        check(c.stats().channelLowerings == cold, "medium move / resize / clock advance perform zero lowering");
        // A per-channel revision edit relowers that channel only.
        auto newEmission = vectorPw(R"({"c":0.4,"factors":{}})", R"({"c":0.0,"factors":{}})", R"({"c":0.0,"factors":{}})");
        homogeneousB.emissionExpr = newEmission.get();
        homogeneousB.emissionRevision = 9001;
        c.formsFor(homogeneousB);
        check(c.stats().channelLowerings == cold + 1, "an emission edit relowers exactly one channel");
        // A structural density edit that the plan must notice (stale reuse would keep the closed form).
        auto affineDensity = scalarPw(R"({"c":0.8,"factors":{}},{"c":0.01,"factors":{"z":1.0}})");
        homogeneousB.densityExpr = affineDensity.get();
        homogeneousB.densityRevision = 9002;
        const auto& forms = c.formsFor(homogeneousB);
        check(c.stats().channelLowerings == cold + 2, "a density edit relowers exactly one channel");
        const auto plan = Rendering::planVolumeRaySegment({{&forms, glm::dvec3(hOrigin), 0.0}}, {eye, forward, false}, span);
        check(plan.strategy == VolumeSegmentStrategy::GeneralLattice,
              "after a structural density edit the forward ray falls back (no stale closed form)");
        // Same revision, different expression identity: relower, never trust the number alone.
        auto twin = scalarPw(R"({"c":0.8,"factors":{}})");
        homogeneousB.densityExpr = twin.get();
        c.formsFor(homogeneousB);
        check(c.stats().channelLowerings == cold + 3, "an identity change under an unchanged revision still relowers");
        // Revision 0 for a present expression is unrevisioned: relowered on every request.
        homogeneousB.densityRevision = 0;
        c.formsFor(homogeneousB);
        c.formsFor(homogeneousB);
        check(c.stats().uncacheableLowerings == 2, "an unrevisioned channel is never served from the cache");
        // Removal / re-add / reorder.
        auto other = fb;
        c.formsFor(other);
        check(c.size() == 2, "two media are cached under their producer identities");
        c.retainOnly({other.producerId});
        check(c.size() == 1, "retainOnly evicts media that left the projected set");
        const auto before = c.stats().channelLowerings;
        c.formsFor(other);
        check(c.stats().channelLowerings == before, "a retained medium survives eviction of another (reorder-independent)");
        homogeneousB.densityRevision = 9003;
        c.formsFor(homogeneousB);
        check(c.stats().channelLowerings == before + 6, "a re-added medium is lowered afresh");
        c.clear();
        check(c.size() == 0, "clear() drops every entry (reload / teardown)");
    }

    // ---- CPU cost (this machine, Debug; not a GPU or frame claim) ---------
    {
        Rendering::VolumeRayFormCache c;
        const auto t0 = std::chrono::steady_clock::now();
        c.formsFor(hb);
        const auto lowerUs = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        const auto sc = scenario(c, {{&hb, hOrigin, 0.0}});
        constexpr int kReps = 2000;
        auto t1 = std::chrono::steady_clock::now();
        double sink = 0;
        for (int i = 0; i < kReps; ++i) {
            const auto plan = Rendering::planVolumeRaySegment(sc.members, {eye, forward, false}, span);
            Rendering::VolumeTransportState st;
            Rendering::executeVolumeSegment(plan, span, false, st);
            sink += st.transmittance;
        }
        const double planUs = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t1).count() / kReps;
        t1 = std::chrono::steady_clock::now();
        for (int i = 0; i < 50; ++i) sink += lattice<double>(sc.mirror, eye, forward, span, 1.0, false).T;
        const double latticeUs = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t1).count() / 50;
        std::cout << "    CPU Debug: lower 6 channels " << lowerUs << " us (once per revision); plan+execute "
                  << planUs << " us/ray; mirror lattice (96 samples, tree-walking evaluator) " << latticeUs
                  << " us/ray  [sink " << sink << "]\n";
    }

    // ---- Census of real authored media (read-only) ------------------------
    {
        namespace fs = std::filesystem;
        fs::path root = fs::exists("saves/zones") ? fs::path("saves/zones") : fs::path("../saves/zones");
        int media = 0, densityAdmitted = 0, allAdmitted = 0, rung2Candidates = 0,
            radialCandidates = 0, noiseBearing = 0;
        bool northernVeilSeen = false, northernVeilRefused = true;
        std::map<std::string, int> refusalCounts;
        if (fs::exists(root)) {
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
                    VolumeDensityBinding b;
                    if (!field || !Rendering::readVolumeDensity(*field, 0.0, 0.0, b)) continue;
                    ++media;
                    const auto forms = Rendering::lowerVolumeRayChannels(b);
                    const bool d = forms.density.present && forms.density.admitted();
                    densityAdmitted += d;
                    bool all = d;
                    for (const auto* ch : {&forms.extinction, &forms.scattering, &forms.chroma, &forms.phase, &forms.emission})
                        all = all && ch->admitted();
                    allAdmitted += all;
                    const std::string zoneName = zoneDir.path().filename().string();
                    std::cout << "    [" << zoneName << "] " << b.producerId << ": density "
                              << (d ? "admitted" : "refused (" + forms.density.refusal + ")")
                              << (all ? "; all channels admitted" : "") << "\n";
                    if (!d) ++refusalCounts[forms.density.refusal];
                    std::set<std::string> blockers;
                    if (b.densityExpr && !b.densityExpr->pieces.empty() && b.densityExpr->pieces[0].mathNode)
                        nonPolynomial(*b.densityExpr->pieces[0].mathNode, blockers);
                    std::string list;
                    for (const auto& x : blockers) list += (list.empty() ? "" : ", ") + x;
                    std::cout << "        density non-polynomial structure: {" << list << "}\n";
                    const bool onlySegmentable = std::all_of(blockers.begin(), blockers.end(), [](const std::string& x) {
                        return x == "Clamp" || x == "Abs" || x == "Union" || x == "Intersection";
                    });
                    rung2Candidates += (!d && onlySegmentable);
                    const bool radialSegmentable = std::all_of(blockers.begin(), blockers.end(), [](const std::string& x) {
                        return x == "Clamp" || x == "Abs" || x == "Union" || x == "Intersection" ||
                               x == "Length" || x == "Distance" || x == "Sqrt";
                    });
                    radialCandidates += (!d && radialSegmentable);
                    noiseBearing += blockers.count("Noise") ? 1 : 0;
                    if (d) std::cout << "        restricted density degree " << forms.density.scalar.form->degree()
                                     << ": " << forms.density.scalar.form->print() << "\n";
                    if (zoneName == "Northern Veil") { northernVeilSeen = true; northernVeilRefused = northernVeilRefused && !d; }
                }
            }
        }
        std::cout << "    census: " << media << " authored media, " << densityAdmitted << " with admitted density, "
                  << allAdmitted << " with every channel admitted\n";
        std::cout << "    census: " << rung2Candidates << " refused densities contain only clamp/abs/min/max "
                  << "over polynomials (piecewise-polynomial on a ray: the Rung 2 class)\n";
        std::cout << "    census: " << radialCandidates << " refused densities contain only clamp/abs/min/max and "
                  << "Length/Distance/Sqrt over polynomials (boundaries are exact polynomial roots on a ray)\n";
        std::cout << "    census: " << noiseBearing << " densities carry Perlin noise\n";
        check(media > 0, "the census found authored participating media in saves/zones");
        check(!northernVeilSeen || northernVeilRefused,
              "every Northern Veil density refuses honestly (|x|, clamp, sin(x), Perlin) and keeps the lattice");
    }

    std::cout << "volume_ray_segment_transport_test: " << (g_checks - g_failures) << "/" << g_checks
              << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
