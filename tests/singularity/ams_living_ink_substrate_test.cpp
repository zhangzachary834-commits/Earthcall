// AMS rung 0 substrate witness: Living Ink needs no new runtime C++.
//
// Zach's AMS vision unifies art hand-control, OntoMath precision and physics;
// on 2026-10-17 he ruled "dont use any new cpp to make AMS unless youre
// bridging a Singular modality/channel gap". This test proves the existing
// substrate already carries the rung-0 slice, using production code only:
//   1. a stroke dab is captured by ordinary PropertyPath coefficient writes
//      (what a Law's `set <coefficient path> to @…pointerU` performs);
//   2. each dab is the exact 2D heat-equation Green's function, so the CPU
//      evaluation equals the closed form at any authored time t;
//   3. the direct Screen emitter accepts the field, and a dab write changes
//      only the parameter block, never the WGSL (no per-stroke recompile).
//
// Pitfall this test pins: MathNode children are owned, never shared. A value
// used twice (W, tau, dx) is two copies, and a capture must write EVERY copy
// or the stale one corrupts the kernel (a stale centre made r² negative,
// e^(+huge) overflowed, and inf·0 painted NaN). Slots are located by
// sentinel coefficients, exactly as the Law generator must locate them.
// Plan: docs/plans/AMS_RUNG_0_LIVING_INK_PLAN_2026-10-17.md
// Claude Code / Claude Opus 5.5 / 8fa01418-49d7-4bfd-8a69-22d286eb6b8b / 2026-10-17.
#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "ConstructedBeing/Singular/Singular.hpp"
#include "Singularity/OntoMath/Field.hpp"
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"

#include <cmath>
#include <cstdio>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace {
using MN = OntoMath::MathNode;
using Op = MN::Op;
int failures = 0;
void check(bool ok, const std::string& what) {
    std::printf("  %s: %s\n", ok ? "ok" : "FAILED", what.c_str());
    if (!ok) ++failures;
}

class Canvas : public Singular {
public:
    std::string getIdentifier() const override { return "ams-canvas-bearer"; }
protected:
    void buildProperties() override {}
};

std::unique_ptr<MN> constant(double c) {
    auto n = std::make_unique<MN>();
    n->scalarForm = OntoMath::ScalarForm::constant(c);
    return n;
}
// One-term leaf `c * name`; its coefficient is the single writable number.
std::unique_ptr<MN> scaled(const std::string& name, double c) {
    auto n = std::make_unique<MN>();
    n->scalarForm = OntoMath::ScalarForm::variable(name);
    n->scalarForm.terms[0].coefficient = c;
    return n;
}
std::unique_ptr<MN> node(Op op, std::unique_ptr<MN> a, std::unique_ptr<MN> b = nullptr,
                         std::unique_ptr<MN> c = nullptr) {
    auto n = std::make_unique<MN>();
    n->op = op;
    for (auto* child : {&a, &b, &c}) if (*child) n->children.push_back(std::move(*child));
    return n;
}
std::unique_ptr<MN> vec(std::unique_ptr<MN> a, std::unique_ptr<MN> b, std::unique_ptr<MN> c) {
    return node(Op::VectorConstruct, std::move(a), std::move(b), std::move(c));
}
std::unique_ptr<MN> copy(const std::unique_ptr<MN>& n) { return std::make_unique<MN>(*n); }

constexpr int K = 4;            // the plan starts at 64; four keeps the witness legible
constexpr double kUnborn = 1e6; // an unborn slot's birth lies in the far future
constexpr double kS2 = 0.0004;  // initial dab width², in framebuffer-height units

// The writable slots of one dab, in capture order.
enum Slot { CU, CV, T0, SPREAD, DR, DG, DB, SLOTS };
const char* kSlotNames[SLOTS] = {"cu", "cv", "t0", "spread", "dr", "dg", "db"};
double sentinel(int k, int s) { return 7777.0 + k * 10 + s; } // no ordinary leaf uses these
double unbornDefault(int s) { return s == T0 ? kUnborn : 0.0; }

// G_k = born · s²/W · e^(−r²/W),  W = s² + spread·max(τ,0),  τ = t − t0
std::unique_ptr<MN> dabTerm(int k) {
    auto dx = node(Op::Sub, scaled("x", 1), scaled("width", sentinel(k, CU)));
    auto dy = node(Op::Sub, scaled("y", 1), scaled("height", sentinel(k, CV)));
    auto r2 = node(Op::Div,
                   node(Op::Add, node(Op::Scale, copy(dx), copy(dx)), node(Op::Scale, copy(dy), copy(dy))),
                   node(Op::Scale, scaled("height", 1), scaled("height", 1)));
    // τ is clamped inside W so W >= s² > 0 even while rewinding before birth.
    auto tau = node(Op::Sub, scaled("t", 1), constant(sentinel(k, T0)));
    auto W = node(Op::Add, constant(kS2),
                  node(Op::Scale, constant(sentinel(k, SPREAD)),
                       node(Op::Clamp, copy(tau), constant(0), constant(kUnborn))));
    auto gauss = node(Op::Pow, constant(std::exp(1.0)),
                      node(Op::Div, node(Op::Scale, constant(-1), std::move(r2)), copy(W)));
    auto mass = node(Op::Div, constant(kS2), std::move(W));
    auto born = node(Op::Clamp, node(Op::Scale, constant(1e4), std::move(tau)), constant(0), constant(1));
    auto G = node(Op::Scale, node(Op::Scale, std::move(mass), std::move(gauss)), std::move(born));
    return node(Op::Scale, std::move(G),
                vec(constant(sentinel(k, DR)), constant(sentinel(k, DG)), constant(sentinel(k, DB))));
}

void findSlots(const MN& at, const std::string& path, std::map<double, std::vector<std::string>>& out) {
    if (at.op == Op::ScalarLeaf && at.scalarForm.terms.size() == 1)
        out[at.scalarForm.terms[0].coefficient].push_back(path + ".scalarForm.terms.0.c");
    for (size_t i = 0; i < at.children.size(); ++i)
        if (at.children[i]) findSlots(*at.children[i], path + ".children." + std::to_string(i), out);
}

double closedForm(double x, double y, double w, double h, double t, const double v[][SLOTS], int channel) {
    double c = 1.0;
    for (int k = 0; k < K; ++k) {
        const double tau = t - v[k][T0];
        const double born = std::fmin(std::fmax(1e4 * tau, 0.0), 1.0);
        const double W = kS2 + v[k][SPREAD] * std::fmin(std::fmax(tau, 0.0), kUnborn);
        const double dx = x - v[k][CU] * w, dy = y - v[k][CV] * h;
        c += v[k][DR + channel] * born * (kS2 / W) * std::exp(-(dx * dx + dy * dy) / (h * h) / W);
    }
    return std::fmin(std::fmax(c, 0.0), 1.0);
}
} // namespace

int main() {
    std::printf("AMS Living Ink substrate\n");
    std::unique_ptr<MN> sum = vec(constant(1), constant(1), constant(1)); // white paper
    for (int k = 0; k < K; ++k) sum = node(Op::Add, std::move(sum), dabTerm(k));
    auto root = std::shared_ptr<MN>(node(Op::Clamp, std::move(sum),
                                         vec(constant(0), constant(0), constant(0)),
                                         vec(constant(1), constant(1), constant(1))));
    std::map<double, std::vector<std::string>> found;
    findSlots(*root, "ams.canvas.astDefinition.pieces.0.mathNode", found);

    std::vector<std::string> paths[K][SLOTS];
    bool located = true;
    for (int k = 0; k < K; ++k)
        for (int s = 0; s < SLOTS; ++s) {
            paths[k][s] = found[sentinel(k, s)];
            // dx² and dy² copy cu/cv; W appears twice (mass, exponent), so
            // spread twice and t0 three times (each W's τ, plus born).
            const size_t expected = (s == T0) ? 3 : (s == CU || s == CV || s == SPREAD) ? 2 : 1;
            if (paths[k][s].size() != expected) {
                located = false;
                std::printf("  dab %d slot %s has %zu copies, expected %zu\n", k, kSlotNames[s],
                            paths[k][s].size(), expected);
            }
        }
    check(located, "every captured value's copies are located by sentinel");

    auto field = std::make_shared<OntoMath::VectorField>();
    field->mode = OntoMath::VectorField::EvaluationMode::AST;
    field->astDefinition = OntoMath::Piecewise::continuous(root);
    Canvas bearer;
    auto ams = std::make_shared<PropertyDict>();
    ams->elements["canvas"] = field;
    bearer.setDynamicProperty("ams", ams);
    auto canvasNow = [&]() {
        PropertyValue v;
        PropertyPath::parse("ams.canvas").getValue(bearer, v);
        return std::get<std::shared_ptr<OntoMath::VectorField>>(v);
    };
    int writes = 0, refused = 0;
    auto capture = [&](int k, const double values[SLOTS]) {
        for (int s = 0; s < SLOTS; ++s)
            for (const auto& path : paths[k][s]) {
                const auto r = PropertyPath::parse(path).setValue(bearer, values[s]);
                if (r == PropertyPath::PathResult::Ok || r == PropertyPath::PathResult::Unchanged) ++writes;
                else { ++refused; std::printf("  refused write %s\n", path.c_str()); }
            }
    };
    double values[K][SLOTS];
    for (int k = 0; k < K; ++k) for (int s = 0; s < SLOTS; ++s) values[k][s] = unbornDefault(s);
    for (int k = 0; k < K; ++k) capture(k, values[k]);
    check(refused == 0, "the generator's unborn defaults commit through PropertyPath");

    const auto before = sdfwgsl::compileScreenForm(canvasNow()->astDefinition, nullptr, true);
    check(before.ok, "direct Screen admits the Living Ink field" + (before.ok ? "" : ": " + before.error));

    // A stroke: the values a dab Law copies from pointerU/V, output.time and
    // the selected brush. Slot 3 stays unborn.
    const double stroke[3][SLOTS] = {{0.30, 0.40, 0.0, 0.002, -1, -1, 0},
                                     {0.34, 0.42, 0.016, 0.002, -1, -1, 0},
                                     {0.62, 0.55, 0.5, 0.01, 0, -0.8, -0.9}};
    writes = 0;
    for (int k = 0; k < 3; ++k) {
        for (int s = 0; s < SLOTS; ++s) values[k][s] = stroke[k][s];
        capture(k, values[k]);
    }
    check(refused == 0 && writes == 3 * 12, "a three-dab stroke commits 36 coefficient writes");

    const auto after = sdfwgsl::compileScreenForm(canvasNow()->astDefinition, nullptr, true);
    check(after.ok, "direct Screen still admits the painted field");
    check(after.wgsl == before.wgsl, "a stroke changes no WGSL: the cached pipeline is reused");
    check(after.params != before.params, "a stroke changes the parameter block");

    const double w = 1920, h = 1080;
    double worst = 0;
    bool finite = true;
    for (double t : {-1.0, 0.0, 0.01, 0.3, 0.6, 2.0, 30.0})
        for (double px : {0.30, 0.31, 0.34, 0.45, 0.62, 0.9})
            for (double py : {0.40, 0.42, 0.55, 0.1}) {
                const double x = px * w, y = py * h;
                auto v = canvasNow()->astDefinition.evaluate({{"x", x}, {"y", y}, {"width", w},
                                                              {"height", h}, {"t", t}});
                if (!v || !std::holds_alternative<glm::vec3>(*v)) { finite = false; continue; }
                const auto c = std::get<glm::vec3>(*v);
                for (int ch = 0; ch < 3; ++ch) {
                    const double expected = closedForm(x, y, w, h, t, values, ch);
                    const double error = std::fabs(c[ch] - expected);
                    if (!std::isfinite(c[ch])) finite = false;
                    if ((error > 1e-6 || !std::isfinite(error)) && worst <= 1e-6)
                        std::printf("  first mismatch t=%g u=%g v=%g channel %d: field %.9g, closed form %.9g\n",
                                    t, px, py, ch, c[ch], expected);
                    worst = std::isfinite(error) ? std::fmax(worst, error) : INFINITY;
                }
            }
    std::printf("  max |field - closed form| = %.3g\n", worst);
    check(finite, "every sample is a finite colour");
    // The field computes in float32 at the glm::vec3 seams; 1e-6 is that budget.
    check(worst < 1e-6, "CPU field equals the exact heat-kernel solution at every sampled time");

    auto at = [&](double t) {
        return std::get<glm::vec3>(*canvasNow()->astDefinition.evaluate(
            {{"x", 0.30 * w}, {"y", 0.40 * h}, {"width", w}, {"height", h}, {"t", t}}));
    };
    check(at(0.001).z > 0.99f && at(0.001).x < 0.2f, "fresh stroke centre shows its blue ink");
    check(at(30.0).x > at(0.3).x, "ink spreads and thins as authored time advances");
    check(at(-1.0).x > 0.999f, "before its birth the dab does not exist (exact rewind)");

    std::printf(failures ? "FAILED (%d)\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
