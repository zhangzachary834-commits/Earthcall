#pragma once

#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "Singularity/Screen/Camera.hpp"
#include "Singularity/Input/MouseHandler.hpp"
#include "OurVerse/Tool.hpp"
#include "Person/Person.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include <string>
#include <filesystem>
#include <iostream>

inline void dump_test_save(const std::string& test_name, World& testWorld, LawManager& testLawManager, Person& testPlayer) {
    std::cout << "[TestSaveHelper] Generating test save: " << test_name << "...\n";

    ZoneManager mgr;
    auto zone = std::make_shared<Zone>(test_name, "test");
    zone->setOwner(testPlayer.getIdentifier());

    // Temporarily move objects from testWorld to the zone's world for serialization
    for (auto& obj : testWorld.getOwnedObjectsMutable()) {
        if (obj) {
            zone->world().addObject(std::move(obj));
        }
    }
    testWorld.getOwnedObjectsMutable().clear();
    
    mgr.addZone(zone);

    Core::Camera camera;
    camera.pos = testPlayer.cameraPos;
    camera.front = testPlayer.cameraForward;
    
    MouseHandler mouseHandler;
    float currentColor[3] = {1.0f, 1.0f, 1.0f};
    Tool currentTool(Tool::Type::Brush);
    double worldTime = 0.0;

    SaveContext ctx;
    ctx.camera = &camera;
    ctx.mouseHandler = &mouseHandler;
    ctx.currentColor = currentColor;
    ctx.currentTool = &currentTool;
    ctx.player = &testPlayer;
    ctx.lawManager = &testLawManager;
    ctx.worldTime = &worldTime;
    ctx.unpackForAuthoring = false;

    std::filesystem::create_directories("saves/tests");
    std::string filepath = "saves/tests/" + test_name + ".json";
    mgr.saveState(filepath, ctx);
    
    // Restore objects back to testWorld so the test can continue using them
    for (auto& obj : zone->world().getOwnedObjectsMutable()) {
        if (obj) {
            testWorld.addObject(std::move(obj));
        }
    }
    zone->world().getOwnedObjectsMutable().clear();

    std::cout << "[TestSaveHelper] Saved test state to " << filepath << std::endl;
}
