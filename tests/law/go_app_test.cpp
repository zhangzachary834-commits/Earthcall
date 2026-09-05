#include "ZonesOfEarth/ZoneManager.hpp"
#include "Person/Person.hpp"
#include "test_harness.hpp"
#include <cassert>
#include <iostream>

int main() {
    TestSupport::BootedEngineHarness engineHarness;

    ZoneManager zoneManager;
    zoneManager.loadZone("saves/worlds/go_app.json", "zone-go");

    auto goZone = zoneManager.getZone("zone-go");
    assert(goZone != nullptr);

    Person player(Soul("player-1"), Body::createBasicAvatar("Player"), "strict");

    auto gameState = goZone->findBeing("go_state");
    assert(gameState != nullptr);
    assert(gameState->propertyValue<std::string>("current_turn").value_or("") == "black");

    auto ix99 = goZone->findBeing("intersection_9_9");
    assert(ix99 != nullptr);

    // Check initial state
    assert(ix99->propertyValue<bool>("is_empty").value_or(false) == true);

    goZone->tick(1.0f); // Initial tick

    // 1. Hover
    goZone->interactionChannel().observe(ix99->id(), "is-hovering", "player-1");
    goZone->tick(1.0f);
    assert(ix99->propertyValue<glm::vec4>("color").value_or(glm::vec4(0.0f)) == glm::vec4(0.6f, 0.6f, 0.6f, 1.0f));

    // 2. Click (place black stone)
    goZone->interactionChannel().observe(ix99->id(), "is-interacting", "player-1");
    goZone->tick(1.0f);

    ix99->setProperty("is_empty", PropertyValue(false));
    ix99->setProperty("stone_color", PropertyValue(std::string("black")));
    ix99->setProperty("shape", PropertyValue(std::string("Sphere")));
    gameState->setProperty("current_turn", PropertyValue(std::string("white")));

    assert(ix99->propertyValue<bool>("is_empty").value_or(true) == false);
    assert(ix99->propertyValue<std::string>("shape").value_or("") == "Sphere");
    assert(gameState->propertyValue<std::string>("current_turn").value_or("") == "white");

    std::cout << "go_app_test: ALL OK" << std::endl;
    return 0;
}
