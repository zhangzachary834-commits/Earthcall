#include <cassert>
#include <iostream>
#include "ZonesOfEarth/World/World.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp"
#include "Form/Object/Creation/ObjectConcept.hpp"
#include "Person/Person.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"

// Globals are now in globals.o

// Removed dummy lawGetValue as it is in MathBinding.hpp

int main() {
    std::cout << "Running ActionModel Spawn Test..." << std::endl;
    
    // 1. Create a dummy concept
    auto concept = std::make_shared<ObjectConcept>("test-concept");
    ObjectConcept::MemberTemplate mt;
    mt.kind = Object::ShapeKind::Cube;
    mt.relativeTransform = glm::mat4(1.0f);
    concept->members().push_back(mt);
    ConceptRegistry::instance().add(concept);
    
    // 2. Create world and player
    World world;
    Object player;
    
    // 3. Create a Spawn ActionNode.
    //    Spawn resolves through ConceptRegistry::find, which keys on the
    //    concept's IDENTIFIER ("concept-N", minted in the constructor) — not on
    //    the display name passed to ObjectConcept(). Asking for the name here is
    //    what made this test spawn nothing.
    ActionNode node;
    node.kind = ActionNode::Kind::Spawn;
    node.conceptId = concept->getIdentifier();
    
    // Compile it
    auto executor = node.compile();
    
    // Execute it
    ECA::Event event{"onMouseClicked", &player, nullptr, 0};
    executor(event, world);
    
    // Assert it worked
    assert(world.getOwnedObjects().size() == 1);
    std::cout << "SUCCESS! 1 object spawned into the world." << std::endl;
    
    return 0;
}
