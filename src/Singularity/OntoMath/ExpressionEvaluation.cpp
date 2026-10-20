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
#include <set>
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
    std::string qualification;   // the upstream report's, when not exact
    std::string why;             // when not known: why the bound is unusable
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
                    // Lineage (Codex, 2026-10-20 15:54): only a recognized
                    // qualification under THIS execution model composes; a
                    // missing, unknown or different model is not silently
                    // relabelled as ours — the bound becomes unusable instead.
                    const auto* qs = q != e.end() ? std::get_if<std::string>(&q->second) : nullptr;
                    const auto m = e.find("executionModel");
                    const auto* model = m != e.end() ? std::get_if<std::string>(&m->second) : nullptr;
                    double b = 0.0;
                    u.qualification = qs ? *qs : std::string("none");
                    if (!propertyValueToNumber(tb->second, b) || !std::isfinite(b) || b < 0.0)
                        u.why = "its report has no bound";
                    else if (!qs || (*qs != "model-qualified" && *qs != "certified"))
                        u.why = "its report's qualification is '" + u.qualification + "'";
                    else if (!model || *model != kIntegralExecutionModel)
                        u.why = "its report names " + (model ? "the execution model '" + *model + "'" : std::string("no execution model")) +
                                ", not this one";
                    u.known = u.why.empty();
                    u.bound = u.known ? b : 0.0;
                    return true;
                }
            }
        }
    }
    return lawGetValue(bearer, PropertyPath::parse(path), value);
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
    // An Integral reading a value that carries ANOTHER evaluation's uncertainty
    // took it as exact: its own bounds would silently drop that input's
    // (Codex reproduced it, 2026-10-20 15:54: input ±0.0307, reported 8e-14).
    // Until that sensitivity is propagated, the bound is unavailable, naming it.
    std::string uncertainIntegralInput;
    if (pieces.size() == 1 && pieces[0].mathNode && pieces[0].mathNode->op == MathNode::Op::Integral) {
        std::set<std::string> deps;
        pieces[0].mathNode->collectDependencies(deps);
        for (const auto& name : deps) {
            const auto it = uncertainty.find(name);
            if (it != uncertainty.end() && !it->second.exact) { uncertainIntegralInput = name; break; }
        }
        if (!uncertainIntegralInput.empty()) {
            r["route"] = std::string("integral over an uncertain input");
            r["boundsUnavailable"] = "the Integral reads '" + uncertainIntegralInput +
                                     "', a value carrying another evaluation's uncertainty; propagating it through an "
                                     "Integral (body, limits) is not built yet";
        }
    }
    if (uncertainIntegralInput.empty() && pieces.size() == 1 && pieces[0].mathNode &&
        pieces[0].mathNode->op == MathNode::Op::Integral) {
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
    // Plan §6am/§6an: a composite — a scalar tree whose inputs carry bounds,
    // or a Let chain (let tau = Integral(…) in exp(−tau)). Codex (2026-10-20
    // 13:38): carry value enclosures, model identity and qualification
    // together; an unknown constituent leaves the composite unavailable; a
    // fully accounted one is model-qualified, never certified by nesting. For
    // exp(−τ), the box [τ − δ, τ + δ] gives exactly his monotone interval.
    else if (pieces.size() == 1 && pieces[0].mathNode &&
             (pieces[0].mathNode->op == MathNode::Op::Let || scalarComposable(*pieces[0].mathNode))) {
        r["route"] = std::string("composition");
        auto inputs = std::make_shared<PropertyDict>();
        std::map<std::string, InputBound> bounds;
        for (const auto& [name, u] : uncertainty) bounds[name] = InputBound{u.bound, u.known, u.exact, u.why};
        const BoundedValue c = evaluateBounded(*pieces[0].mathNode, vars, bounds, &bearer, inputs.get());
        r["inputBounds"] = inputs;
        auto lineage = std::make_shared<PropertyDict>();
        for (const auto& [name, u] : uncertainty)
            if (!u.exact) {
                const std::string model(kIntegralExecutionModel);
                lineage->elements[name] = u.qualification + (u.known ? " under " + model.substr(0, model.find(':'))
                                                                      : " (unusable: " + u.why + ")");
            }
        r["inputLineage"] = lineage;
        if (c.computed && c.bounded) {
            const bool uncertain = c.mathematical > 0.0;
            r["admission"] = std::string(uncertain ? "propagated from its inputs under the execution model" : "exact");
            r["qualification"] = std::string("model-qualified");
            r["executionModel"] = std::string(kIntegralExecutionModel);
            r["mathematicalBound"] = c.mathematical;
            r["executionBound"] = c.execution;
            r["totalBound"] = c.mathematical + c.execution;
        } else {
            r["boundsUnavailable"] = c.unavailable.empty() ? std::string("the composite could not be bounded") : c.unavailable;
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
