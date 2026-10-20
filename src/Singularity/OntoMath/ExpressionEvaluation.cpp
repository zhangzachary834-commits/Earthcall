// The evaluation report an authored expression carries. See the header.
// Claude Code · Claude Opus 5.5 · session f3af84cf-bfe3-429d-b417-2d533119f169 · 2026-10-19.

#include "Singularity/OntoMath/ExpressionEvaluation.hpp"

#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValueJson.hpp"
#include "Singularity/OntoMath/Field.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"

#include <cstdint>
#include <cstdio>

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

    const auto* field = std::get_if<std::shared_ptr<ScalarField>>(&expression);
    if (!field || !*field) return refuse(prefix + ".expression is not a ScalarField");
    r["expressionDigest"] = fnv1a64((*field)->toJson().dump());
    if ((*field)->mode != ScalarField::EvaluationMode::AST || (*field)->astDefinition.pieces.empty())
        return refuse(prefix + ".expression is procedural, not authored mathematics (no AST)");

    std::map<std::string, PropertyValue> vars;
    auto snapshot = std::make_shared<PropertyDict>();
    PropertyValue bindings;
    if (PropertyPath::parse(prefix + ".bindings").getValue(bearer, bindings) == PropertyPath::PathResult::Ok) {
        const auto* dict = std::get_if<std::shared_ptr<PropertyDict>>(&bindings);
        if (!dict || !*dict) return refuse(prefix + ".bindings is not a dictionary of variable → path");
        for (const auto& [variable, pathValue] : (*dict)->elements) {
            const auto* path = std::get_if<std::string>(&pathValue);
            if (!path) return refuse("the binding of '" + variable + "' is not a path");
            PropertyValue value;
            if (!lawGetValue(bearer, PropertyPath::parse(*path), value))
                return refuse("the binding '" + variable + "' → " + *path + " does not read");
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
