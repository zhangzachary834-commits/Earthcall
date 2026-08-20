// Persistence milestone test.
//
// A saved world keeps its covenant, not just its furniture: laws round-trip
// through LawManager::toJson/loadFromJson with their models, scope, drives,
// and TRIGGER bindings, and their authors and targets reattach as world
// references BY IDENTIFIER. An author who is not in the reloaded world
// leaves the law Unauthored — it cannot fire, and that is the point:
// authorship is a covenant, not a copy. Concepts round-trip through
// ConceptRegistry the same way.

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ConstructedBeing/Object/Creation/ObjectConcept.hpp"
#include "ConstructedBeing/Object/Object.hpp"
#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Singularity/Input/Locomotion/LocomotionChannel.hpp"
#include "Singularity/Screen/Camera.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"

#include <GLFW/glfw3.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace {
bool nearf(float a, float b, float eps = 1e-4f) { return std::fabs(a - b) < eps; }
}

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "law_persistence_test: glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "law_persistence_test", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "law_persistence_test: no GL context\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    {
        Object author;                 // in the world: reattaches
        Object authorGhost;            // NOT in the world after "reload"
        Object a, b;
        a.setPosition(glm::vec3(0.0f, 5.0f, 0.0f));
        b.setPosition(glm::vec3(3.0f, 7.0f, 0.0f));

        Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
            beings.push_back(&author);   // authorGhost deliberately absent
            beings.push_back(&a);
            beings.push_back(&b);
        });
        Universe::instance().setClock(50.0, 0.1);

        // ------------------------------------------------------------------
        // 1. Author a small register in one manager...
        // ------------------------------------------------------------------
        nlohmann::json saved;
        std::string raiseId, ghostId;
        {
            LawManager before;

            auto raise = before.createLaw("raise-all-on-boom", {&author});
            raise->setScope(Law::Scope::Everyone);
            raise->setDrives(true);   // the flag itself must survive
            raise->setConditionModel(ConditionNode::compare(
                "position.y", ConditionNode::Op::Lt, PropertyValue(100.0)));
            raise->setActionModel(ActionNode::set("position.y", PropertyValue(42.0)));
            // (no targets: the Everyone sweep ranges over the whole Universe)
            before.bindTrigger(raise->getIdentifier(), "boom");
            before.bindTrigger(raise->getIdentifier(), "second-signal");
            raiseId = raise->getIdentifier();

            auto ghost = before.createLaw("orphaned-law", {&authorGhost});
            ghost->setActionModel(ActionNode::set("position.y", PropertyValue(-9.0)));
            ghost->addTarget(b);   // target reattachment rides on this law
            before.bindTrigger(ghost->getIdentifier(), "boom");
            ghostId = ghost->getIdentifier();

            saved = before.toJson();
        }   // the authoring manager dies with its session

        // ------------------------------------------------------------------
        // 2. ...and reload it into a fresh one: the world remembers.
        // ------------------------------------------------------------------
        LawManager after;
        after.loadFromJson(saved);

        Law* raise = after.find(raiseId);
        assert(raise != nullptr);
        assert(raise->isAuthored());                       // author reattached by id
        assert(raise->scope() == Law::Scope::Everyone);
        assert(raise->drives());
        assert(after.triggersOf(raiseId).size() == 2);     // bindings survived
        assert(after.triggersOf(raiseId)[0] == "boom");

        // The orphan: its author is not in this world. It stays Unauthored —
        // visible, editable, but unable to fire. Never a silent forgery.
        // (Its target still reattached: references are restored either way.)
        Law* ghost = after.find(ghostId);
        assert(ghost != nullptr);
        assert(!ghost->isAuthored());
        assert(ghost->targets().getMembers().size() == 1);
        assert(ghost->targets().getMembers()[0]->getIdentifier() == b.getIdentifier());

        // ------------------------------------------------------------------
        // 3. The reloaded register actually LIVES: the trigger wakes the law,
        //    the sweep raises everyone; the orphan does nothing.
        // ------------------------------------------------------------------
        after.connectToEventBus();
        Core::EventBus::instance().publish(
            ECA::Event{"boom", &a, nullptr, std::time(nullptr)});
        after.tick();
        assert(nearf(a.getPosition().y, 42.0f));           // swept (Everyone scope)
        assert(nearf(b.getPosition().y, 42.0f));
        // Had the orphan fired, y would be -9.
        assert(!nearf(a.getPosition().y, -9.0f));

        // ------------------------------------------------------------------
        // 4. Concepts round-trip through the registry the same way.
        // ------------------------------------------------------------------
        auto concept = ObjectConcept::captureFrom({&a}, "seed-cube", &author);
        assert(concept != nullptr);
        const std::string conceptId = concept->getIdentifier();
        ConceptRegistry::instance().add(concept);
        const nlohmann::json conceptsSaved = ConceptRegistry::instance().toJson();

        ConceptRegistry::instance().loadFromJson(nlohmann::json::object());
        assert(ConceptRegistry::instance().find(conceptId) == nullptr);   // gone

        ConceptRegistry::instance().loadFromJson(conceptsSaved);
        auto reborn = ConceptRegistry::instance().find(conceptId);
        assert(reborn != nullptr);                          // restored, same id
        assert(reborn->name() == "seed-cube");

        ConceptRegistry::instance().loadFromJson(nlohmann::json::object());

        // ------------------------------------------------------------------
        // 5. MULTIPLE laws round-trip without blurring: each keeps its own
        //    name, text, and triggers.
        // ------------------------------------------------------------------
        LawManager multi;
        auto first = multi.createLaw("first", {&author});
        first->setActionModel(ActionNode::set("position.y", PropertyValue(1.0)));
        multi.bindTrigger(first->getIdentifier(), "event-one");
        auto second = multi.createLaw("second", {&author});
        second->setActionModel(ActionNode::set("position.y", PropertyValue(2.0)));
        second->setActivation(Law::Activation::WhileTrue);
        multi.bindTrigger(second->getIdentifier(), "event-two");
        multi.bindTrigger(second->getIdentifier(), "event-three");
        auto third = multi.createLaw("third", {&author});
        third->setConditionModel(ConditionNode::compare(
            "position.y", ConditionNode::Op::Gt, PropertyValue(7.0)));

        LawManager remulti;
        remulti.loadFromJson(multi.toJson());
        assert(remulti.getAll().size() == 3);
        Law* re1 = remulti.find(first->getIdentifier());
        Law* re2 = remulti.find(second->getIdentifier());
        Law* re3 = remulti.find(third->getIdentifier());
        assert(re1 && re2 && re3);
        assert(re1->name() == "first" && re2->name() == "second" && re3->name() == "third");
        assert(re2->activation() == Law::Activation::WhileTrue);
        assert(re1->activation() == Law::Activation::OnEvent);
        assert(remulti.triggersOf(re1->getIdentifier()).size() == 1);
        assert(remulti.triggersOf(re2->getIdentifier()).size() == 2);
        assert(remulti.triggersOf(re3->getIdentifier()).empty());
        assert(!re1->hasConditionModel() && re3->hasConditionModel());

        // ------------------------------------------------------------------
        // 6. Fresh ids stay fresh after loads: a restored identity advances
        //    the counter, so a new law can never collide with a loaded one
        //    (collision = LawManager::add silently discards the newcomer).
        // ------------------------------------------------------------------
        nlohmann::json tallJson = first->toJson();
        tallJson["id"] = "law-99999";
        auto tall = Law::fromJson(tallJson);
        assert(tall->getIdentifier() == "law-99999");
        Law freshAfter("born-after-the-tall-id");
        assert(freshAfter.getIdentifier() != "law-99999");
        assert(std::strtoull(freshAfter.getIdentifier().c_str() + 4, nullptr, 10) >
               99999ULL);

        Object tallObject;
        tallObject.setObjectID(std::string("object-88888"));
        Object bornAfter;
        assert(std::strtoull(bornAfter.getIdentifier().c_str() + 7, nullptr, 10) >
               88888ULL);

        // ------------------------------------------------------------------
        // 7. The application log is a window, not an infinite ledger.
        // ------------------------------------------------------------------
        for (int i = 0; i < 400; ++i) {
            re1->applyTo(a);
        }
        assert(re1->applicationLog().size() <= 256);
        assert(re1->toJson()["applicationLog"].size() <= 16);

        // ------------------------------------------------------------------
        // 8. First movers can be set down. Their C++ stays in the engine;
        //    only the enabled bit is world state, and actuation honors it.
        // ------------------------------------------------------------------
        {
            LawManager movers;
            Singularity::Input::LocomotionChannel::syncRegister(movers);
            auto* loco = Singularity::Input::LocomotionChannel::find(movers);
            assert(loco);
            assert(loco->isFirstMover());
            assert(loco->isEnabled());   // bootstrap default is on

            PropertyValue enabledVal;
            assert(PropertyPath::parse("enabled").getValue(*loco, enabledVal)
                   == PropertyPath::PathResult::Ok);
            assert(std::get<bool>(enabledVal) == true);
            assert(PropertyPath::parse("enabled").setValue(*loco, PropertyValue(false))
                   == PropertyPath::PathResult::Ok);
            assert(!loco->isEnabled());

            Soul soul("walker");
            Body body = Body::createBasicAvatar("Voxel");
            Person person(std::move(soul), std::move(body), "default");
            person.position = glm::vec3(3.0f, 4.0f, 5.0f);
            ::Core::Camera camera;
            ZoneManager zones;
            loco->step(person, camera, window, zones, 0.016f, false, true);
            assert(nearf(person.position.x, 3.0f) && nearf(person.position.y, 4.0f)
                   && nearf(person.position.z, 5.0f));

            nlohmann::json savedMovers = movers.toJson();
            assert(savedMovers.contains("firstMoverEnabled"));
            assert(savedMovers["firstMoverEnabled"]["locomotion-channel"] == false);
            for (const auto& lawJson : savedMovers["laws"]) {
                assert(lawJson.value("id", "") != "locomotion-channel");
            }

            LawManager reloaded;
            Singularity::Input::LocomotionChannel::syncRegister(reloaded);
            assert(Singularity::Input::LocomotionChannel::find(reloaded)->isEnabled());
            reloaded.loadFromJson(savedMovers);
            auto* restored = Singularity::Input::LocomotionChannel::find(reloaded);
            assert(restored);
            assert(!restored->isEnabled());
        }

        Universe::instance().setProvider({});               // leave no dangling refs
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    std::puts("law_persistence_test: ALL OK");
    return 0;
}
