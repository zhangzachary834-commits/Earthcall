#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include <iostream>
#include <filesystem>
#include <memory>
#include <cassert>

namespace {
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
} // namespace

int main() {
    std::cout << "============================================================\n";
    std::cout << "Running zone_manager_test...\n";
    std::cout << "============================================================\n";

    // Isolate save system to a sandbox so authorZone/ensureHomeZone do not write real save data
    std::filesystem::path sandbox = std::filesystem::temp_directory_path() / "zone_manager_test_sandbox";
    std::filesystem::create_directories(sandbox);
    SaveSystem::setSaveRoot(sandbox.string());

    {
        std::cout << "\n[1] Testing basic addZone and state tracking...\n";
        ZoneManager mgr;
        check(mgr.zones().empty(), "New manager is empty");

        auto z1 = std::make_shared<Zone>("test_zone_1", "strict");
        mgr.addZone(z1);
        check(mgr.zones().size() == 1, "Added one zone");
        check(mgr.zones()[0]->name() == "test_zone_1", "Zone name matches");

        auto z2 = std::make_shared<Zone>("test_zone_2", "strict");
        mgr.addZone(z2);
        check(mgr.zones().size() == 2, "Added second zone");

        mgr.switchTo(1);
        check(mgr.currentIndex() == 1, "Switched to index 1");
        check(mgr.active().name() == "test_zone_2", "Active zone is test_zone_2");
    }

    {
        std::cout << "\n[2] Testing home zone creation...\n";
        ZoneManager mgr;
        const std::string personId = "person_123";
        mgr.ensureHomeZone(personId);

        Zone* home = mgr.findPrimaryHome(personId);
        check(home != nullptr, "Primary home was created and found");
        if (home) {
            check(home->owner() == personId, "Home has correct owner");
            check(home->isPrimaryHome(), "Zone is marked as primary home");
            check(mgr.zones().size() == 1, "Home added to manager");
        }

        // Ensure idempotency
        mgr.ensureHomeZone(personId);
        check(mgr.zones().size() == 1, "Calling ensureHomeZone again does not duplicate");
    }

    {
        std::cout << "\n[3] Testing authorZone...\n";
        ZoneManager mgr;
        const std::string personId = "person_456";

        auto authored = mgr.authorZone("Authored_Zone", personId, "strict");
        check(authored != nullptr, "authorZone returned a valid zone");
        if (authored) {
            check(authored->name() == "Authored_Zone", "Authored zone has correct name");
            check(authored->owner() == personId, "Authored zone has correct owner");
            check(mgr.zones().size() == 1, "Authored zone added to manager");
        }
    }

    std::filesystem::remove_all(sandbox);

    std::cout << "------------------------------------------------------------\n";
    std::cout << g_checks - g_failures << "/" << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cerr << "zone_manager_test: FAILED\n";
        return 1;
    }
    std::cout << "zone_manager_test: ALL OK\n";
    return 0;
}
