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
#include <cstdio>
#include <glm/glm.hpp>
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

    // ---- Plan §6am: the first bounded composition, transmittance = exp(−τ) ----
    // τ is an authored Integral (noise along a line, ±5e-2) on one property; the
    // transmittance is an authored ScalarLeaf exp(−tau) on another, its binding
    // reading τ's evaluation. No renderer meaning: an authored composition.
    {
        using TK = OntoMath::TransFactor::Kind;
        Object medium;
        medium.setObjectID("medium");
        auto line = [] {
            return node(Op::Add, node(Op::VectorConstruct, constant(0.3), constant(0.1), constant(0.2)),
                        node(Op::Scale, named("s"), node(Op::VectorConstruct, constant(1.0), constant(0.5), constant(0.25))));
        };
        // A nonnegative density, noise + 4 (|noise| ≤ 3.81), so τ is an optical depth.
        auto depth = node(Op::Integral, node(Op::SDF, node(Op::Add, node(Op::Noise, named("p")), constant(4.0)), line()),
                          constant(0.0), constant(3.0), constant(5e-2));
        depth->variableName = "s";
        medium.setDynamicProperty("depth.expression", field(std::move(depth)));
        medium.setDynamicProperty("transmittance.expression", field(leaf(ScalarForm::transcendental(TK::Exp, "tau", -1.0, 0.0))));
        auto tb = std::make_shared<PropertyDict>();
        tb->elements["tau"] = std::string("depth.evaluation.value");
        medium.setDynamicProperty("transmittance.bindings", PropertyValue(tb));

        // Independent reference for τ: long-double Simpson of the authored Noise (200 001 points).
        auto noiseAt = node(Op::Noise, named("p"));
        auto f = [&](double s) {
            const auto v = noiseAt->evaluate({{"p", PropertyValue(glm::vec3(float(0.3 + s), float(0.1 + 0.5 * s), float(0.2 + 0.25 * s)))}});
            double d = NAN; if (v) propertyValueToNumber(*v, d); return (long double)d;
        };
        long double sum = f(0.0) + f(3.0);
        const int N = 200000; const long double hh = 3.0L / N;
        for (int i = 1; i < N; ++i) sum += f(double(i * hh)) * (i % 2 ? 4.0L : 2.0L);
        const double tauRef = double(sum * hh / 3.0L) + 12.0;   // ∫₀³ 4 ds = 12

        const auto report = std::get<std::shared_ptr<PropertyDict>>(read(medium, "transmittance.evaluation"));
        auto at = [&](const char* k) { const auto it = report->elements.find(k); return it == report->elements.end() ? PropertyValue{} : it->second; };
        const double tau = number(read(medium, "depth.evaluation.value")), delta = number(read(medium, "depth.evaluation.totalBound"));
        const double T = number(at("value")), total = number(at("totalBound"));
        std::printf("    transmittance exp(−τ): %.12g, total bound %.3g (τ = %.10g ± %.3g; |T − exp(−τ_ref)| = %.3g)\n",
                    T, total, tau, delta, std::abs(T - std::exp(-tauRef)));
        check(text(at("status")) == "admitted" && std::abs(T - std::exp(-tau)) <= 1e-15 * T && text(at("route")) == "composition",
              "transmittance = exp(−τ) is admitted through the composition route, its value exp of τ's");
        check(std::isfinite(total) && std::abs(T - std::exp(-tauRef)) <= total,
              "the independent reference exp(−τ_ref) lies inside the propagated bound");
        // The box enters float interval arithmetic widened by ≤ 4 float ulps of τ, and
        // exp's interval is rounded outward: "to rounding" means exactly that much.
        auto ulp32 = [](double v) { const float f = float(std::abs(v)); return double(std::nextafter(f, INFINITY) - f); };
        const double monotone = std::max(std::exp(-(tau - delta)) - T, T - std::exp(-(tau + delta)));
        const double dw = delta + 4.0 * ulp32(tau + delta);
        const double widened = std::max(std::exp(-(tau - dw)) - T, T - std::exp(-(tau + dw))) + 8.0 * ulp32(std::exp(-(tau - dw)));
        std::printf("    monotone %.6g, reported %.6g (ratio %.6f), allowed to %.6g by float rounding\n", monotone, total, total / monotone, widened);
        check(total >= monotone && total <= widened,
              "the bound is Codex's monotone [exp(−(τ+δ)), exp(−(τ−δ))]: no tighter, and no looser than the float interval's rounding");
        check(text(at("qualification")) == "model-qualified" && number(read(medium, "transmittance.evaluation.inputBounds.tau")) == delta,
              "it is model-qualified, and names τ's bound as its input's");
        check(number(at("propagatedInputBound")) > 0.0 && number(at("mathematicalBound")) == 0.0 && number(at("executionBound")) < 1e-12 &&
                  std::abs(number(at("totalBound")) - number(at("propagatedInputBound")) - number(at("executionBound"))) <= 1e-18,
              "its categories say what they mean: τ's error is PROPAGATED, exp's own execution is local, the total their sum");

        // The same transmittance in ONE tree, with Let (Op 41): let tau = ∫(noise + 4) in exp(−tau).
        {
            auto innerDepth = node(Op::Integral, node(Op::SDF, node(Op::Add, node(Op::Noise, named("p")), constant(4.0)), line()),
                                   constant(0.0), constant(3.0), constant(5e-2));
            innerDepth->variableName = "s";
            auto single = node(Op::Let, std::move(innerDepth), leaf(ScalarForm::transcendental(TK::Exp, "tau", -1.0, 0.0)));
            single->variableName = "tau";
            medium.setDynamicProperty("single.expression", field(std::move(single)));
            const double Ts = number(read(medium, "single.evaluation.value")), totals = number(read(medium, "single.evaluation.totalBound"));
            check(text(read(medium, "single.evaluation.route")) == "composition" && Ts == T && std::abs(totals - total) <= 1e-6 * total &&
                      number(read(medium, "single.evaluation.inputBounds.tau")) == delta,
                  "written as one tree with Let, the transmittance and its bound match the two-property composition");
        }

        // An unknown constituent: τ from a composite whose bounds are unavailable.
        medium.setDynamicProperty("shifted.expression",
            field(node(Op::Add, constant(1.0), integral("u", leaf(ScalarForm::variable("u")), constant(0.0), constant(1.0)))));
        medium.setDynamicProperty("t2.expression", field(leaf(ScalarForm::transcendental(TK::Exp, "tau", -1.0, 0.0))));
        auto b2 = std::make_shared<PropertyDict>();
        b2->elements["tau"] = std::string("shifted.evaluation.value");
        medium.setDynamicProperty("t2.bindings", PropertyValue(b2));
        check(text(read(medium, "t2.evaluation.status")) == "admitted" && text(read(medium, "t2.evaluation.totalBound")) == "unavailable" &&
                  text(read(medium, "t2.evaluation.qualification")) == "none",
              "a constituent without a bound leaves the composite's bound unavailable, never zero: " + text(read(medium, "t2.evaluation.boundsUnavailable")));

        // Exact authored input: only the composite's own execution is charged.
        medium.setDynamicProperty("thickness", PropertyValue(0.7));
        medium.setDynamicProperty("t3.expression", field(leaf(ScalarForm::transcendental(TK::Exp, "tau", -1.0, 0.0))));
        auto b3 = std::make_shared<PropertyDict>();
        b3->elements["tau"] = std::string("thickness");
        medium.setDynamicProperty("t3.bindings", PropertyValue(b3));
        const double e3 = number(read(medium, "t3.evaluation.totalBound"));
        check(number(read(medium, "t3.evaluation.mathematicalBound")) == 0.0 && e3 > 0.0 && e3 < 1e-14 &&
                  text(read(medium, "t3.evaluation.inputBounds.tau")) == "exact",
              "exp of exact authored data carries only its execution bound (" + std::to_string(e3) + ")");

        // An opaque medium, τ = 50: magnitude arithmetic alone would charge exp(−50)'s
        // execution as if it were e^(+50) ≈ 5e21. With τ's signed range it is relative.
        medium.setDynamicProperty("opaque", PropertyValue(50.0));
        medium.setDynamicProperty("t5.expression", field(leaf(ScalarForm::transcendental(TK::Exp, "tau", -1.0, 0.0))));
        auto b5 = std::make_shared<PropertyDict>();
        b5->elements["tau"] = std::string("opaque");
        medium.setDynamicProperty("t5.bindings", PropertyValue(b5));
        const double T5 = number(read(medium, "t5.evaluation.value")), e5 = number(read(medium, "t5.evaluation.totalBound"));
        check(std::abs(T5 - std::exp(-50.0)) <= 1e-15 * T5 && e5 > 0.0 && e5 <= 1e-12 * T5,
              "exp(−50)'s execution bound is relative to e^-50 (" + std::to_string(e5 / T5) + " of it), not e^+50");

        // Codex's two reproductions (2026-10-20 15:54), now regressions: an Integral
        // reading a REPORT-derived value took it as exact, publishing 8e-14 for an
        // input uncertain by 0.0307, and promoted an unknown input to model-qualified.
        {
            auto outer = integral("u", named("x"), constant(0.0), constant(1.0));
            medium.setDynamicProperty("outer.expression", field(std::move(outer)));
            auto ob = std::make_shared<PropertyDict>();
            ob->elements["x"] = std::string("depth.evaluation.value");
            medium.setDynamicProperty("outer.bindings", PropertyValue(ob));
            check(text(read(medium, "outer.evaluation.status")) == "admitted" &&
                      text(read(medium, "outer.evaluation.totalBound")) == "unavailable" &&
                      text(read(medium, "outer.evaluation.qualification")) == "none" &&
                      text(read(medium, "outer.evaluation.boundsUnavailable")).find("'x'") != std::string::npos,
                  "∫₀¹ x du with x from an uncertain report: bound unavailable, naming x (was 8e-14, model-qualified)");
            ob->elements["x"] = std::string("shifted.evaluation.value");
            medium.setDynamicProperty("outer.bindings", PropertyValue(ob));
            check(text(read(medium, "outer.evaluation.qualification")) == "none" &&
                      text(read(medium, "outer.evaluation.totalBound")) == "unavailable",
                  "the same with x from a report WITHOUT a bound: unknown stays unknown (was promoted to model-qualified)");
        }
        // Lineage: a report-like record under another execution model is not relabelled as ours.
        {
            auto foreign = std::make_shared<PropertyDict>();
            foreign->elements["value"] = 2.0;
            foreign->elements["totalBound"] = 1e-3;
            foreign->elements["qualification"] = std::string("model-qualified");
            foreign->elements["executionModel"] = std::string("some-other-device/1");
            medium.setDynamicProperty("foreignReport", PropertyValue(foreign));
            medium.setDynamicProperty("t6.expression", field(leaf(ScalarForm::transcendental(TK::Exp, "tau", -1.0, 0.0))));
            auto b6 = std::make_shared<PropertyDict>();
            b6->elements["tau"] = std::string("foreignReport.value");
            medium.setDynamicProperty("t6.bindings", PropertyValue(b6));
            check(text(read(medium, "t6.evaluation.totalBound")) == "unavailable" &&
                      text(read(medium, "t6.evaluation.boundsUnavailable")).find("some-other-device/1") != std::string::npos,
                  "an input under a different execution model is declined, naming it: " + text(read(medium, "t6.evaluation.boundsUnavailable")));
            check(text(read(medium, "transmittance.evaluation.inputLineage.tau")).find("model-qualified under ontomath-cpu/3") == 0,
                  "a composed input's lineage is recorded: " + text(read(medium, "transmittance.evaluation.inputLineage.tau")));
        }

        // A binding that reads its own evaluation is a cycle, refused legibly.
        medium.setDynamicProperty("loop.expression", field(leaf(ScalarForm::variable("x"))));
        auto b4 = std::make_shared<PropertyDict>();
        b4->elements["x"] = std::string("loop.evaluation.value");
        medium.setDynamicProperty("loop.bindings", PropertyValue(b4));
        check(text(read(medium, "loop.evaluation.status")) == "refused",
              "a report bound to its own value refuses instead of recursing: " + text(read(medium, "loop.evaluation.reason")));
        check(text(read(medium, "loop.evaluation.reason")).find("cycle") != std::string::npos, "and the refusal names the cycle");
    }

    std::cout << "ontomath_expression_evaluation_test: " << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
