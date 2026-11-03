// Rung 0 witness for Zach's Ray Transport Canon: OntoMath::restrictToLine.
//
// Path 1 (mathematics in isolation): an admitted authored expression f(p),
// restricted to p = o + s·d, must equal MathNode::evaluate at o + s·d for many
// rays and many s, within an explicitly STATED rounding bound — never a
// tolerance tuned until green. Path 2 lives in
// volume_ray_segment_transport_test, which feeds these forms to the transport
// consumer and compares against the accepted lattice.
//
// Floating-point contract (LineRestriction.hpp): the restricted form is an
// identity over the reals. Expansion reorders arithmetic, so parity is
//     |restricted(s) − evaluate(o+s·d)| ≤ 64·ε·(magnitude(s) + |evaluate|)
// with ε = 2^-52 when every coordinate reaches the tree as a double scalar
// (x/y/z), and ε = 2^-23 when the CPU evaluator itself carries the point or
// intermediate vectors as glm::vec3 (f32). The second case is the CPU
// evaluator's own rounding, characterized here rather than hidden.
//
// Claude Code · Claude Opus 5.5 · session_01HvZSR64QghrdyCpq2M7qvX · 2026-10-16.
#include "Singularity/OntoMath/LineRestriction.hpp"

#include <chrono>
#include <cmath>
#include <iostream>
#include <map>
#include <random>
#include <string>

namespace {

int g_checks = 0;
int g_failures = 0;

void check(bool condition, const std::string& description) {
    ++g_checks;
    if (!condition) {
        ++g_failures;
        std::cout << "  FAILED: " << description << std::endl;
        return;
    }
    std::cout << "  ok: " << description << std::endl;
}

using OntoMath::MathNode;
using OntoMath::ScalarForm;
using Op = MathNode::Op;

std::unique_ptr<MathNode> leaf(const std::string& name) {
    auto n = std::make_unique<MathNode>();
    n->op = Op::ValueLeaf;
    n->variableName = name;
    return n;
}
std::unique_ptr<MathNode> num(double v) {
    auto n = std::make_unique<MathNode>();
    n->op = Op::ScalarLeaf;
    n->scalarForm = ScalarForm::constant(v);
    return n;
}
std::unique_ptr<MathNode> form(ScalarForm f) {
    auto n = std::make_unique<MathNode>();
    n->op = Op::ScalarLeaf;
    n->scalarForm = std::move(f);
    return n;
}
std::unique_ptr<MathNode> node(Op op, std::unique_ptr<MathNode> a, std::unique_ptr<MathNode> b = nullptr,
                               std::unique_ptr<MathNode> c = nullptr) {
    auto n = std::make_unique<MathNode>();
    n->op = op;
    n->children.push_back(std::move(a));
    if (b) n->children.push_back(std::move(b));
    if (c) n->children.push_back(std::move(c));
    return n;
}
std::unique_ptr<MathNode> comp(std::unique_ptr<MathNode> v, const char* axis) {
    auto n = node(Op::Component, std::move(v));
    n->stringArg = axis;
    return n;
}
std::unique_ptr<MathNode> vec(std::unique_ptr<MathNode> a, std::unique_ptr<MathNode> b, std::unique_ptr<MathNode> c) {
    return node(Op::VectorConstruct, std::move(a), std::move(b), std::move(c));
}

struct Ray { glm::dvec3 o, d; };

std::map<std::string, double> lineBindings(const Ray& r, std::map<std::string, double> extra = {}) {
    for (int i = 0; i < 3; ++i) {
        extra[OntoMath::kLineOriginVars[i]] = r.o[i];
        extra[OntoMath::kLineDirectionVars[i]] = r.d[i];
    }
    return extra;
}

std::map<std::string, PropertyValue> pointVars(const glm::dvec3& p, const std::map<std::string, double>& extra) {
    std::map<std::string, PropertyValue> v = {
        {"p", PropertyValue(glm::vec3(p))},
        {"x", PropertyValue(p.x)}, {"y", PropertyValue(p.y)}, {"z", PropertyValue(p.z)}};
    for (const auto& [k, val] : extra) v[k] = PropertyValue(val);
    return v;
}

std::vector<Ray> rays() {
    std::vector<Ray> out = {
        {{0, 0, 0}, {1, 0, 0}},                 // axis, two zero components
        {{1.5, -2, 3}, {0, -1, 0}},             // negative axis
        {{-4, 0.25, 9}, {0.6, 0, -0.8}},        // one zero component
        {{0, 0, 0}, {0, 0, 0}},                 // degenerate direction: constant along s
        {{1e3, -1e3, 5e2}, {-0.3, 0.5, 0.81}},  // far origin
    };
    std::mt19937 rng(20261016u);
    std::uniform_real_distribution<double> u(-10.0, 10.0);
    for (int i = 0; i < 24; ++i) {
        glm::dvec3 d(u(rng), u(rng), u(rng));
        d = glm::length(d) > 0 ? d / glm::length(d) : glm::dvec3(1, 0, 0);
        out.push_back({{u(rng), u(rng), u(rng)}, d});
    }
    return out;
}

// Parity of a restricted scalar against the authority over every ray and s.
bool parity(const MathNode& authored, const OntoMath::LineRestrictedForm& f, double eps,
            const std::map<std::string, double>& extra, double* worstOut = nullptr) {
    double worst = 0.0;
    const double ss[] = {-7.5, -1.0, 0.0, 1e-9, 0.5, 1.0, 3.25, 40.0, 113.0};
    for (const auto& r : rays()) {
        const auto b = lineBindings(r, extra);
        const auto a = f.evaluateCoefficients(b);
        if (!a) return false;
        for (double s : ss) {
            const glm::dvec3 p = r.o + s * r.d;
            const auto ref = authored.evaluate(pointVars(p, extra));
            double expected = 0.0;
            if (!ref || !propertyValueToNumber(*ref, expected)) return false;
            const double got = OntoMath::LineRestrictedForm::evaluatePolynomial(*a, s);
            const auto mag = f.magnitudeBound(b, s);
            if (!mag) return false;
            const double bound = 64.0 * eps * (*mag + std::abs(expected)) + 1e-300;
            const double err = std::abs(got - expected);
            worst = std::max(worst, err / bound);
            if (!(err <= bound)) {
                std::cout << "    parity miss: s=" << s << " got=" << got << " expected=" << expected
                          << " bound=" << bound << "\n";
                return false;
            }
        }
    }
    if (worstOut) *worstOut = worst;
    return true;
}

constexpr double kEps64 = 2.220446049250313e-16;
constexpr double kEps32 = 1.1920928955078125e-07;

} // namespace

int main() {
    std::cout << "ontomath_line_restriction_test\n";

    // ---- The commission's witness: F(p) = 2x + 3y − z + 7 -----------------
    // Authored three ways: one ScalarLeaf signomial, an explicit tree over
    // scalar x/y/z, and Component reads of the vector point p.
    ScalarForm f = ScalarForm::variable("x", 1, 2).plus(ScalarForm::variable("y", 1, 3))
                       .plus(ScalarForm::variable("z", 1, -1)).plus(ScalarForm::constant(7));
    auto asLeaf = form(f);
    auto asTree = node(Op::Add,
        node(Op::Add, node(Op::Scale, num(2), leaf("x")), node(Op::Scale, num(3), leaf("y"))),
        node(Op::Sub, num(7), leaf("z")));
    auto asComponents = node(Op::Add,
        node(Op::Sub, node(Op::Scale, num(2), comp(leaf("p"), "x")), comp(leaf("p"), "z")),
        node(Op::Add, node(Op::Scale, num(3), comp(leaf("p"), "y")), num(7)));

    for (const auto* authored : {asLeaf.get(), asTree.get(), asComponents.get()}) {
        const auto r = OntoMath::restrictToLine(*authored);
        check(bool(r), "affine field admitted: " + authored->print());
        if (!r) { std::cout << "    refusal: " << r.refusal << "\n"; continue; }
        check(r.form->degree() == 1, "normalizes to A·s + B (degree 1): " + r.form->print());
        // A = 2dx + 3dy − dz and B = 2ox + 3oy − oz + 7, checked symbolically by value.
        bool coefficientsRight = true;
        for (const auto& ray : rays()) {
            const auto a = r.form->evaluateCoefficients(lineBindings(ray));
            const double A = 2 * ray.d.x + 3 * ray.d.y - ray.d.z;
            const double B = 2 * ray.o.x + 3 * ray.o.y - ray.o.z + 7;
            if (!a || a->size() != 2 ||
                std::abs((*a)[0] - B) > 1e-12 * (1 + std::abs(B)) ||
                std::abs((*a)[1] - A) > 1e-12 * (1 + std::abs(A))) coefficientsRight = false;
        }
        check(coefficientsRight, "A and B are the ray-local values 2dx+3dy−dz and 2ox+3oy−oz+7");
        const bool vectorPath = authored == asComponents.get();
        double worst = 0;
        check(parity(*authored, *r.form, vectorPath ? kEps32 : kEps64, {}, &worst),
              std::string("parity with MathNode::evaluate at 29 rays x 9 s (") +
                  (vectorPath ? "f32 point: CPU carries p as glm::vec3" : "double scalars") + ")");
        std::cout << "    worst error / stated bound = " << worst << "\n";
    }

    // ---- Exact degree on a ray is proved, not measured --------------------
    {
        const auto r = OntoMath::restrictToLine(*asTree);
        check(r.form->exactDegreeOn(lineBindings({{1, 2, 3}, {0, 0, 0}})) == 0,
              "a zero direction vanishes every s-coefficient exactly (degree 0)");
        // 2x − 2y on d = (1,1,0): A = 2dx − 2dy evaluates to exactly 0.0 in
        // double, but that is cancellation, not a proof — degree stays 1.
        auto cancel = form(ScalarForm::variable("x", 1, 2).plus(ScalarForm::variable("y", 1, -2)));
        const auto rc = OntoMath::restrictToLine(*cancel);
        const auto bc = lineBindings({{0, 0, 0}, {1, 1, 0}});
        check(rc.form->evaluateCoefficients(bc)->at(1) == 0.0 && rc.form->exactDegreeOn(bc) == 1,
              "numerical cancellation to 0.0 is NOT treated as exact vanishing");
        auto height = form(ScalarForm::constant(2).plus(ScalarForm::variable("y", 1, -0.1)));
        const auto rh = OntoMath::restrictToLine(*height);
        check(rh.form->exactDegreeOn(lineBindings({{0, 5, 0}, {0.6, 0.0, 0.8}})) == 0 &&
              rh.form->exactDegreeOn(lineBindings({{0, 5, 0}, {0.6, 0.1, 0.8}})) == 1,
              "height falloff is exactly constant on a horizontal ray, affine on a tilted one");
    }

    // ---- Polynomial, transformed and scaled coordinates -------------------
    {
        // Spherical falloff about a translated, scaled centre:
        // q = (p − c)/4,  g = 1 − dot(q, q)   (degree 2 in s)
        auto q = [] {
            return node(Op::Div, node(Op::Sub, leaf("p"), vec(num(1), num(-2), num(3))), num(4));
        };
        auto g = node(Op::Sub, num(1), node(Op::Dot, q(), q()));
        const auto r = OntoMath::restrictToLine(*g);
        check(bool(r) && r.form->degree() == 2, "translated/scaled dot(q,q) falloff restricts to degree 2");
        if (r) check(parity(*g, *r.form, kEps32, {}), "translated/scaled falloff parity (f32 vec path)");

        // Same falloff in double scalars: ((x−1)² + (y+2)² + (z−3)²)/16 via Pow.
        auto sq = [](const char* v, double c) {
            return node(Op::Pow, node(Op::Sub, leaf(v), num(c)), num(2));
        };
        auto g2 = node(Op::Sub, num(1),
            node(Op::Div, node(Op::Add, node(Op::Add, sq("x", 1), sq("y", -2)), sq("z", 3)), num(16)));
        const auto r2 = OntoMath::restrictToLine(*g2);
        check(bool(r2) && r2.form->degree() == 2, "integer Pow expands exactly (degree 2)");
        if (r2) check(parity(*g2, *r2.form, kEps64, {}), "Pow/Div falloff parity (double scalars)");

        // The ray-tier transform: a medium evaluates at p = worldP − origin.
        // Restricting once and binding o_local = ro − origin must equal
        // evaluating at the translated world point.
        const glm::dvec3 origin(10, -3, 7), ro(2, 1, -5), rd(0.48, 0.6, 0.64);
        const auto a = r2.form->evaluateCoefficients(lineBindings({ro - origin, rd}));
        bool ok = bool(a);
        for (double s : {0.0, 2.0, 17.0}) {
            const auto ref = g2->evaluate(pointVars(ro + s * rd - origin, {}));
            double e = 0;
            ok = ok && ref && propertyValueToNumber(*ref, e) &&
                 std::abs(OntoMath::LineRestrictedForm::evaluatePolynomial(*a, s) - e) < 1e-10;
        }
        check(ok, "medium translation enters only as ray-tier numeric binding (o_local = ro − origin)");

        // Cross and Hadamard stay polynomial.
        auto cr = comp(node(Op::Cross, leaf("p"), vec(num(0.5), num(-1), num(2))), "y");
        const auto rcx = OntoMath::restrictToLine(*cr);
        check(bool(rcx) && rcx.form->degree() == 1 && parity(*cr, *rcx.form, kEps32, {}),
              "Component(Cross(p, c)) restricts exactly");
        auto vv = vec(leaf("x"), num(2), node(Op::Scale, leaf("y"), leaf("z")));
        const auto rv = OntoMath::restrictVectorToLine(*vv);
        check(bool(rv) && (*rv.form)[0].degree() == 1 && (*rv.form)[1].degree() == 0 &&
              (*rv.form)[2].degree() == 2, "vector-valued restriction keeps per-component degree");
    }

    // ---- Symbols the line does not bind stay symbolic ---------------------
    {
        // (x + t)·t² + sin(3t)·y + k
        auto tf = node(Op::Add,
            node(Op::Add,
                node(Op::Scale, node(Op::Add, leaf("x"), leaf("t")), form(ScalarForm::variable("t", 2))),
                form(ScalarForm::transcendental(OntoMath::TransFactor::Kind::Sin, "t", 3).times(
                    ScalarForm::variable("y")))),
            leaf("k"));
        const auto r = OntoMath::restrictToLine(*tf);
        check(bool(r) && r.form->degree() == 1 && r.form->reads.count("t") && r.form->reads.count("k"),
              "time, sin(3t) and an authored parameter remain coefficient symbols");
        if (r) {
            check(parity(*tf, *r.form, kEps64, {{"t", 0.7}, {"k", -2.5}}) &&
                  parity(*tf, *r.form, kEps64, {{"t", 41.3}, {"k", 9.0}}),
                  "one restriction serves every clock value (no relowering when t advances)");
            auto b = lineBindings({{1, 2, 3}, {0, 1, 0}}, {{"t", 1.0}});
            check(!r.form->evaluateCoefficients(b), "an unbound symbol (k) refuses at the ray tier; no value is invented");
            b["k"] = std::nan("");
            check(!r.form->evaluateCoefficients(b), "a non-finite binding refuses");
            b["k"] = 1.0;
            b[OntoMath::kLineOriginVars[0]] = INFINITY;
            check(!r.form->evaluateCoefficients(b), "a non-finite ray origin refuses");
        }
        auto lnT = form(ScalarForm::transcendental(OntoMath::TransFactor::Kind::Ln, "t").times(
            ScalarForm::variable("z")));
        const auto rl = OntoMath::restrictToLine(*lnT);
        check(bool(rl) && !rl.form->evaluateCoefficients(lineBindings({{0, 0, 0}, {0, 0, 1}}, {{"t", -1.0}})),
              "ln(t) at t ≤ 0 is undefined on the ray tier, exactly as in ScalarForm::evaluate");
    }

    // ---- Refusals leave the exact evaluator in charge ---------------------
    {
        struct Case { std::string what; std::unique_ptr<MathNode> n; };
        std::vector<Case> cases;
        cases.push_back({"Noise", node(Op::Noise, leaf("p"))});
        cases.push_back({"Abs", node(Op::Abs, leaf("x"))});
        cases.push_back({"Clamp", node(Op::Clamp, leaf("x"), num(0), num(1))});
        cases.push_back({"Length", node(Op::Length, leaf("p"))});
        cases.push_back({"Sqrt", node(Op::Sqrt, leaf("x"))});
        cases.push_back({"sin(x) factor", form(ScalarForm::transcendental(OntoMath::TransFactor::Kind::Sin, "x"))});
        cases.push_back({"x^0.5 factor", form(ScalarForm::variable("x", 0.5))});
        cases.push_back({"x^-1 factor", form(ScalarForm::variable("x", -1))});
        cases.push_back({"Div by variable", node(Op::Div, num(1), leaf("x"))});
        cases.push_back({"Div by degenerate constant", node(Op::Div, leaf("x"), num(1e-9))});
        cases.push_back({"Pow non-integer", node(Op::Pow, leaf("x"), num(1.5))});
        cases.push_back({"Pow variable exponent", node(Op::Pow, leaf("x"), leaf("t"))});
        cases.push_back({"Pow over budget", node(Op::Pow, leaf("x"), num(17))});
        cases.push_back({"reserved symbol", node(Op::Add, leaf("x"), leaf(OntoMath::kLineParameterVar))});
        cases.push_back({"reserved symbol in signomial", form(ScalarForm::variable("line.d.x"))});
        cases.push_back({"vector + scalar", node(Op::Add, leaf("p"), num(1))});
        cases.push_back({"Scale(vector, vector)", comp(node(Op::Scale, leaf("p"), leaf("p")), "x")});
        auto ternary = node(Op::Add, leaf("x"), leaf("y"), leaf("z"));
        cases.push_back({"Add with three operands (undefined in MathNode::evaluate)", std::move(ternary)});
        cases.push_back({"Unsupported op", MathNode::fromJson(nlohmann::json{{"op", 200}})});
        for (auto& c : cases) {
            const auto r = c.n ? OntoMath::restrictToLine(*c.n) : OntoMath::LineRestriction{};
            check(!r && (!c.n || !r.refusal.empty()), "refuses " + c.what + " — " + r.refusal);
        }
        // Vector-valued where scalar required, and vice versa.
        check(!OntoMath::restrictToLine(*leaf("p")), "a vector expression refuses as a scalar channel");
        check(!OntoMath::restrictVectorToLine(*leaf("x")), "a scalar expression refuses as a vector channel");

        // Piecewise admission.
        using OntoMath::Piecewise;
        auto pw = [](const std::string& j) { return Piecewise::fromJson(nlohmann::json::parse(j)); };
        const std::string c1 = R"({"op":0,"scalarForm":{"terms":[{"c":1.0,"factors":{"y":1.0}}]}})";
        check(bool(OntoMath::restrictToLine(pw(R"({"input":"x","pieces":[{"mathNode":)" + c1 + "}]}"))),
              "an unconditional first piece is admitted");
        check(!OntoMath::restrictToLine(pw(R"({"input":"x","pieces":[{"lo":0.0,"mathNode":)" + c1 + "}]}")),
              "an interval-bounded first piece refuses (Piecewise ray segmentation is Rung 2)");
        check(!OntoMath::restrictToLine(pw(R"({"input":"x","pieces":[]})")),
              "an empty Piecewise refuses");
        Piecewise guarded = pw(R"({"input":"x","pieces":[{"mathNode":)" + c1 + "}]}");
        guarded.pieces[0].whereLEZero = std::shared_ptr<MathNode>(leaf("x").release());
        check(!OntoMath::restrictToLine(guarded), "a pure-guarded first piece refuses");
    }

    // ---- Structural vs value edits are visible in the restricted form -----
    {
        auto a = form(ScalarForm::constant(0.8));
        auto b = form(ScalarForm::constant(0.3));
        auto c = form(ScalarForm::constant(0.8).plus(ScalarForm::variable("y", 1, 0.01)));
        const auto ra = OntoMath::restrictToLine(*a), rb = OntoMath::restrictToLine(*b), rc = OntoMath::restrictToLine(*c);
        const auto bind = lineBindings({{0, 0, 0}, {0, 1, 0}});
        check(ra.form->degree() == 0 && rb.form->degree() == 0 &&
              ra.form->evaluateCoefficients(bind)->at(0) != rb.form->evaluateCoefficients(bind)->at(0),
              "a value-only edit changes coefficients, not degree");
        check(rc.form->degree() == 1, "a structural edit (adding y) changes degree");
    }

    // ---- Compile cost (CPU, this machine; not a GPU claim) ----------------
    {
        auto big = node(Op::Pow, node(Op::Add, node(Op::Add, leaf("x"), leaf("y")), leaf("z")), num(6));
        const auto t0 = std::chrono::steady_clock::now();
        const auto r = OntoMath::restrictToLine(*big);
        const auto us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        check(bool(r) && r.form->degree() == 6, "(x+y+z)^6 restricts within budget");
        std::size_t terms = 0;
        if (r) for (const auto& cf : r.form->coefficients) terms += cf.terms.size();
        std::cout << "    (x+y+z)^6: " << terms << " coefficient terms, lowered in " << us << " us (Debug build)\n";
        if (r) check(parity(*big, *r.form, kEps64, {}), "(x+y+z)^6 parity under the stated bound");
    }

    std::cout << "ontomath_line_restriction_test: " << (g_checks - g_failures) << "/" << g_checks
              << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
