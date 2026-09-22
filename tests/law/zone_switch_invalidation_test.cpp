// Walking into another Zone must not leave a law looking at the room you left.
//
// FORMATION_RETE.md §8 rung 2; docs/Analysis/DERIVED_STATE_AND_THE_SILENCE_OF_LAWS §7
// ("a derived structure whose invalidation nobody declared is unfalsifiable").
//
// THE BUG. EngineInit installs the Universe being-provider ONCE, and it reads
// `mgr.active()` on every call. So `ZoneManager::switchTo` replaces the entire set of
// beings in front of the Person just by setting `_currentIndex` — it calls no setter,
// adds and removes no object, and (until this test's fix) moved no revision.
//
// The vocabulary index that narrows the sweep (rung 2) rebuilds only when
// `Universe::structuralRevision()` moves or the set of required property names
// changes. Switch between two Zones whose laws share vocabulary and neither moves:
// the index keeps offering the PREVIOUS Zone's beings, and a sweep-path law silently
// stops reaching anyone in the Zone the Person is actually standing in.
// Measured before the fix: the law reached the first Zone, and after switchTo(1) did
// not reach the second at all; bumping the existing structural revision restored it.
//
// WHY THE EXISTING COUNTER AND NOT A NEW ONE. `structuralRevision` already means "the
// shape of the world changed — beings appeared or vanished". A Zone switch is exactly
// that, all at once. Zach asked, while this was being built, whether a separate
// change system was needed; it is not, and one was started and reverted. Relation
// mutations are likewise already announced (`relation-formed`, `relation-destroyed`)
// and zone switches already publish `zone-entered`. Extend those; do not build beside
// them.
//
// THE PATH THIS COVERS. A law with no compiled Rete terminals takes the sweep path:
// any law on a disconnected LawManager, and on a connected one any law whose
// conditions do not compile (quantifiers, qualified roots, some Related shapes). The
// manager here is disconnected so the sweep path is certain rather than incidental.
//
// FOR FUTURE AGENTS (Jules especially): any NEW way of changing which beings are in
// front of the Person — another activation path, streamed chunks, a merged or
// hot-reloaded Zone — must call `Universe::instance().bumpStructuralRevision()`. Add a
// section here that goes through your path. The failure is silent: no crash, no log
// line, a law that stops working when someone changes rooms.
// To-do: docs/Agenda/Tasks/Specific Tasks/Formation_Rete/Formation_Rete.md

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"

#include <GLFW/glfw3.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>

namespace {

bool nearf(double a, double b, double eps = 1e-4) { return std::fabs(a - b) < eps; }

double filletOf(Object& obj) {
    PropertyValue v;
    if (PropertyPath::parse("shape.fillet").getValue(obj, v) != PropertyPath::PathResult::Ok) {
        return -12345.0;
    }
    double out = -12345.0;
    propertyValueToNumber(v, out);
    return out;
}

std::shared_ptr<Object> beaconBeing(const std::string& id) {
    auto obj = std::make_shared<Object>();
    obj->setObjectID(id);
    PropertyPath::parse("shape.fillet").setValue(*obj, PropertyValue(0.0f));
    obj->setDynamicProperty("beacon", PropertyValue(1.0));
    return obj;
}

} // namespace

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "zone_switch_invalidation_test: glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "zone_switch_invalidation_test", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "zone_switch_invalidation_test: no GL context\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    {
        ZoneManager zones;
        auto first = std::make_shared<Zone>("switch_test_first", "strict");
        auto second = std::make_shared<Zone>("switch_test_second", "strict");
        zones.addZone(first);
        zones.addZone(second);

        // Placed the way the ENGINE places objects, not with Zone::addObject.
        // switchTo clears the active Zone's object list and rebuilds it from
        // ZoneManager::globalObjects, keeping whatever belongsToZone — so an
        // object added straight to a Zone is wiped on the first switch. (That
        // rebuild also writes the list directly, bypassing addObject and
        // removeObject: a second reason no revision used to move.)
        auto inFirst = beaconBeing("switch-test-in-first");
        auto inSecond = beaconBeing("switch-test-in-second");
        inFirst->addZoneDesignation(first->getIdentifier());
        inSecond->addZoneDesignation(second->getIdentifier());
        zones.getGlobalObjects().push_back(inFirst);
        zones.getGlobalObjects().push_back(inSecond);

        Object author;

        // Read LIVE from the active Zone, exactly as EngineInit's provider does.
        Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
            beings.push_back(&author);
            for (const auto& obj : zones.active().objects()) {
                if (obj) beings.push_back(obj.get());
            }
        });
        Universe::instance().setClock(100.0, 0.1);

        LawManager mgr;   // disconnected on purpose: the sweep path, certainly

        auto law = mgr.createLaw("beacon-watch", {&author});
        law->setActivation(Law::Activation::WhileTrue);
        law->setConditionModel(ConditionNode::compare(
            "beacon", ConditionNode::Op::Gt, PropertyValue(0.0)));
        law->setActionModel(ActionNode::set("shape.fillet", PropertyValue(0.5f)));

        // --------------------------------------------------------------
        // A. Standing in the first Zone.
        // --------------------------------------------------------------
        // Enter the first Zone through switchTo too, so its object list is built
        // from globalObjects by the same code every later switch uses.
        assert(zones.switchTo(0) && zones.active().name() == "switch_test_first");
        mgr.tick();
        assert(nearf(filletOf(*inFirst), 0.5) && "the law must reach the Zone the Person is in");
        assert(nearf(filletOf(*inSecond), 0.0) && "a Zone nobody is standing in is not reached");

        // --------------------------------------------------------------
        // B. THE SWITCH, through the real chokepoint. Both Zones' objects were
        //    added long ago, and the law's vocabulary is unchanged — the two
        //    things the index watched before switchTo announced itself.
        // --------------------------------------------------------------
        const auto revisionBefore = Universe::instance().structuralRevision();
        assert(zones.switchTo(1) && zones.active().name() == "switch_test_second");
        assert(Universe::instance().structuralRevision() != revisionBefore &&
               "switching Zones replaces every being in front of the Person and must say so");
        mgr.tick();
        assert(nearf(filletOf(*inSecond), 0.5) &&
               "after walking into a Zone, its beings must be reachable by law");

        // --------------------------------------------------------------
        // C. AND BACK. A signal that fired once and latched would pass B.
        // --------------------------------------------------------------
        PropertyPath::parse("shape.fillet").setValue(*inFirst, PropertyValue(0.0f));
        assert(zones.switchTo(0) && zones.active().name() == "switch_test_first");
        mgr.tick();
        assert(nearf(filletOf(*inFirst), 0.5) &&
               "returning to a Zone must reach its beings again, not the last Zone's");

        Universe::instance().setProvider(nullptr);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    std::printf("zone_switch_invalidation_test: OK\n");
    return 0;
}
