#include "Singularity/Screen/VolumeRayTransport.hpp"
#include "Singularity/Screen/VolumeZeroProof.hpp"

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <iterator>
#include <limits>
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

// Execution provenance for Rung 1 (Codex P1, 2026-10-17): sup|symbol| over the
// lattice span, so OntoMath::executionBound can bound what the device executes
// for an authored channel there, from its ORIGINAL tree.
std::map<std::string, double> spanMagnitudes(const std::map<std::string, double>& b, const VolumeLatticeSpan& span) {
    std::map<std::string, double> m;
    for (const auto& [name, v] : b) m[name] = std::abs(v);
    static const char* const kAxes[3] = {"x", "y", "z"};
    const double s0 = span.firstSample, s1 = span.firstSample + span.step * double(span.count ? span.count - 1u : 0u);
    for (int i = 0; i < 3; ++i) {
        const double o = b.at(OntoMath::kLineOriginVars[i]), d = b.at(OntoMath::kLineDirectionVars[i]);
        m[kAxes[i]] = std::max(std::abs(o + s0 * d), std::abs(o + s1 * d)) * (1.0 + 1e-12);
    }
    return m;
}

// Slack of an authored channel component over the span, or +∞ (refuse).
double channelSlack(const VolumeRayChannelForm& c, int component, const std::map<std::string, double>& mags) {
    if (!c.expr) return std::numeric_limits<double>::infinity();
    const auto b = OntoMath::executionBound(*c.expr, mags, kVolumeExecutionModel, c.vectorValued ? component : -1);
    return b ? b->slack : std::numeric_limits<double>::infinity();
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
    std::vector<std::map<std::string, double>> bindings, magnitudes;
    std::vector<double> densities;
    bool executedEmpty = true;
    for (const auto& m : members) {
        if (!m.forms) return lattice("member without lowered forms");
        bindings.push_back(rayBindings(m, ray));
        magnitudes.push_back(spanMagnitudes(bindings.back(), span));
        std::string why;
        const auto d = constantDensity(m, bindings.back(), why);
        if (!d) return lattice(why);
        densities.push_back(*d);
        // The raw constant (before max(·, 0)) must stay ≤ 0 under execution too.
        const auto raw = constantOn(*m.forms->density.scalar.form, bindings.back(), "density", why);
        const double sl = channelSlack(m.forms->density, -1, magnitudes.back());
        if (!raw || !(*raw + sl <= 0.0)) executedEmpty = false;
        if (*d > 0.0 && !(sl <= kExecutionCancellationGuard * kVolumeSupportDeviceTolerance * (1.0 + *d)))
            return lattice("density: execution may drift from the exact constant (cancellation guard)");
    }
    if (std::all_of(densities.begin(), densities.end(), [](double d) { return d <= 0.0; })) {
        if (!executedEmpty)
            return lattice("density is exactly constant and non-positive, but execution may exceed 0 (original-tree slack)");
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
                                                       const char* what, std::string& reason) -> std::optional<double> {
            const auto v = constantOn(c.vectorValued ? (*c.vector.form)[component] : *c.scalar.form, b, what, reason);
            if (!v) return std::nullopt;
            if (!(channelSlack(c, component, magnitudes[j]) <=
                  kExecutionCancellationGuard * kVolumeSupportDeviceTolerance * (1.0 + std::abs(*v)))) {
                reason = std::string(what) + ": execution may drift from the exact constant (cancellation guard)";
                return std::nullopt;
            }
            return v;
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
            // Every proof below is execution-faithful: signs and emptiness are
            // decided beyond the authored tree's execution slack (Codex P1).
            ev->setExecutionModel(kVolumeExecutionModel);
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
            const uint32_t root = evaluators[e]->form().roots[0];
            const auto r = evaluators[e]->range(root, piece.s0, piece.s1, piece.signs[e], &undefined);
            if (!undefined && double(r.hi) + evaluators[e]->slack(root, piece.s0, piece.s1, piece.signs[e]) <= 0.0)
                active[j] = false;
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
                const double sl = ev.slack(root, piece.s0, piece.s1, piece.signs[it->second]);
                if (!(sl <= kExecutionCancellationGuard * kVolumeSupportDeviceTolerance * (1.0 + std::abs(*v)))) {
                    reason = std::string(what) + ": execution may drift from the exact constant (cancellation guard)";
                    return std::nullopt;
                }
                return v;
            };
            const auto density = pieceConstant(f.density, 0, "density", why);
            if (!density) return general(why);
            const double d = std::max(*density, 0.0);
            if (!(d > 0.0)) {
                // Non-participation is an execution-zero claim: the device's D
                // must stay ≤ 0 too, beyond its slack.
                const std::size_t e = memberEvals[j].index.at(&f.density);
                const uint32_t root = evaluators[e]->form().roots[0];
                if (!(*density + evaluators[e]->slack(root, piece.s0, piece.s1, piece.signs[e]) <= 0.0))
                    return general("density is constant at the zero boundary within execution slack");
                continue;
            }
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
            const uint32_t root = densityEvals[j]->form().roots[0];
            const auto r = densityEvals[j]->range(root, piece.s0, piece.s1, piece.signs[j], &undefined);
            if (!undefined && double(r.hi) + densityEvals[j]->slack(root, piece.s0, piece.s1, piece.signs[j]) <= 0.0)
                active[j] = false;
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
        ev.setExecutionModel(kVolumeExecutionModel);
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
                // Killed means the EXECUTED density stays ≤ 0 too.
                if (undefined || !(double(r.hi) + ev.slack(root, sFirst, sLast, signs) <= 0.0)) continue;
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
                // The device evaluates g twice (killer, density): clear 2·slack.
                const double sl = 2.0 * (killer.k == std::size_t(-1) ? ev.slack(root, si, si, zeros)
                                                                     : ev.switchSlack(killer.k, si, si, zeros));
                const double margin = (killer.sign < 0 ? -double(r.hi) : double(r.lo)) - sl;
                const bool atZero = killer.k == std::size_t(-1) && double(r.hi) + sl <= 0.0;   // executed D ≤ 0 here
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

// ---- Rung 2f-native groundwork: authored-tier support certificates ---------
// Claude Code · Claude Opus 5.5 · session_01HvZSR64QghrdyCpq2M7qvX · 2026-10-17.
double volumeClockMagnitudeBucket(double t) {
    const double a = std::abs(t);
    if (!std::isfinite(a)) return std::numeric_limits<double>::infinity();
    double bucket = 1.0;
    while (bucket < a) bucket *= 2.0;
    return bucket;
}

namespace {
// Rung 2b: g = K + n·p + k1·r + ka·|c0 + c1·r| by structure (Polynomial leaves
// of degree ≤ 1 reading only the line, one Euclidean norm of affine components,
// one Abs of a term affine in r, Add/Sub, Mul by a constant). Anything else
// becomes a bounded residual: its range over the box at every clock joins K.
// Leaf coefficients come from their own symbolic coefficients evaluated at unit
// origins/directions (exact for an affine form up to double rounding).
struct ClosedTerm {
    bool ok = false;
    double k0lo = 0.0, k0hi = 0.0;   // K: a point unless a residual widened it
    bool residual = false;
    double k1 = 0.0, ka = 0.0, c0lo = 0.0, c0hi = 0.0, c1 = 0.0;   // |c0 + c1·r|, c0 in [c0lo, c0hi]
    glm::dvec3 n{0.0};
    int norm = -1, abs = -1;
    bool partial = false;   // the norm's components are not all affine: r is only bounded below by them
    bool constant() const { return k1 == 0.0 && ka == 0.0 && n == glm::dvec3(0.0); }
};
bool affineLeaf(const OntoMath::LineRestrictedForm& f, double& beta, glm::dvec3& alpha) {
    if (f.degree() > 1) return false;
    for (const auto& symbol : f.reads) {
        bool line = false;
        for (int i = 0; i < 3; ++i)
            line = line || symbol == OntoMath::kLineOriginVars[i] || symbol == OntoMath::kLineDirectionVars[i];
        if (!line) return false;   // the clock, a direction channel, an unbound symbol
    }
    if (f.degree() < 0) { beta = 0.0; alpha = glm::dvec3(0.0); return true; }
    auto at = [&](const glm::dvec3& o, const glm::dvec3& d) -> std::optional<std::vector<double>> {
        std::map<std::string, double> b;
        for (int i = 0; i < 3; ++i) { b[OntoMath::kLineOriginVars[i]] = o[i]; b[OntoMath::kLineDirectionVars[i]] = d[i]; }
        return f.evaluateCoefficients(b);
    };
    const auto base = at(glm::dvec3(0.0), glm::dvec3(1.0, 0.0, 0.0));
    if (!base || base->empty()) return false;
    beta = (*base)[0];
    for (int j = 0; j < 3; ++j) {
        glm::dvec3 e(0.0);
        e[j] = 1.0;
        const auto c = at(e, e);
        if (!c || c->empty()) return false;
        alpha[j] = (*c)[0] - beta;
        const double slope = c->size() > 1 ? (*c)[1] : 0.0;
        if (!(std::abs(slope - alpha[j]) <= 1e-12 * (1.0 + std::abs(alpha[j])))) return false;
    }
    const glm::dvec3 probe(1.25, -2.5, 3.75);
    const auto c = at(probe, glm::dvec3(0.0, 1.0, 0.0));
    const double expect = beta + glm::dot(alpha, probe);
    return c && !c->empty() && std::abs((*c)[0] - expect) <= 1e-12 * (1.0 + std::abs(expect));
}
struct ClosedContext {
    const OntoMath::LineAlgebraicForm& form;
    const OntoMath::LineAlgebraicEvaluator& ev;
    const std::vector<int8_t>& zeros;
};
ClosedTerm residualTerm(const ClosedContext& cx, uint32_t index) {
    ClosedTerm out;
    bool undefined = false;
    const auto r = cx.ev.range(index, 0.0, 0.0, cx.zeros, &undefined);
    if (undefined || !std::isfinite(r.lo) || !std::isfinite(r.hi)) return out;
    out.ok = true;
    out.residual = true;
    out.k0lo = r.lo;
    out.k0hi = r.hi;
    return out;
}
ClosedTerm closedTerm(const ClosedContext& cx, uint32_t index, int depth) {
    ClosedTerm out;
    if (depth > 64 || index >= cx.form.nodes.size()) return out;
    using Kind = OntoMath::LineAlgebraicNode::Kind;
    const auto& node = cx.form.nodes[index];
    switch (node.kind) {
        case Kind::Polynomial: {
            double beta = 0.0;
            if (!affineLeaf(node.polynomial, beta, out.n)) return residualTerm(cx, index);
            out.ok = true;
            out.k0lo = out.k0hi = beta;
            return out;
        }
        case Kind::Sqrt: {
            bool affine = !node.normComponents.empty() && node.normComponents.size() <= 3 && node.nonnegativeArgument;
            for (const uint32_t c : node.normComponents) {
                double beta = 0.0;
                glm::dvec3 alpha(0.0);
                affine = affine && c < cx.form.nodes.size() && cx.form.nodes[c].kind == Kind::Polynomial &&
                         affineLeaf(cx.form.nodes[c].polynomial, beta, alpha);
            }
            if (!affine && (node.normComponents.empty() || !node.nonnegativeArgument)) return residualTerm(cx, index);
            out.ok = true;
            out.k1 = 1.0;
            out.norm = static_cast<int>(index);
            out.partial = !affine;
            return out;
        }
        case Kind::Abs: {
            // The offset may be a bounded residual (a radius breathing with the
            // clock): the classifier then certifies for every offset in range.
            const auto x = closedTerm(cx, node.a, depth + 1);
            if (x.ok && !x.partial && x.ka == 0.0 && x.n == glm::dvec3(0.0) && x.k1 != 0.0) {
                out.ok = true;
                out.residual = x.residual;
                out.ka = 1.0;
                out.c0lo = x.k0lo;
                out.c0hi = x.k0hi;
                out.c1 = x.k1;
                out.norm = x.norm;
                out.abs = static_cast<int>(index);
                return out;
            }
            return residualTerm(cx, index);
        }
        case Kind::Add: case Kind::Sub: {
            const auto a = closedTerm(cx, node.a, depth + 1), b = closedTerm(cx, node.b, depth + 1);
            if (!a.ok || !b.ok || (a.norm >= 0 && b.norm >= 0 && a.norm != b.norm) ||
                (a.abs >= 0 && b.abs >= 0 && a.abs != b.abs))
                return residualTerm(cx, index);
            const double s = node.kind == Kind::Add ? 1.0 : -1.0;
            out.ok = true;
            out.residual = a.residual || b.residual;
            out.k0lo = a.k0lo + (s > 0 ? b.k0lo : -b.k0hi);
            out.k0hi = a.k0hi + (s > 0 ? b.k0hi : -b.k0lo);
            out.k1 = a.k1 + s * b.k1;
            out.ka = a.ka + s * b.ka;
            out.n = a.n + s * b.n;
            out.norm = a.norm >= 0 ? a.norm : b.norm;
            out.partial = a.partial || b.partial;
            out.abs = a.abs >= 0 ? a.abs : b.abs;
            out.c0lo = a.abs >= 0 ? a.c0lo : b.c0lo;
            out.c0hi = a.abs >= 0 ? a.c0hi : b.c0hi;
            out.c1 = a.abs >= 0 ? a.c1 : b.c1;
            return out;
        }
        case Kind::Mul: {
            const auto a = closedTerm(cx, node.a, depth + 1), b = closedTerm(cx, node.b, depth + 1);
            const bool pointA = a.ok && a.constant() && !a.residual, pointB = b.ok && b.constant() && !b.residual;
            if (!pointA && !pointB) return residualTerm(cx, index);
            const ClosedTerm& v = pointA ? b : a;
            if (!v.ok) return residualTerm(cx, index);
            const double c = pointA ? a.k0lo : b.k0lo;
            out = v;
            out.k0lo = std::min(c * v.k0lo, c * v.k0hi);
            out.k0hi = std::max(c * v.k0lo, c * v.k0hi);
            out.k1 *= c; out.ka *= c; out.n *= c;
            return out;
        }
        default:
            return residualTerm(cx, index);
    }
}
void classifyClosedForm(const ClosedContext& cx, VolumeSupportCertificate& c) {
    const auto& sw = cx.form.switches[c.switchIndex];
    const auto& node = cx.form.nodes[sw.node];
    ClosedTerm g = closedTerm(cx, node.a, 0);
    if (!g.ok) return;
    if (sw.role == 1) {
        const auto b = closedTerm(cx, node.b, 0);
        if (!b.ok || (g.norm >= 0 && b.norm >= 0 && g.norm != b.norm) || (g.abs >= 0 && b.abs >= 0 && g.abs != b.abs)) return;
        g.k0lo -= b.k0hi; g.k0hi -= b.k0lo;
        g.k1 -= b.k1; g.ka -= b.ka; g.n -= b.n;
        g.residual = g.residual || b.residual;
        if (g.norm < 0) g.norm = b.norm;
        g.partial = g.partial || b.partial;
        if (g.abs < 0) { g.abs = b.abs; g.c0lo = b.c0lo; g.c0hi = b.c0hi; g.c1 = b.c1; }
    } else if (sw.role == 2 || sw.role == 3) {
        g.k0lo -= c.offset;
        g.k0hi -= c.offset;
    } else if (sw.role != 0) {
        return;   // Select pieces: never lowered as killers
    }
    // Certified where sign·g > δ; the worst K over the box decides.
    const double sign = c.sign < 0 ? -1.0 : 1.0;
    const double K = sign > 0 ? g.k0lo : -g.k0hi;
    const double T = c.deviceSlack - K;   // certified ⇔ sign·(rest of g) > T
    if (!std::isfinite(T) || !std::isfinite(g.k1) || !std::isfinite(g.ka)) return;
    const bool radial = g.norm >= 0 && g.n == glm::dvec3(0.0) && (g.k1 != 0.0) != (g.ka != 0.0);
    const double inf = std::numeric_limits<double>::infinity();
    using Form = VolumeSupportClosedForm;
    auto rowsOf = [&](const std::vector<uint32_t>& rows, Form& f) {
        for (std::size_t row = 0; row < rows.size() && row < 3; ++row) {
            double beta = 0.0;
            glm::dvec3 alpha(0.0);
            affineLeaf(cx.form.nodes[rows[row]].polynomial, beta, alpha);
            for (int j = 0; j < 3; ++j) f.M[j][row] = alpha[j];   // glm: column-major
            f.b[row] = beta;
        }
    };
    if (radial && g.partial) {
        // Only r > ρout can be used, through r ≥ |c_i|: the affine components
        // together (|(c_i)| ≤ r) as one radial form, and each component that is
        // itself affine in an affine norm (|k0 + k1·r'|) as a shell.
        if (g.ka != 0.0) return;
        const double kappa = sign * g.k1;
        if (!(kappa > 0.0)) return;
        const double rhoOut = T / kappa;
        std::vector<uint32_t> affineRows;
        for (const uint32_t comp : cx.form.nodes[static_cast<std::size_t>(g.norm)].normComponents) {
            double beta = 0.0;
            glm::dvec3 alpha(0.0);
            using Kind = OntoMath::LineAlgebraicNode::Kind;
            if (comp < cx.form.nodes.size() && cx.form.nodes[comp].kind == Kind::Polynomial &&
                affineLeaf(cx.form.nodes[comp].polynomial, beta, alpha)) {
                affineRows.push_back(comp);
                continue;
            }
            const auto t = closedTerm(cx, comp, 1);
            if (!t.ok || t.residual || t.partial || t.ka != 0.0 || t.n != glm::dvec3(0.0) || t.k1 == 0.0 || t.norm < 0)
                continue;
            // |k0 + k1·r'| > ρout: outside the shell [(−ρout − k0)/k1, (ρout − k0)/k1].
            Form f;
            rowsOf(cx.form.nodes[static_cast<std::size_t>(t.norm)].normComponents, f);
            if (rhoOut < 0.0) { f.rhoOut = -1.0; }
            else {
                const double ra = (-rhoOut - t.k0lo) / t.k1, rb = (rhoOut - t.k0lo) / t.k1;
                f.rhoIn = std::min(ra, rb);
                f.rhoOut = std::max(ra, rb);
            }
            c.closedForms.push_back(f);
        }
        if (!affineRows.empty() && affineRows.size() <= 3) {
            Form f;
            rowsOf(affineRows, f);
            f.rhoOut = rhoOut;
            c.closedForms.push_back(f);
        }
        c.closedExact = false;
        return;
    }
    if (radial) {
        double rhoIn = -1.0, rhoOut = inf;
        if (g.k1 != 0.0) {
            const double kappa = sign * g.k1;   // κ·r > T
            if (kappa > 0.0) rhoOut = T / kappa;
            else rhoIn = T / kappa;
        } else {
            const double kappa = sign * g.ka;   // κ·|c0 + c1·r| > T
            if (!(kappa > 0.0) || g.c1 == 0.0) return;   // an empty annulus: not represented
            const double U = T / kappa;               // |c0 + c1·r| > U for every c0 in range
            if (U < 0.0) { rhoOut = -1.0; }           // certified everywhere
            else {
                rhoIn = inf;
                rhoOut = -inf;
                for (const double c0 : {g.c0lo, g.c0hi}) {
                    const double ra = (-U - c0) / g.c1, rb = (U - c0) / g.c1;
                    rhoIn = std::min(rhoIn, std::min(ra, rb));
                    rhoOut = std::max(rhoOut, std::max(ra, rb));
                }
            }
        }
        if (!(rhoIn > 0.0) && !(rhoOut < inf)) return;   // certifies nowhere a ray could use
        Form f;
        rowsOf(cx.form.nodes[static_cast<std::size_t>(g.norm)].normComponents, f);
        f.rhoIn = rhoIn;
        f.rhoOut = rhoOut;
        c.closedForms.push_back(f);
        c.closedExact = !g.residual;
    } else if (g.norm < 0 && g.ka == 0.0 && g.k1 == 0.0 && g.n != glm::dvec3(0.0) &&
               std::isfinite(glm::dot(g.n, g.n))) {
        Form f;
        f.kind = Form::Kind::HalfSpace;
        f.n = sign * g.n;
        f.threshold = T;
        c.closedForms.push_back(f);
        c.closedExact = !g.residual;
    }
}
} // namespace

VolumeSupportClosedInterval volumeSupportClosedInterval(const VolumeSupportClosedForm& c,
                                                        const glm::dvec3& o, const glm::dvec3& d) {
    VolumeSupportClosedInterval out;
    const double inf = std::numeric_limits<double>::infinity();
    if (c.kind == VolumeSupportClosedForm::Kind::HalfSpace) {
        // Certified where N·p > E: a·s > w along the line.
        const double a = glm::dot(c.n, d), w = c.threshold - glm::dot(c.n, o);
        if (a == 0.0) {
            if (w < 0.0) { out.hullLo = inf; out.hullHi = -inf; }   // the whole line is certified
            return out;
        }
        if (a > 0.0) out.hullHi = w / a;
        else out.hullLo = w / a;
        return out;
    }
    // Certified where r > ρout or r < ρin, r = |M p + b|; along the line r² is
    // A·(s − sc)² + |w|² (closest-point form: no cancellation against |u0|²).
    const glm::dvec3 u0 = c.M * o + c.b, du = c.M * d;
    const double A = glm::dot(du, du);
    if (!(A > 0.0)) {
        const double r = glm::length(u0);
        if (r > c.rhoOut) { out.hullLo = inf; out.hullHi = -inf; }
        else if (r < c.rhoIn) { out.holeLo = -inf; out.holeHi = inf; }
        return out;
    }
    const double sc = -glm::dot(u0, du) / A;
    const double w2 = glm::dot(u0 + sc * du, u0 + sc * du);
    if (c.rhoOut < inf) {
        const double h2 = (c.rhoOut * c.rhoOut - w2) / A;
        if (c.rhoOut < 0.0 || h2 < 0.0) { out.hullLo = inf; out.hullHi = -inf; return out; }
        out.hullLo = sc - std::sqrt(h2);
        out.hullHi = sc + std::sqrt(h2);
    }
    if (c.rhoIn > 0.0) {
        const double h2 = (c.rhoIn * c.rhoIn - w2) / A;
        if (h2 > 0.0) { out.holeLo = sc - std::sqrt(h2); out.holeHi = sc + std::sqrt(h2); }
    }
    return out;
}

VolumeSupportCertificates deriveVolumeSupportCertificates(const VolumeRayChannelForms& forms,
                                                          const glm::dvec3& halfExtent,
                                                          double clockMagnitude) {
    VolumeSupportCertificates out;
    if (!std::isfinite(clockMagnitude)) {
        out.refusal = "the clock is not finite: no execution slack can be derived";
        return out;
    }
    if (!forms.density.present || !forms.density.algebraic.form) {
        out.refusal = "density has no admitted algebraic form";
        return out;
    }
    const auto& form = *forms.density.algebraic.form;
    const OntoMath::Interval box[3] = {
        OntoMath::Interval::outward(float(-std::abs(halfExtent.x)), float(std::abs(halfExtent.x))),
        OntoMath::Interval::outward(float(-std::abs(halfExtent.y)), float(std::abs(halfExtent.y))),
        OntoMath::Interval::outward(float(-std::abs(halfExtent.z)), float(std::abs(halfExtent.z)))};
    // Every clock: t unbounded. Directions as the volume channels bind them
    // without a source: wi = 0, wo/omega anywhere on the unit sphere.
    std::map<std::string, OntoMath::Interval> others = {{OntoMath::kTimeVar, OntoMath::Interval::infinite()}};
    for (const char* v : {OntoMath::kWiXVar, OntoMath::kWiYVar, OntoMath::kWiZVar}) others[v] = OntoMath::Interval(0.0f);
    for (const char* v : {OntoMath::kWoXVar, OntoMath::kWoYVar, OntoMath::kWoZVar,
                          OntoMath::kOmegaXVar, OntoMath::kOmegaYVar, OntoMath::kOmegaZVar})
        others[v] = OntoMath::Interval(-1.0f, 1.0f);
    std::map<std::string, OntoMath::Interval> b = others;
    for (int i = 0; i < 3; ++i) {
        b[OntoMath::kLineOriginVars[i]] = box[i];
        b[OntoMath::kLineDirectionVars[i]] = OntoMath::Interval(i == 0 ? 1.0f : 0.0f);
    }
    OntoMath::LineAlgebraicEvaluator ev(form, b, true);
    // Range and Lipschitz hold at every clock; execution slack is the device's
    // and grows with |t|, so it is derived for |t| ≤ clockMagnitude only.
    ev.setExecutionModel(kVolumeExecutionModel, {{OntoMath::kTimeVar, clockMagnitude}});
    if (!ev.ok()) { out.refusal = "density over the box: " + ev.refusal(); return out; }
    const std::vector<int8_t> zeros(form.switches.size(), 0);
    for (std::size_t k = 0; k < form.switches.size(); ++k) {
        for (int8_t sg : {int8_t(-1), int8_t(1)}) {
            auto signs = zeros;
            signs[k] = sg;
            bool undefined = false;
            // s ∈ [0, 0] with o over the whole box: the density's range anywhere in it.
            const auto r = ev.range(form.roots[0], 0.0, 0.0, signs, &undefined);
            // Killed must hold for the EXECUTED density as well (a saturated
            // clamp executes its literal bound exactly; a cancelled leaf does not).
            if (undefined || !(double(r.hi) + ev.slack(form.roots[0], 0.0, 0.0, signs) <= 0.0)) continue;
            const double gSlack = ev.switchSlack(k, 0.0, 0.0, zeros);
            if (!std::isfinite(gSlack)) continue;
            const auto L = OntoMath::lipschitzBoundOverBox(form, k, box, others, true);
            if (!L) continue;
            const auto& sw = form.switches[k];
            const auto& node = form.nodes[sw.node];
            VolumeSupportCertificate c;
            c.switchIndex = k;
            c.sign = sg;
            c.spatialLipschitz = *L;
            c.role = sw.role;
            c.sourceA = node.sourceA;
            c.sourceB = node.sourceB;
            c.offset = sw.role == 2 ? node.lo : (sw.role == 3 ? node.hi : 0.0);
            c.deviceSlack = kVolumeSupportDeviceTolerance + 2.0 * gSlack;
            classifyClosedForm(ClosedContext{form, ev, zeros}, c);
            c.perSampleCheck = c.closedForms.empty() || !c.closedExact;
            out.killers.push_back(c);
        }
    }
    if (out.killers.empty()) out.refusal = "no switch kills the density over the whole box at every time";
    return out;
}

namespace {
// Authored operation count; a node in `slots` is read from a filled shared slot.
// Mirrors sdfwgsl's expensiveTimeOnly: a leaf of the clock alone with a
// transcendental, which the production evaluator prepares once per medium and
// frame on the GPU and reads from a slot. The certificate check's own lowering
// does not prepare (it recomputes per check), so only the evaluator's cost
// treats it as a read (2026-10-18: costing both alike kept the breathing
// nebula's check, which the M5 ran 14% slower than dropping it).
bool preparedTimeLeaf(const OntoMath::ScalarForm& form) {
    bool expensive = false;
    for (const auto& term : form.terms) {
        for (const auto& [variable, exponent] : term.factors)
            if (variable != OntoMath::kTimeVar) return false;
        for (const auto& tr : term.trans) {
            if (tr.variable != OntoMath::kTimeVar) return false;
            expensive = true;
        }
    }
    return expensive;
}
double executionCost(const OntoMath::MathNode& node, const std::set<const OntoMath::MathNode*>* slots,
                     bool preparedTime = false) {
    if (slots && slots->count(&node)) return 1.0;
    using Op = OntoMath::MathNode::Op;
    double cost = 0.0;
    switch (node.op) {
        case Op::Noise: cost = 40.0; break;
        case Op::ScalarLeaf:
            if (preparedTime && preparedTimeLeaf(node.scalarForm)) { cost = 1.0; break; }
            for (const auto& t : node.scalarForm.terms) cost += 1.0 + double(t.factors.size()) + 4.0 * double(t.trans.size());
            break;
        case Op::Sqrt: case Op::Length: case Op::Distance: case Op::Div: case Op::Pow: cost = 4.0; break;
        default: cost = 1.0; break;
    }
    for (const auto& c : node.children)
        if (c) cost += executionCost(*c, slots, preparedTime);
    return cost;
}
} // namespace

double volumeExecutionCost(const OntoMath::MathNode& node) { return executionCost(node, nullptr); }

double volumeDensitySampleCost(const OntoMath::Piecewise& density, bool preludeBeforeDensity,
                               const std::vector<const OntoMath::MathNode*>& preludeGroups,
                               const std::vector<const OntoMath::MathNode*>& densitySites) {
    const std::set<const OntoMath::MathNode*> slots(densitySites.begin(), densitySites.end());
    double cost = 0.0;
    // Pieces the emitter can answer with, up to the first unbounded one.
    for (const auto& piece : density.pieces) {
        if (!piece.mathNode) continue;
        cost += executionCost(*piece.mathNode, &slots, true);
        if (!piece.hasLo && !piece.hasHi) break;
    }
    if (preludeBeforeDensity)
        for (const auto* g : preludeGroups)
            if (g) cost += 1.0 + executionCost(*g, nullptr, true);   // + the slot store
    return cost;
}

VolumeSupportCertificateEstimate estimateVolumeSupportCertificateGain(const VolumeRayChannelForms& forms,
                                                                      const VolumeSupportCertificates& enabled,
                                                                      const VolumeZeroProof* proof,
                                                                      const glm::dvec3& halfExtent,
                                                                      int samplesPerChord, double clock,
                                                                      double densitySampleCost) {
    VolumeSupportCertificateEstimate out;
    if (enabled.killers.empty()) { out.reason = "no enabled certificate"; return out; }
    if (!forms.density.expr || !forms.density.algebraic.form) { out.reason = "density has no algebraic form"; return out; }
    const int n = std::max(samplesPerChord, 1);
    const glm::dvec3 h = glm::abs(halfExtent);
    // What an undecided sample's density evaluation costs (the caller's shared
    // prelude included) and each certificate's check cost (its authored
    // operands, as the shader lowers them).
    const double densityCost = densitySampleCost >= 0.0
        ? densitySampleCost : volumeDensitySampleCost(*forms.density.expr, false, {}, {});
    std::vector<double> checkCost;
    for (const auto& c : enabled.killers)
        checkCost.push_back(2.0 + (c.sourceA ? volumeExecutionCost(*c.sourceA) : 1.0) +
                            (c.sourceB ? volumeExecutionCost(*c.sourceB) : 0.0));
    // Rung 2b: closed-form killers are solved once per ray (volumeSupportClosed)
    // and never checked per sample; the rest keep the per-sample law.
    std::vector<std::size_t> reachKillers, closedKillers;
    for (std::size_t k = 0; k < enabled.killers.size(); ++k) {
        const auto& c = enabled.killers[k];
        if (!c.closedForms.empty()) closedKillers.push_back(k);
        if (c.perSampleCheck) reachKillers.push_back(k);
    }
    constexpr double kLookup = 2.0;   // one proof-cell or support lookup
    constexpr double kClosedCost = 40.0;   // one closed-form killer solved per ray (a quadratic, padded)
    const auto& form = *forms.density.algebraic.form;
    const std::vector<int8_t> zeros(form.switches.size(), 0);

    auto cellOf = [&](const glm::dvec3& p, glm::uvec3& cell) {
        if (!proof || !proof->any()) return false;
        for (int a = 0; a < 3; ++a) {
            const double u = (p[a] + h[a]) / (2.0 * h[a]) * double(proof->dims[a]);
            if (!(u >= 0.0) || u >= double(proof->dims[a])) return false;
            cell[a] = static_cast<uint32_t>(u);
        }
        return proof->proven(cell.x, cell.y, cell.z);
    };
    auto cellExit = [&](const glm::dvec3& o, const glm::dvec3& d, const glm::uvec3& cell) {
        double tExit = std::numeric_limits<double>::infinity();
        for (int a = 0; a < 3; ++a) {
            const double size = 2.0 * h[a] / double(proof->dims[a]);
            const double lo = -h[a] + size * double(cell[a]), hi = lo + size;
            if (d[a] > 0.0) tExit = std::min(tExit, (hi - o[a]) / d[a]);
            else if (d[a] < 0.0) tExit = std::min(tExit, (lo - o[a]) / d[a]);
        }
        return tExit;
    };

    constexpr uint32_t kRays = 96;
    const double golden = 2.399963229728653;   // golden angle
    for (uint32_t r = 0; r < kRays; ++r) {
        // Deterministic: Fibonacci-sphere directions through stratified targets.
        const double z = 1.0 - 2.0 * (double(r) + 0.5) / double(kRays);
        const double rho = std::sqrt(std::max(0.0, 1.0 - z * z));
        const glm::dvec3 d(rho * std::cos(golden * r), z, rho * std::sin(golden * r));
        // Lines as a distant camera's pixels meet the box (Crofton measure):
        // uniform over the box's silhouette across d, so short corner chords —
        // where n samples crowd and one certified reach skips many — weigh as
        // much as they do on screen.
        const glm::dvec3 helper = std::abs(d.y) < 0.9 ? glm::dvec3(0, 1, 0) : glm::dvec3(1, 0, 0);
        const glm::dvec3 eu = glm::normalize(glm::cross(d, helper)), ev2 = glm::cross(d, eu);
        const double ru = glm::dot(h, glm::abs(eu)), rv = glm::dot(h, glm::abs(ev2));
        const glm::dvec3 target = eu * (ru * (std::fmod(0.7548776662 * (r + 1), 1.0) * 2.0 - 1.0)) +
                                  ev2 * (rv * (std::fmod(0.5698402910 * (r + 1), 1.0) * 2.0 - 1.0));
        const glm::dvec3 o = target - d * (3.0 * glm::length(h) + 1.0);
        double t0 = -std::numeric_limits<double>::infinity(), t1 = std::numeric_limits<double>::infinity();
        for (int a = 0; a < 3; ++a) {
            if (d[a] == 0.0) { if (std::abs(o[a]) > h[a]) { t0 = 1.0; t1 = 0.0; } continue; }
            const double ta = (-h[a] - o[a]) / d[a], tb = (h[a] - o[a]) / d[a];
            t0 = std::max(t0, std::min(ta, tb));
            t1 = std::min(t1, std::max(ta, tb));
        }
        if (!(t1 > t0)) continue;
        ++out.rays;
        const double step = (t1 - t0) / double(n);
        VolumeSegmentMember member{&forms, glm::dvec3(0.0), clock};
        OntoMath::LineAlgebraicEvaluator ev(form, rayBindings(member, {o, d, false}), true);
        if (!ev.ok()) continue;
        auto sampleT = [&](int i) { return t0 + (double(i) + 0.5) * step; };
        auto firstAfter = [&](double t) { return std::max(0, int(std::ceil((t - t0) / step - 0.5))); };
        // Without certificates (the production grid lane).
        for (int i = 0; i < n;) {
            ++out.samples;
            const double s = sampleT(i);
            glm::uvec3 cell;
            if (cellOf(o + d * s, cell)) {
                out.costWithout += kLookup;
                ++out.gridSamples;
                i = std::max(i + 1, firstAfter(cellExit(o, d, cell)));
                continue;
            }
            out.costWithout += kLookup + densityCost;
            ++i;
        }
        // With certificates: the shader's law (first success, converse recheck).
        // Closed forms first: the support hull is the intersection of every
        // closed killer's, the hole the longest one (the shader keeps one).
        double hullLo = -std::numeric_limits<double>::infinity(), hullHi = std::numeric_limits<double>::infinity();
        double holeLo = 0.0, holeHi = 0.0;
        for (const std::size_t k : closedKillers)
            for (const auto& form : enabled.killers[k].closedForms) {
                const auto c = volumeSupportClosedInterval(form, o, d);
                hullLo = std::max(hullLo, c.hullLo);
                hullHi = std::min(hullHi, c.hullHi);
                if (c.holeHi - c.holeLo > holeHi - holeLo) { holeLo = c.holeLo; holeHi = c.holeHi; }
                out.costWith += kClosedCost;
            }
        double recheck = -std::numeric_limits<double>::infinity();
        const int closedFirst = hullLo > t0 ? std::max(0, firstAfter(hullLo) - 1) : 0;
        if (closedFirst > 0) out.closedSamples += uint32_t(std::min(closedFirst, n));
        for (int i = closedFirst; i < n;) {
            const double s = sampleT(i);
            if (s > hullHi) { out.closedSamples += uint32_t(n - i); break; }
            glm::uvec3 cell;
            if (s < hullLo || (s > holeLo && s < holeHi)) {
                out.costWith += kLookup;
                const int next = std::max(i + 1, firstAfter(s < hullLo ? hullLo : holeHi));
                out.closedSamples += uint32_t(std::min(next, n) - i);
                i = next;
                continue;
            }
            if (cellOf(o + d * s, cell)) {
                out.costWith += kLookup;
                i = std::max(i + 1, firstAfter(cellExit(o, d, cell)));
                continue;
            }
            out.costWith += kLookup;
            if (s >= recheck && !reachKillers.empty()) {
                double best = -1.0;
                bool certified = false;
                for (const std::size_t k : reachKillers) {
                    const auto& c = enabled.killers[k];
                    out.costWith += checkCost[k];
                    ++out.checks;
                    bool undefined = false;
                    const auto g = ev.switchRange(c.switchIndex, s, s, zeros, &undefined);
                    if (undefined) continue;
                    const double margin = (c.sign < 0 ? -double(g.hi) : double(g.lo)) - c.deviceSlack;
                    const double v = c.spatialLipschitz > 0.0 ? margin / c.spatialLipschitz : (margin > 0.0 ? 1e30 : -1.0);
                    if (v > 0.0) { best = v; certified = true; break; }
                    best = std::max(best, v);
                }
                if (certified) {
                    out.certifiedSamples += 1;
                    i = std::max(i + 1, firstAfter(s + best));
                    continue;
                }
                recheck = s - best;
            }
            out.costWith += densityCost;
            ++i;
        }
    }
    if (out.rays == 0) { out.reason = "no estimate ray crossed the box"; return out; }
    out.eligible = kVolumeSupportCertificateVariantOverhead * out.costWith <
                   kVolumeSupportCertificateGainMargin * out.costWithout;
    out.reason = out.eligible ? "estimated cheaper with certificates" : "estimated no cheaper than the grid lane alone";
    return out;
}

VolumeSupportCertificateEstimate selectVolumeSupportCertificateKillers(const VolumeRayChannelForms& forms,
                                                                       const VolumeSupportCertificates& candidates,
                                                                       const VolumeZeroProof* proof,
                                                                       const glm::dvec3& halfExtent,
                                                                       int samplesPerChord, double clock,
                                                                       double densitySampleCost,
                                                                       VolumeSupportCertificates& chosen) {
    const std::size_t n = candidates.killers.size();
    // Eligibility stays with every killer enabled: ranking subsets promoted two
    // Northern Veil curtains, and the M5 ran the emerald one 16–20% slower even
    // with only its two killers compiled (2026-10-18). A subset only narrows
    // what an eligible medium checks and compiles.
    auto all = estimateVolumeSupportCertificateGain(forms, candidates, proof, halfExtent,
                                                    samplesPerChord, clock, densitySampleCost);
    if (!all.eligible) { chosen = candidates; return all; }
    std::vector<bool> in(n, false);
    auto subset = [&](std::size_t extra) {
        VolumeSupportCertificates s;
        s.refusal = candidates.refusal;
        for (std::size_t k = 0; k < n; ++k)
            if (in[k] || k == extra) s.killers.push_back(candidates.killers[k]);
        return s;
    };
    VolumeSupportCertificateEstimate best;
    best.reason = "no enabled certificate";
    double current = std::numeric_limits<double>::infinity();   // nothing chosen yet
    for (;;) {
        std::size_t pick = n;
        VolumeSupportCertificateEstimate pickEstimate;
        for (std::size_t k = 0; k < n; ++k) {
            if (in[k]) continue;
            auto e = estimateVolumeSupportCertificateGain(forms, subset(k), proof, halfExtent,
                                                          samplesPerChord, clock, densitySampleCost);
            if (e.rays > 0 && e.costWith < (pick == n ? current : pickEstimate.costWith)) {
                pick = k;
                pickEstimate = std::move(e);
            }
        }
        if (pick == n) break;
        in[pick] = true;
        best = std::move(pickEstimate);
        current = best.costWith;
    }
    // Greedy can stop where no single addition helps but the whole set would:
    // never choose worse than every candidate together.
    if (all.costWith <= current) {
        in.assign(n, true);
        best = std::move(all);
    }
    best.eligible = true;
    best.reason = "estimated cheaper with certificates";
    chosen = subset(n);
    // A closed form with a residual or a bound keeps its per-sample check by
    // default; drop it where the estimate says it costs more than it skips.
    for (auto& c : chosen.killers) {
        if (c.closedForms.empty() || !c.perSampleCheck) continue;
        c.perSampleCheck = false;
        auto e = estimateVolumeSupportCertificateGain(forms, chosen, proof, halfExtent,
                                                      samplesPerChord, clock, densitySampleCost);
        if (e.rays > 0 && e.costWith < best.costWith) best = std::move(e);
        else c.perSampleCheck = true;
    }
    best.eligible = true;
    best.reason = "estimated cheaper with certificates";
    return best;
}

VolumeSupportTrace traceVolumeSupportCertified(const std::vector<VolumeSegmentMember>& members,
                                               const std::vector<const VolumeSupportCertificates*>& certificates,
                                               const VolumeRayInput& ray, const VolumeLatticeSpan& span) {
    VolumeSupportTrace trace;
    trace.emptySample.assign(span.count, 0);
    if (span.count == 0 || !(span.step > 0.0) || members.empty() || certificates.size() != members.size()) {
        trace.reason = "degenerate span or mismatched certificates";
        return trace;
    }
    const double dirLen = glm::length(ray.direction);
    std::vector<char> all(span.count, 1);
    for (std::size_t j = 0; j < members.size(); ++j) {
        const auto* certs = certificates[j];
        const auto& m = members[j];
        if (!certs || certs->killers.empty() || !m.forms || !m.forms->density.algebraic.form) {
            trace.reason = "a member has no support certificate";
            trace.emptySample.assign(span.count, 0);
            return trace;
        }
        // Point values of g only: the per-ray binding is the shader's per-ray work.
        OntoMath::LineAlgebraicEvaluator ev(*m.forms->density.algebraic.form, rayBindings(m, ray), true);
        if (!ev.ok()) { trace.reason = "density: " + ev.refusal(); trace.emptySample.assign(span.count, 0); return trace; }
        const std::vector<int8_t> zeros(m.forms->density.algebraic.form->switches.size(), 0);
        trace.killers += static_cast<uint32_t>(certs->killers.size());
        std::vector<char> mine(span.count, 0);
        for (uint32_t i = 0; i < span.count; ++i) {
            if (mine[i]) continue;
            const double si = span.firstSample + span.step * double(i);
            for (const auto& c : certs->killers) {
                ++trace.pointEvaluations;
                bool undefined = false;
                const auto r = ev.switchRange(c.switchIndex, si, si, zeros, &undefined);
                if (undefined) continue;
                // The shader's law: clear the certificate's device slack first.
                const double margin = (c.sign < 0 ? -double(r.hi) : double(r.lo)) - c.deviceSlack;
                if (!(margin > 0.0)) continue;
                const double rate = c.spatialLipschitz * dirLen;
                const double reach = rate > 0.0 ? si + (margin / rate) * (1.0 - 1e-9) : span.firstSample + span.step * double(span.count);
                for (uint32_t q = i; q < span.count && span.firstSample + span.step * double(q) <= reach; ++q) mine[q] = 1;
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
