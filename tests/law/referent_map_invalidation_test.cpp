// A law that names a being by "@name" must find it when that being arrives later.
//
// `resolveLawRoot` (`MathBinding.hpp`) caches every being's "@identifier" in a
// static map and rebuilds it when `Universe::structuralRevision()` moves. It is a
// derived structure like every other one in law/DERIVED_STATE_LEDGER.md, and
// until 2026-09-16 its SPEED was guarded (referent_resolution_test measures a
// shut gate against an open one) while its CURRENCY was not: that test builds its
// population before the first tick and never touches it again.
//
// What rides on the currency: every `@`-rooted path a Person can author.
// `@state.studio.voice`, `@world.occlusionToCamera`, a named gate. If the map
// does not rebuild when a being is admitted, the referent resolves to nothing,
// `lawGetValue` fails, and the law is silently inert — no error, no log, the law
// still enabled and authored. That is the same silence rungs 0, 2, 4 and 7 each
// found somewhere else.
//
// Lifetime regression: this test used to require ONE connected LawManager.
// A second manager in the same process caused the first manager's stale
// EventBus [this] handler to fire after destruction and segfault. The second
// block below is the deterministic witness that teardown now revokes those
// subscriptions.
//
// FOR FUTURE AGENTS (Jules especially): if you make the referent map key on
// something else, or cache resolution per law, this is the test that says whether
// a being admitted at runtime is still reachable by name.
// To-do: docs/Agenda/Tasks/To-do list.md — the derived-state ledger bullet.

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"

#include <GLFW/glfw3.h>
#include <cstdio>
#include <string>
#include <vector>

namespace {

int g_failures = 0;

void check(bool ok, const std::string& what) {
    if (!ok) { ++g_failures; std::printf("  FAILED: %s\n", what.c_str()); }
    else std::printf("  ok: %s\n", what.c_str());
}

double zOf(Object& o) {
    PropertyValue v;
    if (PropertyPath::parse("position.z").getValue(o, v) != PropertyPath::PathResult::Ok) return -1;
    double d = -1; propertyValueToNumber(v, d); return d;
}

} // namespace

int main() {
    if (!glfwInit()) { std::fprintf(stderr, "referent_map_invalidation_test: glfwInit failed\n"); return 1; }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "referent_map_invalidation_test", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);

    {
        Object author;
        Object subject;  subject.setObjectID("subject");
        PropertyPath::parse("position.z").setValue(subject, PropertyValue(0.0f));

        std::vector<Singular*> population{&author, &subject};
        Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
            for (Singular* being : population) beings.push_back(being);
        });
        Universe::instance().setClock(100.0, 0.1);

        LawManager mgr;
        mgr.connectToEventBus();

        auto law = mgr.createLaw("reads-a-late-gate", {&author});
        law->setActivation(Law::Activation::WhileTrue);
        law->setConditionModel(ConditionNode::comparePaths(
            "position.z", ConditionNode::Op::Lt, "@late-gate.ceiling"));
        law->setActionModel(ActionNode::add("position.z", 1.0));

        // The map is built in a world with no such gate in it.
        mgr.tick();
        mgr.tick();
        check(zOf(subject) == 0.0,
              "a law whose referent is not in the world does nothing (undefined, never guessed)");

        // The gate arrives, exactly as Zone admission brings a being in.
        Object gate;  gate.setObjectID("late-gate");
        gate.setDynamicProperty("ceiling", PropertyValue(100.0));
        population.push_back(&gate);
        Universe::instance().bumpStructuralRevision();

        mgr.tick();
        std::printf("  after the gate arrives: position.z = %.0f\n", zOf(subject));
        check(zOf(subject) > 0.0,
              "a being named by an @-path and admitted AFTER the referent map was built "
              "is still found by name");

        // And it keeps working as the world keeps moving.
        for (int i = 0; i < 3; ++i) mgr.tick();
        check(zOf(subject) >= 4.0, "and keeps being found on later ticks");

        Universe::instance().setProvider(nullptr);
    }

    // Warden regression: the first connected manager is gone. Creating a law
    // in a second connected manager publishes an ECA::Event. Before EventBus
    // subscriptions were revocable, that event also entered the dead first
    // manager through its captured `this` and this test segfaulted.
    {
        Object secondAuthor;
        LawManager secondManager;
        secondManager.connectToEventBus();
        auto secondLaw = secondManager.createLaw("second-manager-after-teardown", {&secondAuthor});
        check(secondLaw != nullptr,
              "a second connected LawManager may publish after the first manager is destroyed");
        Universe::instance().setProvider(nullptr);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    std::printf("%s\n", g_failures ? "referent_map_invalidation_test: FAILURES"
                                   : "referent_map_invalidation_test: OK");
    return g_failures ? 1 : 0;
}
