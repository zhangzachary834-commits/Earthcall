// Creation, composition, and their counterparts as ACTION NODES.
//
// Spawn instantiates a remembered thing (an ObjectConcept). These six let a
// law author a being nobody captured for it, grant and revoke its properties,
// compose it out of other beings, and unmake it:
//
//   Create / Destroy            being and unbeing
//   AddProperty / RemoveProperty  the vocabulary a Person adds
//   AddElement / RemoveElement    what a thing is made of
//
// Run: make test-creation

#include "ConstructedBeing/Object/Object.hpp"
#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"

#include <GLFW/glfw3.h>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace {

int g_checks = 0;
void check(bool condition, const std::string& what) {
    ++g_checks;
    if (!condition) {
        std::cout << "  FAILED: " << what << std::endl;
        assert(false);
    }
    std::cout << "  ok: " << what << std::endl;
}

Object* findByType(Zone& world, const std::string& type) {
    for (const auto& obj : world.getOwnedObjects()) {
        if (obj && obj->getObjectType() == type) return obj.get();
    }
    return nullptr;
}

} // namespace

int main() {
    std::cout << "Running Law Creation Test..." << std::endl;

    // Constructing an Object uploads its face textures, which needs a live GL
    // context (Object::initFaceTextures -> glGenTextures). Same hidden-window
    // bootstrap the render tests use.
    if (!glfwInit()) {
        std::cout << "law_creation_test: glfwInit failed" << std::endl;
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "law_creation_test", nullptr, nullptr);
    if (!window) {
        std::cout << "law_creation_test: window creation failed" << std::endl;
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    Zone world("test-zone", "default");
    Object subject;
    subject.setObjectType("subject");
    subject.setPosition(glm::vec3(2.0f, 3.0f, 4.0f));

    // The Universe is what @-paths and world resolution range over.
    Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
        beings.push_back(&world);
        beings.push_back(&subject);
        for (const auto& obj : world.getOwnedObjects()) {
            if (obj) beings.push_back(obj.get());
        }
    });

    const ECA::Event event{"test", &subject, nullptr, 0};

    // ------------------------------------------------------------------
    // 1. Create — a generic Object, no concept required.
    // ------------------------------------------------------------------
    std::cout << "\n[1] Create" << std::endl;
    {
        ActionNode create = ActionNode::create(
            static_cast<int>(Object::ShapeKind::Sphere), "lantern");
        create.compile()(event, subject);

        check(world.getOwnedObjects().size() == 1, "one object was born");
        Object* born = findByType(world, "lantern");
        check(born != nullptr, "the newborn carries its authored type");
        check(born && born->getShapeKind() == Object::ShapeKind::Sphere,
              "the newborn has the authored shape kind");
        // No placement path authored: the newborn appears where its subject is.
        check(born && glm::length(born->getPosition() - subject.getPosition()) < 1e-4f,
              "an unplaced newborn stands where the subject stands");
        check(born && !born->getIdentifier().empty(),
              "the newborn has an identity of its own");
    }

    // ------------------------------------------------------------------
    // 2. Create with children — the newborn is the children's SUBJECT, so
    //    the whole action vocabulary shapes it at birth.
    // ------------------------------------------------------------------
    std::cout << "\n[2] Create with children" << std::endl;
    {
        ActionNode create = ActionNode::create(
            static_cast<int>(Object::ShapeKind::Cube), "beacon",
            {ActionNode::set("position.y", PropertyValue(9.0)),
             ActionNode::addProperty("", "mood", PropertyValue(std::string("bright")))});
        create.compile()(event, subject);

        Object* beacon = findByType(world, "beacon");
        check(beacon != nullptr, "the second newborn exists");
        check(beacon && std::abs(beacon->getPosition().y - 9.0f) < 1e-4f,
              "a child Set shaped the newborn, not the subject");
        check(std::abs(subject.getPosition().y - 3.0f) < 1e-4f,
              "the subject was left alone");
        PropertyValue mood;
        check(beacon && beacon->getDynamicProperty("mood", mood) &&
                  std::get<std::string>(mood) == "bright",
              "a child AddProperty granted the newborn a property at birth");
    }

    // ------------------------------------------------------------------
    // 3. AddProperty / RemoveProperty — the vocabulary a law grants.
    // ------------------------------------------------------------------
    std::cout << "\n[3] AddProperty / RemoveProperty" << std::endl;
    {
        ActionNode::addProperty("", "warmth", PropertyValue(0.75)).compile()(event, subject);
        PropertyValue warmth;
        check(subject.getDynamicProperty("warmth", warmth), "the property was granted");
        check(std::get<double>(warmth) == 0.75, "it was granted its opening value");

        // A granted property is reachable through the ordinary path vocabulary,
        // which is what makes it a real part of the being.
        PropertyValue readBack;
        check(PropertyPath::parse("warmth").getValue(subject, readBack) == PropertyPath::PathResult::Ok,
              "the granted property reads through PropertyPath");

        // Shadowing a first-mover property is refused: a path that read one
        // value and wrote another would be a trap.
        ActionNode::addProperty("", "position", PropertyValue(0.0)).compile()(event, subject);
        check(!subject.hasDynamicProperty("position"),
              "granting a name the engine already owns is refused");

        // Revoking an authored property erases it outright.
        ActionNode::removeProperty("", "warmth").compile()(event, subject);
        check(!subject.hasDynamicProperty("warmth"), "the granted property was revoked");

        // A first-mover property is a C++ member: the slot cannot be erased,
        // so it is CLEARED instead. Honest either way.
        ActionNode::removeProperty("", "position").compile()(event, subject);
        check(glm::length(subject.getPosition()) < 1e-4f,
              "a first-mover property clears to empty rather than vanishing");
        check(subject.findProperty("position") != nullptr,
              "...and its slot still exists");
    }

    // ------------------------------------------------------------------
    // 4. AddElement / RemoveElement — composition.
    // ------------------------------------------------------------------
    std::cout << "\n[4] AddElement / RemoveElement" << std::endl;
    {
        Object* lantern = findByType(world, "lantern");
        Object* beacon = findByType(world, "beacon");
        check(lantern && beacon, "both newborns are still present");

        ActionNode::addElement(lantern->getIdentifier(), beacon->getIdentifier())
            .compile()(event, subject);
        check(lantern->elementCount() == 1, "the container gained an element");
        check(lantern->hasElement(beacon), "...and it is the being we named");
        check(beacon->elementCount() == 0, "composition is directed, not mutual");

        // "" means the law's subject, in both roles.
        ActionNode::addElement("", lantern->getIdentifier()).compile()(event, subject);
        check(subject.hasElement(lantern), "an empty container token means the subject");

        ActionNode::removeElement(lantern->getIdentifier(), beacon->getIdentifier())
            .compile()(event, subject);
        check(lantern->elementCount() == 0, "the element was released");
        check(findByType(world, "beacon") != nullptr,
              "releasing an element does not destroy it");
    }

    // ------------------------------------------------------------------
    // 5. Destroy — and the dangling-pointer guarantee that makes it safe.
    // ------------------------------------------------------------------
    std::cout << "\n[5] Destroy" << std::endl;
    {
        Object* lantern = findByType(world, "lantern");
        Object* beacon = findByType(world, "beacon");
        lantern->addElement(beacon);
        check(lantern->hasElement(beacon), "the lantern holds the beacon");

        bool heardIt = false;
        Core::EventBus::instance().subscribe<ECA::Event>(
            [&heardIt](const ECA::Event& e) {
                if (e.type == "object-destroyed") heardIt = true;
            });

        const std::size_t before = world.getOwnedObjects().size();
        ActionNode::destroy(beacon->getIdentifier()).compile()(event, subject);

        // Unmaking is DEFERRED: the action asks, and the reaper frees once no
        // pointer to the victim is still live. Destroying a being mid-action
        // used to free it under its own law's feet — applyTo goes on to write
        // the record and publish the applied event through that reference.
        check(world.getOwnedObjects().size() == before,
              "the ask alone frees nothing — the caller's pointers stay valid");
        check(Universe::instance().isUnmade(beacon),
              "...but it is already gone as far as law is concerned");
        for (Singular* being : Universe::instance().beings()) {
            check(being != beacon, "no sweep can reach a being awaiting the reaper");
        }

        reapUnmadeBeings();   // LawManager::tick() does this at the end of a tick

        check(world.getOwnedObjects().size() == before - 1, "the object is gone");
        check(findByType(world, "beacon") == nullptr, "...by identity");
        check(heardIt, "the unmaking was announced while the being still existed");
        check(lantern->elementCount() == 0,
              "every composition that held it released it first");
    }

    // ------------------------------------------------------------------
    // 6. JSON round-trip — creation is law TEXT, so it must survive a save.
    // ------------------------------------------------------------------
    std::cout << "\n[6] JSON round-trip" << std::endl;
    {
        ActionNode original = ActionNode::create(
            static_cast<int>(Object::ShapeKind::Torus), "ring",
            {ActionNode::addProperty("", "glow", PropertyValue(1.5)),
             ActionNode::addElement("", "@event.object")});
        ActionNode restored = ActionNode::fromJson(original.toJson());

        check(restored.kind == ActionNode::Kind::Create, "kind survives");
        check(restored.createShapeKind == original.createShapeKind, "shape kind survives");
        check(restored.createType == "ring", "authored type survives");
        check(restored.children.size() == 2, "children survive");
        check(restored.children[0].kind == ActionNode::Kind::AddProperty &&
                  restored.children[0].propertyName == "glow",
              "a granted property's name survives");
        check(restored.children[1].elementToken == "@event.object",
              "participant tokens survive");

        for (ActionNode n : {ActionNode::removeProperty("", "glow"),
                             ActionNode::removeElement("a", "b"),
                             ActionNode::destroy("@event.object")}) {
            ActionNode back = ActionNode::fromJson(n.toJson());
            check(back.kind == n.kind && back.describe() == n.describe(),
                  std::string("round-trip: ") + ActionNode::kindName(n.kind));
        }
    }

    std::cout << "\nSUCCESS — " << g_checks << " checks passed." << std::endl;
    Universe::instance().setProvider(nullptr);   // the world outlives this scope
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
