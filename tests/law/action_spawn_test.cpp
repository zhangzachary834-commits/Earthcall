#include <cassert>
#include <iostream>
#include <cmath>
#include <GLFW/glfw3.h>
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp"
#include "ConstructedBeing/Singular/Object/Creation/ObjectConcept.hpp"
#include "Person/Person.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"

// Globals are now in globals.o

// Removed dummy lawGetValue as it is in MathBinding.hpp

int main() {
    std::cout << "Running ActionModel Spawn Test..." << std::endl;

    // This is a Law/OntoMath semantic witness, not a rendering witness.
    // A macOS CI runner may have no window server; keep a GL context when
    // available, but never skip the spawn assertions merely because GLFW is
    // unavailable.
    const bool glfwReady = glfwInit() == GLFW_TRUE;
    GLFWwindow* window = nullptr;
    if (glfwReady) {
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        window = glfwCreateWindow(64, 64, "action_spawn_test", nullptr, nullptr);
        if (window) glfwMakeContextCurrent(window);
    }

    // 1. Create a dummy concept
    auto concept = std::make_shared<ObjectConcept>("test-concept");
    ObjectConcept::MemberTemplate mt;
    mt.kind = Object::ShapeKind::Cube;
    mt.relativeTransform = glm::mat4(1.0f);
    concept->members().push_back(mt);
    ConceptRegistry::instance().add(concept);

    // 2. Create world and player
    Zone world("test-zone", "default");
    Zone inactiveWorld("World", "default");
    Object player;
    player.setPosition(glm::vec3(2.5f, -1.25f, 7.0f));
    Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
        beings = {&world, &player, &inactiveWorld};
    });

    // 3. Create a Spawn ActionNode
    ActionNode node;
    node.kind = ActionNode::Kind::Spawn;
    // The registry keys by IDENTIFIER ("concept-N"), not by the display
    // name passed to the constructor — Spawn resolves the same way.
    node.conceptId = concept->getIdentifier();

    // Compile it
    auto executor = node.compile();

    // Execute it
    ECA::Event event{"onMouseClicked", &player, nullptr, 0};
    executor(event, world);

    // Assert both the OntoMath-authored placement and canonical Zone routing.
    assert(world.getOwnedObjects().size() == 1);
    Object* born = world.getOwnedObjects().front().get();
    assert(born);
    assert(glm::length(born->getPosition() - player.getPosition()) < 1e-4f);

    executor(event, player);
    assert(world.getOwnedObjects().size() == 2);
    assert(inactiveWorld.getOwnedObjects().empty());
    executor(event, inactiveWorld);
    assert(inactiveWorld.getOwnedObjects().size() == 1);
    Universe::instance().setProvider(nullptr);

    std::cout << "SUCCESS! Spawn preserves OntoMath placement and Zone destinations." << std::endl;

    if (window) glfwDestroyWindow(window);
    if (glfwReady) glfwTerminate();
    return 0;
}
