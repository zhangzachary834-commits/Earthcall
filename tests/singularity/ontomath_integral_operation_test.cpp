// ontomath_integral_operation_test — MathNode::Op::Integral (40), the authored
// integral, and the one antiderivative office beneath it.
//
// Human origin: Zachary Zhang, 2026-10-19 — "yeah lets as integral operation
// biuld it on top of antiderivatives so we dont have two competing
// antiderivative code". The acceptance question it answers is Codex's,
// relayed by Zach: can the world express the mathematics WITHOUT the
// renderer? Nothing below calls the Screen path to evaluate anything. Trees
// are authored, evaluated through MathNode::evaluate / evaluateIntegral,
// serialized, typed and scoped.
//
// References: mpmath 1.3.0 quad, values agreeing to 40 digits between 50- and
// 80-digit runs (the scratch generator is recorded in the plan, §6aa).
// Claude Code · Claude Opus 5.5 · session f3af84cf-bfe3-429d-b417-2d533119f169 · 2026-10-19.

#include "Singularity/OntoMath/ScalarForm.hpp"
#include "Singularity/Screen/MathEditors.hpp"

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
using OntoMath::TransFactor;
using Op = MathNode::Op;
using K = TransFactor::Kind;
using Node = std::unique_ptr<MathNode>;

Node leaf(ScalarForm f) {
    auto n = std::make_unique<MathNode>();
    n->op = Op::ScalarLeaf;
    n->scalarForm = std::move(f);
    return n;
}
Node constant(double c) { return leaf(ScalarForm::constant(c)); }
Node named(const std::string& v) {
    auto n = std::make_unique<MathNode>();
    n->op = Op::ValueLeaf;
    n->variableName = v;
    return n;
}
template <class... Kids>
Node node(Op op, Kids... kids) {
    auto n = std::make_unique<MathNode>();
    n->op = op;
    (n->children.push_back(std::move(kids)), ...);
    return n;
}
Node integral(const std::string& v, Node body, Node lo, Node hi) {
    auto n = node(Op::Integral, std::move(body), std::move(lo), std::move(hi));
    n->variableName = v;
    return n;
}
Node integralTol(const std::string& v, Node body, Node lo, Node hi, Node tolerance) {
    auto n = node(Op::Integral, std::move(body), std::move(lo), std::move(hi), std::move(tolerance));
    n->variableName = v;
    return n;
}
ScalarForm var(const std::string& v) { return ScalarForm::variable(v); }
ScalarForm trans(K k, const std::string& v, double scale, double shift = 0.0) {
    return ScalarForm::transcendental(k, v, scale, shift);
}

std::optional<double> run(const MathNode& n, const std::map<std::string, PropertyValue>& vars, std::string* why) {
    return OntoMath::evaluateIntegral(n, vars, nullptr, why);
}

// The stated bound for every closed form here: relative 1e-13 (≈ 450 ulp),
// generous for tabular sums of a few terms; each case prints what it measured.
constexpr double kBound = 1e-13;
void against(const MathNode& n, double reference, const std::string& what,
             const std::map<std::string, PropertyValue>& vars = {}) {
    std::string why;
    const auto got = run(n, vars, &why);
    const double rel = got ? std::abs(*got - reference) / std::max(1.0, std::abs(reference)) : INFINITY;
    std::cout << "    " << what << ": " << (got ? std::to_string(*got) : "refused (" + why + ")")
              << ", relative error " << rel << "\n";
    check(got && rel <= kBound, what + " matches its reference within 1e-13");
}
} // namespace

int main() {
    std::cout << "Starting ontomath_integral_operation_test...\n";

    // ---- The term algebra, through the one office -------------------------------
    against(*integral("x", leaf(trans(K::Sin, "x", 1.0)), constant(0.0), constant(M_PI)), 2.0,
            "∫_0^π sin x dx");
    against(*integral("x", leaf(var("x").times(trans(K::Exp, "x", 2.0)).times(trans(K::Cos, "x", 3.0, 0.5))),
                      constant(0.0), constant(1.0)),
            -1.67528464400067847441447, "∫_0^1 x·e^(2x)·cos(3x + 0.5) dx (exp·cos, new to the office)");
    against(*integral("x", leaf(var("x").times(var("x")).times(trans(K::Sin, "x", 2.0)).times(trans(K::Cos, "x", 5.0))),
                      constant(0.3), constant(2.1)),
            0.9294615867391679897298404, "∫ x²·sin 2x·cos 5x dx (product-to-sum, then tabular)");
    against(*integral("x", leaf(var("x").times(trans(K::Exp, "x", 0.001)).times(trans(K::Cos, "x", 0.002, 0.1))),
                      constant(1000.0), constant(1000.5)),
            -687.0870487071256228348198, "a slow wave far from the origin (re-centred, Taylor series)");
    against(*integral("x", leaf(var("x").times(trans(K::Exp, "x", 1.0)).times(trans(K::Exp, "x", -2.0, 0.25))),
                      constant(-1.0), constant(3.0)),
            -0.2557114448268302908097201, "∫ x·e^x·e^(−2x + ¼) dx (exp·exp)");
    against(*integral("x", leaf(trans(K::Sin, "x", 3.0).times(trans(K::Sin, "x", 3.0)).times(trans(K::Sin, "x", 3.0))),
                      constant(0.0), constant(1.7)),
            0.1022297093102467209531086, "∫ sin³ 3x dx (a triple product reduced twice)");

    // ---- Scope: nested (cumulative) integrals, outer bindings ---------------------
    {
        // ∫_0^2 [∫_0^s (u² + 1) du] ds = 10/3: the inner hi is the outer variable.
        auto inner = integral("u", leaf(var("u").times(var("u")).plus(ScalarForm::constant(1.0))), constant(0.0), named("s"));
        against(*integral("s", std::move(inner), constant(0.0), constant(2.0)), 10.0 / 3.0,
                "a cumulative integral: ∫_0^2 ∫_0^s (u² + 1) du ds");
        auto innerCos = integral("u", leaf(trans(K::Cos, "u", 3.0)), constant(0.0), named("s"));
        against(*integral("s", std::move(innerCos), constant(0.0), constant(1.0)), 0.221110277400049495252397,
                "∫_0^1 ∫_0^s cos 3u du ds (substitution through a transcendental)");
        // Shadowing: the inner s is the inner variable, whatever the outer s is.
        auto shadow = integral("s", leaf(var("s")), constant(0.0), constant(1.0));
        std::string why;
        const auto v = run(*shadow, {{"s", PropertyValue(100.0)}}, &why);
        check(v && std::abs(*v - 0.5) < 1e-15, "the integration variable shadows an outer binding of the same name");
    }
    {
        // Codex's addendum A (commissioned by Zach), its witnesses verbatim:
        //   ∫_{s=0}^{1} ∫_{u=0}^{s} u du ds = 1/6;
        //   the same with the inner variable ALSO named s: its hi sees the outer s, its body the inner s;
        //   ∫_{s=0}^{1} ∫_{s=0}^{2} s ds ds = 2: the inner binding does not leak outward.
        std::string why;
        auto distinct = integral("s", integral("u", named("u"), constant(0.0), named("s")), constant(0.0), constant(1.0));
        const auto a = run(*distinct, {}, &why);
        check(a && std::abs(*a - 1.0 / 6.0) < 1e-15, "∫_0^1 ∫_0^s u du ds = 1/6 (distinct names)");
        auto same = integral("s", integral("s", named("s"), constant(0.0), named("s")), constant(0.0), constant(1.0));
        const auto b = run(*same, {}, &why);
        check(b && std::abs(*b - 1.0 / 6.0) < 1e-15, "the same with the inner variable also named s: hi sees the outer s, the body the inner");
        auto sealed = integral("s", integral("s", named("s"), constant(0.0), constant(2.0)), constant(0.0), constant(1.0));
        const auto c = run(*sealed, {}, &why);
        check(c && std::abs(*c - 2.0) < 1e-15, "∫_0^1 ∫_0^2 s ds ds = 2: the inner binding does not leak");
        // Renaming a bound variable preserves meaning; JSON keeps both scopes.
        auto renamed = integral("q", integral("r", named("r"), constant(0.0), named("q")), constant(0.0), constant(1.0));
        const auto d = run(*renamed, {}, &why);
        const auto restored = MathNode::fromJson(same->toJson());
        const auto e = restored ? run(*restored, {}, &why) : std::nullopt;
        check(d && b && *d == *b && e && *e == *b, "alpha-renaming and a JSON round trip preserve the nested meaning");
        // A free-variable edit changes the answer (nothing is cached across bindings).
        auto free = integral("s", leaf(var("k").times(var("s"))), constant(0.0), constant(1.0));
        const auto k2 = run(*free, {{"k", PropertyValue(2.0)}}, &why), k5 = run(*free, {{"k", PropertyValue(5.0)}}, &why);
        check(k2 && k5 && *k2 == 1.0 && *k5 == 2.5, "editing a free variable's binding changes the result (1 → 2.5)");
    }
    {
        auto scaled = integral("x", leaf(var("k").times(var("x"))), constant(0.0), named("t"));
        std::string why;
        const auto v = run(*scaled, {{"k", PropertyValue(3.0)}, {"t", PropertyValue(2.0)}}, &why);
        check(v && std::abs(*v - 6.0) < 1e-14, "an outer variable inside the body and a limit read in the enclosing scope");
        const auto unbound = run(*scaled, {{"t", PropertyValue(2.0)}}, &why);
        check(!unbound && why.find("'k'") != std::string::npos, "an unbound outer variable refuses, naming it: " + why);
        std::set<std::string> deps;
        scaled->collectDependencies(deps);
        check(deps == std::set<std::string>{"k", "t"}, "dependencies are k and t; the bound x is not one");
    }

    // ---- A field along a line: SDF(f, q), q affine in v ---------------------------
    {
        // f(p) = x·y + z² + sin 2x·cos y, along q(s) = (1, −2, ½) + s·(0.3, 0.7, −0.2).
        auto field = [] {
            return leaf(var("x").times(var("y")).plus(var("z").times(var("z")))
                            .plus(trans(K::Sin, "x", 2.0).times(trans(K::Cos, "y", 1.0))));
        };
        auto q1 = node(Op::VectorConstruct, leaf(ScalarForm::constant(1.0).plus(var("s").scaled(0.3))),
                       leaf(ScalarForm::constant(-2.0).plus(var("s").scaled(0.7))),
                       leaf(ScalarForm::constant(0.5).plus(var("s").scaled(-0.2))));
        against(*integral("s", node(Op::SDF, field(), std::move(q1)), constant(0.0), constant(4.0)),
                -3.477823979134871722298859, "a field along a line, the point built component-wise");
        auto q2 = node(Op::Add, node(Op::VectorConstruct, constant(1.0), constant(-2.0), constant(0.5)),
                       node(Op::Scale, named("s"), node(Op::VectorConstruct, constant(0.3), constant(0.7), constant(-0.2))));
        against(*integral("s", node(Op::SDF, field(), std::move(q2)), constant(0.0), constant(4.0)),
                -3.477823979134871722298859, "the same line written a + s·d");
        auto q3 = node(Op::Add, node(Op::VectorConstruct, constant(1.0), constant(-2.0), constant(0.5)),
                       node(Op::Scale, named("s"), node(Op::VectorConstruct, constant(0.3), constant(0.7), constant(-0.2))));
        against(*integral("s", node(Op::SDF, field(), std::move(q3)), constant(4.0), constant(0.0)),
                3.477823979134871722298859, "reversed limits negate");
    }
    {
        // clamp(1 − p·p, 0, 1) through the unit ball: value branches at the sphere.
        auto f = node(Op::Clamp, node(Op::Sub, constant(1.0), node(Op::Dot, named("p"), named("p"))),
                      constant(0.0), constant(1.0));
        auto q = node(Op::Add, node(Op::VectorConstruct, constant(-2.0), constant(0.3), constant(0.0)),
                      node(Op::Scale, named("s"), node(Op::VectorConstruct, constant(1.0), constant(0.0), constant(0.0))));
        auto ball = integral("s", node(Op::SDF, std::move(f), std::move(q)), constant(0.0), constant(4.0));
        std::string why;
        const auto got = run(*ball, {}, &why);
        const double reference = 1.157446231052560720971848;
        std::cout << "    ball chord: " << (got ? std::to_string(*got) : "refused (" + why + ")") << "\n";
        // The sphere's switch leaves an unresolved sliver: the exact path refuses
        // rather than omitting it (Codex, 2026-10-19 21:30).
        check(!got && why.find("omitting them is an approximation") != std::string::npos,
              "a branched field whose switch leaves an unresolved sliver refuses in the exact path: " + why);
        // Codex's counterexample to the refuted "below half an ulp" rule: at 1.5,
        // 0.5·ε·|1.5| is 0.75 ulp, and omitting 5·2⁻⁵⁵ (0.625 ulp) changes the
        // correctly rounded answer. No positive bound is admitted.
        check(!OntoMath::exactPathAdmitsOmittedContribution(1.5, 5.0 * std::ldexp(1.0, -55)) &&
                  OntoMath::exactPathAdmitsOmittedContribution(1.5, 0.0),
              "an omitted contribution is admitted only when proved zero (1.5 / 0.625-ulp regression)");
        (void)reference;
        // A JUMP at the switch (a Piecewise-like step) leaves a sliver whose
        // contribution is not below rounding: that refuses rather than approximating.
        auto step = node(Op::Clamp, node(Op::Scale, constant(1e12), node(Op::Sub, constant(1.0), node(Op::Dot, named("p"), named("p")))),
                         constant(0.0), constant(1.0));
        auto q2 = node(Op::Add, node(Op::VectorConstruct, constant(-2.0), constant(0.3), constant(0.0)),
                       node(Op::Scale, named("s"), node(Op::VectorConstruct, constant(1.0), constant(0.0), constant(0.0))));
        auto steep = integral("s", node(Op::SDF, std::move(step), std::move(q2)), constant(0.0), constant(4.0));
        const auto steepGot = run(*steep, {}, &why);
        std::cout << "    near-step chord: " << (steepGot ? std::to_string(*steepGot) : "refused (" + why + ")") << "\n";
        check(!steepGot && why.find("omitting them is an approximation") != std::string::npos,
              "a near-discontinuous switch whose sliver is visible refuses (never approximates)");
    }

    // ---- Refusals: exact or nothing -----------------------------------------------
    {
        std::string why;
        auto sinc = integral("x", leaf(var("x").times(var("x")).times(ScalarForm::variable("x", -3.0)).times(trans(K::Sin, "x", 1.0))),
                             constant(1.0), constant(2.0));
        check(!run(*sinc, {}, &why) && why.find("no elementary antiderivative") != std::string::npos,
              "sin x / x refuses (the sine integral is not elementary): " + why);
        auto curved = integral("s", node(Op::SDF, leaf(var("x")),
                                         node(Op::VectorConstruct, leaf(var("s").times(var("s"))), constant(0.0), constant(0.0))),
                               constant(0.0), constant(1.0));
        check(!run(*curved, {}, &why) && why.find("not affine") != std::string::npos,
              "a point quadratic in the variable refuses (a curve, not a line): " + why);
        auto noisy = integral("s", node(Op::SDF, node(Op::Noise, named("p")),
                                        node(Op::Scale, named("s"), node(Op::VectorConstruct, constant(1.0), constant(0.2), constant(0.1)))),
                              constant(0.0), constant(3.0));
        check(!run(*noisy, {}, &why), "noise refuses: it has no closed form, and no error contract is authored: " + why);
        auto improper = integral("x", leaf(trans(K::Exp, "x", -1.0)), constant(0.0), constant(INFINITY));
        check(!run(*improper, {}, &why), "an infinite limit refuses: " + why);
    }

    // ---- The same op wherever authored mathematics lives --------------------------
    {
        // As a Piecewise (the form property routes and Laws carry): ∫_0^t u du.
        auto model = OntoMath::Piecewise::continuous(integral("u", named("u"), constant(0.0), named("t")));
        const auto v = model.evaluate({{"t", PropertyValue(2.0)}});
        double d = 0.0;
        check(v && propertyValueToNumber(*v, d) && std::abs(d - 2.0) < 1e-15,
              "inside a Piecewise, MathNode::evaluate answers the integral");
        // JSON keeps the bound variable.
        auto original = integral("w", leaf(trans(K::Cos, "w", 2.0)), constant(0.0), constant(1.0));
        const auto restored = MathNode::fromJson(original->toJson());
        std::string why;
        const auto a = run(*original, {}, &why), b = restored ? run(*restored, {}, &why) : std::nullopt;
        check(restored && restored->variableName == "w" && a && b && *a == *b, "JSON round trip keeps the variable and the value");
        // Types: the body sees v; outside the integral v is unbound; limits are scalars.
        std::string error;
        check(original->checkTypes(OntoMath::TypeEnv{}, error), "the body type-checks with v bound by the Integral itself");
        auto leaky = node(Op::Add, std::move(original), named("w"));
        check(!leaky->checkTypes(OntoMath::TypeEnv{}, error), "v does not leak out of the Integral: " + error);
        auto vectorLimit = integral("x", named("x"), constant(0.0), node(Op::VectorConstruct, constant(1.0), constant(0.0), constant(0.0)));
        check(!vectorLimit->checkTypes(OntoMath::TypeEnv{}, error), "a vector limit is a type error: " + error);
        // The existing math editor gives it its shape.
        MathNode authored;
        Rendering::MathEd::initializeMathNodeForEditor(authored, Op::Integral);
        check(authored.children.size() == 3 && authored.variableName == "s", "the math editor authors body, lo, hi and a variable");
    }

    // ---- Addendum C: an authored tolerance and whole-result error admission -------
    {
        auto ballField = [] {
            return node(Op::Clamp, node(Op::Sub, constant(1.0), node(Op::Dot, named("p"), named("p"))), constant(0.0), constant(1.0));
        };
        auto ballLine = [] {
            return node(Op::Add, node(Op::VectorConstruct, constant(-2.0), constant(0.3), constant(0.0)),
                        node(Op::Scale, named("s"), node(Op::VectorConstruct, constant(1.0), constant(0.0), constant(0.0))));
        };
        std::string why;
        // Invalid tolerances refuse; 0 means exact only.
        for (double bad : {-1.0, double(NAN), double(INFINITY)}) {
            auto n = integralTol("s", node(Op::SDF, ballField(), ballLine()), constant(0.0), constant(4.0), constant(bad));
            check(!run(*n, {}, &why) && why.find("tolerance must be") != std::string::npos,
                  "tolerance " + std::to_string(bad) + " refuses: " + why);
        }
        auto exactOnly = integralTol("s", node(Op::SDF, ballField(), ballLine()), constant(0.0), constant(4.0), constant(0.0));
        check(!run(*exactOnly, {}, &why) && why.find("omitting them") != std::string::npos,
              "tolerance 0 means exact only: the sphere's sliver still refuses");

        // The sphere chord under a tolerance: admitted, within its own total.
        const double ballReference = 1.157446231052560720971848;
        auto ball = integralTol("s", node(Op::SDF, ballField(), ballLine()), constant(0.0), constant(4.0), constant(1e-12));
        OntoMath::IntegralBounds bounds;
        const auto ballValue = OntoMath::evaluateIntegral(*ball, {}, nullptr, &why, &bounds);
        std::printf("    sphere chord ±1e-12: %.17g, mathematical %.3g, execution %.3g, total %.3g, |error| %.3g\n",
                    ballValue.value_or(NAN), bounds.mathematical, bounds.execution, bounds.total(),
                    ballValue ? std::abs(*ballValue - ballReference) : NAN);
        check(ballValue && bounds.approximate && bounds.mathematical > 0.0 && bounds.total() <= 1e-12 &&
                  std::abs(*ballValue - ballReference) <= bounds.total(),
              "with a tolerance the sphere chord is admitted, its omitted sliver bounded, and the reference lies within the total");

        // Noise along a line: refused at an unreachable tolerance, admitted at a reachable one.
        auto noiseLine = [] {
            return node(Op::Add, node(Op::VectorConstruct, constant(0.3), constant(0.1), constant(0.2)),
                        node(Op::Scale, named("s"), node(Op::VectorConstruct, constant(1.0), constant(0.5), constant(0.25))));
        };
        auto tight = integralTol("s", node(Op::SDF, node(Op::Noise, named("p")), noiseLine()), constant(0.0), constant(3.0), constant(1e-12));
        check(!run(*tight, {}, &why) && why.find("exceeds the tolerance") != std::string::npos,
              "noise at 1e-12 refuses at the work cap, legibly: " + why);
        // The remainder's proved radius is first order (a Lipschitz bound per panel),
        // so it shrinks only linearly with the cells, though the estimate itself is
        // fourth order: at the 4096-cell cap it proves 0.024 here. 1e-3 therefore
        // refuses honestly; a second-derivative enclosure is the next tightening.
        auto medium = integralTol("s", node(Op::SDF, node(Op::Noise, named("p")), noiseLine()), constant(0.0), constant(3.0), constant(1e-3));
        check(!run(*medium, {}, &why) && why.find("exceeds the tolerance") != std::string::npos,
              "noise at 1e-3 refuses: the first-order proved radius cannot certify it within the work cap");
        auto loose = integralTol("s", node(Op::SDF, node(Op::Noise, named("p")), noiseLine()), constant(0.0), constant(3.0), constant(5e-2));
        const auto noiseValue = OntoMath::evaluateIntegral(*loose, {}, nullptr, &why, &bounds);
        // Independent reference: long-double Simpson of the authored Noise, evaluated by
        // MathNode::evaluate at 200 001 points (not the hybrid's samples or weights).
        auto noiseAt = node(Op::Noise, named("p"));
        auto f = [&](double s) {
            const auto v = noiseAt->evaluate({{"p", PropertyValue(glm::vec3(float(0.3 + s), float(0.1 + 0.5 * s), float(0.2 + 0.25 * s)))}});
            double d = NAN;
            if (v) propertyValueToNumber(*v, d);
            return (long double)d;
        };
        long double sum = f(0.0) + f(3.0);
        const int N = 200000;
        const long double hh = 3.0L / N;
        for (int i = 1; i < N; ++i) sum += f(double(i * hh)) * (i % 2 ? 4.0L : 2.0L);
        const double noiseReference = double(sum * hh / 3.0L);
        std::printf("    noise ±5e-2: %.10g (%s), mathematical %.3g, execution %.3g, |error vs reference| %.3g\n",
                    noiseValue.value_or(NAN), bounds.route.c_str(), bounds.mathematical, bounds.execution,
                    noiseValue ? std::abs(*noiseValue - noiseReference) : NAN);
        check(noiseValue && bounds.approximate && bounds.total() <= 5e-2 &&
                  bounds.route.find("hybrid") != std::string::npos && std::abs(*noiseValue - noiseReference) <= bounds.total(),
              "noise at 5e-2 is admitted through the hybrid route and the independent reference lies within its total bound");

        // A mathematically exact integral whose EXECUTION bound exceeds the tolerance refuses.
        auto large = integralTol("x", leaf(trans(K::Cos, "x", 1.0).scaled(1e12)), constant(0.0), constant(1.0), constant(1e-6));
        check(!run(*large, {}, &why) && why.find("execution bound") != std::string::npos,
              "an exact result whose execution bound exceeds the tolerance refuses: " + why);
        auto largeOk = integralTol("x", leaf(trans(K::Cos, "x", 1.0).scaled(1e12)), constant(0.0), constant(1.0), constant(1.0));
        const auto largeValue = OntoMath::evaluateIntegral(*largeOk, {}, nullptr, &why, &bounds);
        check(largeValue && !bounds.approximate && bounds.mathematical == 0.0 && std::abs(*largeValue - 1e12 * std::sin(1.0)) <= bounds.total(),
              "the same integral at a tolerance its execution bound fits is admitted as exact mathematics");

        // An approximate inner integral does not compose into an outer one (no propagated bound yet).
        auto inner = integralTol("u", node(Op::SDF, node(Op::Noise, named("p")), noiseLine()), constant(0.0), named("s"), constant(1e-3));
        auto outer = integralTol("s", std::move(inner), constant(0.0), constant(1.0), constant(1e-2));
        check(!run(*outer, {}, &why), "an approximate inner integral is not composed into an outer one: " + why);

        // The tolerance is read in the enclosing scope.
        auto scoped = integralTol("s", node(Op::SDF, ballField(), ballLine()), constant(0.0), constant(4.0), named("eps"));
        const auto scopedValue = run(*scoped, {{"eps", PropertyValue(1e-9)}}, &why);
        check(scopedValue && std::abs(*scopedValue - ballReference) <= 1e-9, "a tolerance bound in the enclosing scope is honoured");
        // JSON keeps the tolerance child.
        const auto restored = MathNode::fromJson(ball->toJson());
        check(restored && restored->children.size() == 4, "JSON keeps the tolerance child");
    }

    // ---- The centred decomposition backends lower (step 2b) equals the office's own definite value
    {
        const ScalarForm body = var("u").times(var("u")).times(trans(K::Exp, "u", 0.001, 0.2)).times(trans(K::Cos, "u", 0.002, 0.1))
                                    .plus(var("u").times(trans(K::Sin, "u", 5.0, -1.0)))
                                    .plus(ScalarForm::variable("u", -1.0).scaled(0.5))   // uncentred (ln)
                                    .plus(var("k").times(var("u")).times(var("u")).times(var("u")));
        const auto centred = OntoMath::centredIntegral(body, "u", "\x01m", "\x01h");
        double worst = 0.0;
        bool ok = centred.has_value();
        for (const auto& [lo, hi] : std::vector<std::pair<double, double>>{{1.0, 2.0}, {999.0, 1000.5}, {1e3, 1.05e3}, {0.25, 0.2501}, {3.0, 40.0}}) {
            const auto a = centred ? OntoMath::evaluateCentredIntegral(*centred, "u", "\x01m", "\x01h", lo, hi, {{"k", 0.3}}) : std::nullopt;
            const auto b = OntoMath::definiteIntegral(body, "u", lo, hi, {{"k", 0.3}});
            ok = ok && a && b;
            if (a && b) worst = std::max(worst, std::abs(*a - *b) / std::max(1.0, std::abs(*b)));
        }
        std::cout << "    centred decomposition vs the office's definite value: worst relative " << worst << "\n";
        check(ok && worst < 1e-12, "the centred decomposition (what backends lower) equals the office's definite integral");
    }

    // ---- The office itself: exp·cos differentiates back -----------------------------
    {
        const ScalarForm f = var("x").times(var("x")).times(trans(K::Exp, "x", -0.7, 0.2)).times(trans(K::Sin, "x", 2.5, 1.0));
        const auto F = f.antiderivative("x");
        double worst = 0.0;
        if (F) {
            const ScalarForm dF = F->derivative("x");
            for (double x = -3.0; x <= 3.0; x += 0.25) {
                const double want = *f.evaluate({{"x", x}}), got = *dF.evaluate({{"x", x}});
                worst = std::max(worst, std::abs(got - want) / std::max(1.0, std::abs(want)));
            }
        }
        std::cout << "    d/dx ∫ x²·e^(−0.7x+0.2)·sin(2.5x+1): worst relative difference " << worst << "\n";
        check(F && worst < 1e-13, "the exp·sin antiderivative differentiates back to its integrand");
    }

    std::cout << "ontomath_integral_operation_test: " << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
