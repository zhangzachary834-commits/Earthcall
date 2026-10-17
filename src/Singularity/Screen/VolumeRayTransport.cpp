#include "Singularity/Screen/VolumeRayTransport.hpp"

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <iterator>
#include <memory>

// See VolumeRayTransport.hpp for the transport law this mirrors and the
// exactness contract. Every constant below that is not new mathematics is a
// mirror of a named line in SdfWgsl.cpp's single/fused marchers:
//   kMinExtinction       max(volumeExtinctionEval(...), 1e-6)
//   kLegacyCutoff        `transmittance < 0.01` break under output budget 0
//   kDirectionEpsilon    woLen > 1e-8 / emissionLen > 1e-8 gates
// Claude Code · Claude Opus 5.5 · session_01HvZSR64QghrdyCpq2M7qvX · 2026-10-16.
namespace Rendering {
namespace {

constexpr double kMinExtinction = 1e-6;
constexpr double kLegacyCutoff = 0.01;
// The marchers gate direction-reading channels on |ro − worldP| > 1e-8 (f32).
// The closed form requires the FIRST sample to clear that by 100×, so the gate
// is provably open at every sample of the span; otherwise the lattice runs.
constexpr double kDirectionGateMargin = 1e-6;

void collectReads(const OntoMath::MathNode& node, std::set<std::string>& out) {
    if (node.op == OntoMath::MathNode::Op::ScalarLeaf) {
        for (const auto& term : node.scalarForm.terms) {
            for (const auto& [name, exponent] : term.factors) out.insert(name);
            for (const auto& tf : term.trans) out.insert(tf.variable);
        }
    }
    if (node.op == OntoMath::MathNode::Op::ValueLeaf && !node.variableName.empty()) {
        out.insert(node.variableName);
    }
    for (const auto& c : node.children) if (c) collectReads(*c, out);
}

// What emitPiecewise binds/reads: the input variable first, then each piece
// carrying a MathNode IN ORDER, stopping after the first one without interval
// bounds — the emitter writes `return` there and never visits later pieces.
// (An earlier draft walked every piece; volume_ray_segment_transport_test caught
// it against the real emitter's VOLUME_PHASE_READS_WI.)
std::set<std::string> emitterReadsOf(const OntoMath::Piecewise& model) {
    std::set<std::string> out;
    out.insert(model.inputVariable);
    for (const auto& piece : model.pieces) {
        if (!piece.mathNode) continue;
        collectReads(*piece.mathNode, out);
        if (!piece.hasLo && !piece.hasHi) break;
    }
    return out;
}

bool isLineSymbol(const std::string& name) {
    if (name == OntoMath::kLineParameterVar) return true;
    for (int i = 0; i < 3; ++i) {
        if (name == OntoMath::kLineOriginVars[i] || name == OntoMath::kLineDirectionVars[i]) return true;
    }
    return false;
}

// The channel's execution-context binding domain, mirroring pointComponent():
// every medium channel binds the point and its own clock; phase additionally
// binds wi/wo, emission binds omega. A restricted form reading anything else
// would be refused by the emitter, so it is refused here too.
enum class ChannelContext { Medium, Phase, Emission };

bool bindsInContext(const std::string& name, ChannelContext ctx) {
    if (isLineSymbol(name) || name == OntoMath::kTimeVar) return true;
    if (ctx == ChannelContext::Phase) {
        return name == OntoMath::kWiXVar || name == OntoMath::kWiYVar || name == OntoMath::kWiZVar ||
               name == OntoMath::kWoXVar || name == OntoMath::kWoYVar || name == OntoMath::kWoZVar;
    }
    if (ctx == ChannelContext::Emission) {
        return name == OntoMath::kOmegaXVar || name == OntoMath::kOmegaYVar || name == OntoMath::kOmegaZVar;
    }
    return false;
}

VolumeRayChannelForm lowerChannel(const OntoMath::Piecewise* expr, uint64_t revision,
                                  bool vectorValued, ChannelContext ctx) {
    VolumeRayChannelForm form;
    form.expr = expr;
    form.revision = revision;
    form.vectorValued = vectorValued;
    form.present = expr && !expr->pieces.empty();
    if (!form.present) return form;
    form.emitterReads = emitterReadsOf(*expr);

    // Rung 2 algebraic restriction first: independent of whether the polynomial
    // class below admits the channel.
    form.algebraic = OntoMath::restrictAlgebraicToLine(*expr, vectorValued);
    if (form.algebraic) {
        for (const auto& name : form.algebraic.form->reads) {
            if (!bindsInContext(name, ctx)) {
                form.algebraic.form.reset();
                form.algebraic.refusal = "reads '" + name + "', which this channel's execution context does not bind";
                break;
            }
        }
    }

    std::set<std::string> reads;
    if (vectorValued) {
        form.vector = OntoMath::restrictVectorToLine(*expr);
        if (!form.vector) { form.refusal = form.vector.refusal; return form; }
        for (const auto& component : *form.vector.form) reads.insert(component.reads.begin(), component.reads.end());
    } else {
        form.scalar = OntoMath::restrictToLine(*expr);
        if (!form.scalar) { form.refusal = form.scalar.refusal; return form; }
        reads = form.scalar.form->reads;
    }
    for (const auto& name : reads) {
        if (!bindsInContext(name, ctx)) {
            form.refusal = "reads '" + name + "', which this channel's execution context does not bind";
            return form;
        }
    }
    return form;
}

struct ChannelSlot {
    VolumeRayChannelForm VolumeRayChannelForms::*member;
    const OntoMath::Piecewise* VolumeDensityBinding::*expr;
    uint64_t VolumeDensityBinding::*revision;
    bool vectorValued;
    ChannelContext ctx;
};

const ChannelSlot kSlots[] = {
    {&VolumeRayChannelForms::density, &VolumeDensityBinding::densityExpr,
     &VolumeDensityBinding::densityRevision, false, ChannelContext::Medium},
    {&VolumeRayChannelForms::extinction, &VolumeDensityBinding::extinctionExpr,
     &VolumeDensityBinding::extinctionRevision, false, ChannelContext::Medium},
    {&VolumeRayChannelForms::scattering, &VolumeDensityBinding::scatteringExpr,
     &VolumeDensityBinding::scatteringRevision, false, ChannelContext::Medium},
    {&VolumeRayChannelForms::chroma, &VolumeDensityBinding::volumeChromaExpr,
     &VolumeDensityBinding::volumeChromaRevision, true, ChannelContext::Medium},
    {&VolumeRayChannelForms::phase, &VolumeDensityBinding::phaseExpr,
     &VolumeDensityBinding::phaseRevision, false, ChannelContext::Phase},
    {&VolumeRayChannelForms::emission, &VolumeDensityBinding::emissionExpr,
     &VolumeDensityBinding::emissionRevision, true, ChannelContext::Emission},
};

// Numeric premises of one member on this ray.
std::map<std::string, double> rayBindings(const VolumeSegmentMember& m, const VolumeRayInput& ray) {
    std::map<std::string, double> b;
    const glm::dvec3 o = ray.origin - m.origin;   // p = worldP − origin
    const double len = glm::length(ray.direction);
    const glm::dvec3 wo = len > 0.0 ? -ray.direction / len : glm::dvec3(0.0);
    for (int i = 0; i < 3; ++i) {
        b[OntoMath::kLineOriginVars[i]] = o[i];
        b[OntoMath::kLineDirectionVars[i]] = ray.direction[i];
    }
    b[OntoMath::kTimeVar] = m.temporalCoordinate;
    // No incident source: the marchers bind wi = 0 and wo = ω = normalize(ro − worldP),
    // which is exactly −d/|d| for every sample at s > 0.
    b[OntoMath::kWiXVar] = 0.0; b[OntoMath::kWiYVar] = 0.0; b[OntoMath::kWiZVar] = 0.0;
    b[OntoMath::kWoXVar] = wo.x; b[OntoMath::kWoYVar] = wo.y; b[OntoMath::kWoZVar] = wo.z;
    b[OntoMath::kOmegaXVar] = wo.x; b[OntoMath::kOmegaYVar] = wo.y; b[OntoMath::kOmegaZVar] = wo.z;
    return b;
}

// Exactly-constant value of a restricted scalar on this ray, or a reason.
std::optional<double> constantOn(const OntoMath::LineRestrictedForm& f,
                                 const std::map<std::string, double>& b,
                                 const std::string& what, std::string& reason) {
    if (f.exactDegreeOn(b) > 0) {
        reason = what + " varies along this ray (exact degree " + std::to_string(f.exactDegreeOn(b)) + ")";
        return std::nullopt;
    }
    const auto a = f.evaluateCoefficients(b);
    if (!a) { reason = what + " is undefined or non-finite on this ray"; return std::nullopt; }
    return a->empty() ? 0.0 : (*a)[0];
}

// D_j on the ray when exactly constant; std::nullopt (with reason) otherwise.
std::optional<double> constantDensity(const VolumeSegmentMember& m, const std::map<std::string, double>& b,
                                      std::string& reason) {
    const auto& d = m.forms->density;
    if (!d.present) { reason = "medium has no authored density"; return std::nullopt; }
    if (!d.admitted()) { reason = "density: " + d.refusal; return std::nullopt; }
    const auto value = constantOn(*d.scalar.form, b, "density", reason);
    if (!value) return std::nullopt;
    return std::max(*value, 0.0);
}

// One participating member's constant transport contribution under the
// marchers' laws (compatibility defaults, clamps, Li = 1 without a source, the
// READS_WI gate). `constant` answers the exactly-constant value of a present
// channel component on the region in question, or nullopt with a reason.
using ConstantChannel = std::function<std::optional<double>(const VolumeRayChannelForm&, int component,
                                                            const char* what, std::string& why)>;

bool accumulateConstantMember(const VolumeRayChannelForms& f, double density, const ConstantChannel& constant,
                              VolumeSegmentPlan& plan, std::string& why) {
    auto vec = [&](const VolumeRayChannelForm& c, const char* what) -> std::optional<glm::dvec3> {
        glm::dvec3 v(0.0);
        for (int i = 0; i < 3; ++i) {
            const auto x = constant(c, i, what, why);
            if (!x) return std::nullopt;
            v[i] = *x;
        }
        return v;
    };
    double extinction = 0.5 * density;   // V1 compatibility law
    if (f.extinction.present) {
        const auto v = constant(f.extinction, 0, "extinction", why);
        if (!v) return false;
        extinction = *v;
    }
    extinction = std::max(extinction, kMinExtinction);
    double scattering = density;          // V2 compatibility law
    if (f.scattering.present) {
        const auto v = constant(f.scattering, 0, "scattering", why);
        if (!v) return false;
        scattering = std::max(*v, 0.0);
    }
    glm::dvec3 chroma(1.0);               // V2 neutral white
    if (f.chroma.present) {
        const auto v = vec(f.chroma, "medium chroma");
        if (!v) return false;
        chroma = *v;
    }
    double phase = 1.0;                   // V3 isotropic identity
    if (f.phase.present) {
        // Without a source, the marchers evaluate phase only when the emitted
        // phase never reads wi; otherwise phase = 0.
        phase = 0.0;
        if (!f.phase.reads("wi.")) {
            const auto v = constant(f.phase, 0, "phase", why);
            if (!v) return false;
            phase = std::max(*v, 0.0);
        }
    }
    glm::dvec3 emitted(0.0);
    if (f.emission.present) {
        const auto v = vec(f.emission, "emission");
        if (!v) return false;
        emitted = glm::max(*v, glm::dvec3(0.0));
    }
    // Li = 1 without a source (the marchers' incidentLi default).
    plan.totalExtinction += extinction;
    plan.totalSource += chroma * scattering * phase + emitted;
    ++plan.participatingMembers;
    return true;
}

bool finiteTransport(const VolumeSegmentPlan& plan) {
    return std::isfinite(plan.totalExtinction) && std::isfinite(plan.totalSource.x) &&
           std::isfinite(plan.totalSource.y) && std::isfinite(plan.totalSource.z);
}

} // namespace

bool VolumeRayChannelForm::reads(const std::string& prefix) const {
    for (const auto& name : emitterReads) {
        if (name.compare(0, prefix.size(), prefix) == 0) return true;
    }
    return false;
}

VolumeRayChannelForms lowerVolumeRayChannels(const VolumeDensityBinding& medium) {
    VolumeRayChannelForms forms;
    forms.producerId = medium.producerId;
    for (const auto& slot : kSlots) {
        forms.*slot.member = lowerChannel(medium.*slot.expr, medium.*slot.revision,
                                          slot.vectorValued, slot.ctx);
    }
    forms.occluderPresent = medium.occluderSdf != nullptr;
    return forms;
}

const VolumeRayChannelForms& VolumeRayFormCache::formsFor(const VolumeDensityBinding& medium) {
    ++_stats.requests;
    if (medium.producerId.empty()) {
        _scratch = lowerVolumeRayChannels(medium);
        _stats.channelLowerings += std::size(kSlots);
        _stats.uncacheableLowerings += std::size(kSlots);
        return _scratch;
    }
    auto it = _entries.find(medium.producerId);
    const bool fresh = it == _entries.end();
    if (fresh) it = _entries.emplace(medium.producerId, VolumeRayChannelForms{}).first;
    auto& forms = it->second;
    forms.producerId = medium.producerId;
    for (const auto& slot : kSlots) {
        auto& channel = forms.*slot.member;
        const auto* expr = medium.*slot.expr;
        const uint64_t revision = medium.*slot.revision;
        const bool present = expr && !expr->pieces.empty();
        const bool unrevisioned = present && revision == 0;
        if (!fresh && !unrevisioned && channel.expr == expr && channel.revision == revision) continue;
        channel = lowerChannel(expr, revision, slot.vectorValued, slot.ctx);
        ++_stats.channelLowerings;
        if (unrevisioned) ++_stats.uncacheableLowerings;
    }
    forms.occluderPresent = medium.occluderSdf != nullptr;
    return forms;
}

void VolumeRayFormCache::retainOnly(const std::set<std::string>& producerIds) {
    for (auto it = _entries.begin(); it != _entries.end();) {
        if (producerIds.count(it->first)) ++it;
        else it = _entries.erase(it);
    }
}

const char* volumeSegmentStrategyName(VolumeSegmentStrategy s) {
    switch (s) {
        case VolumeSegmentStrategy::GeneralLattice: return "general-lattice";
        case VolumeSegmentStrategy::ExactEmpty: return "exact-empty";
        case VolumeSegmentStrategy::ConstantClosedForm: return "constant-closed-form";
    }
    return "unknown";
}

VolumeSegmentPlan planVolumeRaySegment(const std::vector<VolumeSegmentMember>& members,
                                       const VolumeRayInput& ray,
                                       const VolumeLatticeSpan& span) {
    VolumeSegmentPlan plan;
    auto lattice = [&](std::string why) {
        plan = VolumeSegmentPlan{};
        plan.reason = std::move(why);
        return plan;
    };
    if (span.count == 0 || !(span.step > 0.0) || !std::isfinite(span.step) ||
        !std::isfinite(span.firstSample)) {
        return lattice("degenerate lattice span");
    }
    const double dirLen = glm::length(ray.direction);
    if (!(dirLen > 0.0) || !std::isfinite(dirLen) || !std::isfinite(glm::length(ray.origin))) {
        return lattice("degenerate or non-finite ray");
    }
    if (members.empty()) return lattice("no member media on this segment");

    // Pass 1: density alone decides exact emptiness, with or without a source.
    std::vector<std::map<std::string, double>> bindings;
    std::vector<double> densities;
    for (const auto& m : members) {
        if (!m.forms) return lattice("member without lowered forms");
        bindings.push_back(rayBindings(m, ray));
        std::string why;
        const auto d = constantDensity(m, bindings.back(), why);
        if (!d) return lattice(why);
        densities.push_back(*d);
    }
    if (std::all_of(densities.begin(), densities.end(), [](double d) { return d <= 0.0; })) {
        plan.strategy = VolumeSegmentStrategy::ExactEmpty;
        plan.reason = "every member's density is exactly constant and non-positive on this ray";
        return plan;
    }

    // Pass 2: participating members must be fully constant.
    if (ray.incidentSourceActive) {
        return lattice("incident light varies along the ray (source distance, radiance, visibility)");
    }
    if (!(span.firstSample > 0.0) || span.firstSample * dirLen <= kDirectionGateMargin) {
        return lattice("first sample too close to the eye for the direction gates to be provably open");
    }
    for (std::size_t j = 0; j < members.size(); ++j) {
        const double density = densities[j];
        if (density <= 0.0) continue;
        const auto& f = *members[j].forms;
        const auto& b = bindings[j];
        std::string why;
        for (const auto* c : {&f.extinction, &f.scattering, &f.chroma, &f.phase, &f.emission}) {
            if (!c->admitted()) return lattice("channel refused: " + c->refusal);
        }
        const ConstantChannel polynomialConstant = [&](const VolumeRayChannelForm& c, int component,
                                                       const char* what, std::string& reason) {
            return constantOn(c.vectorValued ? (*c.vector.form)[component] : *c.scalar.form, b, what, reason);
        };
        if (!accumulateConstantMember(f, density, polynomialConstant, plan, why)) return lattice(why);
    }
    if (!finiteTransport(plan)) return lattice("non-finite constant transport coefficients");
    plan.strategy = VolumeSegmentStrategy::ConstantClosedForm;
    plan.reason = "every participating channel is exactly constant on this ray";
    return plan;
}

bool executeVolumeSegment(const VolumeSegmentPlan& plan, const VolumeLatticeSpan& span,
                          bool outputBudgetZero, VolumeTransportState& state) {
    if (plan.strategy == VolumeSegmentStrategy::GeneralLattice || span.count == 0) return false;
    const double tIn = state.transmittance;
    if (plan.strategy == VolumeSegmentStrategy::ExactEmpty) {
        // Nothing participates: T and L are untouched, exactly as in the lattice.
        // The cutoff still runs after every sample.
        const bool cut = outputBudgetZero && tIn < kLegacyCutoff;
        state.samplesConsumed += cut ? 1u : span.count;
        state.terminated = cut;
        return true;
    }
    const double sigma = plan.totalExtinction;
    const double sigmaStep = sigma * span.step;
    uint64_t k = span.count;
    bool cut = false;
    if (outputBudgetZero) {
        // First step k ≥ 1 with T_in·exp(−σΔk) < 0.01.
        uint64_t kStar;
        if (!(tIn >= kLegacyCutoff)) {
            kStar = 1;
        } else {
            const double x = std::log(kLegacyCutoff / tIn) / -sigmaStep;   // ≥ 0
            kStar = x >= static_cast<double>(span.count) ? uint64_t(span.count) + 1u
                                                       : static_cast<uint64_t>(std::floor(x)) + 1u;
        }
        if (kStar <= span.count) { k = kStar; cut = true; }
    }
    const double decay = sigmaStep * static_cast<double>(k);
    state.transmittance = tIn * std::exp(-decay);
    state.radiance += plan.totalSource * (tIn * -std::expm1(-decay) / sigma);
    state.samplesConsumed += static_cast<uint32_t>(k);
    state.terminated = cut;
    return true;
}

std::optional<double> affineSegmentTransmittance(const std::vector<VolumeSegmentMember>& members,
                                                 const VolumeRayInput& ray,
                                                 const VolumeLatticeSpan& span,
                                                 double transmittanceIn,
                                                 bool outputBudgetZero,
                                                 std::string* reason) {
    auto refuse = [&](const std::string& why) -> std::optional<double> {
        if (reason) *reason = why;
        return std::nullopt;
    };
    if (span.count == 0 || !(span.step > 0.0) || !std::isfinite(span.step) || !std::isfinite(span.firstSample))
        return refuse("degenerate lattice span");
    if (members.empty()) return refuse("no member media on this segment");
    const double sFirst = span.firstSample;
    const double sLast = span.firstSample + span.step * double(span.count - 1u);
    const double sMid = 0.5 * (sFirst + sLast);
    double tau = 0.0;
    for (const auto& m : members) {
        if (!m.forms) return refuse("member without lowered forms");
        const auto b = rayBindings(m, ray);
        const auto& d = m.forms->density;
        if (!d.present || !d.admitted()) return refuse("density not admitted: " + d.refusal);
        if (d.scalar.form->exactDegreeOn(b) > 1) return refuse("density is not affine on this ray");
        const auto dc = d.scalar.form->evaluateCoefficients(b);
        if (!dc) return refuse("density undefined on this ray");
        const double d0 = OntoMath::LineRestrictedForm::evaluatePolynomial(*dc, sFirst);
        const double d1 = OntoMath::LineRestrictedForm::evaluatePolynomial(*dc, sLast);
        // An affine function ≤ 0 at both end samples is ≤ 0 at every sample:
        // the member never participates. Mixed signs put a clamp inside the span.
        if (d0 <= 0.0 && d1 <= 0.0) continue;
        if (!(d0 > 0.0 && d1 > 0.0)) return refuse("density changes sign inside the span (Piecewise/root segmentation rung)");
        std::vector<double> sc;
        if (m.forms->extinction.present) {
            const auto& e = m.forms->extinction;
            if (!e.admitted()) return refuse("extinction not admitted: " + e.refusal);
            if (e.scalar.form->exactDegreeOn(b) > 1) return refuse("extinction is not affine on this ray");
            const auto ec = e.scalar.form->evaluateCoefficients(b);
            if (!ec) return refuse("extinction undefined on this ray");
            sc = *ec;
        } else {
            sc = *dc;
            for (auto& c : sc) c *= 0.5;   // V1 compatibility law σ_t = 0.5·D
        }
        const double e0 = OntoMath::LineRestrictedForm::evaluatePolynomial(sc, sFirst);
        const double e1 = OntoMath::LineRestrictedForm::evaluatePolynomial(sc, sLast);
        if (!(e0 > kMinExtinction && e1 > kMinExtinction))
            return refuse("extinction clamp may be active inside the span");
        tau += double(span.count) * span.step * OntoMath::LineRestrictedForm::evaluatePolynomial(sc, sMid);
    }
    const double out = transmittanceIn * std::exp(-tau);
    if (!std::isfinite(out)) return refuse("non-finite optical depth");
    if (outputBudgetZero && !(out >= kLegacyCutoff))
        return refuse("legacy opacity cutoff could fire inside the span");
    return out;
}

// ---- Rung 2: per-ray segment schedule --------------------------------------
// Claude Code · Claude Opus 5.5 · session_01HvZSR64QghrdyCpq2M7qvX · 2026-10-17.

namespace {

bool samePlan(const VolumeSegmentPlan& a, const VolumeSegmentPlan& b) {
    if (a.strategy != b.strategy) return false;
    if (a.strategy != VolumeSegmentStrategy::ConstantClosedForm) return true;
    return a.totalExtinction == b.totalExtinction && a.totalSource == b.totalSource;
}

// One member's evaluators on this ray, one per admitted present channel.
struct MemberEvaluators {
    std::map<const VolumeRayChannelForm*, std::size_t> index;   // into the shared list
};

} // namespace

VolumeSegmentSchedule scheduleVolumeRaySegment(const std::vector<VolumeSegmentMember>& members,
                                               const VolumeRayInput& ray,
                                               const VolumeLatticeSpan& span,
                                               double resolutionInSteps,
                                               bool analyticProposals) {
    VolumeSegmentSchedule schedule;
    auto allGeneral = [&](std::string why) {
        schedule.runs.clear();
        schedule.reason = std::move(why);
        if (span.count > 0) {
            VolumeScheduleRun run;
            run.firstSample = 0;
            run.sampleCount = span.count;
            run.plan.reason = schedule.reason;
            schedule.runs.push_back(run);
        }
        schedule.emptySamples = schedule.constantSamples = 0;
        schedule.generalSamples = span.count;
        return schedule;
    };
    if (span.count == 0 || !(span.step > 0.0) || !std::isfinite(span.step) || !std::isfinite(span.firstSample))
        return allGeneral("degenerate lattice span");
    const double dirLen = glm::length(ray.direction);
    if (!(dirLen > 0.0) || !std::isfinite(dirLen) || !std::isfinite(glm::length(ray.origin)))
        return allGeneral("degenerate or non-finite ray");
    if (members.empty()) return allGeneral("no member media on this segment");

    std::vector<std::unique_ptr<OntoMath::LineAlgebraicEvaluator>> owned;
    std::vector<const OntoMath::LineAlgebraicEvaluator*> evaluators;
    std::vector<MemberEvaluators> memberEvals(members.size());
    for (std::size_t j = 0; j < members.size(); ++j) {
        const auto* f = members[j].forms;
        if (!f) return allGeneral("member without lowered forms");
        const auto b = rayBindings(members[j], ray);
        for (const auto* c : {&f->density, &f->extinction, &f->scattering, &f->chroma, &f->phase, &f->emission}) {
            if (!c->present || !c->algebraic.form) continue;
            // The volume emitter returns 0 when no piece matches: this channel
            // binding (not OntoMath) decides that Undefined reads as zero.
            auto ev = std::make_unique<OntoMath::LineAlgebraicEvaluator>(*c->algebraic.form, b, true);
            if (!ev->ok()) {
                if (c == &f->density) return allGeneral("density: " + ev->refusal());
                continue;   // that channel simply cannot prove anything on this ray
            }
            memberEvals[j].index[c] = evaluators.size();
            evaluators.push_back(ev.get());
            owned.push_back(std::move(ev));
        }
        if (!f->density.present) return allGeneral("medium has no authored density");
        if (!memberEvals[j].index.count(&f->density))
            return allGeneral("density not admitted: " + f->density.algebraic.refusal);
    }

    const double sFirst = span.firstSample;
    const double sLast = span.firstSample + span.step * double(span.count - 1u);
    const double resolution = std::max(resolutionInSteps, 1e-6) * span.step;
    const bool gateOpen = span.firstSample > 0.0 && span.firstSample * dirLen > kDirectionGateMargin;

    struct FinalPiece { double s0, s1; VolumeSegmentPlan plan; };
    std::vector<FinalPiece> finals;
    auto addStats = [&](const OntoMath::LineSegmentationStats& st, std::size_t pieceCount) {
        schedule.rangeEvaluations += st.rangeEvaluations;
        schedule.budgetExhausted = schedule.budgetExhausted || st.budgetExhausted;
        schedule.pieces += static_cast<uint32_t>(pieceCount);
    };

    // Phase 1 — density alone decides emptiness. Other channels' switches are
    // irrelevant to it, so they are not segmented over empty space at all.
    std::vector<const OntoMath::LineAlgebraicEvaluator*> densityEvals;
    for (std::size_t j = 0; j < members.size(); ++j)
        densityEvals.push_back(evaluators[memberEvals[j].index.at(&members[j].forms->density)]);
    // The marchers branch on max(D, 0) > 0, so each member's density sign is a
    // segmentation target in its own right (it splits even a structurally
    // non-negative D at its zeros).
    std::vector<std::pair<std::size_t, uint32_t>> densitySigns;
    for (std::size_t j = 0; j < densityEvals.size(); ++j) densitySigns.push_back({j, densityEvals[j]->form().roots[0]});
    OntoMath::LineSegmentationStats st1;
    const auto coarse = OntoMath::segmentLine(densityEvals, sFirst, sLast, resolution, &st1, 1u << 16, analyticProposals,
                                              nullptr, &densitySigns);
    addStats(st1, 0);
    if (coarse.empty()) return allGeneral("segmentation produced no pieces");

    // Phase 2 classifier over ALL evaluators' signs for one refined piece.
    auto classifyFull = [&](const OntoMath::LineSegmentationPiece& piece) {
        VolumeSegmentPlan plan;
        auto general = [&](std::string why) { plan = VolumeSegmentPlan{}; plan.reason = std::move(why); return plan; };
        const double mid = 0.5 * (piece.s0 + piece.s1);
        std::vector<bool> active(members.size(), true);
        for (std::size_t j = 0; j < members.size(); ++j) {
            const std::size_t e = memberEvals[j].index.at(&members[j].forms->density);
            bool undefined = false;
            const auto r = evaluators[e]->range(evaluators[e]->form().roots[0], piece.s0, piece.s1,
                                                piece.signs[e], &undefined);
            if (!undefined && r.hi <= 0.0f) active[j] = false;
        }
        if (std::none_of(active.begin(), active.end(), [](bool a) { return a; })) {
            plan.strategy = VolumeSegmentStrategy::ExactEmpty;
            plan.reason = "every member's density is ≤ 0 over this piece (proved by interval range)";
            return plan;
        }
        for (std::size_t j = 0; j < members.size(); ++j) {
            if (!active[j]) continue;
            const auto& f = *members[j].forms;
            std::string why;
            const ConstantChannel pieceConstant = [&](const VolumeRayChannelForm& c, int component,
                                                      const char* what, std::string& reason) -> std::optional<double> {
                const auto it = memberEvals[j].index.find(&c);
                if (it == memberEvals[j].index.end()) {
                    reason = std::string(what) + " has no admitted algebraic form on this ray";
                    return std::nullopt;
                }
                const auto& ev = *evaluators[it->second];
                const uint32_t root = ev.form().roots[static_cast<std::size_t>(component)];
                if (!ev.constantUnder(root, piece.signs[it->second])) {
                    reason = std::string(what) + " varies on this piece";
                    return std::nullopt;
                }
                const auto v = ev.value(root, mid);
                if (!v || !std::isfinite(*v)) { reason = std::string(what) + " undefined on this piece"; return std::nullopt; }
                return v;
            };
            const auto density = pieceConstant(f.density, 0, "density", why);
            if (!density) return general(why);
            const double d = std::max(*density, 0.0);
            if (!(d > 0.0)) continue;   // exactly constant and non-positive: does not participate
            if (!accumulateConstantMember(f, d, pieceConstant, plan, why)) return general(why);
        }
        if (plan.participatingMembers == 0) {
            plan.strategy = VolumeSegmentStrategy::ExactEmpty;
            plan.reason = "every active member's density is exactly constant and ≤ 0 on this piece";
            return plan;
        }
        if (!finiteTransport(plan)) return general("non-finite constant transport coefficients");
        plan.strategy = VolumeSegmentStrategy::ConstantClosedForm;
        plan.reason = "every participating channel's surviving branch is exactly constant on this piece";
        return plan;
    };

    for (const auto& piece : coarse) {
        // Emptiness under density signs alone. A merged piece's recorded target
        // sign is the proof (each merged leaf was proved ≤ 0 over itself); a fresh
        // range over the whole merged piece is looser and must not overrule it.
        std::vector<bool> active(members.size(), true);
        for (std::size_t j = 0; j < members.size(); ++j) {
            if (j < piece.targetSigns.size() && piece.targetSigns[j] < 0) { active[j] = false; continue; }
            bool undefined = false;
            const auto r = densityEvals[j]->range(densityEvals[j]->form().roots[0], piece.s0, piece.s1,
                                                  piece.signs[j], &undefined);
            if (!undefined && r.hi <= 0.0f) active[j] = false;
        }
        VolumeSegmentPlan plan;
        if (std::none_of(active.begin(), active.end(), [](bool a) { return a; })) {
            plan.strategy = VolumeSegmentStrategy::ExactEmpty;
            plan.reason = "every member's density is ≤ 0 over this piece (proved by interval range)";
            finals.push_back({piece.s0, piece.s1, plan});
            continue;
        }
        std::string why;
        if (ray.incidentSourceActive) why = "incident light varies along the ray";
        else if (!gateOpen) why = "first sample too close to the eye for the direction gates";
        else if (!piece.definite) why = "a possible density switch lies inside this sliver";
        for (std::size_t j = 0; j < members.size() && why.empty(); ++j) {
            // Definite piece: every relevant density switch is decided, so this
            // answer is final — refining other channels cannot make D constant.
            if (active[j] && !densityEvals[j]->constantUnder(densityEvals[j]->form().roots[0], piece.signs[j]))
                why = "density varies on this piece";
        }
        for (std::size_t j = 0; j < members.size() && why.empty(); ++j) {
            if (!active[j]) continue;
            const auto& f = *members[j].forms;
            for (const auto* c : {&f.extinction, &f.scattering, &f.chroma, &f.phase, &f.emission})
                if (c->present && !memberEvals[j].index.count(c)) { why = "a channel has no admitted algebraic form on this ray"; break; }
        }
        if (!why.empty()) {
            plan.reason = why;
            finals.push_back({piece.s0, piece.s1, plan});
            continue;
        }
        // Phase 2 — refine only where constancy is still possible.
        std::vector<std::vector<int8_t>> inherited;
        for (const auto* ev : evaluators) inherited.emplace_back(ev->form().switches.size(), int8_t(0));
        for (std::size_t j = 0; j < members.size(); ++j)
            inherited[memberEvals[j].index.at(&members[j].forms->density)] = piece.signs[j];
        OntoMath::LineSegmentationStats st2;
        const auto fine = OntoMath::segmentLine(evaluators, piece.s0, piece.s1, resolution, &st2, 1u << 16,
                                                analyticProposals, &inherited);
        addStats(st2, 0);
        for (const auto& sub : fine) finals.push_back({sub.s0, sub.s1, classifyFull(sub)});
    }
    schedule.pieces = static_cast<uint32_t>(finals.size());
    if (finals.empty()) return allGeneral("segmentation produced no pieces");

    // Assign each accepted-lattice sample to the piece containing it (pieces
    // are closed, contiguous and ordered; a shared endpoint satisfies both).
    std::size_t p = 0;
    for (uint32_t i = 0; i < span.count; ++i) {
        const double s = span.firstSample + span.step * double(i);
        while (p + 1 < finals.size() && s > finals[p].s1) ++p;
        const auto& plan = finals[p].plan;
        if (!schedule.runs.empty() && samePlan(schedule.runs.back().plan, plan) &&
            schedule.runs.back().firstSample + schedule.runs.back().sampleCount == i) {
            ++schedule.runs.back().sampleCount;
        } else {
            schedule.runs.push_back({i, 1u, plan});
        }
        switch (plan.strategy) {
            case VolumeSegmentStrategy::ExactEmpty: ++schedule.emptySamples; break;
            case VolumeSegmentStrategy::ConstantClosedForm: ++schedule.constantSamples; break;
            case VolumeSegmentStrategy::GeneralLattice: ++schedule.generalSamples; break;
        }
    }
    return schedule;
}

void executeVolumeSchedule(const VolumeSegmentSchedule& schedule, const VolumeLatticeSpan& span,
                           bool outputBudgetZero, VolumeTransportState& state,
                           const VolumeLatticeRunner& lattice) {
    for (const auto& run : schedule.runs) {
        if (state.terminated) return;
        if (run.plan.strategy == VolumeSegmentStrategy::GeneralLattice) {
            lattice(run.firstSample, run.sampleCount, state);
            continue;
        }
        const VolumeLatticeSpan sub{span.firstSample + span.step * double(run.firstSample), span.step, run.sampleCount};
        executeVolumeSegment(run.plan, sub, outputBudgetZero, state);
    }
}

// ---- Rung 2f prototype: Lipschitz-certified support tracing ----------------
// Claude Code · Claude Opus 5.5 · session_01HvZSR64QghrdyCpq2M7qvX · 2026-10-17.
VolumeSupportTrace traceVolumeSupport(const std::vector<VolumeSegmentMember>& members,
                                      const VolumeRayInput& ray, const VolumeLatticeSpan& span) {
    VolumeSupportTrace trace;
    trace.emptySample.assign(span.count, 0);
    if (span.count == 0 || !(span.step > 0.0) || members.empty()) { trace.reason = "degenerate span"; return trace; }
    const double sFirst = span.firstSample;
    const double sLast = span.firstSample + span.step * double(span.count - 1u);
    std::vector<char> all(span.count, 1);
    for (const auto& m : members) {
        if (!m.forms || !m.forms->density.present || !m.forms->density.algebraic.form) {
            trace.reason = "a member's density has no admitted algebraic form";
            trace.emptySample.assign(span.count, 0);
            return trace;
        }
        const auto& form = *m.forms->density.algebraic.form;
        OntoMath::LineAlgebraicEvaluator ev(form, rayBindings(m, ray), true);
        if (!ev.ok()) { trace.reason = "density: " + ev.refusal(); trace.emptySample.assign(span.count, 0); return trace; }
        const uint32_t root = form.roots[0];
        const std::vector<int8_t> zeros(form.switches.size(), 0);

        struct Killer { std::size_t k; int8_t sign; double lipschitz; };
        std::vector<Killer> killers;
        for (std::size_t k = 0; k < form.switches.size(); ++k) {
            for (int8_t sg : {int8_t(-1), int8_t(1)}) {
                auto signs = zeros;
                signs[k] = sg;
                bool undefined = false;
                ++trace.setupEvaluations;
                const auto r = ev.range(root, sFirst, sLast, signs, &undefined);
                if (undefined || !(r.hi <= 0.0f)) continue;
                ++trace.setupEvaluations;
                const auto d = ev.switchDerivativeRange(k, sFirst, sLast, zeros);
                if (!d.bounded()) continue;
                killers.push_back({k, sg, std::max(std::abs(double(d.lo)), std::abs(double(d.hi)))});
            }
        }
        // The density sign itself, when D is Lipschitz: −D(s) > 0 certifies D < 0 nearby.
        {
            ++trace.setupEvaluations;
            const auto d = ev.derivativeRange(root, sFirst, sLast, zeros);
            if (d.bounded()) killers.push_back({std::size_t(-1), -1, std::max(std::abs(double(d.lo)), std::abs(double(d.hi)))});
        }
        trace.killers += static_cast<uint32_t>(killers.size());

        std::vector<char> mine(span.count, 0);
        for (uint32_t i = 0; i < span.count; ++i) {
            if (mine[i]) continue;
            const double si = span.firstSample + span.step * double(i);
            for (const auto& killer : killers) {
                ++trace.pointEvaluations;
                bool undefined = false;
                const auto r = killer.k == std::size_t(-1) ? ev.range(root, si, si, zeros, &undefined)
                                                           : ev.switchRange(killer.k, si, si, zeros, &undefined);
                if (undefined) continue;
                const double margin = killer.sign < 0 ? -double(r.hi) : double(r.lo);
                const bool atZero = killer.k == std::size_t(-1) && r.hi <= 0.0f;   // D ≤ 0 here exactly
                if (!(margin > 0.0) && !atZero) continue;
                // Certified on [si, si + margin/L]; a tiny relative guard keeps the
                // double division from rounding the reach outward.
                const double reach = killer.lipschitz > 0.0 && margin > 0.0
                                         ? si + (margin / killer.lipschitz) * (1.0 - 1e-9)
                                         : (killer.lipschitz == 0.0 && margin > 0.0 ? sLast : si);
                for (uint32_t j = i; j < span.count && span.firstSample + span.step * double(j) <= reach; ++j) mine[j] = 1;
                mine[i] = 1;
                break;
            }
        }
        for (uint32_t i = 0; i < span.count; ++i) all[i] = all[i] && mine[i];
    }
    trace.emptySample = std::move(all);
    for (char e : trace.emptySample) trace.certifiedSamples += e ? 1u : 0u;
    return trace;
}

} // namespace Rendering
