// ============================================================================
// MathNode::Op::Integral — ∫_lo^hi body d(v), evaluated EXACTLY OR NOT AT ALL.
//
// Human origin: Zachary Zhang, 2026-10-19 — "yeah lets as integral operation
// biuld it on top of antiderivatives so we dont have two competing
// antiderivative code" — answering the ownership correction he commissioned
// from Codex (an integral the renderer alone could ask for is a gift shop; the
// world must be able to express the mathematics without the renderer).
//
// Two routes, ONE office (ScalarForm.hpp, "One office for every exact
// integral"):
//   1. The body is in the term algebra (toScalarForm, nested Integrals
//      included): definiteIntegral, i.e. ScalarForm::antiderivative evaluated
//      stably.
//   2. The body is SDF(f, q) with q affine in v — a field along a straight
//      line. f is restricted to the line (LineRestriction), the line is
//      segmented by f's value branches (|·|, min/max, clamp, √, Piecewise),
//      and each proved piece is integrated through the same office.
// Anything else answers nullopt with the reason: no quadrature, no sampling.
// A branch switch the segmentation could not place exactly leaves a sliver about
// 1e-9 wide, and the exact path REFUSES it. Omitting a nonzero contribution is
// an approximation, and approximations wait for an authored error contract
// (the integration hierarchy, ONTOMATH_FRAMEWORK.md §6, levels 4–5). An earlier
// rule here admitted slivers proved below 0.5·ε·|answer| as "exact to rounding".
// Codex refuted it (2026-10-19 21:30): at 1.5 that threshold is 0.75 ulp, and an
// omitted 5·2⁻⁵⁵ (0.625 ulp) changes the correctly rounded result; nor did it
// count the error already in the rest of the answer.
//
// Nothing here names a camera, a medium, a pixel or a shader.
// Claude Code · Claude Opus 5.5 · session f3af84cf-bfe3-429d-b417-2d533119f169 · 2026-10-19.
// ============================================================================

#include "Singularity/OntoMath/LineRestriction.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"

#include <cmath>
#include <cstdio>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <glm/glm.hpp>

namespace OntoMath {
namespace {

void say(std::string* why, const std::string& text) {
    if (why) *why = text;
}

// c0 + c1·v for a scalar subtree, with every other variable bound.
struct Affine { double c0 = 0.0, c1 = 0.0; };
struct AffineVector { glm::dvec3 c0{0.0}, c1{0.0}; };

std::optional<Affine> affineScalar(const MathNode& node, const std::string& v,
                                   const std::map<std::string, double>& numeric, std::string* why) {
    const auto form = toScalarForm(node, why);
    if (!form) return std::nullopt;
    const auto bound = bindAllBut(*form, v, numeric, why);
    if (!bound) return std::nullopt;
    const auto powers = collectPowers(*bound, v, why);
    if (!powers) return std::nullopt;
    if (powers->size() > 2) { say(why, "the point is not affine in " + v + " (degree " + std::to_string(powers->size() - 1) + ")"); return std::nullopt; }
    Affine a;
    for (std::size_t k = 0; k < powers->size(); ++k) {
        const auto c = (*powers)[k].evaluate({});
        if (!c) { say(why, "a coefficient of the point did not evaluate"); return std::nullopt; }
        (k == 0 ? a.c0 : a.c1) = *c;
    }
    return a;
}

// q(v) = c0 + c1·v for a vector subtree, PROVED affine from its structure:
// VectorConstruct of affine scalars, sums and differences, a scalar times a
// vector when one factor does not depend on v, and any subtree that does not
// mention v at all (evaluated once).
std::optional<AffineVector> affineVector(const MathNode& node, const std::string& v,
                                         const std::map<std::string, PropertyValue>& vars,
                                         const std::map<std::string, double>& numeric,
                                         const Singular* subject, std::string* why) {
    using Op = MathNode::Op;
    switch (node.op) {
        case Op::VectorConstruct: {
            if (node.children.size() != 3) break;
            AffineVector out;
            for (int i = 0; i < 3; ++i) {
                if (!node.children[i]) { say(why, "VectorConstruct with a missing component"); return std::nullopt; }
                const auto a = affineScalar(*node.children[i], v, numeric, why);
                if (!a) return std::nullopt;
                out.c0[i] = a->c0;
                out.c1[i] = a->c1;
            }
            return out;
        }
        case Op::Add:
        case Op::Sub: {
            if (node.children.size() != 2 || !node.children[0] || !node.children[1]) break;
            const auto a = affineVector(*node.children[0], v, vars, numeric, subject, why);
            const auto b = a ? affineVector(*node.children[1], v, vars, numeric, subject, why) : std::nullopt;
            if (!b) return std::nullopt;
            const double k = node.op == Op::Add ? 1.0 : -1.0;
            return AffineVector{a->c0 + k * b->c0, a->c1 + k * b->c1};
        }
        case Op::Scale: {
            if (node.children.size() != 2 || !node.children[0] || !node.children[1]) break;
            // One side is a scalar, the other a vector; try both orders.
            for (int s = 0; s < 2; ++s) {
                const MathNode& scalar = *node.children[s];
                const MathNode& vector = *node.children[1 - s];
                std::string ignored;
                const auto a = affineScalar(scalar, v, numeric, &ignored);
                if (!a) continue;
                const auto b = affineVector(vector, v, vars, numeric, subject, why);
                if (!b) return std::nullopt;
                if (a->c1 != 0.0 && b->c1 != glm::dvec3(0.0)) {
                    say(why, "the point is quadratic in " + v + " (a product of two affine factors)");
                    return std::nullopt;
                }
                return AffineVector{a->c0 * b->c0, a->c0 * b->c1 + a->c1 * b->c0};
            }
            break;
        }
        default: break;
    }
    // A subtree that never mentions v is a constant point, evaluated once. Last,
    // because evaluate() answers a float vec3: the structural cases above keep
    // authored constants in double.
    std::set<std::string> deps;
    node.collectDependencies(deps);
    if (!deps.count(v)) {
        const auto value = node.evaluate(vars, subject);
        if (!value || !std::holds_alternative<glm::vec3>(*value)) { say(why, "the point does not evaluate to a vector"); return std::nullopt; }
        return AffineVector{glm::dvec3(std::get<glm::vec3>(*value)), glm::dvec3(0.0)};
    }
    say(why, std::string("the point is not proved affine in ") + v + " (" + mathOpName(node.op) + ")");
    return std::nullopt;
}

// ∫_lo^hi f(q0 + v·q1) dv: f restricted to the line, segmented by its value
// branches. With no tolerance every proved piece integrates exactly and an
// unresolved sliver refuses. With a tolerance (addendum C) each piece goes
// through the hybrid integrator (closed form plus a sampled remainder with a
// proved radius), slivers are omitted with their proved bound, and the cells
// double until mathematical + execution bound ≤ tolerance or the work cap.
std::optional<double> integrateAlongLine(const MathNode& f, const AffineVector& q, double lo, double hi,
                                         const std::map<std::string, double>& numeric, double tolerance,
                                         IntegralBounds& bounds, std::string* why) {
    const auto restricted = restrictAlgebraicToLine(f, /*vectorValued=*/false);
    if (!restricted) { say(why, "the field does not restrict to a line: " + restricted.refusal); return std::nullopt; }
    std::map<std::string, double> bindings = numeric;
    for (int i = 0; i < 3; ++i) {
        bindings[kLineOriginVars[i]] = q.c0[i];
        bindings[kLineDirectionVars[i]] = q.c1[i];
    }
    const LineAlgebraicEvaluator ev(*restricted.form, bindings);
    if (!ev.ok()) { say(why, "the line form does not bind: " + ev.refusal()); return std::nullopt; }
    const double a = std::min(lo, hi), b = std::max(lo, hi);
    const double resolution = std::max(1e-12, (b - a) * 1e-9);
    const uint32_t root = restricted.form->roots[0];
    bounds.route = "line segmentation";

    if (tolerance == 0.0) {
        // Exact mode integrates the mathematics, so its branches are decided
        // under exact semantics; nothing is sampled.
        const auto pieces = segmentLine({&ev}, a, b, resolution);
        IntegralAccounting accounting;
        IntegralAccountingScope scope(accounting);
        double total = 0.0, sliverBound = 0.0;
        for (const auto& piece : pieces) {
            if (!piece.definite) {
                bool undefined = false;
                const Interval r = ev.range(root, piece.s0, piece.s1, piece.signs[0], &undefined);
                if (undefined || !r.bounded()) {
                    say(why, "the field may be undefined or unbounded in an unresolved sliver at v ≈ " + std::to_string(piece.s0));
                    return std::nullopt;
                }
                const double sup = std::max(std::abs(double(r.lo)), std::abs(double(r.hi)));
                sliverBound += (piece.s1 - piece.s0) * (sup * (1.0 + 1e-6) + 1e-30);
                continue;
            }
            const auto v = ev.integral(root, piece.s0, piece.s1, piece.signs[0], why);
            if (!v) return std::nullopt;
            total += *v;
        }
        if (!exactPathAdmitsOmittedContribution(total, sliverBound)) {
            char bound[64];
            std::snprintf(bound, sizeof(bound), "%.3g (answer without them %.17g)", sliverBound, total);
            say(why, std::string("unresolved value-branch slivers could contribute up to ") + bound +
                         ": omitting them is an approximation, and no error contract is authored");
            return std::nullopt;
        }
        bounds.execution = accounting.executionBound();
        return lo <= hi ? total : -total;
    }

    // Approximation permitted: refine until the whole bound fits. The samples
    // are executed values, so this route runs under the CPU's declared
    // execution model (binary64 scalars, binary32 vectors, the glm::perlin
    // premise): segmentation decides a branch only beyond what execution could
    // blur, and every sample's slack enters the execution bound (plan §6aj).
    LineAlgebraicEvaluator executed(*restricted.form, bindings);
    executed.setExecutionModel(cpuExecutionModel());
    const auto executedPieces = segmentLine({&executed}, a, b, resolution);
    double bestTotal = std::numeric_limits<double>::infinity();
    for (uint32_t density = 8; density <= 4096; density *= 2) {
        IntegralAccounting accounting;
        IntegralAccountingScope scope(accounting);
        double estimate = 0.0, mathematical = 0.0;
        bool sampled = false;
        std::uint64_t samples = 0;
        for (const auto& piece : executedPieces) {
            const double width = piece.s1 - piece.s0;
            bool undefined = false;
            if (!piece.definite) {
                const Interval r = executed.range(root, piece.s0, piece.s1, piece.signs[0], &undefined);
                if (undefined || !r.bounded()) {
                    say(why, "the field may be undefined or unbounded in an unresolved sliver at v ≈ " + std::to_string(piece.s0));
                    return std::nullopt;
                }
                mathematical += width * (std::max(std::abs(double(r.lo)), std::abs(double(r.hi))) * (1.0 + 1e-6) + 1e-30);
                continue;
            }
            const uint32_t cells = std::max<uint32_t>(2, uint32_t(std::ceil(double(density) * width / (b - a))));
            const auto h = executed.hybridIntegral(root, piece.s0, piece.s1, piece.signs[0], cells, why);
            if (!h) return std::nullopt;
            estimate += h->estimate;
            recordSummand(h->estimate);
            mathematical += h->radius;
            sampled = sampled || h->remainderEvaluations > 0;
            samples += h->remainderEvaluations;
        }
        const double execution = accounting.executionBound();
        bestTotal = std::min(bestTotal, mathematical + execution);
        if (mathematical + execution <= tolerance) {
            bounds.mathematical = mathematical;
            bounds.execution = execution;
            bounds.approximate = mathematical > 0.0;
            bounds.samples = samples;
            bounds.route = sampled ? "line segmentation, hybrid (sampled remainder)" : "line segmentation";
            return lo <= hi ? estimate : -estimate;
        }
        // Only sampling refines; omitted slivers and execution do not shrink with cells.
        if (!sampled) break;
    }
    char text[160];
    std::snprintf(text, sizeof(text), "the best justified total bound %.3g exceeds the tolerance %.3g", bestTotal, tolerance);
    say(why, std::string(text) + " (work cap: 4096 cells over the interval; or a bound that refinement cannot reduce)");
    return std::nullopt;
}

// ---- The sampled-body route (plan §6ao) ------------------------------------
// ∫ body dx where the body holds what no closed form or line segmentation
// integrates — transport's exp(−∫₀ˣ σ) · ε(x), say. Each sample is a BOUNDED
// value (evaluateBounded: inner integrals bring their own bounds); each panel's
// remainder is bounded by an enclosure of the body's derivative in x.

Interval outwardInterval(double lo, double hi) {
    float l = static_cast<float>(lo), h = static_cast<float>(hi);
    if (double(l) > lo) l = std::nextafter(l, -INFINITY);
    if (double(h) < hi) h = std::nextafter(h, INFINITY);
    return Interval(l, h);
}
double magnitudeOf(const Interval& i) { return std::max(std::abs(double(i.lo)), std::abs(double(i.hi))); }

struct PanelJet { Interval value, derivative; };   // enclosures over x ∈ [a, b]

struct CachedLine {
    LineAlgebraicRestriction restriction;
    std::unique_ptr<LineAlgebraicEvaluator> evaluator;
};

struct JetContext {
    std::string x;
    double a = 0.0, b = 0.0;
    const std::map<std::string, PropertyValue>* constants = nullptr;   // enclosing scope, exact
    std::map<std::string, double> numeric;
    const Singular* subject = nullptr;
    // Inner Integrals' bounded values at x = a (from the sample there): anchors.
    const std::map<const MathNode*, std::pair<double, double>>* anchors = nullptr;
    std::map<const MathNode*, std::unique_ptr<CachedLine>>* lines = nullptr;
};

void scalarVariables(const ScalarForm& f, std::set<std::string>& out) {
    for (const auto& t : f.terms) {
        for (const auto& [v, e] : t.factors) out.insert(v);
        for (const auto& tf : t.trans) out.insert(tf.variable);
    }
}

std::optional<PanelJet> panelJet(const MathNode& n, const std::map<std::string, PanelJet>& env, JetContext& c,
                                 std::string* why) {
    using Op = MathNode::Op;
    auto child = [&](std::size_t i) -> std::optional<PanelJet> {
        if (i >= n.children.size() || !n.children[i]) { say(why, std::string(mathOpName(n.op)) + " is missing a child"); return std::nullopt; }
        return panelJet(*n.children[i], env, c, why);
    };
    auto scalarConstant = [&](const std::string& name) -> std::optional<double> {
        if (!c.constants) return std::nullopt;
        const auto it = c.constants->find(name);
        double d = 0.0;
        if (it == c.constants->end() || !propertyValueToNumber(it->second, d) || !std::isfinite(d)) return std::nullopt;
        return d;
    };
    switch (n.op) {
        case Op::ScalarLeaf: {
            std::set<std::string> names;
            scalarVariables(n.scalarForm, names);
            std::map<std::string, Interval> ranges;
            for (const auto& name : names) {
                if (const auto it = env.find(name); it != env.end()) ranges[name] = it->second.value;
                else if (const auto k = scalarConstant(name)) ranges[name] = outwardInterval(*k, *k);
                else { say(why, "'" + name + "' has no value on the panel"); return std::nullopt; }
            }
            PanelJet j{n.scalarForm.evalRange(ranges), Interval(0.0f)};
            for (const auto& name : names) {
                const auto it = env.find(name);
                if (it == env.end() || it->second.derivative.exactZero()) continue;
                j.derivative = j.derivative + n.scalarForm.derivative(name).evalRange(ranges) * it->second.derivative;
            }
            return j;
        }
        case Op::ValueLeaf: {
            if (const auto it = env.find(n.variableName); it != env.end()) return it->second;
            if (const auto k = scalarConstant(n.variableName)) return PanelJet{outwardInterval(*k, *k), Interval(0.0f)};
            say(why, "'" + n.variableName + "' is not a scalar with a value on the panel");
            return std::nullopt;
        }
        case Op::Add: case Op::Sub: {
            const auto l = child(0);
            const auto r = l ? child(1) : std::nullopt;
            if (!r) return std::nullopt;
            return n.op == Op::Add ? PanelJet{l->value + r->value, l->derivative + r->derivative}
                                   : PanelJet{l->value - r->value, l->derivative - r->derivative};
        }
        case Op::Scale: {
            const auto l = child(0);
            const auto r = l ? child(1) : std::nullopt;
            if (!r) return std::nullopt;
            return PanelJet{l->value * r->value, l->derivative * r->value + l->value * r->derivative};
        }
        case Op::Let: {
            const auto value = child(0);
            if (!value || n.children.size() != 2 || !n.children[1]) return std::nullopt;
            auto inner = env;
            inner[n.variableName] = *value;
            return panelJet(*n.children[1], inner, c, why);
        }
        case Op::Integral: {
            // V(x) = ∫_{lo(x)}^{hi(x)} g(u) du, g not depending on x (no Leibniz
            // term yet): V′ = g(hi)·hi′ − g(lo)·lo′, the fundamental theorem.
            if (n.children.size() < 3 || !n.children[0] || !n.children[1] || !n.children[2]) return std::nullopt;
            const MathNode& g = *n.children[0];
            std::set<std::string> gDeps;
            g.collectDependencies(gDeps);
            gDeps.erase(n.variableName);
            for (const auto& d : gDeps)
                if (d == c.x || env.count(d)) { say(why, "an inner Integral whose body reads '" + d + "' (needs a Leibniz term)"); return std::nullopt; }
            const auto lo = child(1);
            const auto hi = lo ? child(2) : std::nullopt;
            if (!hi) return std::nullopt;
            auto gRange = [&](const Interval& U) -> std::optional<Interval> {
                if (!U.bounded()) return std::nullopt;
                JetContext inner = c;
                inner.x = n.variableName;
                inner.a = double(U.lo);
                inner.b = double(U.hi);
                inner.anchors = nullptr;
                std::map<std::string, PanelJet> e;
                e[n.variableName] = PanelJet{U, Interval(1.0f)};
                const auto j = panelJet(g, e, inner, why);
                if (!j || !j->value.bounded()) return std::nullopt;
                return j->value;
            };
            PanelJet out;
            // The limits at x = a, where the anchor (if any) was taken.
            std::map<std::string, PropertyValue> atA = c.constants ? *c.constants : std::map<std::string, PropertyValue>{};
            atA[c.x] = c.a;
            const auto loA = n.children[1]->evaluate(atA, c.subject);
            const auto hiA = n.children[2]->evaluate(atA, c.subject);
            double la = 0.0, ha = 0.0;
            const bool limitsAtA = loA && hiA && propertyValueToNumber(*loA, la) && propertyValueToNumber(*hiA, ha);
            const std::pair<double, double>* anchor = nullptr;
            if (c.anchors) {
                const auto it = c.anchors->find(&n);
                if (it != c.anchors->end()) anchor = &it->second;
            }
            if (anchor && limitsAtA) {
                // V(x) = V(a) + ∫_{hi(a)}^{hi(x)} g − ∫_{lo(a)}^{lo(x)} g.
                const Interval hiHull = outwardInterval(ha, ha).joined(hi->value);
                const Interval loHull = outwardInterval(la, la).joined(lo->value);
                const auto gh = gRange(hiHull);
                const auto gl = gh ? gRange(loHull) : std::nullopt;
                if (!gl) { say(why, "the inner Integral's body has no range on the panel"); return std::nullopt; }
                const Interval base = outwardInterval(anchor->first - anchor->second, anchor->first + anchor->second);
                out.value = base + (hi->value - outwardInterval(ha, ha)) * *gh - (lo->value - outwardInterval(la, la)) * *gl;
                out.derivative = *gh * hi->derivative - *gl * lo->derivative;
            } else {
                const auto gs = gRange(lo->value.joined(hi->value));
                const auto gh = gs ? gRange(hi->value) : std::nullopt;
                const auto gl = gh ? gRange(lo->value) : std::nullopt;
                if (!gl) { say(why, "the inner Integral's body has no range on the panel"); return std::nullopt; }
                out.value = (hi->value - lo->value) * *gs;
                out.derivative = *gh * hi->derivative - *gl * lo->derivative;
            }
            return out;
        }
        case Op::SDF: {
            // f(q(x)) with q affine in x: the line machinery's own range and
            // derivative enclosures (noise, polynomials, branches hulled).
            if (n.children.size() != 2 || !n.children[0] || !n.children[1] || !c.lines) return std::nullopt;
            auto& slot = (*c.lines)[&n];
            if (!slot) {
                std::map<std::string, PropertyValue> outer = c.constants ? *c.constants : std::map<std::string, PropertyValue>{};
                outer.erase(c.x);
                const auto q = affineVector(*n.children[1], c.x, outer, c.numeric, c.subject, why);
                if (!q) return std::nullopt;
                auto cached = std::make_unique<CachedLine>();
                cached->restriction = restrictAlgebraicToLine(*n.children[0], /*vectorValued=*/false);
                if (!cached->restriction) { say(why, "the field does not restrict to a line: " + cached->restriction.refusal); return std::nullopt; }
                std::map<std::string, double> bindings = c.numeric;
                for (int i = 0; i < 3; ++i) {
                    bindings[kLineOriginVars[i]] = q->c0[i];
                    bindings[kLineDirectionVars[i]] = q->c1[i];
                }
                cached->evaluator = std::make_unique<LineAlgebraicEvaluator>(*cached->restriction.form, bindings);
                if (!cached->evaluator->ok()) { say(why, "the line form does not bind: " + cached->evaluator->refusal()); return std::nullopt; }
                slot = std::move(cached);
            }
            const auto& ev = *slot->evaluator;
            const uint32_t root = slot->restriction.form->roots[0];
            const std::vector<int8_t> undecided(slot->restriction.form->switches.size(), 0);
            bool undefined = false;
            const Interval value = ev.range(root, c.a, c.b, undecided, &undefined);
            if (undefined) { say(why, "the field may be undefined on the panel"); return std::nullopt; }
            return PanelJet{value, ev.derivativeRange(root, c.a, c.b, undecided)};
        }
        default:
            say(why, std::string(mathOpName(n.op)) + " has no derivative enclosure in the sampled route yet");
            return std::nullopt;
    }
}

std::optional<double> integrateSampledBody(const MathNode& body, const std::string& v, double lo, double hi,
                                           const std::map<std::string, PropertyValue>& vars,
                                           const std::map<std::string, double>& numeric, const Singular* subject,
                                           double tolerance, IntegralBounds& bounds, std::string* why) {
    const double a0 = std::min(lo, hi), b0 = std::max(lo, hi);
    struct Sample {
        BoundedValue value;
        std::map<const MathNode*, std::pair<double, double>> integrals;
    };
    std::map<double, Sample> samples;   // reused across refinements (the nodes nest)
    std::map<const MathNode*, std::unique_ptr<CachedLine>> lines;
    std::map<std::string, PropertyValue> scope = vars;
    scope.erase(v);
    auto sampleAt = [&](double x) -> const Sample* {
        auto it = samples.find(x);
        if (it != samples.end()) return &it->second;
        Sample s;
        auto at = scope;
        at[v] = x;
        s.value = evaluateBounded(body, at, {}, subject, nullptr, &s.integrals);
        if (!s.value.computed) {
            say(why, "the body is undefined at " + v + " = " + std::to_string(x) + (s.value.unavailable.empty() ? "" : ": " + s.value.unavailable));
            return nullptr;
        }
        if (!s.value.bounded) {
            say(why, "the body's value at " + v + " = " + std::to_string(x) + " carries no bound: " + s.value.unavailable);
            return nullptr;
        }
        return &samples.emplace(x, std::move(s)).first->second;
    };
    double best = std::numeric_limits<double>::infinity();
    std::string panelWhy;
    for (uint32_t cells = 4; cells <= 4096; cells *= 2) {
        auto node = [&](uint32_t k) { return k == cells ? b0 : a0 + (b0 - a0) * double(k) / double(cells); };
        double estimate = 0.0, sampleMath = 0.0, sampleExec = 0.0, sampleProp = 0.0, radius = 0.0, magnitude = 0.0;
        for (uint32_t j = 0; j + 2 <= cells; j += 2) {
            const double xa = node(j), xm = node(j + 1), xb = node(j + 2);
            const Sample* s0 = sampleAt(xa);
            const Sample* s1 = s0 ? sampleAt(xm) : nullptr;
            const Sample* s2 = s1 ? sampleAt(xb) : nullptr;
            if (!s2) return std::nullopt;
            // Simpson on the panel: the quadratic interpolant's exact integral.
            const double w[3] = {(xb - xa) / 6.0, 4.0 * (xb - xa) / 6.0, (xb - xa) / 6.0};
            const Sample* ss[3] = {s0, s1, s2};
            for (int k = 0; k < 3; ++k) {
                estimate += w[k] * ss[k]->value.value;
                magnitude += std::abs(w[k] * ss[k]->value.value);
                sampleMath += w[k] * ss[k]->value.mathematical;
                sampleExec += w[k] * ss[k]->value.execution;
                sampleProp += w[k] * ss[k]->value.propagated;
            }
            // First-order remainder: |G − P| ≤ (1 + Λ)·min(range width, L′·hw), Λ = 5/4.
            JetContext ctx;
            ctx.x = v; ctx.a = xa; ctx.b = xb; ctx.constants = &scope; ctx.numeric = numeric;
            ctx.subject = subject; ctx.anchors = &s0->integrals; ctx.lines = &lines;
            std::map<std::string, PanelJet> env;
            env[v] = PanelJet{outwardInterval(xa, xb), Interval(1.0f)};
            const auto jet = panelJet(body, env, ctx, &panelWhy);
            if (!jet || !jet->value.bounded()) {
                say(why, "the body has no derivative enclosure on a panel: " + panelWhy);
                return std::nullopt;
            }
            const double width = double(jet->value.hi) - double(jet->value.lo);
            const double lip = jet->derivative.bounded() ? magnitudeOf(jet->derivative) : std::numeric_limits<double>::infinity();
            radius += (xb - xa) * (1.0 + 1.25) * std::min(width, lip * 0.5 * (xb - xa));
        }
        // Summation of the weighted samples (Higham), on top of each sample's own.
        const double u = std::ldexp(1.0, -53), n = 3.0 * double(cells) + 8.0;
        const double execution = sampleExec + n * u / (1.0 - n * u) * magnitude;
        const double mathematical = radius + sampleMath;
        best = std::min(best, mathematical + execution + sampleProp);
        if (mathematical + execution + sampleProp <= tolerance) {
            bounds.mathematical = mathematical;
            bounds.execution = execution;
            bounds.propagated = sampleProp;
            bounds.approximate = true;
            bounds.samples = samples.size();
            bounds.innerSamples = 0;
            for (const auto& [x, sample] : samples) bounds.innerSamples += sample.value.work;
            bounds.route = "sampled body (first-order radius; bounded samples)";
            return lo <= hi ? estimate : -estimate;
        }
    }
    char text[200];
    std::snprintf(text, sizeof(text), "the best justified total bound %.3g exceeds the tolerance %.3g", best, tolerance);
    say(why, std::string(text) + " (work cap: 4096 cells; the body's samples and their panel enclosures)");
    return std::nullopt;
}

} // namespace

bool exactPathAdmitsOmittedContribution(double /*answer*/, double provedBound) {
    // Only a contribution PROVED zero may be omitted. A positive bound refuses
    // however small, until the error contract (addendum C) carries it in a total.
    return provedBound == 0.0;
}

bool scalarComposable(const MathNode& n) {
    using Op = MathNode::Op;
    switch (n.op) {
        case Op::ScalarLeaf: case Op::ValueLeaf: case Op::Add: case Op::Sub: case Op::Div:
        case Op::Pow: case Op::Abs: case Op::Clamp: case Op::Sqrt: break;
        default: return false;
    }
    for (const auto& c : n.children)
        if (!c || !scalarComposable(*c)) return false;
    return true;
}

BoundedValue evaluateBounded(const MathNode& n, std::map<std::string, PropertyValue> vars,
                             std::map<std::string, InputBound> unc, const Singular* subject,
                             PropertyDict* inputs, std::map<const MathNode*, std::pair<double, double>>* integrals) {
    BoundedValue out;
    if (n.op == MathNode::Op::Let) {
        if (n.children.size() != 2 || !n.children[0] || !n.children[1]) return out;
        const BoundedValue value = evaluateBounded(*n.children[0], vars, unc, subject, inputs, integrals);
        if (!value.computed) return value;
        vars[n.variableName] = value.value;
        InputBound u;
        u.exact = false;
        u.known = value.bounded;
        u.bound = value.bounded ? value.total() : 0.0;
        u.why = value.unavailable;
        unc[n.variableName] = u;
        if (inputs)
            inputs->elements[n.variableName] = value.bounded ? PropertyValue(u.bound) : PropertyValue(std::string("unavailable"));
        BoundedValue body = evaluateBounded(*n.children[1], vars, unc, subject, inputs, integrals);
        if (body.computed && !value.bounded && body.unavailable.empty())
            body.unavailable = "the Let value '" + n.variableName + "' carries no bound" +
                               (value.unavailable.empty() ? std::string() : " (" + value.unavailable + ")");
        if (!value.bounded) body.bounded = false;
        body.work += value.work;
        return body;
    }
    if (n.op == MathNode::Op::Integral) {
        IntegralBounds b;
        std::string why;
        const auto v = evaluateIntegral(n, vars, subject, &why, &b);
        if (!v) { out.unavailable = why; return out; }
        out.computed = out.bounded = true;
        out.value = *v;
        out.mathematical = b.mathematical;
        out.execution = b.execution;
        out.propagated = b.propagated;
        out.work = b.samples + b.innerSamples;
        // An Integral that READS an uncertain input took it as exact: its
        // sensitivity to that input is not composed yet.
        std::set<std::string> deps;
        n.collectDependencies(deps);
        for (const auto& name : deps) {
            const auto it = unc.find(name);
            if (it != unc.end() && !it->second.exact) {
                out.bounded = false;
                out.unavailable = "an Integral reading the uncertain input '" + name + "' (not composed yet)";
            }
        }
        if (integrals && out.bounded) (*integrals)[&n] = {out.value, out.total()};
        return out;
    }
    const auto v = n.evaluate(vars, subject);
    double number = 0.0;
    if (!v || !propertyValueToNumber(*v, number)) return out;
    out.computed = true;
    out.value = number;
    if (!scalarComposable(n)) {
        out.unavailable = std::string(mathOpName(n.op)) + " does not compose bounds yet";
        return out;
    }
    std::map<std::string, double> magnitudes;
    SignedRanges signedRanges;
    std::map<std::string, MathNode::RangeValue> box;
    bool uncertain = false;
    std::set<std::string> deps;
    n.collectDependencies(deps);
    for (const auto& [name, value] : vars) {
        if (!deps.count(name)) continue;   // only what this subtree reads
        double x = 0.0;
        if (!propertyValueToNumber(value, x) || !std::isfinite(x)) { out.unavailable = "the input '" + name + "' is not a finite scalar"; return out; }
        const auto it = unc.find(name);
        const InputBound u = it == unc.end() ? InputBound{} : it->second;
        if (!u.known) {
            out.unavailable = "the input '" + name + "' carries no usable bound" + (u.why.empty() ? std::string() : " (" + u.why + ")");
            return out;
        }
        if (inputs && !inputs->elements.count(name))
            inputs->elements[name] = u.exact ? PropertyValue(std::string("exact")) : PropertyValue(u.bound);
        uncertain = uncertain || u.bound > 0.0;
        magnitudes[name] = std::abs(x) + u.bound;
        signedRanges[name] = {x - u.bound, x + u.bound};
        // The box, rounded outward into the float interval arithmetic.
        float lo = static_cast<float>(x - u.bound), hi = static_cast<float>(x + u.bound);
        if (double(lo) > x - u.bound) lo = std::nextafter(lo, -INFINITY);
        if (double(hi) < x + u.bound) hi = std::nextafter(hi, INFINITY);
        box[name] = MathNode::RangeValue::makeScalar(Interval(std::nextafter(lo, -INFINITY), std::nextafter(hi, INFINITY)));
    }
    const auto exec = executionBound(n, magnitudes, cpuExecutionModel(), -1, kAmbientPointVar, &signedRanges);
    if (!exec || !std::isfinite(exec->slack)) { out.unavailable = "the composite's execution could not be bounded"; return out; }
    out.execution = exec->slack;
    if (uncertain) {
        const auto range = n.evalRange(box);
        if (!range || range->kind != ValueKind::Scalar || !range->scalar.bounded()) {
            out.unavailable = "the composite's range over its inputs could not be bounded";
            return out;
        }
        // |f(x_true) − f(x_used)| ≤ max(hi − f(x_used), f(x_used) − lo), and the
        // computed value is within the execution slack of f(x_used): what the
        // inputs' errors do to this value — propagated, not its own.
        out.propagated = std::max(double(range->scalar.hi) - number, number - double(range->scalar.lo)) + exec->slack;
    }
    out.bounded = true;
    return out;
}

// The three routes, after the common entry has read the limits and the
// tolerance (and priced computed limits). `tolerance` here is what remains for
// the route: the authored one minus the limits' and tolerance's own error.
std::optional<double> integralRoutes(const MathNode& node, const std::map<std::string, PropertyValue>& vars,
                                     const Singular* subject, std::string* why, IntegralBounds& bounds,
                                     double lo, double hi, double tolerance, const std::map<std::string, double>& numeric) {
    const std::string& v = node.variableName;
    const MathNode& body = *node.children[0];
    std::string termWhy;
    if (const auto form = toScalarForm(body, &termWhy)) {
        IntegralAccounting accounting;
        IntegralAccountingScope scope(accounting);
        const auto value = definiteIntegral(*form, v, lo, hi, numeric, why);
        if (!value) return std::nullopt;
        bounds.route = "term algebra";
        bounds.execution = accounting.executionBound();
        // A closed form satisfies any tolerance mathematically; its execution
        // must still fit one that was asked for.
        if (tolerance > 0.0 && bounds.execution > tolerance) {
            char text[160];
            std::snprintf(text, sizeof(text), "the execution bound %.3g exceeds the tolerance %.3g (the mathematics is exact)",
                          bounds.execution, tolerance);
            say(why, text);
            return std::nullopt;
        }
        return value;
    }

    if (body.op == MathNode::Op::SDF && body.children.size() == 2 && body.children[0] && body.children[1]) {
        std::map<std::string, PropertyValue> outer = vars;
        outer.erase(v);
        const auto q = affineVector(*body.children[1], v, outer, numeric, subject, why);
        if (!q) return std::nullopt;
        return integrateAlongLine(*body.children[0], *q, lo, hi, numeric, tolerance, bounds, why);
    }
    // Approximation permitted, and the body is neither closed-form nor a field
    // along a line: sample it as a bounded value (plan §6ao) — transport's
    // exp(−∫₀ˣ σ)·ε(x), an outer Integral over approximate inner ones.
    if (tolerance > 0.0) {
        std::string sampledWhy;
        const auto value = integrateSampledBody(body, v, lo, hi, vars, numeric, subject, tolerance, bounds, &sampledWhy);
        if (value) return value;
        say(why, "the body has no exact integral here (" + termWhy + "), and the sampled route refuses: " + sampledWhy);
        return std::nullopt;
    }
    say(why, "the body has no exact integral here: " + termWhy +
                 " (admitted: the term algebra, or SDF(field, point) with the point affine in " + v + ")");
    return std::nullopt;
}

std::optional<double> evaluateIntegral(const MathNode& node, const std::map<std::string, PropertyValue>& vars,
                                       const Singular* subject, std::string* why, IntegralBounds* boundsOut) {
    IntegralBounds scratch;
    IntegralBounds& bounds = boundsOut ? *boundsOut : scratch;
    bounds = IntegralBounds{};
    if (node.op != MathNode::Op::Integral || (node.children.size() != 3 && node.children.size() != 4) ||
        !node.children[0] || !node.children[1] || !node.children[2] ||
        (node.children.size() == 4 && !node.children[3])) {
        say(why, "Integral needs a body, lo and hi (and optionally a tolerance)");
        return std::nullopt;
    }
    const std::string& v = node.variableName;
    if (v.empty()) { say(why, "Integral names no integration variable"); return std::nullopt; }
    double lo = 0.0, hi = 0.0;
    const auto loValue = node.children[1]->evaluate(vars, subject);
    const auto hiValue = node.children[2]->evaluate(vars, subject);
    if (!loValue || !hiValue || !propertyValueToNumber(*loValue, lo) || !propertyValueToNumber(*hiValue, hi) ||
        !std::isfinite(lo) || !std::isfinite(hi)) {
        say(why, "a limit is undefined, not a number, or infinite");
        return std::nullopt;
    }
    // The authored tolerance (addendum C): absolute, in the result's units, read
    // in the enclosing scope. Absent or 0: exact only. Negative, NaN or
    // infinite: refused, never reinterpreted.
    double tolerance = 0.0;
    if (node.children.size() == 4) {
        const auto t = node.children[3]->evaluate(vars, subject);
        if (!t || !propertyValueToNumber(*t, tolerance) || !std::isfinite(tolerance) || tolerance < 0.0) {
            say(why, "the tolerance must be a finite number ≥ 0 (absolute, in the integral's units; 0 means exact only)");
            return std::nullopt;
        }
    }
    bounds.tolerance = tolerance;
    // Scalars only: a vector binding never enters the term algebra, and v is
    // shadowed inside the body.
    std::map<std::string, double> numeric;
    for (const auto& [name, value] : vars) {
        double d = 0.0;
        if (std::holds_alternative<glm::vec3>(value)) continue;
        if (propertyValueToNumber(value, d)) numeric[name] = d;
    }
    numeric.erase(v);

    // Computed limits (Codex reproduced it, 2026-10-20 16:23): a limit tree is
    // evaluated in binary64 BEFORE any accounting, so (2^54 + 1) − 2^54 became
    // 0 and ∫₀¹ 1 ds was admitted as 0 with a total bound of 0. A literal or a
    // read variable is exact; anything else is priced by its execution bound δ
    // under the CPU model, and moving an endpoint by δ changes the integral by
    // ≤ δ·sup|body| near it. That is execution error, charged as such, and
    // taken out of the tolerance before the route sees it. A computed
    // tolerance is likewise reduced by its own δ. Unboundable: refused under a
    // tolerance; infinite execution bound in exact mode.
    auto computedSlack = [&](const MathNode& tree) -> double {
        if (tree.op == MathNode::Op::ValueLeaf || literalConstant(tree)) return 0.0;
        std::map<std::string, double> magnitudes;
        for (const auto& [name, value] : numeric) magnitudes[name] = std::abs(value);
        const auto b = executionBound(tree, magnitudes, cpuExecutionModel());
        return b && std::isfinite(b->slack) ? b->slack : std::numeric_limits<double>::infinity();
    };
    auto supNear = [&](double x, double delta) -> double {
        if (delta == 0.0) return 0.0;
        if (!std::isfinite(delta)) return std::numeric_limits<double>::infinity();
        std::map<std::string, PropertyValue> scope = vars;
        scope.erase(v);
        std::map<const MathNode*, std::unique_ptr<CachedLine>> lines;
        JetContext ctx;
        ctx.x = v; ctx.a = x - delta; ctx.b = x + delta; ctx.constants = &scope; ctx.numeric = numeric;
        ctx.subject = subject; ctx.lines = &lines;
        std::map<std::string, PanelJet> env;
        env[v] = PanelJet{outwardInterval(x - delta, x + delta), Interval(1.0f)};
        std::string ignored;
        const auto j = panelJet(*node.children[0], env, ctx, &ignored);
        return j && j->value.bounded() ? magnitudeOf(j->value) : std::numeric_limits<double>::infinity();
    };
    const double loSlack = computedSlack(*node.children[1]), hiSlack = computedSlack(*node.children[2]);
    const double tolSlack = node.children.size() == 4 ? computedSlack(*node.children[3]) : 0.0;
    const double endpoint = (loSlack > 0.0 ? loSlack * supNear(lo, loSlack) : 0.0) +
                            (hiSlack > 0.0 ? hiSlack * supNear(hi, hiSlack) : 0.0);
    double routeTolerance = tolerance;
    if (tolerance > 0.0) {
        routeTolerance = tolerance - tolSlack - endpoint;
        if (!(routeTolerance > 0.0)) {
            char text[200];
            std::snprintf(text, sizeof(text),
                          "the computed limits and tolerance err by up to %.3g in the result (limits evaluated in binary64), "
                          "which leaves nothing of the tolerance %.3g", tolSlack + endpoint, tolerance);
            say(why, text);
            return std::nullopt;
        }
    }
    const auto value = integralRoutes(node, vars, subject, why, bounds, lo, hi, routeTolerance, numeric);
    if (!value) return std::nullopt;
    bounds.tolerance = tolerance;
    bounds.execution += endpoint;
    return value;
}


} // namespace OntoMath
