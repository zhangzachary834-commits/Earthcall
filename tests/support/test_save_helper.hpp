#pragma once

#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "Singularity/Screen/Camera.hpp"
#include "Singularity/Input/Mouse/MouseHandler.hpp"
#include "Person/Person.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include <glm/glm.hpp>
#include <cmath>
#include <string>
#include <filesystem>
#include <iostream>

inline void dump_test_save(const std::string& test_name, Zone& testWorld, LawManager& testLawManager, Person& testPlayer,
                           const std::string& filepathOverride = "") {
    std::cout << "[TestSaveHelper] Generating test save: " << test_name << "...\n";

    ZoneManager mgr;
    auto zone = std::make_shared<Zone>(test_name, "test");
    zone->setOwner(testPlayer.getIdentifier());

    // Temporarily move objects from testWorld to the zone's world for serialization
    for (auto& obj : testWorld.getOwnedObjectsMutable()) {
        if (obj) {
            zone->addObject(std::move(obj));
        }
    }
    testWorld.getOwnedObjectsMutable().clear();
    
    mgr.addZone(zone);

    Core::Camera camera;
    glm::vec3 minP(1e9f), maxP(-1e9f);
    int objectCount = 0;
    for (const auto& obj : zone->getOwnedObjects()) {
        if (!obj) continue;
        obj->addZoneDesignation(zone->name());
        const glm::vec3 p = obj->getPosition();
        minP = glm::min(minP, p);
        maxP = glm::max(maxP, p);
        ++objectCount;
    }
    if (objectCount > 0) {
        const glm::vec3 center = 0.5f * (minP + maxP);
        float radius = 0.5f * glm::length(maxP - minP);
        if (radius < 1.5f) radius = 1.5f;
        camera.pos = center + glm::vec3(0.0f, radius * 0.45f + 1.6f, radius * 2.2f + 3.0f);
        glm::vec3 dir = center - camera.pos;
        camera.front = glm::length(dir) > 1e-4f ? glm::normalize(dir) : glm::vec3(0.0f, 0.0f, -1.0f);
    } else {
        camera.pos = testPlayer.position + glm::vec3(0.0f, testPlayer.getBody().getEyeHeight(), 0.0f);
        camera.front = testPlayer.cameraForward;
    }
    testPlayer.cameraPos = camera.pos;
    testPlayer.cameraForward = camera.front;
    
    MouseHandler mouseHandler;
    {
        const float rad2deg = 57.2957795f;
        const glm::vec3 dir = glm::normalize(camera.front);
        mouseHandler.setPitch(std::asin(glm::clamp(dir.y, -0.999f, 0.999f)) * rad2deg);
        mouseHandler.setYaw(std::atan2(dir.z, dir.x) * rad2deg);
    }
    float currentColor[3] = {1.0f, 1.0f, 1.0f};
    double worldTime = 0.0;

    SaveContext ctx;
    ctx.camera = &camera;
    ctx.mouseHandler = &mouseHandler;
    ctx.currentColor = currentColor;
    ctx.player = &testPlayer;
    ctx.lawManager = &testLawManager;
    ctx.worldTime = &worldTime;
    ctx.unpackForAuthoring = false;

    std::string filepath = filepathOverride;
    if (filepath.empty()) {
        std::filesystem::create_directories("saves/tests");
        filepath = "saves/tests/" + test_name + ".json";
    } else {
        std::filesystem::create_directories(std::filesystem::path(filepath).parent_path());
    }
    mgr.saveState(filepath, ctx);
    
    // Restore objects back to testWorld so the test can continue using them
    for (auto& obj : zone->getOwnedObjectsMutable()) {
        if (obj) {
            testWorld.addObject(std::move(obj));
        }
    }
    zone->getOwnedObjectsMutable().clear();

    std::cout << "[TestSaveHelper] Saved test state to " << filepath << std::endl;
}
