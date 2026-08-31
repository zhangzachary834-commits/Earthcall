// The Person's save/load office is saveStateWithLog (Quick Save / Save As)
// then loadState of the json sidecar. Existing tests never called that pair:
// world_switch uses saveState (a direct JSON write), observation uses
// dump_test_save, object_roundtrip serializes one Object, law_persistence
// round-trips LawManager JSON. A green suite could not see:
//   - saveStateWithLog skipping index 0 and 1 (baseline cube/ground that no
//     longer live in the Zone), so the top-level objects fallback lost the
//     first two beings a Person spawned
//   - loadState moving the camera and not Person.position, so the next
//     LocomotionChannel step snapped the view back
//   - json + .ecsave of the same stem listing as two worlds
// This test calls those two methods, which is what the Assets console calls.

#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Singularity/Input/Mouse/MouseHandler.hpp"
#include "Singularity/Screen/Camera.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/SaveContext.hpp"
#include "ZonesOfEarth/HomesOfEarth/Home.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "json.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

namespace {

int g_checks = 0;
int g_failures = 0;

void check(bool condition, const std::string& description) {
    ++g_checks;
    if (!condition) {
        ++g_failures;
        std::cout << "  FAILED: " << description << std::endl;
        return;
    }
    std::cout << "  ok: " << description << std::endl;
}

std::shared_ptr<Object> makeCube(const std::string& id, const glm::vec3& p) {
    auto obj = std::make_shared<Object>();
    obj->setShape(Object::ShapeKind::Cube);
    obj->setObjectID(id);
    obj->setTransform(glm::translate(glm::mat4(1.0f), p));
    return obj;
}

std::size_t countId(const ZoneManager& mgr, const std::string& id) {
    std::size_t n = 0;
    for (const auto& z : mgr.zones()) {
        if (!z) continue;
        for (const auto& o : z->getOwnedObjects()) {
            if (o && o->getIdentifier() == id) ++n;
        }
    }
    return n;
}

glm::vec3 objectPos(const ZoneManager& mgr, const std::string& id) {
    for (const auto& z : mgr.zones()) {
        if (!z) continue;
        for (const auto& o : z->getOwnedObjects()) {
            if (o && o->getIdentifier() == id) return o->getPosition();
        }
    }
    return glm::vec3(0.0f);
}

} // namespace

int main() {
    std::cout << "============================================================\n";
    std::cout << "Running save round-trip (Person Save As / Load path)...\n";
    std::cout << "============================================================\n";

    auto sandbox = std::filesystem::temp_directory_path() / "earthcall_save_roundtrip";
    std::filesystem::remove_all(sandbox);
    std::filesystem::create_directories(sandbox / "worlds");
    SaveSystem::setSaveRoot(sandbox.string());

    Soul soul("Player");
    Body body("humanoid", "default");
    Person player(std::move(soul), std::move(body), "default");
    Core::Camera camera;
    camera.pos = glm::vec3(4.0f, 6.0f, 8.0f);
    MouseHandler mouse;
    LawManager laws;
    float color[3] = {1.0f, 0.5f, 0.25f};
    double worldTime = 33.0;
    SaveContext ctx;
    ctx.camera = &camera;
    ctx.mouseHandler = &mouse;
    ctx.currentColor = color;
    ctx.person = &player;
    ctx.lawManager = &laws;
    ctx.worldTime = &worldTime;

    ZoneManager mgr;
    auto sanctum = std::make_shared<Zone>("Sanctum of Beginnings", "default");
    sanctum->addObject(makeCube("first-spawned", glm::vec3(1.0f, 0.0f, 0.0f)));
    sanctum->addObject(makeCube("second-spawned", glm::vec3(2.0f, 0.0f, 0.0f)));
    sanctum->addObject(makeCube("third-spawned", glm::vec3(3.0f, 0.0f, 0.0f)));
    mgr.addZone(sanctum);
    auto home = std::make_shared<Home>("Home", "default");
    home->setOwner(player.getIdentifier());
    mgr.addZone(home);

    mgr.saveStateWithLog("roundtrip_world", ctx);
    check(mgr.getSaveLoadState().lastSaveReport.find("SAVE FAILED") == std::string::npos &&
              !mgr.getSaveLoadState().lastSaveReport.empty(),
          "saveStateWithLog wrote a report and did not refuse");

    const auto jsonPath = sandbox / "worlds" / "roundtrip_world.json";
    const auto ecsavePath = sandbox / "worlds" / "roundtrip_world.ecsave";
    check(std::filesystem::exists(jsonPath), "Save As writes the readable json sidecar");
    check(std::filesystem::exists(ecsavePath), "Save As writes the binary .ecsave");
    check(std::filesystem::exists(sandbox / "zones" / "Sanctum of Beginnings" / "zone.json"),
          "Save As also writes the Sanctum identity under saves/zones/");
    check(std::filesystem::exists(sandbox / "homes" / "Home" / "home.json"),
          "Save As writes Home under saves/homes/, not as a Zone file");

    {
        std::ifstream in(jsonPath);
        nlohmann::json j;
        in >> j;
        check(j.contains("objects") && j["objects"].is_array() && j["objects"].size() == 3,
              "top-level objects array keeps all three spawns (not skip-2)");
        std::size_t zoneObjs = 0;
        if (j.contains("zones") && j["zones"].is_array() && !j["zones"].empty()) {
            const auto& w = j["zones"][0]["world"];
            if (w.contains("objects") && w["objects"].is_array())
                zoneObjs = w["objects"].size();
        }
        check(zoneObjs == 3, "zone world JSON also keeps all three spawns");
        check(j.value("currentZone", 99) == 0, "saved currentZone is the Sanctum");
    }

    auto listed = SaveSystem::listWorlds(SaveSystem::SaveType::WORLD);
    std::size_t rows = 0;
    std::string listedPath;
    for (const auto& w : listed) {
        if (w.label != "roundtrip_world") continue;
        ++rows;
        listedPath = w.path;
    }
    check(rows == 1, "json and .ecsave of one stem are one list row");
    check(listedPath.size() >= 5 &&
              listedPath.compare(listedPath.size() - 5, 5, ".json") == 0,
          "the list prefers the readable json");

    player.position() = glm::vec3(100.0f, 0.0f, 100.0f);
    camera.pos = glm::vec3(0.0f, 0.0f, 0.0f);
    worldTime = 0.0;
    mgr.loadState(jsonPath.string(), ctx);

    check(countId(mgr, "first-spawned") == 1, "load restores the first spawned being");
    check(countId(mgr, "second-spawned") == 1, "load restores the second spawned being");
    check(countId(mgr, "third-spawned") == 1, "load restores the third spawned being");
    check(glm::length(objectPos(mgr, "first-spawned") - glm::vec3(1.0f, 0.0f, 0.0f)) < 1e-3f,
          "first spawned being kept its position");
    check(mgr.currentIndex() == 0, "load restores the saved current Zone");

    const float eyeH = player.getBody().getEyeHeight();
    const glm::vec3 expectedPerson = camera.pos - glm::vec3(0.0f, eyeH, 0.0f);
    check(glm::distance(player.position(), expectedPerson) < 1e-3f,
          "Person.position matches camera after loadState so locomotion will not snap the view");
    check(glm::distance(camera.pos, glm::vec3(4.0f, 6.0f, 8.0f)) < 1e-3f,
          "camera returns to the saved viewpoint");

    player.position() = glm::vec3(50.0f, 0.0f, 50.0f);
    mgr.loadState(ecsavePath.string(), ctx);
    check(countId(mgr, "first-spawned") == 1 &&
              countId(mgr, "second-spawned") == 1 &&
              countId(mgr, "third-spawned") == 1,
          "loading the .ecsave twin restores the same three beings");
    check(glm::distance(player.position(), camera.pos - glm::vec3(0.0f, eyeH, 0.0f)) < 1e-3f,
          "Person is settled after an .ecsave load too");

    mgr.switchTo(1);
    check(mgr.active().name() == "Home", "switched to Home before the second save");
    check(countId(mgr, "first-spawned") == 1,
          "switchTo Home does not steal Sanctum's objects");
    mgr.saveStateWithLog("saved_from_home", ctx);
    player.position() = glm::vec3(70.0f, 0.0f, 70.0f);
    mgr.loadState((sandbox / "worlds" / "saved_from_home.json").string(), ctx);
    check(mgr.currentIndex() == 1, "load restores Home as the current Zone");
    check(countId(mgr, "first-spawned") == 1 &&
              countId(mgr, "second-spawned") == 1 &&
              countId(mgr, "third-spawned") == 1,
          "objects in a non-active Zone survive Save As / Load");

    std::filesystem::remove_all(sandbox);
    SaveSystem::setSaveRoot("");

    std::cout << "------------------------------------------------------------\n";
    std::cout << g_checks - g_failures << "/" << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cout << "save_roundtrip_test: FAILED\n";
        return 1;
    }
    std::cout << "save_roundtrip_test: ALL OK\n";
    return 0;
}
