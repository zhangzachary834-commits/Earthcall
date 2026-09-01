// Dedicated test suite for ZoneManager::forkZone
//
// Tests happy paths, error conditions, validation edge cases,
// home vs ordinary zone handling, and disk-store hydration for forking zones.

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

std::shared_ptr<Zone> findZone(ZoneManager& mgr, const std::string& id) {
    for (auto& z : mgr.zones()) {
        if (z && z->getIdentifier() == id) return z;
    }
    return nullptr;
}

} // namespace

int main() {
    std::cout << "============================================================\n";
    std::cout << "Running dedicated ZoneManager::forkZone tests...\n";
    std::cout << "============================================================\n";

    auto sandbox = std::filesystem::temp_directory_path() / "earthcall_zone_fork_test";
    std::filesystem::remove_all(sandbox);
    std::filesystem::create_directories(sandbox / "worlds");
    SaveSystem::setSaveRoot(sandbox.string());

    // 1. Invalid Arguments Validation
    {
        ZoneManager mgr;
        auto z = std::make_shared<Zone>("SourceZone", "strict");
        mgr.addZone(z);

        check(!mgr.forkZone("", "NewZone"), "forkZone refuses empty sourceId");
        check(!mgr.forkZone("SourceZone", ""), "forkZone refuses empty newId");
        check(!mgr.forkZone("SourceZone", "SourceZone"), "forkZone refuses sourceId == newId");
        check(!mgr.forkZone("SourceZone", "."), "forkZone refuses unsanitizable label (single dot)");
    }

    // 2. Collision with Existing Live Zone
    {
        ZoneManager mgr;
        auto z1 = std::make_shared<Zone>("Alpha", "strict");
        auto z2 = std::make_shared<Zone>("Beta", "strict");
        mgr.addZone(z1);
        mgr.addZone(z2);

        check(!mgr.forkZone("Alpha", "Beta"), "forkZone refuses when target identifier already exists in live zones");
    }

    // 3. Non-Existent Source Zone
    {
        ZoneManager mgr;
        auto z1 = std::make_shared<Zone>("Alpha", "strict");
        mgr.addZone(z1);

        check(!mgr.forkZone("NonExistent", "AlphaFork"), "forkZone refuses when source zone does not exist");
    }

    // 4. Forking Ordinary Zone
    {
        ZoneManager mgr;
        auto workshop = std::make_shared<Zone>("Workshop", "strict");
        workshop->setOwner("person-42");
        workshop->setQuality("kind", "ordinary");
        workshop->addObject(makeCube("workshop-table", glm::vec3(1.0f, 2.0f, 3.0f)));
        mgr.addZone(workshop);

        check(mgr.forkZone("Workshop", "Workshop.Branch"), "forkZone succeeds for ordinary zone");
        check(SaveSystem::zoneIdentityExists("Workshop.Branch"), "forked ordinary zone exists in zone store");

        auto branch = findZone(mgr, "Workshop.Branch");
        check(branch != nullptr, "forked ordinary zone is added to live zones");
        if (branch) {
            check(branch->getQualities().count("forkedFrom") > 0 &&
                  branch->getQualities().at("forkedFrom") == "Workshop",
                  "forked ordinary zone records forkedFrom=Workshop");
            check(branch->getOwnedObjects().size() == 1, "forked zone inherits owned objects");
            if (!branch->getOwnedObjects().empty() && branch->getOwnedObjects()[0]) {
                check(branch->getOwnedObjects()[0]->belongsToZone("Workshop.Branch"),
                      "forked object updated with new zone designation");
            }
        }
    }

    // 5. Forking Home Zone
    {
        ZoneManager mgr;
        auto home = std::make_shared<Zone>("Home", "strict");
        home->setOwner("person-42");
        home->setQuality("kind", "home");
        home->setQuality("primary", "true");
        home->addObject(makeCube("home-chair", glm::vec3(0.0f, 1.0f, 0.0f)));
        mgr.addZone(home);

        check(mgr.forkZone("Home", "Home.Garden"), "forkZone succeeds for home zone");
        check(SaveSystem::homeIdentityExists("Home.Garden"), "forked home zone written to home identity store");

        auto garden = findZone(mgr, "Home.Garden");
        check(garden != nullptr, "forked home zone loaded into manager");
        if (garden) {
            check(garden->getQualities().count("forkedFrom") > 0 &&
                  garden->getQualities().at("forkedFrom") == "Home",
                  "forked home records forkedFrom=Home");
            check(garden->getQualities().count("primary") == 0 ||
                  garden->getQualities().at("primary") != "true",
                  "forked home is not marked primary");
        }
    }

    // 6. Forking Source From Disk Store (Not currently in live _zones list)
    {
        {
            ZoneManager setupMgr;
            auto offline = std::make_shared<Zone>("OfflineZone", "strict");
            offline->setQuality("kind", "ordinary");
            offline->addObject(makeCube("offline-cube", glm::vec3(5.0f, 5.0f, 5.0f)));
            setupMgr.addZone(offline);
            setupMgr.persistZones();
        }

        // Fresh manager with no live zones loaded yet
        ZoneManager freshMgr;
        check(freshMgr.forkZone("OfflineZone", "OfflineZone.Fork"),
              "forkZone succeeds when source zone is loaded from disk store");

        auto offlineFork = findZone(freshMgr, "OfflineZone.Fork");
        check(offlineFork != nullptr, "forked zone from disk store is added to live zones");
        if (offlineFork) {
            check(offlineFork->getOwnedObjects().size() == 1,
                  "forked zone from disk store correctly loaded owned objects");
        }
    }

    std::filesystem::remove_all(sandbox);
    SaveSystem::setSaveRoot("");

    std::cout << "------------------------------------------------------------\n";
    std::cout << g_checks - g_failures << "/" << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cout << "zone_fork_test: FAILED\n";
        return 1;
    }
    std::cout << "zone_fork_test: ALL OK\n";
    return 0;
}
