// The vocabulary filter — what it costs to find WHOM a law is about.
//
// FORMATION_RETE.md §3.0 and §8 rung 2. `LawManager::sweepSubjects` answers
// "whom does this law apply to" by calling `Universe::instance().beings()` —
// a vector the provider REBUILDS on every call — and filtering it with
// `Law::couldApplyTo`, which tests every required property name against every
// being. That runs once per sweeping law, per tick.
//
// §3.0 names it exactly: couldApplyTo's filter is "a degenerate, implicit,
// unauthored category (beings carrying `position`)", and rung 2 is to make it
// an authored Category Formation instead.
//
// WHAT DECIDES WHETHER THAT IS WORTH BUILDING: selectivity. If every being
// carries the property a law needs, an index returns everyone and saves only
// the predicate calls. If a handful carry it, an index turns O(beings) into
// O(matching) and the sweep stops being paid for beings that could never match.
// So this measures the SELECTIVE case, which is the case rung 2 exists for, and
// reports the wasted work directly rather than inferring it from a total.
//
// The laws here are OnBecomeTrue on purpose. `Law.cpp` gates the reactive
// candidate path on `WhileTrue`, so an OnBecomeTrue law always sweeps — it is
// the shape that isolates sweepSubjects instead of measuring the Rete.

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

double fittedExponent(const std::vector<double>& ns, const std::vector<double>& costs) {
    double sx = 0, sy = 0, sxx = 0, sxy = 0;
    int m = 0;
    for (std::size_t i = 0; i < ns.size(); ++i) {
        if (costs[i] <= 1e-6) continue;
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

// `n` beings, of which only `matching` carry the property the laws read.
// `laws` OnBecomeTrue laws, each of which must therefore sweep every tick.
double costPerTick(int n, int matching, int laws, int ticks) {
    std::vector<std::unique_ptr<Object>> owned;
    std::vector<Singular*> population;
    owned.reserve(static_cast<std::size_t>(n));
    population.reserve(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        auto obj = std::make_unique<Object>();
        // Only the first `matching` beings carry `beacon`. The rest cannot
        // satisfy the laws and cannot be made to — which is precisely what a
        // Category Formation would let the engine know without looking.
        if (i < matching) obj->setDynamicProperty("beacon", PropertyValue(1.0));
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

    for (int l = 0; l < laws; ++l) {
        auto law = mgr.createLaw("beacon-law-" + std::to_string(l), {&author});
        law->setActivation(Law::Activation::OnBecomeTrue);
        law->setConditionModel(ConditionNode::compare(
            "beacon", ConditionNode::Op::Gt, PropertyValue(0.0)));
        law->setActionModel(ActionNode::add("position.z", 1.0));
    }

    mgr.tick();   // first tick pays seeding and the false->true edge

    const double t0 = nowMs();
    for (int t = 0; t < ticks; ++t) mgr.tick();
    const double elapsed = nowMs() - t0;

    Universe::instance().setProvider(nullptr);
    return elapsed / ticks;
}

} // namespace

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "category_index_scaling_test: glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "category_index_scaling_test", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "category_index_scaling_test: no GL context\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    const int ticks = 12;
    const int kMatching = 8;      // held fixed: the work that is NOT waste
    const int kLaws = 8;

    // ------------------------------------------------------------------
    // A. Population grows; the number of beings that can EVER match does not.
    //    Every extra being is pure waste for these laws. If the cost tracks
    //    the population rather than the matching set, the sweep is paying for
    //    beings a Category would have excluded without looking.
    // ------------------------------------------------------------------
    const std::vector<int> sizes{125, 250, 500, 1000};
    std::vector<double> ns, costs;
    std::printf("\nVOCABULARY FILTER — %d OnBecomeTrue laws, only %d beings can ever match\n",
                kLaws, kMatching);
    std::printf("  %8s  %12s  %14s\n", "beings", "ms/tick", "ms per being");
    for (int n : sizes) {
        const double c = costPerTick(n, kMatching, kLaws, ticks);
        ns.push_back(static_cast<double>(n));
        costs.push_back(c);
        std::printf("  %8d  %12.4f  %14.6f\n", n, c, c / n);
    }
    const double kPop = fittedExponent(ns, costs);
    std::printf("\n  fitted k against POPULATION (matching set held at %d) = %.3f\n",
                kMatching, kPop);
    std::printf("  1.0 means the sweep costs the whole world; 0.0 means it costs\n"
                "  only what could match — which is what a Category Formation buys.\n");

    // ------------------------------------------------------------------
    // B. Laws grow; population fixed. sweepSubjects rebuilds the whole being
    //    vector once per sweeping law, so this is the other half of O(L x N).
    // ------------------------------------------------------------------
    const std::vector<int> lawCounts{2, 4, 8, 16};
    std::vector<double> ls, lawCosts;
    std::printf("\n  %8s  %12s   (1000 beings, %d matching)\n", "laws", "ms/tick", kMatching);
    for (int l : lawCounts) {
        const double c = costPerTick(1000, kMatching, l, ticks);
        ls.push_back(static_cast<double>(l));
        lawCosts.push_back(c);
        std::printf("  %8d  %12.4f\n", l, c);
    }
    const double kLawScaling = fittedExponent(ls, lawCosts);
    std::printf("\n  fitted k against LAW COUNT = %.3f\n\n", kLawScaling);

    // This test is a measurement first. The only assertion is the one that
    // would catch a genuine explosion: neither axis may go super-linear, which
    // would mean the sweep grew a nested scan it does not have today.
    assert(kPop < 1.60 && "the vocabulary sweep has gone super-linear in population");
    assert(kLawScaling < 1.60 && "the vocabulary sweep has gone super-linear in law count");

    glfwDestroyWindow(window);
    glfwTerminate();
    std::printf("category_index_scaling_test: OK\n");
    return 0;
}
