#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "json.hpp"

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

int g_checks = 0;
int g_failures = 0;

void check(bool condition, const std::string& description) {
    ++g_checks;
    if (!condition) {
        ++g_failures;
        std::cerr << "  FAILED: " << description << std::endl;
        return;
    }
    std::cout << "  ok: " << description << std::endl;
}

int main() {
    std::cout << "============================================================\n";
    std::cout << "Running fork_zone_test...\n";
    std::cout << "============================================================\n";

    auto sandbox = std::filesystem::temp_directory_path() / "earthcall_fork_zone_test";
    std::filesystem::remove_all(sandbox);
    std::filesystem::create_directories(sandbox / "worlds");
    SaveSystem::setSaveRoot(sandbox.string());

    {
        ZoneManager mgr;

        // Edge Case: Empty strings
        check(!mgr.forkZone("", "NewZone"), "forkZone rejects empty sourceId");
        check(!mgr.forkZone("OldZone", ""), "forkZone rejects empty newId");
        check(!mgr.forkZone("SameZone", "SameZone"), "forkZone rejects sourceId == newId");

        // Edge Case: Invalid label
        check(!mgr.forkZone("OldZone", "../BadZone"), "forkZone rejects invalid newId label");

        // Edge Case: Source not found
        check(!mgr.forkZone("MissingZone", "NewZone"), "forkZone rejects non-existent source zone");

        auto srcZone = std::make_shared<Zone>("SourceZone", "strict");
        srcZone->setQuality("kind", "home");
        auto obj = std::make_shared<Object>();
        obj->setObjectID("test-object");
        srcZone->addObject(obj);
        mgr.addZone(srcZone);

        // Edge case: Target already exists
        auto conflictZone = std::make_shared<Zone>("ConflictZone", "strict");
        mgr.addZone(conflictZone);
        check(!mgr.forkZone("SourceZone", "ConflictZone"), "forkZone rejects newId if already in memory");

        // Happy Path: Successful fork
        check(mgr.forkZone("SourceZone", "ForkedZone"), "forkZone successfully forks an existing zone");

        bool foundForked = false;
        bool foundObj = false;
        for (const auto& z : mgr.zones()) {
            if (z && z->getIdentifier() == "ForkedZone") {
                foundForked = true;
                check(z->getQualities().at("forkedFrom") == "SourceZone", "forked zone has 'forkedFrom' quality set");
                check(z->getQualities().count("primary") == 0 || z->getQualities().at("primary") == "false", "forked zone is not primary");
                check(z->isPersonalHome(), "forked zone inherits dwelling state if applicable");
                for (const auto& o : z->getOwnedObjects()) {
                    if (o && o->getIdentifier() == "test-object") {
                        foundObj = true;
                    }
                }
            }
        }
        check(foundForked, "forked zone was added to the ZoneManager");
        check(foundObj, "forked zone contains objects from the source zone");

        // Verify disk persistence
        check(SaveSystem::homeIdentityExists("ForkedZone"), "forked dwelling is persisted to disk");

        auto nonDwelling = std::make_shared<Zone>("BasicZone", "strict");
        mgr.addZone(nonDwelling);
        check(mgr.forkZone("BasicZone", "ForkedBasicZone"), "forkZone successfully forks non-dwelling zone");
        check(SaveSystem::zoneIdentityExists("ForkedBasicZone"), "forked non-dwelling is persisted to disk");
    }

    std::filesystem::remove_all(sandbox);
    SaveSystem::setSaveRoot("");

    std::cout << "------------------------------------------------------------\n";
    std::cout << g_checks - g_failures << "/" << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cerr << "fork_zone_test: FAILED\n";
        return 1;
    }
    std::cout << "fork_zone_test: ALL OK\n";
    return 0;
}
