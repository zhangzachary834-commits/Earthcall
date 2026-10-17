// Rung 2 witness (mathematics in isolation): OntoMath::restrictAlgebraicToLine,
// LineAlgebraicEvaluator and segmentLine.
//
// What must hold:
//  - parity: the algebraic form's value at s equals MathNode::evaluate at
//    o + s·d (f32-scaled bound where the CPU evaluator carries vectors as glm::vec3);
//  - range soundness: for any sub-interval and any decided signs, the double
//    value at points inside lies within the interval enclosure;
//  - segmentation soundness: on every definite piece each relevant switch has
//    the recorded sign at every probed point, and every true root of a relevant
//    switch (here: analytic sphere/ray intersections) lies in an uncertain piece
//    or on a piece boundary — never strictly inside a definite one;
//  - relevance: a saturated clamp hides the switches beneath it;
//  - refusals and budgets fail toward the exact evaluator.
//
// Claude Code · Claude Opus 5.5 · session_01HvZSR64QghrdyCpq2M7qvX · 2026-10-17.
#include "Singularity/OntoMath/LineRestriction.hpp"

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
std::unique_ptr<MathNode> node(Op op, std::unique_ptr<MathNode> a, std::unique_ptr<MathNode> b = nullptr,
                               std::unique_ptr<MathNode> c = nullptr) {
    auto n = std::make_unique<MathNode>();
    n->op = op;
    n->children.push_back(std::move(a));
    if (b) n->children.push_back(std::move(b));
    if (c) n->children.push_back(std::move(c));
    return n;
}
std::unique_ptr<MathNode> vec(double x, double y, double z) {
    return node(Op::VectorConstruct, num(x), num(y), num(z));
}

struct Ray { glm::dvec3 o, d; };

std::map<std::string, double> lineBindings(const Ray& r) {
    std::map<std::string, double> b;
    for (int i = 0; i < 3; ++i) {
        b[OntoMath::kLineOriginVars[i]] = r.o[i];
        b[OntoMath::kLineDirectionVars[i]] = r.d[i];
    }
    b["t"] = 0.25;
    return b;
}

std::map<std::string, PropertyValue> pointVars(const glm::dvec3& p) {
    return {{"p", PropertyValue(glm::vec3(p))}, {"x", PropertyValue(p.x)}, {"y", PropertyValue(p.y)},
            {"z", PropertyValue(p.z)}, {"t", PropertyValue(0.25)}};
}

std::vector<Ray> rays() {
    std::vector<Ray> out = {
        {{-20, 0, 0}, {1, 0, 0}},          // through the centre, axis-aligned
        {{-20, 1.5, 0}, {1, 0, 0}},        // off-centre chord
        {{-20, 7.5, 0}, {1, 0, 0}},        // misses the ball
        {{0, 0, 0}, {0, 0, 1}},            // starts at the centre
        {{3, -9, 2}, {-0.2, 0.9, 0.1}},
    };
    std::mt19937 rng(20261017u);
    std::uniform_real_distribution<double> u(-12.0, 12.0);
    for (int i = 0; i < 20; ++i) {
        glm::dvec3 d(u(rng), u(rng), u(rng));
        d /= glm::length(d);
        out.push_back({{u(rng), u(rng), u(rng)}, d});
    }
    return out;
}

constexpr double kEps32 = 1.1920928955078125e-07;

// Radial falloff about c = (1, −2, 0.5), radius 6:  clamp(1.5 − |p − c|/4, 0, 1)
// (core |p−c| ≤ 2 saturates at 1; shell 2..6 varies; outside 6 saturates at 0).
std::unique_ptr<MathNode> radial(bool viaDistance) {
    auto dist = viaDistance ? node(Op::Distance, leaf("p"), vec(1, -2, 0.5))
                            : node(Op::Length, node(Op::Sub, leaf("p"), vec(1, -2, 0.5)));
    return node(Op::Clamp, node(Op::Sub, num(1.5), node(Op::Div, std::move(dist), num(4))), num(0), num(1));
}

// Analytic roots of |o + s·d − c| = R.
std::vector<double> sphereRoots(const Ray& r, glm::dvec3 c, double R) {
    const glm::dvec3 m = r.o - c;
    const double a = glm::dot(r.d, r.d), b = 2 * glm::dot(m, r.d), cc = glm::dot(m, m) - R * R;
    const double disc = b * b - 4 * a * cc;
    if (a == 0 || disc < 0) return {};
    return {(-b - std::sqrt(disc)) / (2 * a), (-b + std::sqrt(disc)) / (2 * a)};
}

} // namespace

int main() {
    std::cout << "ontomath_line_segmentation_test\n";

    // ---- Admission of the radial class ----------------------------------
    for (bool viaDistance : {false, true}) {
        auto authored = radial(viaDistance);
        const auto r = OntoMath::restrictAlgebraicToLine(*authored, false);
        const std::string label = viaDistance ? "Distance" : "Length(p − c)";
        check(bool(r), "radial clamp falloff admitted via " + label + (r ? "" : " — " + r.refusal));
        if (!r) continue;
        check(r.form->switches.size() == 2, "only the clamp switches: |·| of an affine vector is sqrt(quadratic), no switch");
        check(!OntoMath::restrictToLine(*authored), "Rung 0 still refuses it (the polynomial class is unchanged)");

        // Parity against MathNode::evaluate (glm::length is f32 on the CPU).
        bool parity = true;
        double worst = 0;
        for (const auto& ray : rays()) {
            OntoMath::LineAlgebraicEvaluator ev(*r.form, lineBindings(ray));
            if (!ev.ok()) { parity = false; break; }
            for (double s : {-3.0, 0.0, 2.5, 7.0, 13.0, 19.5, 27.0}) {
                const auto ref = authored->evaluate(pointVars(ray.o + s * ray.d));
                double expected = 0;
                const auto got = ev.value(r.form->roots[0], s);
                if (!ref || !propertyValueToNumber(*ref, expected) || !got) { parity = false; break; }
                const double bound = 256 * kEps32 * (1 + std::abs(s) + glm::length(ray.o));
                worst = std::max(worst, std::abs(*got - expected) / bound);
                if (std::abs(*got - expected) > bound) { parity = false; break; }
            }
        }
        check(parity, label + ": value parity with MathNode::evaluate under an f32-scaled bound");
        std::cout << "    worst error / stated bound = " << worst << "\n";
    }

    auto authored = radial(false);
    const auto restriction = OntoMath::restrictAlgebraicToLine(*authored, false);
    const auto& form = *restriction.form;

    // ---- Range soundness under undecided and decided signs ---------------
    {
        std::mt19937 rng(7);
        std::uniform_real_distribution<double> u(-5.0, 30.0);
        bool sound = true;
        int probes = 0;
        for (const auto& ray : rays()) {
            OntoMath::LineAlgebraicEvaluator ev(form, lineBindings(ray));
            for (int k = 0; k < 40; ++k) {
                double a = u(rng), b = u(rng);
                if (a > b) std::swap(a, b);
                const auto pieces = OntoMath::segmentLine({&ev}, a, b, 1e-3);
                for (const auto& piece : pieces) {
                    for (const auto* signs : {&piece.signs[0]}) {
                        std::vector<int8_t> none(signs->size(), 0);
                        const auto rNone = ev.range(form.roots[0], piece.s0, piece.s1, none, nullptr);
                        const auto rDecided = ev.range(form.roots[0], piece.s0, piece.s1, *signs, nullptr);
                        for (int q = 0; q <= 8; ++q) {
                            const double s = piece.s0 + (piece.s1 - piece.s0) * q / 8.0;
                            const double v = *ev.value(form.roots[0], s);
                            ++probes;
                            if (!(rNone.lo <= v && v <= rNone.hi && rDecided.lo <= v && v <= rDecided.hi)) sound = false;
                        }
                    }
                }
            }
        }
        check(sound, "range enclosures contain every probed value (" + std::to_string(probes) + " probes, decided and undecided signs)");
    }

    // ---- Segmentation soundness: signs hold, roots are never inside a definite piece
    {
        const glm::dvec3 c(1, -2, 0.5);
        bool signsHold = true, rootsCovered = true, widthsBounded = true;
        int definite = 0, uncertain = 0;
        const double resolution = 0.02;
        for (const auto& ray : rays()) {
            OntoMath::LineAlgebraicEvaluator ev(form, lineBindings(ray));
            const auto pieces = OntoMath::segmentLine({&ev}, -5.0, 35.0, resolution);
            for (const auto& piece : pieces) {
                if (!piece.definite) {
                    ++uncertain;
                    if (piece.s1 - piece.s0 > 2 * resolution + 1e-9) {
                        widthsBounded = false;
                        std::cout << "    wide uncertain piece [" << piece.s0 << ", " << piece.s1 << "] on ray o=("
                                  << ray.o.x << "," << ray.o.y << "," << ray.o.z << ") d=(" << ray.d.x << "," << ray.d.y << "," << ray.d.z << ")\n";
                    }
                    continue;
                }
                ++definite;
                const auto& clampNode = form.nodes[form.switches[0].node];
                for (int q = 0; q <= 16; ++q) {
                    const double s = piece.s0 + (piece.s1 - piece.s0) * q / 16.0;
                    const double a = *ev.value(clampNode.a, s);
                    for (std::size_t k = 0; k < form.switches.size(); ++k) {
                        const int8_t sg = piece.signs[0][k];
                        if (sg == 0) continue;
                        const double g = form.switches[k].role == 2 ? a - clampNode.lo : a - clampNode.hi;
                        if (sg * g < -1e-9) signsHold = false;
                    }
                }
            }
            // Clamp switches are the sphere boundaries R = 6 (value 0) and R = 2 (value 1).
            for (double R : {2.0, 6.0}) {
                for (double root : sphereRoots(ray, c, R)) {
                    if (root <= -5.0 || root >= 35.0) continue;
                    for (const auto& piece : pieces) {
                        if (piece.definite && root > piece.s0 + 1e-9 && root < piece.s1 - 1e-9) {
                            // A definite piece may still contain a root of a switch that is
                            // IRRELEVANT there; both clamp switches are relevant until saturation,
                            // so a root strictly inside a definite piece is a soundness failure.
                            rootsCovered = false;
                        }
                    }
                }
            }
        }
        check(signsHold, "every recorded switch sign holds at every probed point of its definite piece");
        check(rootsCovered, "no analytic sphere/ray root lies strictly inside a definite piece");
        check(widthsBounded, "uncertain pieces stay within the requested resolution");
        std::cout << "    " << definite << " definite and " << uncertain << " uncertain pieces over 25 rays\n";
    }

    // ---- Classification on one chord: empty / shell / core ---------------
    {
        const Ray ray{{-20, -2, 0.5}, {1, 0, 0}};   // straight through the centre
        OntoMath::LineAlgebraicEvaluator ev(form, lineBindings(ray));
        const auto pieces = OntoMath::segmentLine({&ev}, 0.0, 40.0, 0.01);
        // Count maximal RUNS by classification: adjacent pieces may differ only
        // in the signs of switches that saturation made irrelevant.
        int zeroRuns = 0, coreRuns = 0;
        char last = 0;
        for (const auto& piece : pieces) {
            char label = 'g';
            if (piece.definite) {
                const auto r = ev.range(form.roots[0], piece.s0, piece.s1, piece.signs[0], nullptr);
                if (r.lo == 0.0f && r.hi == 0.0f) label = 'z';
                else if (r.lo == 1.0f && r.hi == 1.0f && ev.constantUnder(form.roots[0], piece.signs[0])) label = 'c';
            }
            if (label != last) { zeroRuns += label == 'z'; coreRuns += label == 'c'; }
            last = label;
        }
        check(zeroRuns == 2, "outside the ball the clamp saturates to an exact 0 on both sides");
        check(coreRuns == 1, "the saturated core is exactly constant (value 1) in one run");
        std::cout << "    chord pieces: " << pieces.size() << "\n";
    }

    // ---- Relevance: a saturated clamp hides switches beneath it ----------
    {
        // clamp(|x| − 5, 0, 1): on |x| < 5 the clamp is saturated at 0, so the
        // abs switch at x = 0 must not force an uncertain piece there.
        auto hidden = node(Op::Clamp, node(Op::Sub, node(Op::Abs, leaf("x")), num(5)), num(0), num(1));
        const auto r = OntoMath::restrictAlgebraicToLine(*hidden, false);
        check(bool(r) && r.form->switches.size() == 3, "clamp(|x| − 5) admitted with abs + two clamp switches");
        OntoMath::LineAlgebraicEvaluator ev(*r.form, lineBindings({{-4, 0, 0}, {1, 0, 0}}));
        const auto pieces = OntoMath::segmentLine({&ev}, 0.0, 8.0, 1e-3);   // x from −4 to 4
        // Rung 2d proposals may cut at the abs root, so boundaries can appear;
        // the claim is that an irrelevant switch never forces UNCERTAINTY.
        bool allDefinite = !pieces.empty();
        for (const auto& p : pieces) allDefinite = allDefinite && p.definite;
        check(allDefinite, "the abs switch at x = 0 is irrelevant under saturation: no uncertain piece");
    }

    // ---- Abs / min / max / bounded Piecewise -----------------------------
    {
        // abs(x − 2) + min(y, 3)  on a diagonal line, and a Piecewise in x.
        auto mixed = node(Op::Add, node(Op::Abs, node(Op::Sub, leaf("x"), num(2))),
                          node(Op::Union, leaf("y"), num(3)));
        const auto r = OntoMath::restrictAlgebraicToLine(*mixed, false);
        check(bool(r) && r.form->switches.size() == 2, "abs + min admitted (two switches)");
        bool parity = true, sound = true;
        for (const auto& ray : rays()) {
            OntoMath::LineAlgebraicEvaluator ev(*r.form, lineBindings(ray));
            for (double s = -10; s <= 30; s += 1.37) {
                double e = 0;
                const auto ref = mixed->evaluate(pointVars(ray.o + s * ray.d));
                if (!ref || !propertyValueToNumber(*ref, e) || std::abs(*ev.value(r.form->roots[0], s) - e) > 1e-9 * (1 + std::abs(e)))
                    parity = false;
            }
            for (const auto& piece : OntoMath::segmentLine({&ev}, -10, 30, 0.01)) {
                const auto rg = ev.range(r.form->roots[0], piece.s0, piece.s1, piece.signs[0], nullptr);
                for (int q = 0; q <= 4; ++q) {
                    const double v = *ev.value(r.form->roots[0], piece.s0 + (piece.s1 - piece.s0) * q / 4);
                    if (!(rg.lo <= v && v <= rg.hi)) sound = false;
                }
            }
        }
        check(parity, "abs/min parity with MathNode::evaluate (double scalars)");
        check(sound, "abs/min piece ranges contain their probed values");

        using OntoMath::Piecewise;
        const auto pw = Piecewise::fromJson(nlohmann::json::parse(R"({"input":"x","pieces":[
            {"hi":0.0,"mathNode":{"op":0,"scalarForm":{"terms":[{"c":1.0,"factors":{}}]}}},
            {"lo":0.0,"hi":5.0,"mathNode":{"op":0,"scalarForm":{"terms":[{"c":1.0,"factors":{"x":2.0}}]}}}]})"));
        const auto rp = OntoMath::restrictAlgebraicToLine(pw, false);
        check(bool(rp) && rp.form->switches.size() == 3, "bounded Piecewise admitted as a Select with three strict switches");
        OntoMath::LineAlgebraicEvaluator undefinedEv(*rp.form, lineBindings({{-3, 0, 0}, {1, 0, 0}}), false);
        OntoMath::LineAlgebraicEvaluator zeroEv(*rp.form, lineBindings({{-3, 0, 0}, {1, 0, 0}}), true);
        check(!undefinedEv.value(rp.form->roots[0], 10.0) && *zeroEv.value(rp.form->roots[0], 10.0) == 0.0,
              "no matching piece: Undefined in OntoMath, 0 only under a channel's explicit undefinedAsZero");
        check(*zeroEv.value(rp.form->roots[0], 1.0) == 1.0 && *zeroEv.value(rp.form->roots[0], 5.0) == 4.0,
              "Select picks the first matching piece (x = −2 → 1; x = 2 → x²)");
        const auto pieces = OntoMath::segmentLine({&zeroEv}, 0.0, 12.0, 1e-3);
        bool boundariesUncertain = true;
        for (const auto& piece : pieces) {
            for (double xBound : {0.0, 5.0}) {
                const double sBound = xBound + 3.0;
                if (piece.definite && sBound >= piece.s0 && sBound <= piece.s1) boundariesUncertain = false;
            }
        }
        check(boundariesUncertain, "strict Select switches: no definite piece contains a Piecewise bound (CPU/GPU include flags differ there)");
        check(*zeroEv.value(rp.form->roots[0], 7.0) == 16.0, "inside [0,5] the x² piece answers (x = 4)");
    }

    // ---- Rung 2c: affine-argument transcendentals of space ----------------
    {
        // clamp(0.6 + 0.3·sin(0.7x + 0.2)·y − 0.25·cos(1.3z) + 0.1·exp(−0.05x), 0, 1)
        auto leafForm = num(0);
        leafForm->scalarForm =
            ScalarForm::constant(0.6)
                .plus(ScalarForm::transcendental(OntoMath::TransFactor::Kind::Sin, "x", 0.7, 0.2, 0.3)
                          .times(ScalarForm::variable("y")))
                .plus(ScalarForm::transcendental(OntoMath::TransFactor::Kind::Cos, "z", 1.3, 0.0, -0.25))
                .plus(ScalarForm::transcendental(OntoMath::TransFactor::Kind::Exp, "x", -0.05, 0.0, 0.1));
        auto wave = node(Op::Clamp, std::move(leafForm), num(0), num(1));
        const auto r = OntoMath::restrictAlgebraicToLine(*wave, false);
        check(bool(r), "sin/cos/exp of space admitted as ray-affine transcendentals" + (r ? std::string() : " — " + r.refusal));
        check(!OntoMath::restrictToLine(*wave), "Rung 0 still refuses them (polynomial class unchanged)");
        if (r) {
            bool parity = true, sound = true;
            int probes = 0;
            for (const auto& ray : rays()) {
                OntoMath::LineAlgebraicEvaluator ev(*r.form, lineBindings(ray));
                if (!ev.ok()) { parity = false; break; }
                for (double s = -10; s <= 30; s += 0.91) {
                    double e = 0;
                    const auto ref = wave->evaluate(pointVars(ray.o + s * ray.d));
                    if (!ref || !propertyValueToNumber(*ref, e) || std::abs(*ev.value(r.form->roots[0], s) - e) > 1e-9 * (1 + std::abs(e)))
                        parity = false;
                }
                for (const auto& piece : OntoMath::segmentLine({&ev}, -10, 30, 0.01)) {
                    const auto rg = ev.range(r.form->roots[0], piece.s0, piece.s1, piece.signs[0], nullptr);
                    for (int q = 0; q <= 6; ++q) {
                        const double v = *ev.value(r.form->roots[0], piece.s0 + (piece.s1 - piece.s0) * q / 6);
                        ++probes;
                        if (!(rg.lo <= v && v <= rg.hi)) sound = false;
                    }
                }
            }
            check(parity, "transcendental parity with MathNode::evaluate (double scalars, 1e-9 relative)");
            check(sound, "transcendental piece ranges contain every probed value (" + std::to_string(probes) + " probes)");
            // A ray with dx = dz = 0 leaves only y varying linearly: sin(0.7x+0.2) is ray-constant.
            OntoMath::LineAlgebraicEvaluator flat(*r.form, lineBindings({{1, -50, 2}, {0, 1, 0}}));
            std::vector<int8_t> none(r.form->switches.size(), 0);
            const auto& tnode = r.form->nodes;
            bool transConstant = false;
            for (uint32_t i = 0; i < tnode.size(); ++i)
                if (tnode[i].kind == OntoMath::LineAlgebraicNode::Kind::Trans && flat.constantUnder(i, none)) transConstant = true;
            check(transConstant, "a transcendental whose argument has zero ray-direction component is proved constant");
        }
    }

    // ---- Rung 2d: analytic proposals cut cost, never soundness ------------
    {
        uint32_t evalsOn = 0, evalsOff = 0, proposed = 0;
        bool sameCover = true, soundOn = true;
        const glm::dvec3 c(1, -2, 0.5);
        for (const auto& ray : rays()) {
            OntoMath::LineAlgebraicEvaluator ev(form, lineBindings(ray));
            OntoMath::LineSegmentationStats on, off;
            const auto pOn = OntoMath::segmentLine({&ev}, -5.0, 35.0, 0.01, &on, 1u << 16, true);
            const auto pOff = OntoMath::segmentLine({&ev}, -5.0, 35.0, 0.01, &off, 1u << 16, false);
            evalsOn += on.rangeEvaluations; evalsOff += off.rangeEvaluations; proposed += on.proposedRoots;
            sameCover = sameCover && pOn.front().s0 == -5.0 && pOn.back().s1 == 35.0;
            for (double R : {2.0, 6.0})
                for (double root : sphereRoots(ray, c, R))
                    for (const auto& piece : pOn)
                        if (piece.definite && root > piece.s0 + 1e-9 && root < piece.s1 - 1e-9) soundOn = false;
        }
        std::cout << "    radial segmentation over 25 rays: " << evalsOff << " range evaluations without proposals, "
                  << evalsOn << " with " << proposed << " analytic proposals\n";
        check(soundOn && sameCover, "with proposals, no analytic root lies inside a definite piece and the cover is intact");
        check(evalsOn < evalsOff, "analytic root proposals reduce interval range evaluations");
    }

    // ---- Refusals: the exact evaluator stays in charge -------------------
    {
        struct Case { std::string what; std::unique_ptr<MathNode> n; bool vector = false; };
        std::vector<Case> cases;
        cases.push_back({"Noise", node(Op::Noise, leaf("p"))});
        cases.push_back({"ln(x) factor (CPU undefined at ≤ 0, emitter clamps)", [] { auto n = num(0); n->scalarForm = ScalarForm::transcendental(OntoMath::TransFactor::Kind::Ln, "x"); return n; }()});
        cases.push_back({"Clamp with a variable bound", node(Op::Clamp, leaf("x"), num(0), leaf("y"))});
        cases.push_back({"Clamp with inverted bounds", node(Op::Clamp, leaf("x"), num(1), num(0))});
        cases.push_back({"vector Clamp", node(Op::Clamp, leaf("p"), num(0), num(1)), true});
        cases.push_back({"Tan", node(Op::Tan, leaf("x"))});
        cases.push_back({"Normalize", node(Op::Normalize, leaf("p")), true});
        cases.push_back({"Div by a variable", node(Op::Div, num(1), node(Op::Abs, leaf("x")))});
        for (auto& c : cases) {
            const auto r = OntoMath::restrictAlgebraicToLine(*c.n, c.vector);
            check(!r && !r.refusal.empty(), "refuses " + c.what + " — " + r.refusal);
        }
        using OntoMath::Piecewise;
        auto pw = Piecewise::fromJson(nlohmann::json::parse(
            R"({"input":"p","pieces":[{"lo":0.0,"mathNode":{"op":0,"scalarForm":{"terms":[{"c":1.0,"factors":{}}]}}}]})"));
        check(!OntoMath::restrictAlgebraicToLine(pw, false), "a Piecewise cut on the vector point refuses (as the emitter does)");
        auto guarded = Piecewise::fromJson(nlohmann::json::parse(
            R"({"input":"x","pieces":[{"mathNode":{"op":0,"scalarForm":{"terms":[{"c":1.0,"factors":{}}]}}}]})"));
        guarded.pieces[0].whereLEZero = std::shared_ptr<MathNode>(leaf("x").release());
        check(!OntoMath::restrictAlgebraicToLine(guarded, false), "a guarded piece refuses");
        auto budget = node(Op::Abs, node(Op::Pow, leaf("x"), num(17)));
        const auto rb = OntoMath::restrictAlgebraicToLine(*budget, false);
        check(!rb && rb.refusal.find("budget") != std::string::npos,
              "a Rung 0 budget refusal is not routed around by the algebraic path");
    }

    // ---- Ray-tier refusals and segmentation budget -----------------------
    {
        auto withK = node(Op::Abs, node(Op::Sub, leaf("x"), leaf("k")));
        const auto r = OntoMath::restrictAlgebraicToLine(*withK, false);
        OntoMath::LineAlgebraicEvaluator ev(*r.form, lineBindings({{0, 0, 0}, {1, 0, 0}}));
        check(bool(r) && !ev.ok(), "an unbound coefficient symbol refuses the evaluator; no value invented");
        OntoMath::LineAlgebraicEvaluator rad(form, lineBindings({{-20, 0, 0}, {1, 0, 0}}));
        OntoMath::LineSegmentationStats stats;
        const auto pieces = OntoMath::segmentLine({&rad}, 0.0, 40.0, 1e-6, &stats, 6);
        bool anyUncertain = false;
        for (const auto& p : pieces) anyUncertain = anyUncertain || !p.definite;
        check(stats.budgetExhausted && anyUncertain && pieces.front().s0 == 0.0 && pieces.back().s1 == 40.0,
              "an exhausted evaluation budget leaves the remainder uncertain and the cover intact");
    }

    // ---- Observation (not an assertion): ScalarForm::evalRange constants ----
    {
        const auto r = ScalarForm::constant(0.1).evalRange({});
        std::cout << "    observation: ScalarForm::constant(0.1).evalRange = [" << double(r.lo) << ", " << double(r.hi)
                  << "] contains 0.1: " << (double(r.lo) <= 0.1 && 0.1 <= double(r.hi) ? "yes" : "NO")
                  << " (Rung 2 encloses coefficients itself)\n";
    }

    std::cout << "ontomath_line_segmentation_test: " << (g_checks - g_failures) << "/" << g_checks
              << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
