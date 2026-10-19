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

    // ---- Rung 2e: Perlin noise through OntoMath's own box theorem --------
    {
        // clamp(0.7 + 0.3·noise(0.08·p), 0, 1) · clamp(1 − |x|/6, 0, 1)
        auto noisy = node(Op::Scale,
            node(Op::Clamp, node(Op::Add, num(0.7), node(Op::Scale, num(0.3), node(Op::Noise, node(Op::Scale, num(0.08), leaf("p"))))), num(0), num(1)),
            node(Op::Clamp, node(Op::Sub, num(1), node(Op::Div, node(Op::Abs, leaf("x")), num(6))), num(0), num(1)));
        const auto r = OntoMath::restrictAlgebraicToLine(*noisy, false);
        check(bool(r), "noise-modulated envelope admitted (Noise ranged by OntoMath's lattice/Lipschitz theorem)" +
                           (r ? std::string() : " — " + r.refusal));
        if (r) {
            bool parity = true, sound = true, emptyOutside = true;
            int probes = 0;
            for (const auto& ray : rays()) {
                OntoMath::LineAlgebraicEvaluator ev(*r.form, lineBindings(ray));
                for (double s = -10; s <= 30; s += 1.13) {
                    double e = 0;
                    const auto ref = noisy->evaluate(pointVars(ray.o + s * ray.d));
                    // Noise reads its argument as glm::vec3 on both paths: identical f32 inputs.
                    if (!ref || !propertyValueToNumber(*ref, e) || std::abs(*ev.value(r.form->roots[0], s) - e) > 1e-6)
                        parity = false;
                }
                for (const auto& piece : OntoMath::segmentLine({&ev}, -10, 30, 0.01)) {
                    const auto rg = ev.range(r.form->roots[0], piece.s0, piece.s1, piece.signs[0], nullptr);
                    for (int q = 0; q <= 6; ++q) {
                        const double s = piece.s0 + (piece.s1 - piece.s0) * q / 6;
                        const double v = *ev.value(r.form->roots[0], s);
                        ++probes;
                        if (!(rg.lo <= v && v <= rg.hi)) sound = false;
                        // |x| ≥ 6 makes the envelope saturate at exactly 0, noise or not.
                        if (std::abs((ray.o + s * ray.d).x) > 6.5 && piece.definite && !(rg.lo == 0.0f && rg.hi == 0.0f))
                            emptyOutside = false;
                    }
                }
            }
            check(parity, "noise parity with MathNode::evaluate (same f32 lattice argument)");
            check(sound, "noise piece ranges contain every probed value (" + std::to_string(probes) + " probes)");
            check(emptyOutside, "a saturated envelope proves exact emptiness THROUGH the noise factor");
        }
    }

    // ---- Rung 2f: interval derivative ranges obey the mean-value bound ----
    {
        auto wave = num(0);
        wave->scalarForm = ScalarForm::constant(0.2)
            .plus(ScalarForm::transcendental(OntoMath::TransFactor::Kind::Sin, "x", 0.7, 0.2, 1.5))
            .plus(ScalarForm::transcendental(OntoMath::TransFactor::Kind::Cos, "z", 1.3, 0.0, -0.25).times(ScalarForm::variable("y")));
        auto noisy = node(Op::Noise, node(Op::Scale, num(0.3), leaf("p")));
        auto kinked = node(Op::Union, node(Op::Abs, node(Op::Sub, leaf("x"), num(2))), node(Op::Clamp, leaf("y"), num(-1), num(4)));
        struct Subject { std::string what; const MathNode* n; double tol; };
        auto rad = radial(false);
        const std::vector<Subject> subjects = {{"radial clamp", rad.get(), 1e-9}, {"sin/cos of space", wave.get(), 1e-9},
                                               {"Perlin", noisy.get(), 1e-5}, {"abs/min/clamp kinks", kinked.get(), 1e-9}};
        std::mt19937 rng(2026u);
        std::uniform_real_distribution<double> u(-8.0, 28.0);
        for (const auto& sub : subjects) {
            const auto r = OntoMath::restrictAlgebraicToLine(*sub.n, false);
            bool ok = bool(r);
            int bounded = 0, checks = 0;
            for (const auto& ray : rays()) {
                if (!ok) break;
                OntoMath::LineAlgebraicEvaluator ev(*r.form, lineBindings(ray));
                const std::vector<int8_t> none(r.form->switches.size(), 0);
                for (int k = 0; k < 20; ++k) {
                    double a = u(rng), b = u(rng);
                    if (a > b) std::swap(a, b);
                    const auto d = ev.derivativeRange(r.form->roots[0], a, b, none);
                    if (!d.bounded()) continue;
                    ++bounded;
                    const double M = std::max(std::abs(double(d.lo)), std::abs(double(d.hi)));
                    for (int q = 0; q < 6; ++q) {
                        const double s1 = a + (b - a) * q / 6.0, s2 = a + (b - a) * (q + 1) / 6.0;
                        const double dv = std::abs(*ev.value(r.form->roots[0], s2) - *ev.value(r.form->roots[0], s1));
                        ++checks;
                        if (dv > M * (s2 - s1) * (1 + 1e-6) + sub.tol) ok = false;
                    }
                }
            }
            check(ok && bounded > 0, sub.what + ": |Δv| ≤ sup|v′|·|Δs| on every probe (" + std::to_string(checks) +
                                         " probes, " + std::to_string(bounded) + " bounded intervals)");
        }
        // Honesty: a possible jump (undecided Select) refuses a Lipschitz claim.
        using OntoMath::Piecewise;
        const auto pw = Piecewise::fromJson(nlohmann::json::parse(R"({"input":"x","pieces":[
            {"hi":0.0,"mathNode":{"op":0,"scalarForm":{"terms":[{"c":1.0,"factors":{}}]}}}]})"));
        const auto rp = OntoMath::restrictAlgebraicToLine(pw, false);
        OntoMath::LineAlgebraicEvaluator ev(*rp.form, lineBindings({{-3, 0, 0}, {1, 0, 0}}), true);
        const std::vector<int8_t> none(rp.form->switches.size(), 0);
        check(!ev.derivativeRange(rp.form->roots[0], 0.0, 6.0, none).bounded(),
              "an interval containing a Piecewise jump answers an unbounded derivative (no false Lipschitz claim)");
    }

    // ---- Rung 2f-native groundwork: spatial Lipschitz bounds over a box ---
    {
        const OntoMath::Interval box[3] = {OntoMath::Interval(-8.0f, 8.0f), OntoMath::Interval(-8.0f, 8.0f),
                                           OntoMath::Interval(-8.0f, 8.0f)};
        const std::map<std::string, OntoMath::Interval> others = {{"t", OntoMath::Interval::infinite()}};
        // Radial clamp: switch a − 0 with a = 1.5 − |p − c|/4, so ‖∇g‖ = 1/4 exactly.
        std::size_t clampLo = 0;
        for (std::size_t k = 0; k < form.switches.size(); ++k) if (form.switches[k].role == 2) clampLo = k;
        const auto L = OntoMath::lipschitzBoundOverBox(form, clampLo, box, others);
        // Per-axis partials are bounded independently and combined as √(Σ m_i²):
        // sound, but for an isotropic g each axis reaches the full ‖∇g‖ = 1/4, so the
        // bound is 1/4·√3. Tighter directional bounds are future work (§6d).
        check(L && *L >= 0.25 && *L <= 0.25 * std::sqrt(3.0) * 1.001,
              "radial clamp: spatial Lipschitz bound is sound (≥ 1/4) and within the per-axis √3 factor, got " +
                  (L ? std::to_string(*L) : std::string("none")));
        // A curtain like Northern Veil's: clamp(1 − |z − 15·sin(0.026x) − 5.5·cos(0.068x + 0.5)|/3.2, 0, 1)
        auto f = num(0);
        f->scalarForm = ScalarForm::variable("z")
            .plus(ScalarForm::transcendental(OntoMath::TransFactor::Kind::Sin, "x", 0.026, 0.0, -15.0))
            .plus(ScalarForm::transcendental(OntoMath::TransFactor::Kind::Cos, "x", 0.068, 0.5, -5.5));
        auto curtain = node(Op::Clamp, node(Op::Sub, num(1), node(Op::Div, node(Op::Abs, std::move(f)), num(3.2))), num(0), num(1));
        const auto rc = OntoMath::restrictAlgebraicToLine(*curtain, false);
        std::size_t ck = 0;
        for (std::size_t k = 0; k < rc.form->switches.size(); ++k) if (rc.form->switches[k].role == 2) ck = k;
        const auto Lc = OntoMath::lipschitzBoundOverBox(*rc.form, ck, box, others);
        // Sample difference quotients of g = a − 0 in random directions inside the box.
        std::mt19937 rng(77u);
        std::uniform_real_distribution<double> u(-8.0, 8.0), du(-1.0, 1.0);
        double worst = 0.0;
        for (int i = 0; i < 4000; ++i) {
            const glm::dvec3 p(u(rng), u(rng), u(rng));
            glm::dvec3 d(du(rng), du(rng), du(rng));
            d /= glm::length(d);
            const double h = 1e-3;
            if (glm::any(glm::greaterThan(glm::abs(p + h * d), glm::dvec3(8.0)))) continue;
            auto a = [&](const glm::dvec3& q) {
                double v = 0;
                propertyValueToNumber(*curtain->children[0]->evaluate(pointVars(q)), v);
                return v;
            };
            worst = std::max(worst, std::abs(a(p + h * d) - a(p)) / h);
        }
        std::cout << "    curtain: spatial Lipschitz bound " << (Lc ? *Lc : -1.0) << ", sampled max quotient " << worst << "\n";
        check(Lc && *Lc >= worst, "curtain: the spatial Lipschitz bound dominates every sampled difference quotient");
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

    // ---- Execution provenance (Codex review P1, 2026-10-17) -----------------
    {
        const OntoMath::ExecutionModel model{1.0 / 65536.0, 1.0 / 1024.0};
        // F = ((A + x) − A) − x: the algebra says 0, the original tree does not.
        auto cancelled = [](double A) {
            return node(Op::Sub, node(Op::Sub, node(Op::Add, num(A), leaf("x")), num(A)), leaf("x"));
        };
        const auto F54 = cancelled(18014398509481984.0);   // 2^54
        const std::map<std::string, double> mags = {{"x", 4.0}, {"y", 4.0}, {"z", 4.0}, {"t", 1.0}};
        const auto b = OntoMath::executionBound(*F54, mags, model);
        check(b && b->slack >= model.relative * 2.0 * 18014398509481984.0,
              "the original-tree slack of a cancellation scales with what cancelled, not with the exact 0");
        const auto zero = OntoMath::executionBound(*num(0.0), mags, model);
        const auto tenth = OntoMath::executionBound(*num(0.1), mags, model);
        check(zero && zero->slack == 0.0 && tenth && tenth->slack > 0.0,
              "an f32-exact literal executes exactly; 0.1 carries its representation slack");
        check(OntoMath::literalConstant(*num(2.0)) && !OntoMath::literalConstant(*node(Op::Sub, num(3.0), num(1.0))),
              "a literal is a lone constant leaf; a computed constant is not");

        // Segmentation: D = −min(F, 0) at A = 2^25 (f32 cancels; the algebra says D ≡ 0).
        OntoMath::Piecewise pw;
        pw.inputVariable = "x";
        OntoMath::Piecewise::Piece piece;
        piece.mathNode = std::shared_ptr<MathNode>(
            node(Op::Scale, num(-1.0), node(Op::Union, cancelled(33554432.0), num(0.0))).release());
        pw.pieces.push_back(piece);
        const auto form = OntoMath::restrictAlgebraicToLine(pw, false);
        check(bool(form), "the cancelled density is admitted (its algebra is exact)");
        if (form) {
            const Ray ray{{-3.0, 0.2, 0.1}, {1.0, 0.0, 0.0}};
            const std::vector<std::pair<std::size_t, uint32_t>> target = {{0, form.form->roots[0]}};
            OntoMath::LineAlgebraicEvaluator exact(*form.form, lineBindings(ray), true);
            const auto exactPieces = OntoMath::segmentLine({&exact}, 0.0, 6.0, 1e-3, nullptr, 1u << 16, true, nullptr, &target);
            bool exactEmpty = !exactPieces.empty();
            for (const auto& pc : exactPieces) exactEmpty = exactEmpty && !pc.targetSigns.empty() && pc.targetSigns[0] < 0;
            OntoMath::LineAlgebraicEvaluator faithful(*form.form, lineBindings(ray), true);
            faithful.setExecutionModel(model);
            const auto faithfulPieces = OntoMath::segmentLine({&faithful}, 0.0, 6.0, 1e-3, nullptr, 1u << 16, true, nullptr, &target);
            bool anyEmpty = false;
            for (const auto& pc : faithfulPieces) anyEmpty = anyEmpty || (!pc.targetSigns.empty() && pc.targetSigns[0] < 0);
            check(exactEmpty, "exact semantics prove D ≤ 0 along the whole ray (the cancellation is erased)");
            check(!anyEmpty, "under the execution model no stretch of the cancelled density is proved empty");
        }

        // A computed clamp bound (B − (B − 1): 1 in double, 0 in f32) refuses the line form.
        OntoMath::Piecewise clampPw;
        clampPw.inputVariable = "x";
        OntoMath::Piecewise::Piece clampPiece;
        clampPiece.mathNode = std::shared_ptr<MathNode>(
            node(Op::Sub, num(0.5),
                 node(Op::Clamp, leaf("x"), node(Op::Sub, num(33554432.0), num(33554431.0)), num(1.0))).release());
        clampPw.pieces.push_back(clampPiece);
        const auto clampForm = OntoMath::restrictAlgebraicToLine(clampPw, false);
        check(!clampForm && clampForm.refusal.find("literal constant is required") != std::string::npos,
              "a computed clamp bound refuses the line form (" + clampForm.refusal + ")");
    }

    // ---- ScalarForm::evalRange constants enclose their double (was an
    // observation until 2026-10-18; plan §5 observation 2, fixed at its owner) ----
    {
        const auto r = ScalarForm::constant(0.1).evalRange({});
        std::cout << "    ScalarForm::constant(0.1).evalRange = [" << double(r.lo) << ", " << double(r.hi) << "]\n";
        check(double(r.lo) <= 0.1 && 0.1 <= double(r.hi), "a lone double constant's float range contains the double");
        const auto exact = ScalarForm::constant(0.25).evalRange({});
        check(exact.lo == 0.25f && exact.hi == 0.25f, "a float-exact constant stays a point (no needless widening)");
        const auto huge = ScalarForm::constant(1e300).evalRange({});
        check(double(huge.lo) <= 1e300 && 1e300 <= double(huge.hi), "a constant beyond float range still encloses itself");
    }

    // ---- Value-branch segmentation (2026-10-19): what integral() now holds ----
    // Each against its analytic antiderivative, never against quadrature.
    {
        using TK = OntoMath::TransFactor::Kind;
        auto scalar = [](ScalarForm f) {
            auto n = std::make_unique<MathNode>();
            n->op = Op::ScalarLeaf;
            n->scalarForm = std::move(f);
            return n;
        };
        auto integrate = [&](const MathNode& m, const Ray& ray, double s0, double s1, std::string* why) -> std::optional<double> {
            const auto r = OntoMath::restrictAlgebraicToLine(m, false);
            if (!r) { if (why) *why = "not admitted: " + r.refusal; return std::nullopt; }
            OntoMath::LineAlgebraicEvaluator ev(*r.form, lineBindings(ray), true);
            return ev.integral(r.form->roots[0], s0, s1, std::vector<int8_t>(r.form->switches.size(), 0), why);
        };
        auto close = [](std::optional<double> v, double want) { return v && std::abs(*v - want) <= 1e-12 * std::max(1.0, std::abs(want)); };
        // ∫√(x²+1), ∫x√(x²+1), ∫x²√(x²+1) dx in closed form.
        auto F0 = [](double x) { return 0.5 * (x * std::sqrt(x * x + 1) + std::asinh(x)); };
        auto F1 = [](double x) { return std::pow(x * x + 1, 1.5) / 3.0; };
        auto F2 = [](double x) { return (x * (2 * x * x + 1) * std::sqrt(x * x + 1) - std::asinh(x)) / 8.0; };

        // (1 + x + x²)·|p| along y = 1: a polynomial times a square root.
        auto poly = scalar(ScalarForm::constant(1).plus(ScalarForm::variable("x")).plus(ScalarForm::variable("x", 2.0)));
        const auto radialPoly = node(Op::Scale, std::move(poly), node(Op::Length, leaf("p")));
        std::string why;
        const auto v1 = integrate(*radialPoly, {{-3, 1, 0}, {1, 0, 0}}, 0.0, 5.0, &why);
        const double want1 = (F0(2) - F0(-3)) + (F1(2) - F1(-3)) + (F2(2) - F2(-3));
        std::cout << "    ∫(1 + x + x²)|p| = " << (v1 ? *v1 : NAN) << " (closed form " << want1 << ")" << (why.empty() ? "" : " — " + why) << "\n";
        check(close(v1, want1), "a polynomial times a square root integrates exactly (moment recurrence)");

        // sin(x)·cos(2x) over [0, π] = −2/3 (product-to-sum).
        const auto waves = node(Op::Scale, scalar(ScalarForm::transcendental(TK::Sin, "x")),
                                scalar(ScalarForm::transcendental(TK::Cos, "x", 2.0)));
        why.clear();
        const auto v2 = integrate(*waves, {{0, 0, 0}, {1, 0, 0}}, 0.0, M_PI, &why);
        check(close(v2, -2.0 / 3.0), "sin·cos integrates exactly by product-to-sum" + (why.empty() ? "" : " — " + why));

        // clamp(10 + noise(p), 0, 1) is saturated: the noise beneath it is never read.
        const auto saturated = node(Op::Clamp, node(Op::Add, num(10), node(Op::Noise, leaf("p"))), num(0), num(1));
        why.clear();
        const auto v3 = integrate(*saturated, {{0.3, 0.2, 0.1}, {1, 0, 0}}, 0.0, 5.0, &why);
        check(close(v3, 5.0), "a saturated clamp integrates its bound without reading the noise beneath" + (why.empty() ? "" : " — " + why));

        // min(|p|, 20 + noise(p)) picks |p| ≤ √5 throughout (with margin for the proved noise
        // range over a 4-unit box, which is wider than ±1): the unchosen side holds noise.
        const auto unchosen = node(Op::Union, node(Op::Length, leaf("p")), node(Op::Add, num(20), node(Op::Noise, leaf("p"))));
        why.clear();
        const auto v4 = integrate(*unchosen, {{-2, 1, 0}, {1, 0, 0}}, 0.0, 4.0, &why);
        check(close(v4, F0(2) - F0(-2)), "min reads only its decided side (noise on the other is never integrated)" + (why.empty() ? "" : " — " + why));

        // Honest refusals: two different roots (elliptic), a root times a wave.
        const auto twoRoots = node(Op::Scale, node(Op::Length, leaf("p")), node(Op::Distance, leaf("p"), vec(1.2, 0, 0)));
        why.clear();
        const auto v5 = integrate(*twoRoots, {{-3, 1, 0}, {1, 0, 0}}, 0.0, 5.0, &why);
        check(!v5 && why.find("elliptic") != std::string::npos, "two different square roots multiplied refuse as elliptic: " + why);
        const auto rootWave = node(Op::Scale, node(Op::Length, leaf("p")), scalar(ScalarForm::transcendental(TK::Sin, "x")));
        why.clear();
        const auto v6 = integrate(*rootWave, {{-3, 1, 0}, {1, 0, 0}}, 0.0, 5.0, &why);
        check(!v6 && why.find("square root times a wave") != std::string::npos, "a square root times a wave refuses: " + why);
    }

    // ---- Hybrid integration (2026-10-19): only the remainder is sampled -------
    {
        using TK = OntoMath::TransFactor::Kind;
        auto scalar = [](ScalarForm f) {
            auto n = std::make_unique<MathNode>();
            n->op = Op::ScalarLeaf;
            n->scalarForm = std::move(f);
            return n;
        };
        // (2 + sin 40x) · (0.5 + 0.3·noise(0.2p)): a fast closed-form weight times a
        // slow noise factor, along x from −3 to 3.
        const auto m = node(Op::Scale, scalar(ScalarForm::constant(2).plus(ScalarForm::transcendental(TK::Sin, "x", 40.0))),
                            node(Op::Add, num(0.5), node(Op::Scale, num(0.3), node(Op::Noise, node(Op::Scale, num(0.2), leaf("p"))))));
        const auto r = OntoMath::restrictAlgebraicToLine(*m, false);
        check(bool(r), "hybrid witness form admitted");
        if (r) {
            const Ray ray{{-3, 0.37, 0.11}, {1, 0, 0}};
            OntoMath::LineAlgebraicEvaluator ev(*r.form, lineBindings(ray), true);
            const uint32_t root = r.form->roots[0];
            const std::vector<int8_t> none(r.form->switches.size(), 0);
            auto midpoint = [&](int n) {
                const double h = 6.0 / n;
                double sum = 0.0;
                for (int i = 0; i < n; ++i) sum += ev.value(root, (i + 0.5) * h).value_or(0.0) * h;
                return sum;
            };
            const double fine = midpoint(1 << 20), coarse = midpoint(1 << 19);
            const double reference = fine + (fine - coarse) / 3.0;
            std::string why;
            check(!ev.integral(root, 0.0, 6.0, none, &why), "the exact integral still refuses noise: " + why);
            const auto hyb = ev.hybridIntegral(root, 0.0, 6.0, none, 15, &why);
            // Fourth order where the remainder is smooth (product Simpson), against
            // the midpoint sum's second: each doubling of cells cuts the error ≥ 6×.
            {
                double previous = 0.0;
                bool order = true;
                for (uint32_t c : {8u, 16u, 32u}) {
                    const auto hc = ev.hybridIntegral(root, 0.0, 6.0, none, c);
                    const double e = hc ? std::abs(hc->estimate - reference) : 1.0;
                    if (previous > 0.0 && !(e * 6.0 < previous)) order = false;
                    previous = e;
                }
                check(order, "the hybrid converges at better than third order in its cells (8, 16, 32)");
            }
            const double plain = midpoint(16);
            check(hyb.has_value(), "the hybrid integral answers" + (why.empty() ? "" : " — " + why));
            if (hyb) {
                const double eh = std::abs(hyb->estimate - reference), ep = std::abs(plain - reference);
                std::cout << "    hybrid (2 + sin 40x)·(0.5 + 0.3 noise): reference " << reference << "; 16 noise evaluations: hybrid error "
                          << eh << " (proved radius " << hyb->radius << "), plain midpoint error " << ep << "; "
                          << hyb->remainderTerms << " remainder term(s), " << hyb->remainderEvaluations << " evaluations\n";
                check(hyb->remainderEvaluations == 16, "the remainder is sampled exactly cells + 1 times");
                check(eh <= hyb->radius, "the proved radius contains the converged integral");
                check(eh * 10.0 < ep, "at equal noise evaluations the hybrid is over ten times closer than the plain midpoint sum");
            }
            // A slow wave far from the origin: x³·sin(0.0003x + 0.3) for x ∈ [500, 501].
            // The tabular rule divides by α⁴ ≈ 8e−15 and cancels; the series does not.
            {
                const auto slow = scalar(ScalarForm::variable("x", 3.0).times(ScalarForm::transcendental(TK::Sin, "x", 0.0003, 0.3)));
                const auto rs = OntoMath::restrictAlgebraicToLine(*slow, false);
                check(bool(rs), "slow-wave witness admitted" + (rs ? std::string() : " — " + rs.refusal));
                if (rs) {
                    OntoMath::LineAlgebraicEvaluator es(*rs.form, lineBindings({{500, 0.2, 0.1}, {1, 0, 0}}), true);
                    const uint32_t sr = rs.form->roots[0];
                    auto mid = [&](int n) {
                        double sum = 0.0;
                        for (int i = 0; i < n; ++i) sum += es.value(sr, (i + 0.5) / n).value_or(0.0) / n;
                        return sum;
                    };
                    const double fine = mid(1 << 16), ref = fine + (fine - mid(1 << 15)) / 3.0;
                    const auto v = es.integral(sr, 0.0, 1.0, std::vector<int8_t>(rs.form->switches.size(), 0));
                    std::cout << "    slow wave far out: closed form " << (v ? *v : NAN) << ", converged sum " << ref << "\n";
                    check(v && std::abs(*v - ref) <= 1e-10 * std::abs(ref), "a slow wave far from the origin integrates to rounding (Taylor series, not the tabular rule)");
                }
            }
            // A fully closed-form branch: radius 0, nothing sampled.
            const auto ball = node(Op::Clamp, node(Op::Sub, num(1), node(Op::Length, leaf("p"))), num(0), num(1));
            const auto rb = OntoMath::restrictAlgebraicToLine(*ball, false);
            check(bool(rb), "closed-form witness admitted" + (rb ? std::string() : " — " + rb.refusal));
            if (rb) {
                OntoMath::LineAlgebraicEvaluator eb(*rb.form, lineBindings({{-0.5, 0.1, 0}, {1, 0, 0}}), true);
                // Signs decided as production decides them (interval ranges alone
                // leave the clamp's upper switch open where |p| is near its minimum).
                int pieces = 0, agree = 0;
                for (const auto& piece : OntoMath::segmentLine({&eb}, 0.0, 1.0, 1e-7)) {
                    if (!piece.definite) continue;
                    ++pieces;
                    const auto hb = eb.hybridIntegral(rb.form->roots[0], piece.s0, piece.s1, piece.signs[0], 8);
                    const auto xb = eb.integral(rb.form->roots[0], piece.s0, piece.s1, piece.signs[0]);
                    agree += hb && xb && hb->remainderTerms == 0 && hb->radius == 0.0 && hb->estimate == *xb;
                }
                check(pieces > 0 && agree == pieces,
                      "a closed-form branch answers the hybrid exactly, with nothing sampled (" + std::to_string(agree) + "/" +
                          std::to_string(pieces) + " definite pieces)");
            }
        }
    }

    std::cout << "ontomath_line_segmentation_test: " << (g_checks - g_failures) << "/" << g_checks
              << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
