// Rung 2 witness (consumer path): Rendering::scheduleVolumeRaySegment and
// executeVolumeSchedule over the accepted lattice.
//
// Reference: a CPU mirror of the fused V5 per-sample law with the EMITTER's
// piece semantics (first piece whose >=/<= bounds hold; no match returns 0),
// channels evaluated by OntoMath's own MathNode::evaluate. Executing a schedule
// runs the mirror only on general runs; exact-empty runs are skipped and
// constant runs use the Rung 1 closed form. The final T and radiance must equal
// the mirror over the whole segment (real-number tolerance), and every sample
// the schedule calls empty must have emitter-evaluated density ≤ 0.
//
// The real-media section reads saves/zones read-only and compares, sample by
// sample on random rays, Rung 2's proved-empty samples with the existing
// VolumeZeroProof grid's proven cells (both are sound skips; they differ in
// what they can prove). CPU observations only — no GPU in this environment.
//
// Claude Code · Claude Opus 5.5 · session_01HvZSR64QghrdyCpq2M7qvX · 2026-10-17.
#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "Singularity/Screen/VolumeDensity.hpp"
#include "Singularity/Screen/VolumeRayTransport.hpp"
#include "Singularity/Screen/VolumeZeroProof.hpp"
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <random>

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
std::string constNode(double c) {
    return R"({"op":0,"scalarForm":{"terms":[{"c":)" + std::to_string(c) + R"(,"factors":{}}]}})";
}
std::string leafNode(const std::string& name) { return R"({"op":1,"var":")" + name + R"("})"; }
std::string opNode(int op, const std::vector<std::string>& children) {
    std::string s = R"({"op":)" + std::to_string(op) + R"(,"children":[)";
    for (std::size_t i = 0; i < children.size(); ++i) s += (i ? "," : "") + children[i];
    return s + "]}";
}
std::string unconditional(const std::string& node) { return R"({"input":"x","pieces":[{"mathNode":)" + node + "}]}"; }
// Radial falloff clamp(k − |p − c|/R, 0, 1) as authored OntoMath (ops: 26 Clamp, 5 Sub, 23 Div, 11 Length, 2 Vec).
std::string radialNode(double k, glm::dvec3 c, double R) {
    const std::string centre = opNode(2, {constNode(c.x), constNode(c.y), constNode(c.z)});
    const std::string dist = opNode(11, {opNode(5, {leafNode("p"), centre})});
    return opNode(26, {opNode(5, {constNode(k), opNode(23, {dist, constNode(R)})}), constNode(0), constNode(1)});
}

struct Medium {
    std::shared_ptr<Piecewise> density, extinction, scattering, chroma, phase, emission;
    VolumeDensityBinding binding(const std::string& id, uint64_t base) const {
        VolumeDensityBinding b;
        b.producerId = id;
        b.densityExpr = density.get(); b.densityRevision = density ? base + 1 : 0;
        b.extinctionExpr = extinction.get(); b.extinctionRevision = extinction ? base + 2 : 0;
        b.scatteringExpr = scattering.get(); b.scatteringRevision = scattering ? base + 3 : 0;
        b.volumeChromaExpr = chroma.get(); b.volumeChromaRevision = chroma ? base + 4 : 0;
        b.phaseExpr = phase.get(); b.phaseRevision = phase ? base + 5 : 0;
        b.emissionExpr = emission.get(); b.emissionRevision = emission ? base + 6 : 0;
        return b;
    }
};

std::map<std::string, PropertyValue> varsAt(const glm::dvec3& p, double t, const glm::dvec3& wo) {
    return {{"p", PropertyValue(glm::vec3(p))}, {"x", PropertyValue(p.x)}, {"y", PropertyValue(p.y)},
            {"z", PropertyValue(p.z)}, {OntoMath::kTimeVar, PropertyValue(t)},
            {OntoMath::kWiXVar, PropertyValue(0.0)}, {OntoMath::kWiYVar, PropertyValue(0.0)},
            {OntoMath::kWiZVar, PropertyValue(0.0)}, {OntoMath::kWoXVar, PropertyValue(wo.x)},
            {OntoMath::kWoYVar, PropertyValue(wo.y)}, {OntoMath::kWoZVar, PropertyValue(wo.z)},
            {OntoMath::kOmegaXVar, PropertyValue(wo.x)}, {OntoMath::kOmegaYVar, PropertyValue(wo.y)},
            {OntoMath::kOmegaZVar, PropertyValue(wo.z)}};
}

// The emitter's Piecewise semantics: first piece whose >=/<= bounds hold
// (pieces without a MathNode skipped); no match returns 0.
std::optional<PropertyValue> emitterEval(const Piecewise& f, const std::map<std::string, PropertyValue>& v) {
    for (const auto& piece : f.pieces) {
        if (!piece.mathNode) continue;
        if (piece.hasLo || piece.hasHi) {
            const auto it = v.find(f.inputVariable);
            double x = 0;
            if (it == v.end() || !propertyValueToNumber(it->second, x)) return std::nullopt;
            if ((piece.hasLo && !(x >= piece.lo)) || (piece.hasHi && !(x <= piece.hi))) continue;
        }
        return piece.mathNode->evaluate(v);
    }
    return PropertyValue(0.0);
}
bool scalarAt(const Piecewise& f, const std::map<std::string, PropertyValue>& v, double& out) {
    const auto r = emitterEval(f, v);
    return r && propertyValueToNumber(*r, out);
}
bool vectorAt(const Piecewise& f, const std::map<std::string, PropertyValue>& v, glm::dvec3& out) {
    const auto r = emitterEval(f, v);
    if (!r) return false;
    if (std::holds_alternative<glm::vec3>(*r)) { out = glm::dvec3(std::get<glm::vec3>(*r)); return true; }
    double s = 0;
    if (propertyValueToNumber(*r, s) && s == 0.0) { out = glm::dvec3(0.0); return true; }   // no-match 0
    return false;
}

// Diagnostic: the top-level multiplicative factors of an authored density.
void productFactors(const OntoMath::MathNode& n, std::vector<const OntoMath::MathNode*>& out) {
    if (n.op == OntoMath::MathNode::Op::Scale && n.children.size() == 2 && n.children[0] && n.children[1]) {
        productFactors(*n.children[0], out);
        productFactors(*n.children[1], out);
        return;
    }
    out.push_back(&n);
}

struct MirrorMember { const VolumeDensityBinding* b; glm::dvec3 origin; double time; bool readsWi; bool readsOmega; };

bool emitterFlag(const VolumeDensityBinding& b, const std::string& name) {
    const auto p = sdfwgsl::compileVolume(b.densityExpr, b.extinctionExpr, b.scatteringExpr, b.volumeChromaExpr,
                                          b.phaseExpr, b.emissionExpr);
    return p.wgsl.find("const " + name + ": bool = true;") != std::string::npos;
}

// The accepted lattice over samples [first, first + count), continuing state.
bool latticeRun(const std::vector<MirrorMember>& members, const glm::dvec3& ro, const glm::dvec3& rd,
                const Rendering::VolumeLatticeSpan& span, uint32_t first, uint32_t count, bool budgetZero,
                Rendering::VolumeTransportState& st) {
    for (uint32_t i = first; i < first + count; ++i) {
        const double s = span.firstSample + span.step * double(i);
        const glm::dvec3 worldP = ro + rd * s;
        double totalExtinction = 0;
        glm::dvec3 totalSource(0.0);
        for (const auto& m : members) {
            const glm::dvec3 p = worldP - m.origin;
            const glm::dvec3 woDelta = ro - worldP;
            const double woLen = glm::length(woDelta);
            const glm::dvec3 wo = woLen > 1e-8 ? woDelta / woLen : glm::dvec3(0.0);
            const auto v = varsAt(p, m.time, wo);
            double D = 0;
            if (!scalarAt(*m.b->densityExpr, v, D)) return false;
            D = std::max(D, 0.0);
            if (!(D > 0.0)) continue;
            double ext = 0.5 * D;
            if (m.b->extinctionExpr && !scalarAt(*m.b->extinctionExpr, v, ext)) return false;
            ext = std::max(ext, 1e-6);
            totalExtinction += ext;
            glm::dvec3 emitted(0.0);
            if (m.b->emissionExpr && (!m.readsOmega || woLen > 1e-8)) {
                if (!vectorAt(*m.b->emissionExpr, v, emitted)) return false;
                emitted = glm::max(emitted, glm::dvec3(0.0));
            }
            double sc = D;
            if (m.b->scatteringExpr) { if (!scalarAt(*m.b->scatteringExpr, v, sc)) return false; sc = std::max(sc, 0.0); }
            glm::dvec3 chroma(1.0);
            if (m.b->volumeChromaExpr && !vectorAt(*m.b->volumeChromaExpr, v, chroma)) return false;
            double phase = 1.0;
            if (m.b->phaseExpr) {
                phase = 0.0;
                if (!m.readsWi && woLen > 1e-8) { if (!scalarAt(*m.b->phaseExpr, v, phase)) return false; phase = std::max(phase, 0.0); }
            }
            totalSource += chroma * sc * phase + emitted;
        }
        const double oldT = st.transmittance;
        if (totalExtinction > 0) {
            st.transmittance *= std::exp(-totalExtinction * span.step);
            st.radiance += totalSource * ((oldT - st.transmittance) / totalExtinction);
        } else {
            st.radiance += oldT * totalSource * span.step;
        }
        ++st.samplesConsumed;
        if (budgetZero && st.transmittance < 0.01) { st.terminated = true; return true; }
    }
    return true;
}

double relErr(double a, double b) { return std::abs(a - b) / std::max({std::abs(a), std::abs(b), 1e-30}); }
double relErr(const glm::dvec3& a, const glm::dvec3& b) {
    return std::max({relErr(a.x, b.x), relErr(a.y, b.y), relErr(a.z, b.z)});
}

struct Case {
    std::vector<std::tuple<const VolumeDensityBinding*, glm::dvec3, double>> media;
};

// Schedule + execute vs the full mirror; also the empty-sample soundness sweep.
Rendering::VolumeSegmentSchedule run(const std::string& label, Rendering::VolumeRayFormCache& cache, const Case& c,
                                     const glm::dvec3& ro, const glm::dvec3& rd, const Rendering::VolumeLatticeSpan& span,
                                     bool budgetZero, double tolerance, bool source = false) {
    std::vector<Rendering::VolumeSegmentMember> members;
    std::vector<MirrorMember> mirror;
    for (const auto& [b, origin, time] : c.media) {
        members.push_back({&cache.formsFor(*b), origin, time});
        mirror.push_back({b, origin, time, emitterFlag(*b, "VOLUME_PHASE_READS_WI"), emitterFlag(*b, "VOLUME_EMISSION_READS_OMEGA")});
    }
    const auto sched = Rendering::scheduleVolumeRaySegment(members, {ro, rd, source}, span);
    std::cout << "    " << label << ": " << sched.runs.size() << " runs, " << sched.pieces << " pieces, "
              << sched.rangeEvaluations << " range evals; samples empty/constant/general = " << sched.emptySamples
              << "/" << sched.constantSamples << "/" << sched.generalSamples
              << (sched.reason.empty() ? "" : "  (" + sched.reason + ")") << "\n";
    // Soundness: every empty sample truly has D ≤ 0 for every member.
    uint32_t violations = 0;
    for (const auto& r : sched.runs) {
        if (r.plan.strategy != VolumeSegmentStrategy::ExactEmpty) continue;
        for (uint32_t i = r.firstSample; i < r.firstSample + r.sampleCount; ++i) {
            const glm::dvec3 worldP = ro + rd * (span.firstSample + span.step * double(i));
            for (const auto& m : mirror) {
                double D = 0;
                if (!scalarAt(*m.b->densityExpr, varsAt(worldP - m.origin, m.time, -rd), D) || D > 0.0) ++violations;
            }
        }
    }
    check(violations == 0, label + ": every proved-empty sample has emitter density ≤ 0");
    if (source) return sched;
    Rendering::VolumeTransportState full, mixed;
    const bool okFull = latticeRun(mirror, ro, rd, span, 0, span.count, budgetZero, full);
    bool okRuns = true;
    Rendering::executeVolumeSchedule(sched, span, budgetZero, mixed,
        [&](uint32_t first, uint32_t count, Rendering::VolumeTransportState& st) {
            okRuns = latticeRun(mirror, ro, rd, span, first, count, budgetZero, st) && okRuns;
        });
    const double eT = relErr(mixed.transmittance, full.transmittance);
    const double eL = (full.radiance == glm::dvec3(0.0) && mixed.radiance == glm::dvec3(0.0)) ? 0.0 : relErr(mixed.radiance, full.radiance);
    std::cout << "      T " << mixed.transmittance << " vs lattice " << full.transmittance << " (rel " << eT
              << "), L rel " << eL << ", consumed " << mixed.samplesConsumed << "/" << full.samplesConsumed << "\n";
    check(okFull && okRuns && eT <= tolerance && eL <= tolerance && mixed.samplesConsumed == full.samplesConsumed &&
              mixed.terminated == full.terminated,
          label + ": scheduled execution equals the accepted lattice (tolerance " + std::to_string(tolerance) + ")");
    return sched;
}

} // namespace

int main() {
    std::cout << "volume_ray_radial_segmentation_test\n";
    Rendering::VolumeRayFormCache cache;
    const glm::dvec3 eye(-40.0, 0.5, 0.25);
    const glm::dvec3 forward(1.0, 0.0, 0.0);
    const glm::dvec3 tilted = glm::normalize(glm::dvec3(1.0, 0.05, -0.02));
    // A 96-sample chord over the medium box [−8, 8] on x (s from 32 to 48).
    const Rendering::VolumeLatticeSpan span{32.0 + 0.5 * (16.0 / 96.0), 16.0 / 96.0, 96};

    // ---- A radial ball: empty outside, varying shell, saturated core -------
    Medium ball;
    ball.density = pw(unconditional(radialNode(1.5, glm::dvec3(0.0), 4.0)));   // core |p| ≤ 2, edge |p| = 6
    ball.emission = pw(unconditional(opNode(2, {constNode(0.3), constNode(0.2), constNode(0.05)})));
    const auto bb = ball.binding("medium.ball", 100);
    check(sdfwgsl::compileVolume(bb.densityExpr, nullptr, nullptr, nullptr, nullptr, bb.emissionExpr).ok,
          "the production emitter compiles the radial ball (planner precondition)");
    const auto& forms = cache.formsFor(bb);
    check(!forms.density.admitted() && forms.density.algebraicAdmitted(),
          "density refuses the polynomial class but is admitted by the algebraic class");
    {
        const auto s = run("ball, through centre", cache, {{{&bb, glm::dvec3(0.0), 0.0}}}, eye, forward, span, false, 1e-6);
        check(s.emptySamples > 0 && s.constantSamples > 0 && s.generalSamples > 0,
              "one chord yields empty, constant (saturated core) and general (shell) runs");
        run("ball, tilted chord", cache, {{{&bb, glm::dvec3(0.0), 0.0}}}, eye, tilted, span, false, 1e-6);
        run("ball, legacy cutoff live", cache, {{{&bb, glm::dvec3(0.0), 0.0}}}, eye, forward, span, true, 1e-6);
        const auto miss = run("ball, chord missing the sphere", cache, {{{&bb, glm::dvec3(0.0), 0.0}}},
                              glm::dvec3(-40.0, 7.0, 0.0), forward, span, false, 1e-12);
        check(miss.emptySamples == span.count, "a chord that misses the ball is wholly, exactly empty");
        const auto lit = run("ball, incident source active", cache, {{{&bb, glm::dvec3(0.0), 0.0}}}, eye, forward, span, false, 0.0, true);
        check(lit.constantSamples == 0 && lit.emptySamples > 0,
              "with a source, the core keeps the lattice but empty space still skips");
    }

    // ---- Rung 2f: Lipschitz-certified support tracing on the ball ---------
    {
        std::vector<Rendering::VolumeSegmentMember> members{{&cache.formsFor(bb), glm::dvec3(0.0), 0.0}};
        const auto trace = Rendering::traceVolumeSupport(members, {eye, forward, false}, span);
        const auto sched = Rendering::scheduleVolumeRaySegment(members, {eye, forward, false}, span);
        uint32_t violations = 0;
        for (uint32_t i = 0; i < span.count; ++i) {
            if (!trace.emptySample[i]) continue;
            double D = 0;
            const glm::dvec3 p = eye + forward * (span.firstSample + span.step * double(i));
            if (!scalarAt(*bb.densityExpr, varsAt(p, 0.0, -forward), D) || D > 0.0) ++violations;
        }
        std::cout << "    trace, ball: " << trace.certifiedSamples << " certified-empty samples (schedule " << sched.emptySamples
                  << "), " << trace.killers << " killers, " << trace.setupEvaluations << " setup + " << trace.pointEvaluations
                  << " point evaluations\n";
        check(violations == 0 && trace.certifiedSamples > 0, "tracing certifies only truly empty samples on the ball");
    }

    // ---- Codex P1 (2026-10-17): cancellation and computed bounds -----------
    // D = −min(F, 0), F = ((A + x) − A) − x, A = 2^54: the algebra says D ≡ 0;
    // the retained double evaluator executes D > 0 wherever round-to-4(x) < x.
    // Nothing may be proved empty or certified where the original tree runs > 0.
    {
        const std::string A = constNode(18014398509481984.0), X = R"({"op":1,"var":"x"})";
        const std::string F = opNode(5, {opNode(5, {opNode(4, {A, X}), A}), X});
        Medium cancel;
        cancel.density = pw(unconditional(opNode(6, {constNode(-1.0), opNode(20, {F, constNode(0.0)})})));
        cancel.emission = pw(unconditional(opNode(2, {constNode(0.3), constNode(0.2), constNode(0.05)})));
        const auto cb = cancel.binding("medium.cancel", 900);
        const auto& forms = cache.formsFor(cb);
        check(forms.density.algebraicAdmitted(), "the cancelled density is admitted by the algebra");
        std::vector<Rendering::VolumeSegmentMember> members{{&forms, glm::dvec3(0.0), 0.0}};
        const Rendering::VolumeRayInput ray{{-6.0, 0.3, 0.2}, {1.0, 0.0, 0.0}, false};
        const Rendering::VolumeLatticeSpan along{0.37, 0.113, 96};
        const auto sched = Rendering::scheduleVolumeRaySegment(members, ray, along);
        const auto trace = Rendering::traceVolumeSupport(members, ray, along);
        const auto certs = Rendering::deriveVolumeSupportCertificates(forms, glm::dvec3(8.0), 1.0);
        const auto certTrace = Rendering::traceVolumeSupportCertified(members, {&certs}, ray, along);
        std::vector<char> scheduled(along.count, 0);
        for (const auto& run : sched.runs)
            if (run.plan.strategy == Rendering::VolumeSegmentStrategy::ExactEmpty)
                for (uint32_t i = run.firstSample; i < run.firstSample + run.sampleCount; ++i) scheduled[i] = 1;
        uint32_t positive = 0, violations = 0;
        for (uint32_t i = 0; i < along.count; ++i) {
            double D = 0;
            const glm::dvec3 p = ray.origin + ray.direction * (along.firstSample + along.step * double(i));
            if (!scalarAt(*cb.densityExpr, varsAt(p, 0.0, -ray.direction), D)) { ++violations; continue; }
            if (D > 0.0) {
                ++positive;
                violations += scheduled[i] + (trace.emptySample.size() > i && trace.emptySample[i]) +
                              (certTrace.emptySample.size() > i && certTrace.emptySample[i]);
            }
        }
        std::cout << "    cancelled density: " << positive << "/" << along.count << " samples execute D > 0; schedule empty "
                  << sched.emptySamples << ", trace " << trace.certifiedSamples << ", certificates "
                  << certTrace.certifiedSamples << " (" << certs.killers.size() << " killers"
                  << (certs.killers.empty() ? "" : ", δ " + std::to_string(certs.killers.front().deviceSlack)) << ")\n";
        check(positive > 0, "the original tree really executes positive density on this ray");
        check(violations == 0, "no proof (schedule, trace, certificate) covers a sample the original tree executes > 0");

        // Computed clamp bound: B − (B − 1) folds to 1 on the host, 0 in f32.
        const std::string B = constNode(33554432.0), Bm1 = constNode(33554431.0);
        Medium bound;
        bound.density = pw(unconditional(opNode(5, {constNode(0.5), opNode(26, {X, opNode(5, {B, Bm1}), constNode(1.0)})})));
        const auto boundForms = Rendering::lowerVolumeRayChannels(bound.binding("medium.bound", 910));
        const auto boundCerts = Rendering::deriveVolumeSupportCertificates(boundForms, glm::dvec3(8.0), 1.0);
        check(!boundForms.density.algebraicAdmitted() && boundCerts.killers.empty(),
              "a computed clamp bound gets no line form and no certificate (" + boundForms.density.algebraic.refusal + ")");
    }

    // ---- Overlap: two balls with independent clocks -----------------------
    Medium pulse;
    pulse.density = pw(unconditional(radialNode(2.0, glm::dvec3(0.0), 2.0)));   // core |p| ≤ 2, edge 4
    pulse.emission = pw(unconditional(opNode(2, {R"({"op":0,"scalarForm":{"terms":[{"c":0.4,"factors":{"t":1.0}}]}})",
                                                 constNode(0.1), constNode(0.0)})));
    const auto pb = pulse.binding("medium.pulse", 200);
    run("overlap: ball + offset pulse, clocks 0 / 2.5", cache,
        {{{&bb, glm::dvec3(0.0), 0.0}, {&pb, glm::dvec3(3.0, 0.5, 0.0), 2.5}}}, eye, forward, span, false, 1e-6);

    // ---- Bounded Piecewise: a slab x ∈ [−3, 3], no match → emitter 0 -------
    Medium slab;
    slab.density = pw(R"({"input":"x","pieces":[{"lo":-3.0,"hi":3.0,"mathNode":)" + constNode(0.7) + "}]}");
    const auto sb = slab.binding("medium.slab", 300);
    {
        const auto s = run("slab Piecewise, bounds on x", cache, {{{&sb, glm::dvec3(0.0), 0.0}}}, eye, forward, span, false, 1e-12);
        check(s.emptySamples > 0 && s.constantSamples > 0, "outside the slab is empty (no match = emitter 0), inside is constant");
    }

    // ---- Refusal keeps everything general (Perlin) -----------------------
    {
        Medium noisy;
        noisy.density = pw(unconditional(opNode(29, {leafNode("p")})));
        const auto nb = noisy.binding("medium.noisy", 400);
        const auto s = run("Perlin density", cache, {{{&nb, glm::dvec3(0.0), 0.0}}}, eye, forward, span, false, 1e-12);
        // Rung 2e: Perlin is admitted. D = noise(p) is negative over much of a chord;
        // the density-sign target proves those stretches empty (soundness checked in run()).
        check(s.reason.empty() && s.emptySamples > 0 && s.constantSamples == 0,
              "a pure-noise density: proved-empty where noise ≤ 0, lattice elsewhere, never constant");
    }

    // ---- Resolution trades proofs for lattice samples near switches -------
    {
        std::vector<Rendering::VolumeSegmentMember> members{{&cache.formsFor(bb), glm::dvec3(0.0), 0.0}};
        const auto coarse = Rendering::scheduleVolumeRaySegment(members, {eye, tilted, false}, span, 1.0);
        const auto fine = Rendering::scheduleVolumeRaySegment(members, {eye, tilted, false}, span, 0.01);
        std::cout << "    resolution 1 step: " << coarse.rangeEvaluations << " evals, " << coarse.generalSamples
                  << " general; resolution 0.01 step: " << fine.rangeEvaluations << " evals, " << fine.generalSamples << " general\n";
        check(fine.generalSamples <= coarse.generalSamples, "finer resolution never leaves more samples on the lattice");
    }

    // ---- Real authored media: soundness and sample demand vs the zero-proof grid
    {
        namespace fs = std::filesystem;
        // Other suite tests create fixture Zones under the build directory's own
        // saves/zones; the census reads whichever candidate holds the most Zones
        // (the frozen snapshot), never merely the first that exists.
        fs::path root = "../saves/zones";
        {
            std::size_t best = 0;
            for (const char* candidate : {"saves/zones", "../saves/zones"}) {
                std::error_code ec;
                if (!fs::is_directory(candidate, ec)) continue;
                std::size_t zones = 0;
                for (const auto& entry : fs::directory_iterator(candidate, ec)) zones += entry.is_directory() ? 1u : 0u;
                if (zones > best) { best = zones; root = candidate; }
            }
        }
        struct Totals { uint64_t samples = 0, empty = 0, constant = 0, grid = 0, both = 0, violations = 0; double us = 0; uint64_t rays = 0;
                        uint64_t traced = 0, tracedOrGrid = 0, tracePoints = 0, traceSetup = 0, traceViolations = 0; double traceUs = 0;
                        uint64_t certTraced = 0, certPoints = 0, certViolations = 0; double certUs = 0; };
        Totals all;
        int admitted = 0, media = 0, eligibleMedia = 0, preludeMedia = 0, preludeFlips = 0, preludeDemotions = 0;
        std::mt19937 rng(1017u);
        std::uniform_real_distribution<double> u(-1.0, 1.0);
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
                    Rendering::VolumeRayFormCache local;
                    const auto& f = local.formsFor(b);
                    if (!f.density.algebraicAdmitted()) {
                        std::cout << "    [" << zoneDir.path().filename().string() << "] " << b.producerId
                                  << ": density refused — " << f.density.algebraic.refusal << "\n";
                        continue;
                    }
                    ++admitted;
                    const glm::dvec3 half = glm::abs(glm::dvec3(b.scale));
                    const auto proof = Rendering::buildVolumeZeroProof(*b.densityExpr, glm::vec3(half));
                    const glm::dvec3 cell = 2.0 * half / glm::dvec3(proof.dims);
                    Totals t;
                    const auto certs = Rendering::deriveVolumeSupportCertificates(f, half);
                    // Attribution of samples Rung 2 proves empty but the grid does not:
                    // which top-level density factor is actually zero there.
                    std::vector<const OntoMath::MathNode*> factors;
                    if (b.densityExpr->pieces[0].mathNode) productFactors(*b.densityExpr->pieces[0].mathNode, factors);
                    std::map<std::string, uint64_t> attribution;
                    for (int r = 0; r < 64; ++r) {
                        // A ray from outside the box through a random interior point.
                        const glm::dvec3 target = half * glm::dvec3(u(rng), u(rng), u(rng));
                        glm::dvec3 dir(u(rng), u(rng), u(rng));
                        if (glm::length(dir) < 1e-3) continue;
                        dir /= glm::length(dir);
                        const glm::dvec3 ro = target - dir * (3.0 * glm::length(half) + 1.0);
                        double t0 = -1e300, t1 = 1e300;
                        for (int a = 0; a < 3; ++a) {
                            if (std::abs(dir[a]) < 1e-12) continue;
                            double ta = (-half[a] - ro[a]) / dir[a], tb = (half[a] - ro[a]) / dir[a];
                            if (ta > tb) std::swap(ta, tb);
                            t0 = std::max(t0, ta); t1 = std::min(t1, tb);
                        }
                        if (!(t1 > t0) || t0 <= 0) continue;
                        const Rendering::VolumeLatticeSpan chord{t0 + 0.5 * (t1 - t0) / 96.0, (t1 - t0) / 96.0, 96};
                        std::vector<Rendering::VolumeSegmentMember> members{{&f, glm::dvec3(0.0), 0.0}};
                        const auto c0 = std::chrono::steady_clock::now();
                        const auto sched = Rendering::scheduleVolumeRaySegment(members, {ro, dir, false}, chord);
                        t.us += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - c0).count();
                        ++t.rays;
                        const auto c1 = std::chrono::steady_clock::now();
                        const auto trace = Rendering::traceVolumeSupport(members, {ro, dir, false}, chord);
                        t.traceUs += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - c1).count();
                        t.tracePoints += trace.pointEvaluations;
                        Rendering::VolumeSupportTrace certTrace;
                        certTrace.emptySample.assign(chord.count, 0);
                        if (!certs.killers.empty()) {
                            const auto c2 = std::chrono::steady_clock::now();
                            certTrace = Rendering::traceVolumeSupportCertified(members, {&certs}, {ro, dir, false}, chord);
                            t.certUs += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - c2).count();
                            t.certPoints += certTrace.pointEvaluations;
                        }
                        t.traceSetup += trace.setupEvaluations;
                        std::vector<char> label(chord.count, 'g');
                        for (const auto& run : sched.runs)
                            for (uint32_t i = run.firstSample; i < run.firstSample + run.sampleCount; ++i)
                                label[i] = run.plan.strategy == VolumeSegmentStrategy::ExactEmpty ? 'e'
                                         : run.plan.strategy == VolumeSegmentStrategy::ConstantClosedForm ? 'c' : 'g';
                        for (uint32_t i = 0; i < chord.count; ++i) {
                            const glm::dvec3 p = ro + dir * (chord.firstSample + chord.step * double(i));   // origin bound as 0
                            glm::uvec3 idx;
                            bool inGrid = proof.totalCells > 0;
                            for (int a = 0; a < 3 && inGrid; ++a) {
                                const double k = std::floor((p[a] + half[a]) / cell[a]);
                                if (k < 0 || k >= proof.dims[a]) inGrid = false; else idx[a] = uint32_t(k);
                            }
                            const bool gridSkip = inGrid && proof.proven(idx.x, idx.y, idx.z);
                            const bool traced = trace.emptySample[i] != 0;
                            if (certTrace.emptySample[i]) {
                                ++t.certTraced;
                                double Dc = 0;
                                if (!scalarAt(*b.densityExpr, varsAt(p, 0.0, -dir), Dc) || Dc > 0.0) ++t.certViolations;
                            }
                            t.traced += traced;
                            t.tracedOrGrid += traced || gridSkip;
                            if (traced) {
                                double Dt = 0;
                                if (!scalarAt(*b.densityExpr, varsAt(p, 0.0, -dir), Dt) || Dt > 0.0) ++t.traceViolations;
                            }
                            ++t.samples;
                            t.empty += label[i] == 'e';
                            t.constant += label[i] == 'c';
                            t.grid += gridSkip;
                            t.both += gridSkip && label[i] == 'e';
                            if (label[i] == 'e' && !gridSkip && factors.size() > 1) {
                                std::string zeros;
                                for (std::size_t k = 0; k < factors.size(); ++k) {
                                    const auto fv = factors[k]->evaluate(varsAt(p, 0.0, -dir));
                                    double x = 1;
                                    if (fv && propertyValueToNumber(*fv, x) && x <= 0.0) zeros += (zeros.empty() ? "" : "+") + std::string("f") + std::to_string(k);
                                }
                                ++attribution[zeros.empty() ? "none-zero" : zeros];
                            }
                            if (label[i] == 'e') {
                                double D = 0;
                                if (!scalarAt(*b.densityExpr, varsAt(p, 0.0, -dir), D) || D > 0.0) ++t.violations;
                            }
                        }
                    }
                    std::cout << "    [" << zoneDir.path().filename().string() << "] " << b.producerId << ": "
                              << t.samples << " samples; Rung 2 empty " << (100.0 * t.empty / std::max<uint64_t>(t.samples, 1))
                              << "%, constant " << (100.0 * t.constant / std::max<uint64_t>(t.samples, 1))
                              << "%; zero-proof grid " << (100.0 * t.grid / std::max<uint64_t>(t.samples, 1))
                              << "% (" << proof.provenCells << "/" << proof.totalCells << " cells proven)"
                              << "; both " << (100.0 * t.both / std::max<uint64_t>(t.samples, 1)) << "%; "
                              << (t.us / std::max<uint64_t>(t.rays, 1)) << " us/ray\n";
                    // Cost anatomy on the same 64-ray family: density evaluator
                    // construction vs segmentation with/without analytic proposals.
                    {
                        std::mt19937 rr(7u);
                        std::uniform_real_distribution<double> ur(-1.0, 1.0);
                        double tBind = 0, tOn = 0, tOff = 0;
                        uint64_t eOn = 0, eOff = 0, n = 0;
                        for (int r = 0; r < 16; ++r) {   // 16 rays: the anatomy is a cost probe, not a sweep
                            const glm::dvec3 target = half * glm::dvec3(ur(rr), ur(rr), ur(rr));
                            glm::dvec3 dir(ur(rr), ur(rr), ur(rr));
                            if (glm::length(dir) < 1e-3) continue;
                            dir /= glm::length(dir);
                            const glm::dvec3 ro = target - dir * (3.0 * glm::length(half) + 1.0);
                            std::map<std::string, double> bind;
                            for (int a = 0; a < 3; ++a) {
                                bind[OntoMath::kLineOriginVars[a]] = ro[a];
                                bind[OntoMath::kLineDirectionVars[a]] = dir[a];
                            }
                            bind[OntoMath::kTimeVar] = 0.0;
                            auto c0 = std::chrono::steady_clock::now();
                            OntoMath::LineAlgebraicEvaluator ev(*f.density.algebraic.form, bind, true);
                            auto c1 = std::chrono::steady_clock::now();
                            if (!ev.ok()) continue;
                            OntoMath::LineSegmentationStats son, soff;
                            const double len = 3.0 * glm::length(half) + 1.0;
                            OntoMath::segmentLine({&ev}, len - glm::length(half), len + glm::length(half), 0.02 * glm::length(half), &son, 1u << 16, true);
                            auto c2 = std::chrono::steady_clock::now();
                            OntoMath::segmentLine({&ev}, len - glm::length(half), len + glm::length(half), 0.02 * glm::length(half), &soff, 1u << 16, false);
                            auto c3 = std::chrono::steady_clock::now();
                            tBind += std::chrono::duration<double, std::micro>(c1 - c0).count();
                            tOn += std::chrono::duration<double, std::micro>(c2 - c1).count();
                            tOff += std::chrono::duration<double, std::micro>(c3 - c2).count();
                            eOn += son.rangeEvaluations; eOff += soff.rangeEvaluations; ++n;
                        }
                        if (n) std::cout << "        cost anatomy (us/ray): bind density " << tBind / n << "; segment with proposals "
                                         << tOn / n << " (" << double(eOn) / n << " evals), without " << tOff / n << " ("
                                         << double(eOff) / n << " evals); density form " << f.density.algebraic.form->nodes.size()
                                         << " nodes, " << f.density.algebraic.form->switches.size() << " switches\n";
                    }
                    std::cout << "        trace: certified " << (100.0 * t.traced / std::max<uint64_t>(t.samples, 1))
                              << "%, trace∪grid " << (100.0 * t.tracedOrGrid / std::max<uint64_t>(t.samples, 1))
                              << "%; " << double(t.tracePoints) / std::max<uint64_t>(t.rays, 1) << " point + "
                              << double(t.traceSetup) / std::max<uint64_t>(t.rays, 1) << " setup evals/ray; "
                              << t.traceUs / std::max<uint64_t>(t.rays, 1) << " us/ray\n";
                    std::cout << "        certificates: " << certs.killers.size() << " killers"
                              << (certs.killers.empty() ? " (" + certs.refusal + ")" : "")
                              << "; certified-only trace " << (100.0 * t.certTraced / std::max<uint64_t>(t.samples, 1))
                              << "%, " << double(t.certPoints) / std::max<uint64_t>(t.rays, 1) << " point evals/ray, "
                              << t.certUs / std::max<uint64_t>(t.rays, 1) << " us/ray\n";
                    {
                        // Eligibility (Kernel cost ranking over the renderer's own loop),
                        // with the shared-noise prelude the renderer's memo also counts.
                        const auto prelude = sdfwgsl::inspectVolumeSharedPrelude(
                            b.densityExpr, b.extinctionExpr, b.scatteringExpr, b.volumeChromaExpr, b.phaseExpr, b.emissionExpr);
                        const double sampleCost = Rendering::volumeDensitySampleCost(
                            *b.densityExpr, prelude.densityUses, prelude.groups, prelude.densitySites);
                        const double ownCost = Rendering::volumeDensitySampleCost(*b.densityExpr, false, {}, {});
                        const auto est = Rendering::estimateVolumeSupportCertificateGain(f, certs, &proof, half, 96, 0.0, sampleCost);
                        if (prelude.densityUses) {
                            ++preludeMedia;
                            const auto own = Rendering::estimateVolumeSupportCertificateGain(f, certs, &proof, half, 96, 0.0, ownCost);
                            preludeFlips += (est.eligible && !own.eligible) ? 1 : 0;
                            preludeDemotions += (!est.eligible && own.eligible) ? 1 : 0;
                            std::cout << "        shared prelude before density: " << prelude.groups.size()
                                      << " group(s); sample cost " << sampleCost << " vs density text " << ownCost
                                      << "; ratio without prelude " << (own.costWithout > 0 ? own.costWith / own.costWithout : 0.0) << "\n";
                        }
                        std::cout << "        eligibility: " << (est.eligible ? "ELIGIBLE" : "not eligible") << " — "
                                  << est.reason << "; cost with/without " << est.costWith << "/" << est.costWithout
                                  << " (ratio " << (est.costWithout > 0 ? est.costWith / est.costWithout : 0.0) << "), grid "
                                  << est.gridSamples << "/" << est.samples << ", certified " << est.certifiedSamples << "\n";
                        eligibleMedia += est.eligible ? 1 : 0;
                    }
                    if (!attribution.empty() && zoneDir.path().filename().string() == "Northern Veil") {
                        std::cout << "        extra-proof attribution (Rung 2 empty, grid not):";
                        for (const auto& [k, v] : attribution) std::cout << " " << k << "=" << v;
                        std::cout << "\n";
                        for (std::size_t k = 0; k < factors.size(); ++k)
                            std::cout << "          f" << k << " = " << factors[k]->print().substr(0, 110) << "\n";
                    }
                    all.certTraced += t.certTraced; all.certPoints += t.certPoints; all.certViolations += t.certViolations; all.certUs += t.certUs;
                    all.traced += t.traced; all.tracedOrGrid += t.tracedOrGrid; all.traceViolations += t.traceViolations;
                    all.tracePoints += t.tracePoints; all.traceSetup += t.traceSetup; all.traceUs += t.traceUs;
                    all.samples += t.samples; all.empty += t.empty; all.constant += t.constant; all.grid += t.grid;
                    all.both += t.both; all.violations += t.violations; all.us += t.us; all.rays += t.rays;
                }
            }
        }
        std::cout << "    real media: " << admitted << " of " << media << " densities admitted by the algebraic class\n";
        std::cout << "    totals: Rung 2 empty " << (100.0 * all.empty / std::max<uint64_t>(all.samples, 1))
                  << "%, constant-density " << (100.0 * all.constant / std::max<uint64_t>(all.samples, 1))
                  << "%, zero-proof grid " << (100.0 * all.grid / std::max<uint64_t>(all.samples, 1))
                  << "%, Rung 2 empty but grid not " << (100.0 * (all.empty - all.both) / std::max<uint64_t>(all.samples, 1))
                  << "%, grid but Rung 2 not " << (100.0 * (all.grid - all.both) / std::max<uint64_t>(all.samples, 1))
                  << "%; " << (all.us / std::max<uint64_t>(all.rays, 1)) << " us/ray schedule (CPU, this build type)\n";
        std::cout << "    trace totals: certified " << (100.0 * all.traced / std::max<uint64_t>(all.samples, 1))
                  << "%, trace∪grid " << (100.0 * all.tracedOrGrid / std::max<uint64_t>(all.samples, 1)) << "%; "
                  << double(all.tracePoints) / std::max<uint64_t>(all.rays, 1) << " point + "
                  << double(all.traceSetup) / std::max<uint64_t>(all.rays, 1) << " setup evals/ray; "
                  << all.traceUs / std::max<uint64_t>(all.rays, 1) << " us/ray\n";
        check(all.traceViolations == 0, "Lipschitz tracing certifies no real sample with positive density");
        std::cout << "    certificate totals: certified-only trace " << (100.0 * all.certTraced / std::max<uint64_t>(all.samples, 1))
                  << "%, " << double(all.certPoints) / std::max<uint64_t>(all.rays, 1) << " point evals/ray, "
                  << all.certUs / std::max<uint64_t>(all.rays, 1) << " us/ray\n";
        std::cout << "    eligibility: " << eligibleMedia << " of " << admitted << " admitted media estimated cheaper with certificates\n";
        std::cout << "    shared prelude: " << preludeMedia << " admitted media fill it before density; counting it promotes "
                  << preludeFlips << " and demotes " << preludeDemotions << "\n";
        check(all.certViolations == 0, "certificate-only tracing certifies no real sample with positive density");
        check(media > 0 && admitted > 0, "the census finds real media the algebraic class admits");
        check(all.violations == 0, "no proved-empty sample on any real medium has positive emitter density (" +
                                       std::to_string(all.samples) + " samples)");
    }

    std::cout << "volume_ray_radial_segmentation_test: " << (g_checks - g_failures) << "/" << g_checks
              << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
