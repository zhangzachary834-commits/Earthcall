// Naming another being — what it costs a law to say `@gate.open`.
//
// FORMATION_RETE.md §8 rung 3. A qualified root addresses ONE named being
// (§1.1: there is no free variable in the condition language). Reading it goes
// through `lawGetValue` -> `resolveLawRoot` (`MathBinding.hpp`), which:
//
//   1. calls `Universe::instance().beings()` — a vector the provider REBUILDS,
//      allocating and pushing N pointers, every single call; then
//   2. for each dotted prefix of the path, scans all N beings comparing
//      `getIdentifier()` — a virtual call returning a std::string by value,
//      so a string construction and compare per being per prefix.
//
// That is O(segments x N) with heavy constants, per READ. A qualified-root
// condition is read once per subject per tick, so a law that names another
// being costs O(N^2) a tick — and `ConditionModel.cpp` records that this is
// not hypothetical: "the ambient theme, the draw-mode indicator, the slider
// clamp, the crystal's pulse, and the stroke-drawing law" all read a channel
// or a state being through an `@` root, and art-stroke-draw-law runs sixty
// times a second.
//
// WHAT THIS MEASURES, and why it is not the resolve scan.
//
// The scan above is real but small: at 480 beings it is a few ms of a tick that
// costs hundreds, so optimizing it would be treating a symptom nobody feels.
// The cost rung 3 is actually about is the WIDENING.
//
// `compileToRete` drops a qualified-root conjunct from the index — correctly,
// because no fact carries an "@"-rooted attribute, so compiling it in would
// narrow the candidate set to nothing. But that means the conjunct contributes
// NO filtering at all, and rung 1 established why it cannot: like a quantifier,
// a qualified root is subject-independent. "@gate.open > 0" is one truth about
// the world, the same for every subject you ask it about.
//
// Which makes it an all-or-nothing GATE, not a filter. When the gate is shut,
// the correct candidate set is EMPTY — and the engine should be able to say so
// once, rather than walking every subject to discover it one refusal at a time.
//
// So the two arms are the same law with the gate OPEN and SHUT. If shutting the
// gate does not make the law cheaper, the engine is paying full price to do
// nothing, once per subject, every tick — and `ConditionModel.cpp` names the
// laws that live this way: "the ambient theme, the draw-mode indicator, the
// slider clamp, the crystal's pulse, and the stroke-drawing law".

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

double costPerTick(int n, bool gateOpen, int ticks) {
    std::vector<std::unique_ptr<Object>> owned;
    std::vector<Singular*> population;
    owned.reserve(static_cast<std::size_t>(n));
    population.reserve(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        auto obj = std::make_unique<Object>();
        obj->setObjectID("crowd-" + std::to_string(i));
        // Every being carries `beacon`, so the control's law reaches all of
        // them and both arms fire the same number of times.
        obj->setDynamicProperty("beacon", PropertyValue(1.0));
        population.push_back(obj.get());
        owned.push_back(std::move(obj));
    }

    // The named being, deliberately LAST in the provider's order — the scan in
    // resolveLawRoot walks until it matches, so a referent at the end is the
    // honest worst case and a referent at the front would flatter it.
    auto gateOwned = std::make_unique<Object>();
    gateOwned->setObjectID("gate");
    gateOwned->setDynamicProperty("open", PropertyValue(gateOpen ? 1.0 : 0.0));
    Object* gate = gateOwned.get();
    population.push_back(gate);

    Object author;

    Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
        for (Singular* being : population) beings.push_back(being);
    });
    Universe::instance().setClock(100.0, 0.1);

    LawManager mgr;
    mgr.connectToEventBus();

    auto law = mgr.createLaw(gateOpen ? "gate-open" : "gate-shut", {&author});
    law->setActivation(Law::Activation::WhileTrue);
    law->setConditionModel(
        ConditionNode::compare("@gate.open", ConditionNode::Op::Gt, PropertyValue(0.0)));
    law->setActionModel(ActionNode::add("position.z", 1.0));

    mgr.tick();

    // The gate must actually decide something. An open gate has to fire and a
    // shut one has to stay silent, or the timing below is comparing two things
    // that are not the same law. (This test's first version compared a working
    // law against one that silently resolved to nothing, and two measurements
    // in rung 1 made the same class of mistake.)
    {
        PropertyValue before, after;
        PropertyPath::parse("position.z").getValue(*population[0], before);
        mgr.tick();
        PropertyPath::parse("position.z").getValue(*population[0], after);
        double b = 0.0, a = 0.0;
        propertyValueToNumber(before, b);
        propertyValueToNumber(after, a);
        if (gateOpen) {
            assert(a > b && "an open gate must fire; the timing would be meaningless");
        } else {
            assert(a == b && "a shut gate must not fire; the timing would be meaningless");
        }
    }

    const double t0 = nowMs();
    for (int t = 0; t < ticks; ++t) mgr.tick();
    const double elapsed = nowMs() - t0;

    Universe::instance().setProvider(nullptr);
    return elapsed / ticks;
}

} // namespace

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "referent_resolution_test: glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "referent_resolution_test", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "referent_resolution_test: no GL context\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    const std::vector<int> sizes{60, 120, 240, 480};
    const int ticks = 10;

    std::vector<double> ns, shut, open;
    std::printf("\nA GATE THAT IS SHUT — ms per tick\n");
    std::printf("  %7s  %14s  %14s  %10s\n", "beings", "gate OPEN", "gate SHUT",
                "shut/open");
    for (int n : sizes) {
        const double o = costPerTick(n, true, ticks);
        const double c = costPerTick(n, false, ticks);
        ns.push_back(static_cast<double>(n));
        open.push_back(o);
        shut.push_back(c);
        std::printf("  %7d  %14.4f  %14.4f  %10.2f\n", n, o, c, o > 1e-9 ? c / o : 0.0);
    }

    const double kShut = fittedExponent(ns, shut);
    const double kOpen = fittedExponent(ns, open);
    std::printf("\n  fitted k (cost ~ n^k):  gate OPEN = %.3f   gate SHUT = %.3f\n",
                kOpen, kShut);
    std::printf("  A shut gate has an EMPTY answer. Its cost should not grow with\n"
                "  the population at all — the engine should say 'nobody' once,\n"
                "  not discover it one refusal per subject.\n\n");

    // The ratio is the honest number here, not either absolute: both arms are
    // measured back to back on the same machine, so shared load cancels. This
    // machine is often busy with a concurrent build.
    assert(kShut < kOpen + 0.40 &&
           "a shut gate now costs MORE than an open one, which is backwards");

    glfwDestroyWindow(window);
    glfwTerminate();
    std::printf("referent_resolution_test: OK\n");
    return 0;
}
