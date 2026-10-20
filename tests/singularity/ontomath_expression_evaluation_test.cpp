// ontomath_expression_evaluation_test — addendum B: the evaluation report an
// authored expression carries, read through the actual Property route.
//
// Codex's placement (2026-10-19 21:30, commissioned by Zach): the report sits on
// the expression's existing bearer as a read-only derived PropertyDict. These
// checks are his B witnesses on an ordinary Object, through PropertyPath and the
// Law binding reader, never a private helper:
//  · a successful outcome and a refusal, the refusal replacing the success;
//  · a free binding edit refreshes the evidence;
//  · nested reads, and rejected writes at the report and inside it;
//  · unavailable bounds are unavailable, never zero;
//  · a read is a copy (mutating it changes nothing);
//  · stored state is never shadowed, and no expression means no route.
// Claude Code · Claude Opus 5.5 · session f3af84cf-bfe3-429d-b417-2d533119f169 · 2026-10-19.

#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "Singularity/OntoMath/ExpressionEvaluation.hpp"
#include "Singularity/OntoMath/Field.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"

#include <cmath>
#include <iostream>
#include <memory>
#include <string>

namespace {
int g_checks = 0, g_failures = 0;
void check(bool condition, const std::string& description) {
    ++g_checks;
    if (!condition) ++g_failures;
    std::cout << (condition ? "  ok: " : "  FAILED: ") << description << "\n";
}

using OntoMath::MathNode;
using OntoMath::ScalarForm;
using Op = MathNode::Op;
using Node = std::unique_ptr<MathNode>;

Node leaf(ScalarForm f) { auto n = std::make_unique<MathNode>(); n->op = Op::ScalarLeaf; n->scalarForm = std::move(f); return n; }
Node constant(double c) { return leaf(ScalarForm::constant(c)); }
Node named(const std::string& v) { auto n = std::make_unique<MathNode>(); n->op = Op::ValueLeaf; n->variableName = v; return n; }
template <class... Kids> Node node(Op op, Kids... kids) {
    auto n = std::make_unique<MathNode>(); n->op = op; (n->children.push_back(std::move(kids)), ...); return n;
}
Node integral(const std::string& v, Node body, Node lo, Node hi) {
    auto n = node(Op::Integral, std::move(body), std::move(lo), std::move(hi)); n->variableName = v; return n;
}
PropertyValue field(Node root) {
    auto f = std::make_shared<OntoMath::ScalarField>();
    f->mode = OntoMath::ScalarField::EvaluationMode::AST;
    f->astDefinition = OntoMath::Piecewise::continuous(std::move(root));
    return PropertyValue(f);
}
PropertyValue read(Singular& s, const std::string& path, PropertyPath::PathResult* result = nullptr) {
    PropertyValue out;
    const auto r = PropertyPath::parse(path).getValue(s, out);
    if (result) *result = r;
    return out;
}
std::string text(const PropertyValue& v) { const auto* s = std::get_if<std::string>(&v); return s ? *s : std::string("<not a string>"); }
double number(const PropertyValue& v) { double d = NAN; propertyValueToNumber(v, d); return d; }
} // namespace

int main() {
    std::cout << "Starting ontomath_expression_evaluation_test...\n";
    OntoMath::registerEvaluationReading();

    Object bearer;
    // growth = ∫_0^horizon rate·s ds, with rate and horizon the bearer's own properties.
    bearer.setDynamicProperty("growth.expression",
                              field(integral("s", leaf(ScalarForm::variable("k").times(ScalarForm::variable("s"))),
                                             constant(0.0), named("t"))));
    auto bindings = std::make_shared<PropertyDict>();
    bindings->elements["k"] = std::string("rate");
    bindings->elements["t"] = std::string("horizon");
    bearer.setDynamicProperty("growth.bindings", PropertyValue(bindings));
    bearer.setDynamicProperty("rate", PropertyValue(3.0));
    bearer.setDynamicProperty("horizon", PropertyValue(2.0));

    // ---- A successful outcome, read through the Property route --------------
    PropertyPath::PathResult result{};
    const auto report = read(bearer, "growth.evaluation", &result);
    check(result == PropertyPath::PathResult::Ok && std::holds_alternative<std::shared_ptr<PropertyDict>>(report),
          "an ordinary Object answers growth.evaluation as a dictionary");
    check(text(read(bearer, "growth.evaluation.status")) == "admitted" && number(read(bearer, "growth.evaluation.value")) == 6.0,
          "nested reads: status admitted, value 6 (= 3 · 2²/2)");
    check(text(read(bearer, "growth.evaluation.admission")) == "exact" &&
              text(read(bearer, "growth.evaluation.requestedTolerance")) == "none",
          "the admission is exact and no tolerance is authored");
    // Addendum C: an expression that IS one Integral reports numeric bounds.
    check(number(read(bearer, "growth.evaluation.mathematicalBound")) == 0.0 &&
              number(read(bearer, "growth.evaluation.executionBound")) > 0.0 &&
              number(read(bearer, "growth.evaluation.totalBound")) >= std::abs(number(read(bearer, "growth.evaluation.value")) - 6.0) &&
              text(read(bearer, "growth.evaluation.route")) == "term algebra",
          "an Integral's report carries its bounds: mathematical 0, execution positive (premised), route named");
    // Codex, 2026-10-20: the bound is conditional on a named execution model and
    // says so; nothing is certified yet.
    check(text(read(bearer, "growth.evaluation.qualification")) == "model-qualified" &&
              text(read(bearer, "growth.evaluation.executionModel")).find("binary64") != std::string::npos,
          "the bound is reported as model-qualified, naming its execution model");
    {
        // A composite around an integral does not propagate bounds: unavailable, never zero.
        Object composite;
        composite.setDynamicProperty("sum.expression",
            field(node(Op::Add, constant(1.0), integral("s", leaf(ScalarForm::variable("s")), constant(0.0), constant(1.0)))));
        check(text(read(composite, "sum.evaluation.status")) == "admitted" && number(read(composite, "sum.evaluation.value")) == 1.5 &&
                  text(read(composite, "sum.evaluation.totalBound")) == "unavailable" &&
                  text(read(composite, "sum.evaluation.qualification")) == "none",
              "a composite's bounds are reported as unavailable, never as zero, and it claims no qualification");
    }
    check(number(read(bearer, "growth.evaluation.bindings.k")) == 3.0 && number(read(bearer, "growth.evaluation.bindings.t")) == 2.0,
          "the report carries the bindings it read");
    PropertyValue viaLaw;
    check(lawGetValue(bearer, PropertyPath::parse("growth.evaluation.value"), viaLaw) && number(viaLaw) == 6.0,
          "the Law binding reader reads the same value");

    // ---- Free binding edits refresh the evidence -----------------------------
    const std::string digest = text(read(bearer, "growth.evaluation.expressionDigest"));
    bearer.setDynamicProperty("rate", PropertyValue(5.0));
    check(number(read(bearer, "growth.evaluation.value")) == 10.0 && number(read(bearer, "growth.evaluation.bindings.k")) == 5.0,
          "editing a bound property refreshes the value (6 → 10) and its binding snapshot");
    check(text(read(bearer, "growth.evaluation.expressionDigest")) == digest, "a binding edit leaves the expression digest unchanged");

    // ---- A refusal replaces the success --------------------------------------
    bearer.setDynamicProperty("growth.expression",
                              field(integral("s", node(Op::Noise, node(Op::VectorConstruct, named("s"), constant(0.0), constant(0.0))),
                                             constant(0.0), named("t"))));
    PropertyPath::PathResult valueResult{};
    read(bearer, "growth.evaluation.value", &valueResult);
    const std::string reason = text(read(bearer, "growth.evaluation.reason"));
    check(text(read(bearer, "growth.evaluation.status")) == "refused" && valueResult == PropertyPath::PathResult::NoSuchProperty,
          "a refused expression's report has no value: the old success did not survive");
    check(reason.find("Integral refused") != std::string::npos, "the refusal is legible: " + reason);
    check(text(read(bearer, "growth.evaluation.expressionDigest")) != digest, "a new expression has a new digest");
    // An unresolved branch sliver refuses in the exact path (no approximation permission exists).
    bearer.setDynamicProperty("growth.expression",
        field(integral("s", node(Op::SDF, node(Op::Clamp, node(Op::Sub, constant(1.0), node(Op::Dot, named("p"), named("p"))), constant(0.0), constant(1.0)),
                                  node(Op::Add, node(Op::VectorConstruct, constant(-2.0), constant(0.3), constant(0.0)),
                                       node(Op::Scale, named("s"), node(Op::VectorConstruct, constant(1.0), constant(0.0), constant(0.0))))),
                       constant(0.0), constant(4.0))));
    check(text(read(bearer, "growth.evaluation.status")) == "refused" &&
              text(read(bearer, "growth.evaluation.reason")).find("approximation") != std::string::npos,
          "an expression that would need approximation permission refuses: " + text(read(bearer, "growth.evaluation.reason")));
    // A binding that does not read refuses, naming it.
    bearer.setDynamicProperty("growth.expression",
                              field(integral("s", leaf(ScalarForm::variable("k").times(ScalarForm::variable("s"))), constant(0.0), named("t"))));
    bindings->elements["t"] = std::string("missing.horizon");
    bearer.setDynamicProperty("growth.bindings", PropertyValue(bindings));
    check(text(read(bearer, "growth.evaluation.status")) == "refused" &&
              text(read(bearer, "growth.evaluation.reason")).find("missing.horizon") != std::string::npos,
          "an unreadable binding refuses, naming its path");
    bindings->elements["t"] = std::string("horizon");
    bearer.setDynamicProperty("growth.bindings", PropertyValue(bindings));
    check(text(read(bearer, "growth.evaluation.status")) == "admitted", "restoring the binding restores the admission");

    // ---- Rejected writes; reads are copies -------------------------------------
    check(PropertyPath::parse("growth.evaluation").setValue(bearer, PropertyValue(1.0)) == PropertyPath::PathResult::ReadOnly &&
              PropertyPath::parse("growth.evaluation.value").setValue(bearer, PropertyValue(1.0)) == PropertyPath::PathResult::ReadOnly,
          "writes to the report and inside it refuse (ReadOnly)");
    check(PropertyPath::parse("growth.evaluation.status").setValue(bearer, PropertyValue(std::string("admitted"))) ==
              PropertyPath::PathResult::ReadOnly,
          "a nested string write refuses too");
    auto mutated = std::get<std::shared_ptr<PropertyDict>>(read(bearer, "growth.evaluation"));
    mutated->elements["value"] = 999.0;
    std::get<std::shared_ptr<PropertyDict>>(mutated->elements["bindings"])->elements["k"] = 999.0;
    check(number(read(bearer, "growth.evaluation.value")) == 10.0 && number(read(bearer, "rate")) == 5.0,
          "mutating a returned report changes neither the next read nor the bearer");

    // ---- No route without an expression; stored state is never shadowed ------
    PropertyPath::PathResult absent{};
    read(bearer, "other.evaluation", &absent);
    check(absent == PropertyPath::PathResult::NoSuchProperty, "no <prefix>.expression, no <prefix>.evaluation");
    bearer.setDynamicProperty("plain.expression", field(constant(1.0)));
    bearer.setDynamicProperty("plain.evaluation", PropertyValue(std::string("authored")));
    check(text(read(bearer, "plain.evaluation")) == "authored", "an authored property named evaluation is never shadowed");

    std::cout << "ontomath_expression_evaluation_test: " << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
