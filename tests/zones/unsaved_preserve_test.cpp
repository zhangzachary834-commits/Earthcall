// Load used to replace the live world with no copy. The CRITICAL save-system
// line at the top of the agenda is the fear of that erasure. loadState now
// writes the unsaved present world to a dedicated slot
// (saves/backups/before-load.json) BEFORE it clears Zones, then overwrites.
// Loading that slot itself must not re-stash, or recovery would destroy the
// stash. This test drives ZoneManager::loadState — the same office the
// Load / Restore unsaved buttons call.

#include "ConstructedBeing/Object/Object.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Singularity/Input/Mouse/MouseHandler.hpp"
#include "Singularity/Screen/Camera.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/SaveContext.hpp"
#include "ZonesOfEarth/World/World.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "json.hpp"

#include <glm/gtc/matrix_transform.hpp>
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
        for (const auto& o : z->world().getOwnedObjects()) {
            if (o && o->getIdentifier() == id) return true;
        }
    }
    return false;
}

} // namespace

int main() {
    std::cout << "============================================================\n";
    std::cout << "Running unsaved preserve (before-load snapshot)...\n";
    std::cout << "============================================================\n";

    auto sandbox = std::filesystem::temp_directory_path() / "earthcall_unsaved_preserve";
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
    double worldTime = 1.0;
    SaveContext ctx;
    ctx.camera = &camera;
    ctx.mouseHandler = &mouse;
    ctx.currentColor = color;
    ctx.player = &player;
    ctx.lawManager = &laws;
    ctx.worldTime = &worldTime;

    const std::string otherPath = (sandbox / "worlds" / "other.json").string();
    {
        ZoneManager writer;
        auto z = std::make_shared<Zone>("Sanctum of Beginnings", "default");
        z->world().addObject(makeCube("saved-cube", glm::vec3(9.0f, 0.0f, 0.0f)));
        writer.addZone(z);
        writer.saveState(otherPath, ctx);
    }

    ZoneManager live;
    auto liveZone = std::make_shared<Zone>("Sanctum of Beginnings", "default");
    liveZone->world().addObject(makeCube("unsaved-cube", glm::vec3(1.0f, 0.0f, 0.0f)));
    live.addZone(liveZone);

    check(hasObject(live, "unsaved-cube"), "live world holds unsaved work");
    check(!std::filesystem::exists(ZoneManager::beforeLoadSnapshotPath()),
          "before-load slot is empty before the first overwrite");

    live.loadState(otherPath, ctx);

    check(hasObject(live, "saved-cube"), "load replaced the present world with the file");
    check(!hasObject(live, "unsaved-cube"), "unsaved cube is not in the overwritten world");
    const std::string stash = ZoneManager::beforeLoadSnapshotPath();
    check(std::filesystem::exists(stash) && std::filesystem::file_size(stash) > 0,
          "unsaved work was written to the dedicated before-load slot");
    check(live.getSaveLoadState().lastLoadReport.find("Unsaved work preserved") != std::string::npos,
          "load report names the preserve so failure is loud");

    {
        std::ifstream in(stash);
        nlohmann::json j;
        in >> j;
        bool stashedUnsaved = false;
        if (j.contains("zones")) {
            for (const auto& z : j["zones"]) {
                if (!z.contains("world") || !z["world"].contains("objects")) continue;
                for (const auto& o : z["world"]["objects"]) {
                    if (o.value("objectID", "") == "unsaved-cube") stashedUnsaved = true;
                }
            }
        }
        check(stashedUnsaved, "before-load JSON contains the unsaved cube");
        bool stashedLoaded = false;
        if (j.contains("zones")) {
            for (const auto& z : j["zones"]) {
                if (!z.contains("world") || !z["world"].contains("objects")) continue;
                for (const auto& o : z["world"]["objects"]) {
                    if (o.value("objectID", "") == "saved-cube") stashedLoaded = true;
                }
            }
        }
        check(!stashedLoaded, "before-load is the PREVIOUS world, not the one just loaded");
    }

    const auto stashTime = std::filesystem::last_write_time(stash);
    live.loadState((sandbox / "worlds" / "missing-empty.json").string(), ctx);
    check(hasObject(live, "saved-cube"), "a refused load leaves the present world");
    check(std::filesystem::last_write_time(stash) == stashTime,
          "a refused load does not overwrite the before-load slot");

    live.loadState(stash, ctx);
    check(hasObject(live, "unsaved-cube"), "loading before-load restores the unsaved cube");
    check(!hasObject(live, "saved-cube"), "restored world does not keep the overwritten world's cube");
    {
        std::ifstream in(stash);
        nlohmann::json j;
        in >> j;
        bool stillUnsaved = false;
        if (j.contains("zones")) {
            for (const auto& z : j["zones"]) {
                if (!z.contains("world") || !z["world"].contains("objects")) continue;
                for (const auto& o : z["world"]["objects"]) {
                    if (o.value("objectID", "") == "unsaved-cube") stillUnsaved = true;
                }
            }
        }
        check(stillUnsaved,
              "loading the before-load slot does not re-stash the current world over it");
    }

    std::filesystem::remove_all(sandbox);
    SaveSystem::setSaveRoot("");

    std::cout << "------------------------------------------------------------\n";
    std::cout << g_checks - g_failures << "/" << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cout << "unsaved_preserve_test: FAILED\n";
        return 1;
    }
    std::cout << "unsaved_preserve_test: ALL OK\n";
    return 0;
}
