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
// "Exact" means exact up to the rounding of the answer, the standard Carlson's
// R_F is evaluated to. A branch switch the segmentation could not place exactly
// leaves a sliver ~1e-9 wide. It is admitted only when its PROVED contribution
// (width × proved sup |f|) is below half an ulp of the result. Anything larger
// is an approximation, and approximations wait for an authored error contract
// (the integration hierarchy, ONTOMATH_FRAMEWORK.md §6, levels 4–5).
//
// Nothing here names a camera, a medium, a pixel or a shader.
// Claude Code · Claude Opus 5.5 · session f3af84cf-bfe3-429d-b417-2d533119f169 · 2026-10-19.
// ============================================================================

#include "Singularity/OntoMath/LineRestriction.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"

#include <cmath>
#include <cstdio>
#include <limits>
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
// branches, each proved piece integrated through the office.
std::optional<double> integrateAlongLine(const MathNode& f, const AffineVector& q, double lo, double hi,
                                         const std::map<std::string, double>& numeric, std::string* why) {
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
    double total = 0.0, sliverBound = 0.0;
    for (const auto& piece : segmentLine({&ev}, a, b, resolution)) {
        if (!piece.definite) {
            // A switch the segmentation could not place exactly: the sliver is
            // counted as 0 and its PROVED contribution, width·sup|f| (the float
            // enclosure inflated past its rounding), is carried as a bound.
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
    // Exact up to the rounding of the answer, the standard Carlson's R_F is
    // evaluated to: the slivers' proved contribution must fall below half an
    // ulp of the result, where double cannot see it. Anything larger is an
    // approximation, and approximations wait for an authored error contract.
    if (sliverBound > 0.5 * std::numeric_limits<double>::epsilon() * std::abs(total)) {
        char bound[64];
        std::snprintf(bound, sizeof(bound), "%.3g (answer %.17g)", sliverBound, total);
        say(why, std::string("unresolved value-branch slivers could contribute up to ") + bound +
                     ", above the rounding of the answer: not exact, and no error contract is authored");
        return std::nullopt;
    }
    return lo <= hi ? total : -total;
}

} // namespace

std::optional<double> evaluateIntegral(const MathNode& node, const std::map<std::string, PropertyValue>& vars,
                                       const Singular* subject, std::string* why) {
    if (node.op != MathNode::Op::Integral || node.children.size() != 3 || !node.children[0] ||
        !node.children[1] || !node.children[2]) {
        say(why, "Integral needs a body, lo and hi");
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
    // Scalars only: a vector binding never enters the term algebra, and v is
    // shadowed inside the body.
    std::map<std::string, double> numeric;
    for (const auto& [name, value] : vars) {
        double d = 0.0;
        if (std::holds_alternative<glm::vec3>(value)) continue;
        if (propertyValueToNumber(value, d)) numeric[name] = d;
    }
    numeric.erase(v);

    const MathNode& body = *node.children[0];
    std::string termWhy;
    if (const auto form = toScalarForm(body, &termWhy)) return definiteIntegral(*form, v, lo, hi, numeric, why);

    if (body.op == MathNode::Op::SDF && body.children.size() == 2 && body.children[0] && body.children[1]) {
        std::map<std::string, PropertyValue> outer = vars;
        outer.erase(v);
        const auto q = affineVector(*body.children[1], v, outer, numeric, subject, why);
        if (!q) return std::nullopt;
        return integrateAlongLine(*body.children[0], *q, lo, hi, numeric, why);
    }
    say(why, "the body has no exact integral here: " + termWhy +
                 " (admitted: the term algebra, or SDF(field, point) with the point affine in " + v + ")");
    return std::nullopt;
}

} // namespace OntoMath
