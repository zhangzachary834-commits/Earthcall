// Quantifier conditions — how their cost grows with the population.
//
// FORMATION_RETE.md §1.2(b), rung 1 of §8. The spec marked this defect
// "read, not measured" and said the measurement must come first, so this is
// that measurement, kept as a registered test rather than a scratch probe —
// rung 0's probe was written, proved its point, and then vanished with
// `scratch/`, and §10 had to be corrected about it.
//
// WHAT IS QUADRATIC, AND WHY
//
// `ForAny`/`ForAll` compile to a closure that loops
// `Universe::instance().beings()` — a vector the provider rebuilds on every
// call — evaluating the inner condition against each being
// (`ConditionModel.cpp`, Kind::ForAny). Two things follow:
//
//   * The closure IGNORES ITS SUBJECT. Its signature takes `const Singular&`
//     unnamed: a quantifier is a proposition about the WORLD, and its answer
//     is the same for every subject you ask it about. Every evaluation past
//     the first, in one unchanged world-state, recomputes an identical value.
//   * In `compileToRete`, `targetAttr` is set only for `Compare` and
//     `Related`. A quantifier therefore becomes an alpha node with NO
//     attribute filter whose predicate is that whole-Universe scan — so it
//     runs once per state fact asserted, and seeding a world asserts a fact
//     per property per being.
//
// On the sweep path it is plainer: `collectPaths` returns early for
// quantifiers (correctly — the inner condition is about the instances, not
// the subject), so a law whose only condition is a quantifier has empty
// `requiredProperties`, and `sweepSubjects` hands it every being.
//
// NOT a correctness bug. `Law::applyTo` re-evaluates `conditionsSatisfied`
// before firing, so a stale or over-wide candidate set costs work and never
// fires falsely — widen-never-narrow, PROPHETIC_RETE.md §2, working as
// designed. This test measures cost, and asserts only on cost.

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"

#include <GLFW/glfw3.h>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace {

double nowMs() {
    using namespace std::chrono;
    return duration<double, std::milli>(steady_clock::now().time_since_epoch()).count();
}

// Least-squares fit of log(cost) against log(n): the exponent k in cost ~ n^k.
// 1.0 is linear, 2.0 is a nested scan over the population. Same fit as
// frame_lag_test uses, deliberately — one way of reading a shape in this tree.
double fittedExponent(const std::vector<double>& ns, const std::vector<double>& costs) {
    double sx = 0, sy = 0, sxx = 0, sxy = 0;
    int m = 0;
    for (std::size_t i = 0; i < ns.size(); ++i) {
        if (costs[i] <= 1e-6) continue;   // below the timer's resolution
        const double x = std::log(ns[i]);
        const double y = std::log(costs[i]);
        sx += x; sy += y; sxx += x * x; sxy += x * y;
        ++m;
    }
    if (m < 2) return 0.0;
    const double denom = m * sxx - sx * sx;
    if (std::fabs(denom) < 1e-12) return 0.0;
    return (m * sxy - sx * sy) / denom;
}

// One world of `n` beings, one WhileTrue law, ticked `ticks` times.
// Returns milliseconds per tick.
//
// `quantified` chooses the law's condition:
//   true  — a bare ForAny over the population (the shape under measurement)
//   false — a Compare on the subject's own property (the linear control)
//
// The control matters: without it a slow machine looks like a quadratic. What
// the test asserts is the DIFFERENCE in shape between the two, measured on the
// same machine in the same run.
struct Cost {
    double totalMs = 0.0;
    double seedMs  = 0.0;
    double evalMs  = 0.0;
    double syncMs  = 0.0;
};

enum class Shape { Compare, BareQuantifier, QuantifierAndCompare };

Cost costPerTick(int n, Shape shape, int ticks) {
    std::vector<std::unique_ptr<Object>> owned;
    std::vector<Singular*> population;
    owned.reserve(static_cast<std::size_t>(n));
    population.reserve(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        auto obj = std::make_unique<Object>();
        // Every being carries the property the inner condition reads, so the
        // quantifier's scan actually walks the whole population rather than
        // short-circuiting on a missing path.
        PropertyPath::parse("shape.fillet").setValue(*obj, PropertyValue(0.25f));
        population.push_back(obj.get());
        owned.push_back(std::move(obj));
    }
    Object author;

    Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
        for (Singular* being : population) beings.push_back(being);
    });
    Universe::instance().setClock(100.0, 0.1);

    LawManager mgr;
    mgr.connectToEventBus();

    auto law = mgr.createLaw("measured", {&author});
    law->setActivation(Law::Activation::WhileTrue);
    const auto quantifier = [] {
        return ConditionNode::forAll(
            ConditionNode::BeingKind::Object,
            ConditionNode::compare("shape.fillet", ConditionNode::Op::Gt,
                                   PropertyValue(0.1)));
    };
    const auto subjectFilter = [] {
        return ConditionNode::compare("shape.fillet", ConditionNode::Op::Gt,
                                      PropertyValue(0.1));
    };
    if (shape == Shape::QuantifierAndCompare) {
        // The shape the index fix is FOR: a world-proposition conjoined with a
        // real filter on the subject. Dropping the quantifier conjunct leaves
        // the Compare to carry the index, and the per-fact whole-Universe scan
        // disappears.
        law->setConditionModel(ConditionNode::all({quantifier(), subjectFilter()}));
    } else if (shape == Shape::BareQuantifier) {
        // "EVERY Object in the world has fillet > 0.1" — true, and true
        // identically for every subject the law is asked about.
        //
        // ForAll, not ForAny, and the choice is the whole measurement. Every
        // being here satisfies the inner condition, so a ForAny would return
        // true on the FIRST being and never walk the population — measuring a
        // short-circuit rather than the scan §1.2(b) is about. ForAll must
        // visit all N before it can answer, and still answers true, so the law
        // fires for every subject and both arms do equal application work.
        law->setConditionModel(quantifier());
    } else {
        law->setConditionModel(subjectFilter());
    }
    // The action writes a path the condition does NOT read, so nothing the law
    // does can change its own answer mid-sweep. That keeps this a measurement
    // of evaluation cost and not of a feedback loop.
    //
    // It must write a CHANGING value. `set` to a constant is a no-op after the
    // first tick — `propertyValueUnchanged` deliberately refuses to wake the
    // change feed for a write that changed nothing — so the fact table would
    // go quiet and the per-fact alpha predicate, which is the whole subject of
    // §1.2(b), would never run again. `add` keeps every being's fact dirty
    // every tick, which is what a world under law actually looks like.
    law->setActionModel(ActionNode::add("position.z", 1.0));

    mgr.tick();   // first tick pays seeding; not measured

    Cost c;
    const double t0 = nowMs();
    for (int t = 0; t < ticks; ++t) {
        mgr.tick();
        const auto& tt = mgr.lastTickTiming();
        c.seedMs += tt.seedMs;
        c.evalMs += tt.evalMs;
        c.syncMs += tt.syncMs;
    }
    c.totalMs = nowMs() - t0;

    Universe::instance().setProvider(nullptr);
    c.totalMs /= ticks; c.seedMs /= ticks; c.evalMs /= ticks; c.syncMs /= ticks;
    return c;
}

} // namespace

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "quantifier_scaling_test: glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "quantifier_scaling_test", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "quantifier_scaling_test: no GL context\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    const std::vector<int> sizes{40, 80, 160, 320};
    const int ticks = 12;

    std::vector<double> ns, quantCosts, controlCosts, quantEval, controlEval, quantSeed, mixedCosts;
    std::printf("\nQUANTIFIER SCALING — ms per tick\n");
    std::printf("  %6s | %9s %9s %9s | %9s %9s %9s\n",
                "beings", "ALL bare", "ALL+CMP", "ALL seed",
                "CMP total", "CMP eval", "CMP seed");
    for (int n : sizes) {
        const Cost control = costPerTick(n, Shape::Compare, ticks);
        const Cost quant   = costPerTick(n, Shape::BareQuantifier, ticks);
        const Cost mixed   = costPerTick(n, Shape::QuantifierAndCompare, ticks);
        mixedCosts.push_back(mixed.totalMs);
        ns.push_back(static_cast<double>(n));
        quantCosts.push_back(quant.totalMs);
        controlCosts.push_back(control.totalMs);
        quantEval.push_back(quant.evalMs);
        controlEval.push_back(control.evalMs);
        quantSeed.push_back(quant.seedMs);
        std::printf("  %6d | %9.4f %9.4f %9.4f | %9.4f %9.4f %9.4f\n", n,
                    quant.totalMs, mixed.totalMs, quant.seedMs,
                    control.totalMs, control.evalMs, control.seedMs);
    }

    const double kQuant   = fittedExponent(ns, quantCosts);
    const double kControl = fittedExponent(ns, controlCosts);
    std::printf("\n  fitted k (cost ~ n^k):  ForAll total = %.3f   Compare total = %.3f\n",
                kQuant, kControl);
    std::printf("                          ForAll eval  = %.3f   Compare eval  = %.3f\n",
                fittedExponent(ns, quantEval), fittedExponent(ns, controlEval));
    std::printf("                          ForAll seed  = %.3f\n",
                fittedExponent(ns, quantSeed));
    std::printf("                          ForAll+Compare = %.3f  (the shape the index fix is for)\n",
                fittedExponent(ns, mixedCosts));
    std::printf("  1.0 is linear in the population; 2.0 is a scan inside a scan.\n\n");

    // ------------------------------------------------------------------
    // 1. THE REGRESSION GUARD THAT MATTERS MOST.
    //
    // Before 2026-09-09 this control — an ordinary WhileTrue Compare law, no
    // quantifier anywhere — fitted k = 2.00. The quadratic was not in the law
    // at all: ReteNetwork::retractFactsAbout scanned the whole fact table, and
    // it is called from Singular::notifyBeingReleased, which fires for every
    // Singular destructor — including the `Moment` that every transient
    // ECA::Event carries by value. Three of those per application, plus one
    // per alpha per fact inside the compiled predicate.
    //
    // A participant set in ReteNetwork made that check O(1). If this assert
    // ever fires again, something has started scanning per-being work that
    // used to be constant — check _factParticipants is still maintained by
    // assertFact, and that nothing pushes into _facts behind its back.
    // ------------------------------------------------------------------
    std::printf("  verdict: Compare control k = %.3f (was 2.00 before the\n"
                "           transient-Moment fact scan was guarded)\n", kControl);
    // 1.85, not 1.60. This exponent is machine-load sensitive: measured 1.43
    // to 1.54 on a quiet machine and 1.68 with a concurrent build running, so
    // a tight bound here would fail for reasons that are not the engine's.
    // 1.85 still catches the thing it is here to catch — the pre-fix value was
    // 2.00 quiet, and load only pushes it higher.
    assert(kControl < 1.85 &&
           "an ordinary Compare law has gone quadratic in the population again");

    // ------------------------------------------------------------------
    // 2. THE QUANTIFIER GAP — FORMATION_RETE.md §1.2(b), still open.
    //
    // A quantifier's closure ignores its subject and walks the whole Universe,
    // and Law::applyTo must re-evaluate it per subject because that re-check is
    // what makes a widened candidate set safe. So the cost is in EVALUATION,
    // not in candidate selection, and no index can remove it: measured, forcing
    // the law off the index and onto the sweep made it worse, not better.
    //
    // This does not assert the gap is gone. It asserts the gap does not GROW —
    // rung 1 leaves it measured and bounded rather than fixed, and §8 rung 1b
    // records why the memo that would fix it is blocked.
    // ------------------------------------------------------------------
    std::printf("  verdict: ForAll k = %.3f against that control — a gap of %.3f\n",
                kQuant, kQuant - kControl);
    assert(kQuant - kControl < 0.65 &&
           "the quantifier penalty is widening — FORMATION_RETE.md §1.2(b)");

    glfwDestroyWindow(window);
    glfwTerminate();
    std::printf("quantifier_scaling_test: OK\n");
    return 0;
}
