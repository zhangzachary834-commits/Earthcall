// Ourverse Navigator Prototyping & Verification Probe
// Author: Gemini (Spark) authorized by Zach
// Refusals respected: No new C++ domain classes, no new enum kinds.
// Pure Law + authored geometry & materials + EventBus zone switching.

#include "Singularity/Core/Engine.hpp"
#include "Singularity/Screen/ScreenRecorder.hpp"
#include "Singularity/Screen/Camera.hpp"
#include "Singularity/Input/Mouse/MouseHandler.hpp"
#include "Singularity/Input/Interaction/InteractionChannel.hpp"
#include "Singularity/Terminal/TerminalChannel.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include "Person/Person.hpp"
#include "../../third_party/stb/stb_image.h"
#include <glm/gtc/matrix_transform.hpp>
#include "json.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>

extern ZoneManager mgr;

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << std::endl;
        std::exit(1);
    }
    std::cout << "PASS: " << message << std::endl;
}

void aimCamera(Core::Engine& engine, const glm::vec3& pos, const glm::vec3& target) {
    auto* person = engine.getPerson();
    auto* camera = engine.getCamera();
    auto* mouse = engine.getMouseHandler();

    person->position() = pos - glm::vec3(0.0f, person->getBody().getEyeHeight(), 0.0f);
    camera->setPos(pos);
    glm::vec3 front = glm::normalize(target - pos);
    camera->setFront(front);

    float yaw = glm::degrees(std::atan2(front.z, front.x));
    float pitch = glm::degrees(std::asin(front.y));
    mouse->setYaw(yaw);
    mouse->setPitch(pitch);
}

int main(int argc, char** argv) {
    std::cout.setf(std::ios::unitbuf);
    std::cout << "=== Ourverse Navigator Prototyping Probe ===" << std::endl;

    if (argc < 2) {
        std::cerr << "Usage: ourverse_navigator_probe <workspace_root>" << std::endl;
        return 1;
    }

    const std::filesystem::path sourceRoot = argv[1];
    const auto saves = sourceRoot / "saves";
    const auto outputDir = sourceRoot / "scratch/screenshots";
    std::filesystem::create_directories(outputDir);

    const auto stage = std::filesystem::temp_directory_path() /
        ("earthcall-navigator-probe-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));

    std::cout << "Setting up isolated stage at: " << stage << std::endl;

    // Copy zones needed
    const std::vector<std::string> zonesToCopy = {
        "Ourverse Gathering",
        "Sanctum of Beginnings",
        "Temple of Echoes",
        "Cavern of Light",
        "Character Architect Forge",
        "World"
    };

    for (const auto& zName : zonesToCopy) {
        auto srcZone = saves / "zones" / zName;
        auto dstZone = stage / "zones" / zName;
        if (std::filesystem::exists(srcZone)) {
            std::filesystem::create_directories(dstZone);
            for (const auto& entry : std::filesystem::directory_iterator(srcZone)) {
                std::filesystem::copy_file(entry.path(), dstZone / entry.path().filename(),
                                           std::filesystem::copy_options::overwrite_existing);
            }
        }
    }

    // Copy laws needed
    auto copyLaw = [&](const std::string& lawId) {
        auto srcLaw = saves / "laws" / lawId;
        auto dstLaw = stage / "laws" / lawId;
        if (std::filesystem::exists(srcLaw)) {
            std::filesystem::create_directories(dstLaw);
            for (const auto& entry : std::filesystem::directory_iterator(srcLaw)) {
                std::filesystem::copy_file(entry.path(), dstLaw / entry.path().filename(),
                                           std::filesystem::copy_options::overwrite_existing);
            }
        }
    };
    copyLaw("law-navigator-button");

    // Copy any laws referenced by Ourverse Gathering
    auto doc = SaveSystem::readSaveData((saves / "zones/Ourverse Gathering/zone.json").string());
    if (doc.contains("lawRefs") && doc["lawRefs"].is_array()) {
        for (const auto& ref : doc["lawRefs"]) {
            copyLaw(ref.get<std::string>());
        }
    }

    // Create isolated Person fixture for Zach
    std::filesystem::create_directories(stage / "persons");
    std::ofstream(stage / "persons/Zach.json") << nlohmann::json{
        {"displayName", "Zach"},
        {"soulName", "Zach"},
        {"position", {0.0, 0.0, 0.0}},
        {"velocity", {0.0, 0.0, 0.0}},
        {"body", {{"shape", "humanoid"}, {"artStyle", "default"}, {"height", 1.8}}},
        {"injected_by", "Gemini 3.8 Flash (High) - Personal Ourverse Navigator Prototyping Loop authorized by Zach"}
    }.dump();

    SaveSystem::setSaveRoot(stage.string());

    auto& engine = Core::Engine::instance();
    require(engine.init(1, argv), "Engine initializes successfully with Ourverse Navigator");

    auto* person = engine.getPerson();
    auto* camera = engine.getCamera();
    auto* laws = engine.getLawManager();
    auto* terminal = Singularity::Terminal::TerminalChannel::find(*laws);
    auto* recorder = Singularity::Screen::ScreenRecorder::find(*laws);
    auto* interaction = Singularity::Input::InteractionChannel::find(*laws);

    require(person != nullptr, "Person is valid");
    require(camera != nullptr, "Camera is valid");
    require(laws != nullptr, "LawManager is valid");
    require(recorder != nullptr, "ScreenRecorder is valid");
    require(interaction != nullptr, "InteractionChannel is valid");

    if (terminal) {
        terminal->setSink([](const std::string& s) {
            std::cout << "[Terminal] " << s << std::endl;
        });
    }

    Physics::setFlying(true);

    std::cout << "Active zone at boot: " << mgr.active().name() << std::endl;
    require(mgr.active().name() == "Ourverse Gathering", "Engine boots directly into Ourverse Gathering");

    // Set recorder properties
    lawSetValue(*recorder, PropertyPath::parse("outputPath"), (stage / "capture").string());
    lawSetValue(*recorder, PropertyPath::parse("recordCursor"), false);

    auto capture = [&](const std::string& name) -> std::string {
        lawSetValue(*recorder, PropertyPath::parse("snapshot"), true);
        engine.tick(0.016f);
        PropertyValue pVal, pStatus, pError;
        lawGetValue(*recorder, PropertyPath::parse("lastSnapshotPath"), pVal);
        lawGetValue(*recorder, PropertyPath::parse("status"), pStatus);
        lawGetValue(*recorder, PropertyPath::parse("lastError"), pError);
        std::string statusStr = std::holds_alternative<std::string>(pStatus) ? std::get<std::string>(pStatus) : "unknown";
        std::string errStr = std::holds_alternative<std::string>(pError) ? std::get<std::string>(pError) : "none";
        std::string pathStr = std::holds_alternative<std::string>(pVal) ? std::get<std::string>(pVal) : "";
        std::cout << "[Capture debug] status=" << statusStr << " error=" << errStr << " path=" << pathStr << std::endl;
        require(!pathStr.empty() && std::filesystem::exists(pathStr), "Snapshot file exists");

        std::filesystem::path dst = outputDir / (name + ".png");
        std::filesystem::copy_file(pathStr, dst, std::filesystem::copy_options::overwrite_existing);

        int w = 0, h = 0, channels = 0;
        unsigned char* img = stbi_load(dst.string().c_str(), &w, &h, &channels, 4);
        require(img != nullptr && w > 0 && h > 0, "Captured image decodes validly");
        stbi_image_free(img);

        std::cout << "CAPTURED: " << name << " (" << w << "x" << h << ") -> " << dst.string() << std::endl;
        return dst.string();
    };

    // Warm-up tick
    engine.tick(0.016f);

    // 1. Panoramic view from Observation Plinth
    std::cout << "\n--- Capturing Plinth Panoramic View ---" << std::endl;
    aimCamera(engine, glm::vec3(0.0f, 1.8f, 0.0f), glm::vec3(0.0f, 1.5f, -10.0f));
    for (int i = 0; i < 5; ++i) engine.tick(0.016f);
    capture("navigator_plinth_panorama");

    // 2. Aerial / High Perch View of Entire Archipelago
    std::cout << "\n--- Capturing Aerial Archipelago View ---" << std::endl;
    aimCamera(engine, glm::vec3(0.0f, 12.0f, 8.0f), glm::vec3(0.0f, 1.0f, -11.0f));
    for (int i = 0; i < 5; ++i) engine.tick(0.016f);
    capture("navigator_archipelago_aerial");

    // 3. Close-up: Sanctum of Beginnings Island
    std::cout << "\n--- Capturing Sanctum Island Close-up ---" << std::endl;
    aimCamera(engine, glm::vec3(-7.5f, 3.0f, -8.0f), glm::vec3(-10.0f, 2.5f, -12.0f));
    for (int i = 0; i < 5; ++i) engine.tick(0.016f);
    capture("navigator_island_sanctum");

    // 4. Close-up: Temple of Echoes Island
    std::cout << "\n--- Capturing Temple Island Close-up ---" << std::endl;
    aimCamera(engine, glm::vec3(-2.0f, 3.5f, -11.0f), glm::vec3(-3.5f, 2.5f, -15.5f));
    for (int i = 0; i < 5; ++i) engine.tick(0.016f);
    capture("navigator_island_temple");

    // 5. Close-up: Cavern of Light Island
    std::cout << "\n--- Capturing Cavern Island Close-up ---" << std::endl;
    aimCamera(engine, glm::vec3(2.0f, 3.0f, -11.0f), glm::vec3(3.5f, 2.0f, -15.5f));
    for (int i = 0; i < 5; ++i) engine.tick(0.016f);
    capture("navigator_island_cavern");

    // 6. Close-up: Character Architect Forge Island
    std::cout << "\n--- Capturing Forge Island Close-up ---" << std::endl;
    aimCamera(engine, glm::vec3(7.5f, 3.2f, -8.0f), glm::vec3(10.0f, 2.5f, -12.0f));
    for (int i = 0; i < 5; ++i) engine.tick(0.016f);
    capture("navigator_island_forge");

    // 7. Interactive Navigation Test: Click on Sanctum Beacon
    std::cout << "\n--- Testing Interactive Beacon Navigation (Click to Sanctum) ---" << std::endl;
    Object* sanctumBeacon = nullptr;
    for (const auto& obj : mgr.active().getOwnedObjects()) {
        if (obj->getIdentifier() == "navigator-beacon-sanctum") {
            sanctumBeacon = obj.get();
            break;
        }
    }
    require(sanctumBeacon != nullptr, "Sanctum Beacon object found in Ourverse Gathering");

    // Aim camera at the sanctum beacon
    aimCamera(engine, glm::vec3(0.0f, 1.8f, 0.0f), sanctumBeacon->getPosition());
    for (int i = 0; i < 3; ++i) engine.tick(0.016f);

    // Simulate click on beacon
    Core::EventBus::instance().publish(ECA::Event{"object-clicked", sanctumBeacon, person, std::time(nullptr)});
    laws->tick();
    engine.tick(0.016f);
    engine.tick(0.016f);

    std::cout << "Active zone after clicking Sanctum beacon: " << mgr.active().name() << std::endl;
    require(mgr.active().name() == "Sanctum of Beginnings",
            "Zone navigation successfully switches active zone to Sanctum of Beginnings");

    // Capture destination zone
    aimCamera(engine, glm::vec3(0.0f, 2.0f, 5.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    for (int i = 0; i < 5; ++i) engine.tick(0.016f);
    capture("navigated_destination_sanctum");

    // 8. Test Navigation back to Ourverse Gathering and then to Temple of Echoes
    std::cout << "\n--- Testing Return to Ourverse Gathering ---" << std::endl;
    engine.requestZoneSwitch("Ourverse Gathering");
    engine.tick(0.016f);
    std::cout << "Active zone after return request: " << mgr.active().name() << std::endl;
    require(mgr.active().name() == "Ourverse Gathering", "Successfully returned to Ourverse Gathering");

    std::cout << "\n--- Testing Interactive Beacon Navigation (Click to Temple) ---" << std::endl;
    Object* templeBeacon = nullptr;
    for (const auto& obj : mgr.active().getOwnedObjects()) {
        if (obj->getIdentifier() == "navigator-beacon-temple") {
            templeBeacon = obj.get();
            break;
        }
    }
    require(templeBeacon != nullptr, "Temple Beacon object found in Ourverse Gathering");

    std::cout << "Publishing click on Temple beacon..." << std::endl;
    Core::EventBus::instance().publish(ECA::Event{"object-clicked", templeBeacon, person, std::time(nullptr)});
    std::cout << "Published click. Running laws->tick()..." << std::endl;
    laws->tick();
    std::cout << "laws->tick() finished. Running engine.tick(1)..." << std::endl;
    engine.tick(0.016f);
    std::cout << "engine.tick(1) finished. Running engine.tick(2)..." << std::endl;
    engine.tick(0.016f);
    std::cout << "engine.tick(2) finished." << std::endl;

    std::cout << "Active zone after clicking Temple beacon: " << mgr.active().name() << std::endl;
    require(mgr.active().name() == "Temple of Echoes",
            "Zone navigation successfully switches active zone to Temple of Echoes");

    std::cout << "\nALL NAVIGATOR PROTOTYPE TESTS PASSED WITH VISUAL WITNESSES!" << std::endl;

    engine.shutdown();
    SaveSystem::setSaveRoot("");
    return 0;
}
