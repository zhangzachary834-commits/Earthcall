// Switching between two saved sessions must restore session pose without
// minting a second copy of the same Zone.
//
// What went funky for a Person with my_world + my_second_world:
//   1. json / .ecsave / empty files listed as independent worlds, so Load
//      jumped between twins and a 0-byte ourverse.json.
//   2. A refused read did not log an end, and loadedSaveName still changed.
//   3. Select/Morph kept Object* into the previous world's freed beings.
//   4. (2026-08-21) Home/Sanctum were copied into each session file, so
//      loading the other file replaced the Zone. Same-named Zones now share
//      an identity under saves/zones/; the last save evolves that identity.
// This test drives ZoneManager::loadState — the same office AssetsConsole
// Load calls — and SaveSystem::listWorlds, the listing both windows share.

#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/CreatorConsole/CreatorConsoleState.hpp"
#include "Singularity/Input/Mouse/MouseHandler.hpp"
#include "Singularity/Screen/Camera.hpp"
#include "Singularity/Screen/HighlightSystem.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/SaveContext.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"

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

bool hasObject(const ZoneManager& mgr, const std::string& id) {
    for (const auto& z : mgr.zones()) {
        if (!z) continue;
        for (const auto& o : z->getOwnedObjects()) {
            if (o && o->getIdentifier() == id) return true;
        }
    }
    return false;
}

Object* findObject(ZoneManager& mgr, const std::string& id) {
    for (const auto& z : mgr.zones()) {
        if (!z) continue;
        for (const auto& o : z->getOwnedObjects()) {
            if (o && o->getIdentifier() == id) return o.get();
        }
    }
    return nullptr;
}

} // namespace

int main() {
    std::cout << "============================================================\n";
    std::cout << "Running world switch (multiple saved worlds)...\n";
    std::cout << "============================================================\n";

    auto sandbox = std::filesystem::temp_directory_path() / "earthcall_world_switch";
    std::filesystem::remove_all(sandbox);
    std::filesystem::create_directories(sandbox / "worlds");
    SaveSystem::setSaveRoot(sandbox.string());

    Soul soul("Player");
    Body body("humanoid", "default");
    Person player(std::move(soul), std::move(body), "default");
    Core::Camera camera;
    MouseHandler mouse;
    LawManager laws;
    float color[3] = {1.0f, 1.0f, 1.0f};
    double worldTime = 0.0;
    SaveContext ctx;
    ctx.camera = &camera;
    ctx.mouseHandler = &mouse;
    ctx.currentColor = color;
    ctx.player = &player;
    ctx.lawManager = &laws;
    ctx.worldTime = &worldTime;

    ZoneManager mgr;
    auto zoneA = std::make_shared<Zone>("Sanctum of Beginnings", "default");
    zoneA->addObject(makeCube("cube-world-a", glm::vec3(1.0f, 0.0f, 0.0f)));
    mgr.addZone(zoneA);
    worldTime = 11.0;
    mgr.saveState((sandbox / "worlds" / "alpha.json").string(), ctx);

    zoneA->getOwnedObjectsMutable().clear();
    zoneA->addObject(makeCube("cube-world-b", glm::vec3(9.0f, 0.0f, 0.0f)));
    worldTime = 22.0;
    mgr.saveState((sandbox / "worlds" / "beta.json").string(), ctx);

    // Empty twin: listed as a world before, and a failed load retitled the
    // live world. Must refuse without replacing alpha.
    {
        std::filesystem::create_directories(sandbox / "worlds");
        std::ofstream empty(sandbox / "worlds" / "ourverse.json");
    }

    mgr.loadState((sandbox / "worlds" / "alpha.json").string(), ctx);
    check(hasObject(mgr, "cube-world-b"),
          "alpha load keeps the live Sanctum — beta's save already evolved that Zone");
    check(!hasObject(mgr, "cube-world-a"),
          "alpha's snapshot does not rewind Sanctum; the identity moved on");
    check(mgr.getSaveLoadState().loadedSaveName == "alpha", "loaded name is alpha");
    check(worldTime == 11.0, "session pose (worldTime) still comes from the loaded file");

    Object* liveB = findObject(mgr, "cube-world-b");
    check(liveB != nullptr, "Sanctum cube pointer is live");
    auto& state = Rendering::getCreatorConsoleState();
    state.selectedObject3D = liveB;
    Rendering::HighlightSystem::setSelected(liveB);

    mgr.loadState((sandbox / "worlds" / "beta.json").string(), ctx);
    Rendering::forgetStaleObjectHandles(mgr, &player);
    check(hasObject(mgr, "cube-world-b"), "beta load still has the same Sanctum cube");
    check(!hasObject(mgr, "cube-world-a"), "Sanctum was not replaced with a snapshot copy");
    check(mgr.getSaveLoadState().loadedSaveName == "beta", "loaded name is beta");
    check(state.selectedObject3D == liveB,
          "Select keeps the pointer: the Zone was not torn down by loading another session");
    check(Rendering::HighlightSystem::getSelected() == liveB,
          "HighlightSystem keeps the pointer into the same Zone identity");

    const std::string beforeRefuse = mgr.getSaveLoadState().loadedSaveName;
    mgr.loadState((sandbox / "worlds" / "ourverse.json").string(), ctx);
    check(hasObject(mgr, "cube-world-b"), "refused empty file left beta's cube in place");
    check(mgr.getSaveLoadState().loadedSaveName == beforeRefuse,
          "refused load does not retitle the live world");
    check(mgr.getSaveLoadState().lastLoadReport.find("REFUSED") != std::string::npos ||
              mgr.getSaveLoadState().lastLoadReport.find("COULD NOT") != std::string::npos,
          "refused load reports loudly");

    auto listed = SaveSystem::listWorlds(SaveSystem::SaveType::WORLD);
    bool sawEmpty = false;
    bool sawAlpha = false, sawBeta = false;
    for (const auto& w : listed) {
        if (w.label == "ourverse") sawEmpty = true;
        if (w.label == "alpha") sawAlpha = true;
        if (w.label == "beta") sawBeta = true;
        check(w.label.find("_delta") == std::string::npos, "listWorlds does not offer delta chunks");
    }
    check(!sawEmpty, "listWorlds skips the 0-byte ourverse.json");
    check(sawAlpha && sawBeta, "listWorlds offers alpha and beta once each");

    SaveSystem::removeWorld("alpha", SaveSystem::SaveType::WORLD);
    listed = SaveSystem::listWorlds(SaveSystem::SaveType::WORLD);
    bool alphaAfter = false;
    for (const auto& w : listed) if (w.label == "alpha") alphaAfter = true;
    check(!alphaAfter, "removeWorld drops the stem so a twin cannot linger");

    std::filesystem::remove_all(sandbox);
    SaveSystem::setSaveRoot("");

    std::cout << "------------------------------------------------------------\n";
    std::cout << g_checks - g_failures << "/" << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cout << "world_switch_test: FAILED\n";
        return 1;
    }
    std::cout << "world_switch_test: ALL OK\n";
    return 0;
}
