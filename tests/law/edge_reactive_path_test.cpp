// An edge fires once — on the REACTIVE path, where the running engine lives.
//
// CLAUDE.md non-negotiable: "Event-transitions must be edges, not levels... A
// per-frame 'still happening' event is a bug — that is what WhileTrue is for."
//
// WHY THIS TEST EXISTS SEPARATELY FROM rete_compile_test §C.
// That section already asserts "an edge fires once, not once per tick" — and it
// passed while this exact bug was live. Its OnBecomeTrue law is created DISABLED and
// enabled later, so it never gets compiled terminals, so it only ever exercises the
// SWEEP path, which was never broken. A guard that cannot reach the code it guards is
// not a guard. This one builds the law the way a running world does — connected,
// enabled, with a condition that compiles to Rete terminals — so it takes the
// reactive path.
//
// THE REGRESSION (2026-09-13, `698059e0 Rete performance sweep hunt`). The reactive
// path in LawManager::tick was widened from `WhileTrue` to `WhileTrue || OnBecomeTrue`.
// It correctly computed `newlyTrue` — the subjects that just went false->true — and
// then applied the law to `subjects`, EVERY matching subject, every tick. `newlyTrue`
// was built and never read. Measured before the fix, a connected OnBecomeTrue law whose
// condition held for 10 ticks fired 10 times; the same law disconnected fired once.
// EngineInit always connects, so every OnBecomeTrue law in the running app repeated
// every frame: `add` accumulated, spawn and create and publish and sound repeated at
// frame rate.
//
// THIS IS THE SECOND TIME. The same edge check was deleted once before (04c52ed4),
// also inside a commit about performance, and the tick carried a comment calling it
// "LOAD-BEARING and reads as redundant". The comment went; the bug came back.
//
// FOR FUTURE AGENTS (Jules especially): if you touch the continuous pass in
// LawManager::tick — merging the WhileTrue and OnBecomeTrue branches, adding a JIT or
// bytecode fast path, batching applications — this test must stay green. The rule is
// one line: WhileTrue applies to every subject that HOLDS; OnBecomeTrue applies only to
// subjects that JUST STARTED holding. If a path cannot tell those apart, OnBecomeTrue
// laws must not take it. Do not "simplify" the branch into a single loop over subjects.
// To-do: docs/Agenda/Tasks/Specific Tasks/Formation_Rete/Formation_Rete.md

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"

#include <GLFW/glfw3.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

namespace {

bool nearf(double a, double b, double eps = 1e-4) { return std::fabs(a - b) < eps; }

double counter(Object& obj) {
    PropertyValue v;
    if (PropertyPath::parse("position.z").getValue(obj, v) != PropertyPath::PathResult::Ok) {
        return -12345.0;
    }
    double out = -12345.0;
    propertyValueToNumber(v, out);
    return out;
}

void zero(Object& obj) {
    PropertyPath::parse("position.z").setValue(obj, PropertyValue(0.0f));
}

} // namespace

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "edge_reactive_path_test: glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "edge_reactive_path_test", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "edge_reactive_path_test: no GL context\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    {
        Object author;
        Object edgeSubject;    // watched by the OnBecomeTrue law
        Object levelSubject;   // watched by the WhileTrue law
        zero(edgeSubject);
        zero(levelSubject);
        edgeSubject.setDynamicProperty("armed", PropertyValue(1.0));
        levelSubject.setDynamicProperty("lit", PropertyValue(1.0));

        std::vector<Singular*> population{&author, &edgeSubject, &levelSubject};
        Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
            for (Singular* being : population) beings.push_back(being);
        });
        Universe::instance().setClock(100.0, 0.1);

        LawManager mgr;
        mgr.connectToEventBus();   // THE POINT: this is what EngineInit does

        // The action ACCUMULATES, so "fired once" and "fired every tick" produce
        // different numbers. An idempotent `set` would hide the bug completely.
        auto edge = mgr.createLaw("edge", {&author});
        edge->setActivation(Law::Activation::OnBecomeTrue);
        edge->setConditionModel(ConditionNode::compare(
            "armed", ConditionNode::Op::Gt, PropertyValue(0.0)));
        edge->setActionModel(ActionNode::add("position.z", 1.0));

        auto level = mgr.createLaw("level", {&author});
        level->setActivation(Law::Activation::WhileTrue);
        level->setConditionModel(ConditionNode::compare(
            "lit", ConditionNode::Op::Gt, PropertyValue(0.0)));
        level->setActionModel(ActionNode::add("position.z", 1.0));

        // --------------------------------------------------------------
        // A. HELD FOR TEN TICKS. The edge fires exactly once.
        // --------------------------------------------------------------
        for (int i = 0; i < 10; ++i) mgr.tick();
        std::printf("  OnBecomeTrue held 10 ticks -> fired %.0f   (must be 1)\n", counter(edgeSubject));
        std::printf("  WhileTrue    held 10 ticks -> fired %.0f   (must be 10)\n", counter(levelSubject));
        assert(nearf(counter(edgeSubject), 1.0) &&
               "OnBecomeTrue re-fired while its condition kept holding: a level wearing an edge");

        // --------------------------------------------------------------
        // B. THE FIX MUST NOT BREAK LEVELS. WhileTrue applies every tick it
        //    holds — that is its whole meaning, and the reactive path exists
        //    for it. Narrowing both branches to `newlyTrue` would silence it.
        // --------------------------------------------------------------
        assert(nearf(counter(levelSubject), 10.0) &&
               "WhileTrue must apply on every tick its condition holds");

        // --------------------------------------------------------------
        // C. RELEASE AND RE-HOLD: the edge re-arms and fires ONCE more.
        //    A fix that fired once per law lifetime would pass A and fail here.
        // --------------------------------------------------------------
        edgeSubject.setDynamicProperty("armed", PropertyValue(0.0));
        mgr.tick();
        mgr.tick();
        assert(nearf(counter(edgeSubject), 1.0) && "a released edge must not fire");
        edgeSubject.setDynamicProperty("armed", PropertyValue(1.0));
        for (int i = 0; i < 5; ++i) mgr.tick();
        assert(nearf(counter(edgeSubject), 2.0) &&
               "a re-held edge fires exactly once more, not once per tick");

        Universe::instance().setProvider(nullptr);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    std::printf("edge_reactive_path_test: OK\n");
    return 0;
}
