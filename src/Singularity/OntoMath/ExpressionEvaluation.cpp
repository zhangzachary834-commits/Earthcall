// The evaluation report an authored expression carries. See the header.
// Claude Code · Claude Opus 5.5 · session f3af84cf-bfe3-429d-b417-2d533119f169 · 2026-10-19.

#include "Singularity/OntoMath/ExpressionEvaluation.hpp"

#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValueJson.hpp"
#include "Singularity/OntoMath/Field.hpp"
#include "Singularity/OntoMath/LineRestriction.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <utility>
#include <vector>

namespace OntoMath {
namespace {

std::string fnv1a64(const std::string& text) {
    std::uint64_t h = 1469598103934665603ull;
    for (unsigned char c : text) { h ^= c; h *= 1099511628211ull; }
    char buf[17];
    std::snprintf(buf, sizeof(buf), "%016llx", static_cast<unsigned long long>(h));
    return buf;
}

// The first Integral in the tree that refuses at these bindings, and why. The
// body of an Integral has its own scope, so the walk does not descend into it.
bool integralRefusal(const MathNode& node, const std::map<std::string, PropertyValue>& vars,
                     const Singular* subject, std::string& why) {
    if (node.op == MathNode::Op::Integral) {
        std::string reason;
        if (!evaluateIntegral(node, vars, subject, &reason)) { why = "an Integral refused: " + reason; return true; }
        return false;
    }
    for (const auto& child : node.children)
        if (child && integralRefusal(*child, vars, subject, why)) return true;
    return false;
}

// What a bound input contributes to a composite (plan §6am). Authored data is
// exact; a value read from another evaluation report carries that report's
// total bound and qualification, or is unknown.
struct InputUncertainty {
    double bound = 0.0;
    bool known = true;
    bool exact = true;
};

// Reads one binding. A path ending ".value" whose parent is a report-like
// record (it carries "value" and "totalBound") is read ONCE, as that record:
// value, bound and qualification then come from the same evaluation, never
// one read's value with another read's evidence (Codex, 2026-10-19 23:58).
bool readBinding(Singular& bearer, const std::string& path, PropertyValue& value, InputUncertainty& u,
                 std::string& why) {
    u = {};
    static const std::string kSuffix = ".value";
    if (path.size() > kSuffix.size() && path.compare(path.size() - kSuffix.size(), kSuffix.size(), kSuffix) == 0) {
        PropertyValue parent;
        if (lawGetValue(bearer, PropertyPath::parse(path.substr(0, path.size() - kSuffix.size())), parent)) {
            const auto* d = std::get_if<std::shared_ptr<PropertyDict>>(&parent);
            if (d && *d) {
                const auto& e = (*d)->elements;
                const auto v = e.find("value"), tb = e.find("totalBound"), q = e.find("qualification");
                // A refused report has no value: pass its reason on, so a cycle
                // or an upstream refusal is legible where it lands.
                const auto reason = e.find("reason");
                if (v == e.end() && reason != e.end())
                    if (const auto* text = std::get_if<std::string>(&reason->second)) why = *text;
                if (v != e.end() && tb != e.end()) {
                    value = v->second;
                    u.exact = false;
                    double b = 0.0;
                    const auto* qs = q != e.end() ? std::get_if<std::string>(&q->second) : nullptr;
                    u.known = propertyValueToNumber(tb->second, b) && std::isfinite(b) && b >= 0.0 && qs && *qs != "none";
                    u.bound = u.known ? b : 0.0;
                    return true;
                }
            }
        }
    }
    return lawGetValue(bearer, PropertyPath::parse(path), value);
}

// The scalar operations a composite bound is propagated through: no Integral
// (a nested approximate integral stays unavailable), nothing vector-valued
// (the CPU carries vectors in binary32, which this model does not price).
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

// Re-entering a report already being computed on this thread (a binding that
// reads its own evaluation, directly or through others) would recurse forever.
thread_local std::vector<std::pair<const Singular*, std::string>> t_reportsInProgress;
struct ReportGuard {
    bool cyclic = false;
    ReportGuard(const Singular* bearer, const std::string& prefix) {
        for (const auto& [b, p] : t_reportsInProgress)
            if (b == bearer && p == prefix) cyclic = true;
        if (!cyclic) t_reportsInProgress.emplace_back(bearer, prefix);
    }
    ~ReportGuard() { if (!cyclic) t_reportsInProgress.pop_back(); }
};

// A copy through the typed codec, so the report never shares canonical storage.
PropertyValue detached(const PropertyValue& v) {
    try { return propertyValueFromJson(propertyValueToJson(v)); } catch (...) { return PropertyValue{}; }
}

} // namespace

std::shared_ptr<PropertyDict> evaluationReport(Singular& bearer, const std::string& prefix) {
    PropertyValue expression;
    if (PropertyPath::parse(prefix + ".expression").getValue(bearer, expression) != PropertyPath::PathResult::Ok)
        return nullptr;

    auto report = std::make_shared<PropertyDict>();
    auto& r = report->elements;
    r["admission"] = std::string("exact");
    r["evaluation"] = std::string("double precision: closed forms, Carlson and proved-piecewise integrals; "
                                  "slow waves by a truncated Taylor series; no quadrature, no sampling");
    r["requestedTolerance"] = std::string("none");
    r["mathematicalBound"] = std::string("unavailable");
    r["executionBound"] = std::string("unavailable");
    r["totalBound"] = std::string("unavailable");
    r["qualification"] = std::string("none");
    r["backend"] = std::string("cpu: OntoMath Piecewise::evaluate");
    auto refuse = [&](const std::string& why) {
        r["status"] = std::string("refused");
        r["reason"] = why;
        return report;
    };

    const ReportGuard guard(&bearer, prefix);
    if (guard.cyclic) return refuse(prefix + ".evaluation depends on itself through its bindings (a cycle)");

    const auto* field = std::get_if<std::shared_ptr<ScalarField>>(&expression);
    if (!field || !*field) return refuse(prefix + ".expression is not a ScalarField");
    r["expressionDigest"] = fnv1a64((*field)->toJson().dump());
    if ((*field)->mode != ScalarField::EvaluationMode::AST || (*field)->astDefinition.pieces.empty())
        return refuse(prefix + ".expression is procedural, not authored mathematics (no AST)");

    std::map<std::string, PropertyValue> vars;
    std::map<std::string, InputUncertainty> uncertainty;
    auto snapshot = std::make_shared<PropertyDict>();
    PropertyValue bindings;
    if (PropertyPath::parse(prefix + ".bindings").getValue(bearer, bindings) == PropertyPath::PathResult::Ok) {
        const auto* dict = std::get_if<std::shared_ptr<PropertyDict>>(&bindings);
        if (!dict || !*dict) return refuse(prefix + ".bindings is not a dictionary of variable → path");
        for (const auto& [variable, pathValue] : (*dict)->elements) {
            const auto* path = std::get_if<std::string>(&pathValue);
            if (!path) return refuse("the binding of '" + variable + "' is not a path");
            PropertyValue value;
            std::string upstream;
            if (!readBinding(bearer, *path, value, uncertainty[variable], upstream))
                return refuse("the binding '" + variable + "' → " + *path + " does not read" +
                              (upstream.empty() ? std::string() : ": " + upstream));
            vars[variable] = value;
            snapshot->elements[variable] = detached(value);
        }
    }
    r["bindings"] = snapshot;

    const auto value = (*field)->astDefinition.evaluate(vars, &bearer);
    double number = 0.0;
    if (!value || !propertyValueToNumber(*value, number)) {
        std::string why = "the expression is undefined at these bindings";
        for (const auto& piece : (*field)->astDefinition.pieces)
            if (piece.mathNode && integralRefusal(*piece.mathNode, vars, &bearer, why)) break;
        return refuse(why);
    }
    r["status"] = std::string("admitted");
    r["value"] = number;
    // Addendum C: when the expression IS one Integral, its bounds are known and
    // reported in the result's units. A composite expression around integrals
    // does not propagate them yet, so its bounds stay "unavailable".
    const auto& pieces = (*field)->astDefinition.pieces;
    if (pieces.size() == 1 && pieces[0].mathNode && pieces[0].mathNode->op == MathNode::Op::Integral) {
        IntegralBounds bounds;
        if (evaluateIntegral(*pieces[0].mathNode, vars, &bearer, nullptr, &bounds)) {
            // "within tolerance" alone reads as a guarantee; it is one only under
            // the named model, so the admission says so and `qualification`
            // lets a consumer that needs more decline (Codex, 2026-10-20).
            r["admission"] = std::string(bounds.approximate ? "within tolerance under the execution model" : "exact");
            r["qualification"] = std::string(bounds.certified ? "certified" : "model-qualified");
            r["executionModel"] = bounds.executionModel;
            if (bounds.tolerance > 0.0) r["requestedTolerance"] = bounds.tolerance;
            r["mathematicalBound"] = bounds.mathematical;
            r["executionBound"] = bounds.execution;
            r["totalBound"] = bounds.total();
            r["route"] = bounds.route;
            r["executionPremise"] = std::string(
                "Higham summation bound over every summand the office added, plus proved series tails; "
                "assumes each summand within γ of itself (libm within one ulp). Sampled values carry their proved "
                "executed-vs-exact slack (binary32 noise positions included); glm::perlin's own float arithmetic "
                "is taken within 2^-14, corroborated by measurement, not proved");
        }
    }
    // Plan §6am: a SCALAR composite whose inputs carry bounds (another
    // report's total) or are exact authored data. Codex (2026-10-20 13:38):
    // carry value enclosures, model identity and qualification together; an
    // unknown constituent leaves the composite unavailable; a fully accounted
    // one is model-qualified, never certified by nesting. For exp(−τ) the
    // interval evaluation over [τ − δ, τ + δ] is exactly his monotone
    // [exp(−(τ+δ)), exp(−(τ−δ))].
    else if (pieces.size() == 1 && pieces[0].mathNode && scalarComposable(*pieces[0].mathNode)) {
        const MathNode& root = *pieces[0].mathNode;
        r["route"] = std::string("composition");
        auto inputs = std::make_shared<PropertyDict>();
        std::map<std::string, double> magnitudes;
        SignedRanges signedRanges;
        std::map<std::string, MathNode::RangeValue> box;
        bool known = true, uncertain = false;
        std::string unknownInput;
        for (const auto& [name, v] : vars) {
            double x = 0.0;
            if (!propertyValueToNumber(v, x) || !std::isfinite(x)) { known = false; unknownInput = name + " (not a finite scalar)"; break; }
            const auto& u = uncertainty[name];
            if (!u.known) { known = false; unknownInput = name; break; }
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
        r["inputBounds"] = inputs;
        const auto exec = known ? executionBound(root, magnitudes, cpuExecutionModel(), -1, kAmbientPointVar, &signedRanges)
                                : std::nullopt;
        double mathematical = 0.0;
        bool bounded = known && exec && std::isfinite(exec->slack);
        if (bounded && uncertain) {
            const auto range = root.evalRange(box);
            bounded = range && range->kind == ValueKind::Scalar && range->scalar.bounded();
            // |f(x_true) − f(x_used)| ≤ max(hi − f(x_used), f(x_used) − lo), and the
            // computed value is within the execution slack of f(x_used).
            if (bounded)
                mathematical = std::max(double(range->scalar.hi) - number, number - double(range->scalar.lo)) + exec->slack;
        }
        if (bounded) {
            r["admission"] = std::string(uncertain ? "propagated from its inputs under the execution model" : "exact");
            r["qualification"] = std::string("model-qualified");
            r["executionModel"] = std::string(kIntegralExecutionModel);
            r["mathematicalBound"] = mathematical;
            r["executionBound"] = exec->slack;
            r["totalBound"] = mathematical + exec->slack;
        } else {
            r["boundsUnavailable"] = std::string(!known ? "the input '" + unknownInput + "' carries no bound"
                                                        : "the composite's range or execution could not be bounded");
        }
    }
    return report;
}

void registerEvaluationReading() {
    registerDerivedReading("evaluation", [](Singular& root, const std::string& prefix, PropertyValue& out) {
        auto report = evaluationReport(root, prefix);
        if (!report) return false;
        out = report;
        return true;
    });
}

} // namespace OntoMath
