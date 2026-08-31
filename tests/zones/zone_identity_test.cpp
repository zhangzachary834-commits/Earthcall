// Zone identity store — the click-through Zach named on the agenda.
//
// Creating beings in Home, saving session A, loading session B, and walking
// back to Home used to show an empty room: every "world" file carried its own
// copy of every Zone, and loadState replaced the lot. EarthcallOurverse.md
// says each Person has at least one Singularity-fixed Home; a session file
// may name, fork, and evolve that Home, not mint a new one.
//
// This test drives ZoneManager::saveState / loadState / forkZone / diffZones
// — the same offices Save As / Load call.

#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Singularity/Input/Mouse/MouseHandler.hpp"
#include "Singularity/Screen/Camera.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/SaveContext.hpp"
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
        for (const auto& o : z->getOwnedObjects()) {
            if (o && o->getIdentifier() == id) return true;
        }
    }
    return false;
}

std::shared_ptr<Zone> findZone(ZoneManager& mgr, const std::string& id) {
    for (auto& z : mgr.zones()) {
        if (z && z->getIdentifier() == id) return z;
    }
    return nullptr;
}

struct Harness {
    Soul soul;
    Body body;
    Person player;
    Core::Camera camera;
    MouseHandler mouse;
    LawManager laws;
    float color[3] = {1.0f, 1.0f, 1.0f};
    double worldTime = 0.0;
    SaveContext ctx;

    Harness()
        : soul("Player"),
          body("humanoid", "default"),
          player(std::move(soul), std::move(body), "default") {
        ctx.camera = &camera;
        ctx.mouseHandler = &mouse;
        ctx.currentColor = color;
        ctx.person = &player;
        ctx.lawManager = &laws;
        ctx.worldTime = &worldTime;
    }
};

} // namespace

int main() {
    std::cout << "============================================================\n";
    std::cout << "Running zone identity (Home persists across session files)...\n";
    std::cout << "============================================================\n";

    auto sandbox = std::filesystem::temp_directory_path() / "earthcall_zone_identity";
    std::filesystem::remove_all(sandbox);
    std::filesystem::create_directories(sandbox / "worlds");
    SaveSystem::setSaveRoot(sandbox.string());

    Harness h;
    const std::string worldA = (sandbox / "worlds" / "session_a.json").string();
    const std::string worldB = (sandbox / "worlds" / "session_b.json").string();

    {
        ZoneManager mgr;
        auto home = std::make_shared<Zone>("Home", "strict");
        home->setOwner(h.player.getIdentifier());
        home->setQuality("kind", "home");
        home->addObject(makeCube("home-cube", glm::vec3(1.0f, 0.0f, 0.0f)));
        mgr.addZone(home);
        auto workshop = std::make_shared<Zone>("Workshop", "strict");
        workshop->setOwner(h.player.getIdentifier());
        workshop->addObject(makeCube("workshop-cube", glm::vec3(4.0f, 0.0f, 0.0f)));
        mgr.addZone(workshop);
        h.worldTime = 11.0;
        mgr.saveState(worldA, h.ctx);
        mgr.saveStateWithLog("session_a", h.ctx);
    }

    const auto homePath = sandbox / "homes" / "Home" / "home.json";
    const auto workshopPath = sandbox / "zones" / "Workshop" / "zone.json";
    check(std::filesystem::exists(homePath),
          "Save writes Home to saves/homes/Home/home.json");
    check(std::filesystem::exists(workshopPath),
          "Save writes Workshop to its own directory, not only the session file");
    {
        std::ifstream in(worldA);
        nlohmann::json j;
        in >> j;
        check(j.value("saveFormat", std::string{}) == "zone-identity-v1",
              "session file names the zone-identity format");
        check(j.contains("zoneRefs") && j["zoneRefs"].is_array() && j["zoneRefs"].size() >= 2,
              "session file lists zoneRefs rather than owning the Zones");
        check(j.contains("zones") && j["zones"].is_array(),
              "session file dual-writes zones[] so existing files still load");
    }

    {
        ZoneManager other;
        auto home = std::make_shared<Zone>("Home", "strict");
        home->setOwner(h.player.getIdentifier());
        home->setQuality("kind", "home");
        other.addZone(home);
        auto plaza = std::make_shared<Zone>("Plaza", "strict");
        plaza->addObject(makeCube("plaza-cube", glm::vec3(8.0f, 0.0f, 0.0f)));
        other.addZone(plaza);
        h.worldTime = 22.0;
        other.saveState(worldB, h.ctx);
    }

    check(std::filesystem::exists(homePath), "saving session B does not delete Home's identity file");
    {
        std::ifstream in(homePath);
        nlohmann::json hj;
        in >> hj;
        bool stillHomeCube = false;
        if (hj.contains("world") && hj["world"].contains("objects")) {
            for (const auto& o : hj["world"]["objects"]) {
                if (o.value("objectID", "") == "home-cube") stillHomeCube = true;
            }
        }
        check(stillHomeCube,
              "saving a session whose live Home is empty does not wipe the stored Home");
    }

    {
        ZoneManager loaded;
        loaded.loadState(worldA, h.ctx);
        check(hasObject(loaded, "home-cube"), "loading session A restores Home's cube from the Zone store");
        check(hasObject(loaded, "workshop-cube"), "Workshop is its own identity, restored with A");
        loaded.loadState(worldB, h.ctx);
        check(hasObject(loaded, "home-cube"),
              "loading session B does not replace Home — the cube authored in A is still there");
        check(hasObject(loaded, "plaza-cube"), "session B's Plaza is admitted alongside Home");
        check(hasObject(loaded, "workshop-cube"),
              "a Person's other Zone (Workshop) is not dropped when another session loads");
        auto home = findZone(loaded, "Home");
        check(home && home->getOwnedObjects().size() >= 1, "Home is the same Zone, not a new empty one");
        check(h.worldTime == 22.0, "session pose (worldTime) still comes from the loaded session file");
        loaded.switchTo(0);
        for (size_t i = 0; i < loaded.zones().size(); ++i) {
            if (loaded.zones()[i] && loaded.zones()[i]->getIdentifier() == "Home") {
                loaded.switchTo(i);
                break;
            }
        }
        check(loaded.active().getIdentifier() == "Home", "switched into Home after loading session B");
        check(hasObject(loaded, "home-cube"),
              "walking into Home after loading the other file still shows what was created there");
    }

    {
        ZoneManager fresh;
        fresh.hydrateFromZoneStore();
        check(hasObject(fresh, "home-cube"),
              "a new manager hydrating from the Zone store sees Home without loading a session");
        check(hasObject(fresh, "workshop-cube"), "hydrate admits every stored Zone, not only Home");
    }

    {
        ZoneManager mgr;
        mgr.hydrateFromZoneStore();
        check(mgr.forkZone("Home", "Home.garden"), "forkZone copies Home under a new name");
        check(SaveSystem::homeIdentityExists("Home.garden"),
              "the fork has its own directory in the Home store");
        auto garden = findZone(mgr, "Home.garden");
        check(garden && garden->getQualities().count("forkedFrom") &&
                  garden->getQualities().at("forkedFrom") == "Home",
              "the fork records forkedFrom=Home (named, not a silent twin)");
        check(hasObject(mgr, "home-cube"), "the original Home is still here after the fork");
        nlohmann::json same = mgr.diffZones("Home", "Home.garden");
        check(same.contains("shared") && same["shared"].size() >= 1,
              "diff of a fresh fork reports the cube as shared");
        check(same["onlyInA"].empty() && same["onlyInB"].empty(),
              "a fresh fork has no object-id divergence yet");
        garden->addObject(makeCube("garden-bench", glm::vec3(2.0f, 0.0f, 0.0f)));
        mgr.persistZones();
        nlohmann::json diverged = mgr.diffZones("Home", "Home.garden");
        bool benchOnlyInB = false;
        for (const auto& id : diverged["onlyInB"]) {
            if (id.get<std::string>() == "garden-bench") benchOnlyInB = true;
        }
        check(benchOnlyInB, "after evolving the fork, diff names garden-bench as onlyInB");
        bool cubeShared = false;
        for (const auto& id : diverged["shared"]) {
            if (id.get<std::string>() == "home-cube") cubeShared = true;
        }
        check(cubeShared, "the original Home cube remains shared after the fork evolved");
    }

    std::filesystem::remove_all(sandbox);
    SaveSystem::setSaveRoot("");

    std::cout << "------------------------------------------------------------\n";
    std::cout << g_checks - g_failures << "/" << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cout << "zone_identity_test: FAILED\n";
        return 1;
    }
    std::cout << "zone_identity_test: ALL OK\n";
    return 0;
}
