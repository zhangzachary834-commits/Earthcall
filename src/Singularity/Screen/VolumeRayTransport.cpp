#include "Singularity/Screen/VolumeRayTransport.hpp"

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <iterator>

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

std::optional<glm::dvec3> constantVectorOn(const VolumeRayChannelForm& c,
                                           const std::map<std::string, double>& b,
                                           const std::string& what, std::string& reason) {
    glm::dvec3 v(0.0);
    for (int i = 0; i < 3; ++i) {
        const auto x = constantOn((*c.vector.form)[i], b, what, reason);
        if (!x) return std::nullopt;
        v[i] = *x;
    }
    return v;
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
        double extinction = 0.5 * density;   // V1 compatibility law
        if (f.extinction.present) {
            const auto v = constantOn(*f.extinction.scalar.form, b, "extinction", why);
            if (!v) return lattice(why);
            extinction = *v;
        }
        extinction = std::max(extinction, kMinExtinction);

        double scattering = density;          // V2 compatibility law
        if (f.scattering.present) {
            const auto v = constantOn(*f.scattering.scalar.form, b, "scattering", why);
            if (!v) return lattice(why);
            scattering = std::max(*v, 0.0);
        }
        glm::dvec3 chroma(1.0);               // V2 neutral white
        if (f.chroma.present) {
            const auto v = constantVectorOn(f.chroma, b, "medium chroma", why);
            if (!v) return lattice(why);
            chroma = *v;
        }
        double phase = 1.0;                   // V3 isotropic identity
        if (f.phase.present) {
            // Without a source, the marchers evaluate phase only when the
            // emitted phase never reads wi (any piece); otherwise phase = 0.
            phase = 0.0;
            if (!f.phase.reads("wi.")) {
                const auto v = constantOn(*f.phase.scalar.form, b, "phase", why);
                if (!v) return lattice(why);
                phase = std::max(*v, 0.0);
            }
        }
        glm::dvec3 emitted(0.0);
        if (f.emission.present) {
            const auto v = constantVectorOn(f.emission, b, "emission", why);
            if (!v) return lattice(why);
            emitted = glm::max(*v, glm::dvec3(0.0));
        }
        // Li = 1 without a source (the marchers' incidentLi default).
        plan.totalExtinction += extinction;
        plan.totalSource += chroma * scattering * phase + emitted;
        ++plan.participatingMembers;
    }
    if (!std::isfinite(plan.totalExtinction) || !std::isfinite(plan.totalSource.x) ||
        !std::isfinite(plan.totalSource.y) || !std::isfinite(plan.totalSource.z)) {
        return lattice("non-finite constant transport coefficients");
    }
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

} // namespace Rendering
