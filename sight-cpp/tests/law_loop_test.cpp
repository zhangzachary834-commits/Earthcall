// Law hearing-loop milestone test (LAW_AND_CREATION_SYSTEM.md, commit 4):
//   laws stop being invoked and start LISTENING.
//
// Exercises: EventBus re-entrant publish (the deadlock fix), Formation unique
// identity, ECA::Event → ReteFact → agenda → applyTo end-to-end, one-shot
// fact consumption, and law-chains-law within a tick bounded by
// kMaxChainRounds (the first anti-Babel ceiling in code).

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "Form/Object/Object.hpp"
#include "Form/Object/Formation/Formation.hpp"

#include <GLFW/glfw3.h>
#include <cassert>
#include <cmath>
#include <cstdio>

namespace {

bool nearf(float a, float b, float eps = 1e-4f) { return std::fabs(a - b) < eps; }

struct Ping {};
struct Pong { int depth = 0; };

} // namespace

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "law_loop_test: glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "law_loop_test", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "law_loop_test: no GL context\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    {
        // ------------------------------------------------------------------
        // 1. Re-entrant publish must not deadlock (the EventBus fix).
        //    A handler that publishes from inside handling used to relock the
        //    held mutex.
        // ------------------------------------------------------------------
        bool pongHeard = false;
        Core::EventBus::instance().subscribe<Ping>([](const Ping&) {
            Core::EventBus::instance().publish(Pong{1});
        });
        Core::EventBus::instance().subscribe<Pong>([&pongHeard](const Pong&) {
            pongHeard = true;
        });
        Core::EventBus::instance().publish(Ping{});
        assert(pongHeard);   // reaching here at all proves no deadlock

        // ------------------------------------------------------------------
        // 2. Formation identity: unique per instance; a copy is a new being.
        // ------------------------------------------------------------------
        Formation fa(Form::ShapeType::Cube, glm::vec3(1.0f));
        Formation fb(Form::ShapeType::Cube, glm::vec3(1.0f));
        assert(fa.getIdentifier() != "Formation");
        assert(fa.getIdentifier() != fb.getIdentifier());
        Formation fc(fa);
        assert(fc.getIdentifier() != fa.getIdentifier());   // copy mints identity

        // ------------------------------------------------------------------
        // 3. The loop: event → fact → rete → applyTo.
        // ------------------------------------------------------------------
        Object author;
        LawManager mgr;
        mgr.connectToEventBus();

        auto floorLaw = mgr.createLaw("ground-rest", {&author});
        floorLaw->setConditionModel(
            ConditionNode::compare("position.y", ConditionNode::Op::Lt, PropertyValue(0.0)));
        floorLaw->setActionModel(ActionNode::set("position.y", PropertyValue(0.0f)));

        const std::size_t alphaEnters = mgr.rete().addAlphaNode(
            "type == enters-world",
            [](const ReteFact& f) { return f.type == "enters-world"; });
        mgr.rete().bindLawToAlpha(floorLaw->getIdentifier(), alphaEnters);

        Object obj;
        obj.setPosition(glm::vec3(0.0f, -5.0f, 0.0f));

        // Nothing happens without an event: the law is registered but silent.
        assert(mgr.tick().empty());
        assert(nearf(obj.getPosition().y, -5.0f));

        // The event arrives; the law hears it and fires on its subject.
        Core::EventBus::instance().publish(
            ECA::Event{"enters-world", &obj, nullptr, std::time(nullptr)});
        auto records = mgr.tick();
        assert(!records.empty());
        assert(records.front().result == Law::ApplicationResult::Applied);
        assert(nearf(obj.getPosition().y, 0.0f));

        // ------------------------------------------------------------------
        // 4. Event facts are one-shot: consumed by the round that saw them.
        // ------------------------------------------------------------------
        obj.setPosition(glm::vec3(0.0f, -5.0f, 0.0f));
        assert(mgr.tick().empty());                    // no new event, no fire
        assert(nearf(obj.getPosition().y, -5.0f));

        // ------------------------------------------------------------------
        // 5. Laws chain on laws within one tick — bounded, never runaway.
        //    gildLaw binds to the "law-applied" echo floorLaw emits; its own
        //    applications echo too, so it would chain forever without
        //    kMaxChainRounds. The test completing IS the ceiling working.
        // ------------------------------------------------------------------
        auto gildLaw = mgr.createLaw("gild-what-law-touches", {&author});
        gildLaw->setActionModel(ActionNode::set("shape.fillet", PropertyValue(0.9f)));
        const std::size_t alphaApplied = mgr.rete().addAlphaNode(
            "type == law-applied",
            [](const ReteFact& f) { return f.type == "law-applied"; });
        mgr.rete().bindLawToAlpha(gildLaw->getIdentifier(), alphaApplied);

        Core::EventBus::instance().publish(
            ECA::Event{"enters-world", &obj, nullptr, std::time(nullptr)});
        records = mgr.tick();

        assert(nearf(obj.getPosition().y, 0.0f));      // floorLaw fired (round 1)
        PropertyValue fillet;
        assert(PropertyPath::parse("shape.fillet").getValue(obj, fillet));
        assert(nearf(std::get<float>(fillet), 0.9f));  // gildLaw chained (round 2+)
        assert(static_cast<int>(records.size()) <= LawManager::kMaxChainRounds + 1);

        // ------------------------------------------------------------------
        // 6. Unbinding: a law released from its trigger goes silent (the
        //    editor's "unbind" button is real, not cosmetic).
        // ------------------------------------------------------------------
        mgr.rete().unbindLaw(gildLaw->getIdentifier());
        assert(PropertyPath::parse("shape.fillet").setValue(obj, PropertyValue(0.0f)));
        obj.setPosition(glm::vec3(0.0f, -5.0f, 0.0f));
        Core::EventBus::instance().publish(
            ECA::Event{"enters-world", &obj, nullptr, std::time(nullptr)});
        mgr.tick();
        assert(nearf(obj.getPosition().y, 0.0f));      // floorLaw still bound
        assert(PropertyPath::parse("shape.fillet").getValue(obj, fillet));
        assert(nearf(std::get<float>(fillet), 0.0f));  // gildLaw no longer hears
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    std::puts("law_loop_test: ALL OK");
    return 0;
}
