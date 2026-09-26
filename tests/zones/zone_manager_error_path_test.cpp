// WITNESS DOCUMENTATION:
// What the old test could prove:
//   That calling ZoneManager::switchTo(0) on an in-memory Zone with no lawRefs
//   returns without throwing an unhandled exception to the caller.
// What the old test could NOT prove:
//   That ZoneManager::switchTo handles actual Zone activation errors (such as missing
//   Law roots or malformed lawRefs in a Zone's identity) by catching the exception,
//   logging the refusal, returning false, and leaving the current active Zone intact.
//   It could not prove this because ZoneManager::switchTo NEVER calls Zone::load().
//   The FailingZone::load() override was dead code that was never executed, meaning
//   no exception was ever thrown or caught during the old test's switchTo(0) call.

#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "json.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>

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

class FailingZone : public Zone {
public:
    FailingZone(const std::string& name) : Zone(name, "strict") {}

    // Override load (legacy unused Zone method kept for historical test structure)
    void load() override {
        throw std::runtime_error("Simulated zone load failure");
    }
};

int main() {
    std::cout << "============================================================\n";
    std::cout << "Running zone_manager_error_path_test...\n";
    std::cout << "============================================================\n";

    // ------------------------------------------------------------------
    // Legacy test structure: in-memory Zone activation with 0 lawRefs
    // ------------------------------------------------------------------
    ZoneManager mgr;
    auto failZone = std::make_shared<FailingZone>("FailingZone");
    mgr.addZone(failZone);

    bool threw = false;
    try {
        mgr.switchTo(0);
    } catch (...) {
        threw = true;
    }

    check(!threw, "switchTo(0) on in-memory Zone returned without throwing unhandled exception");

    // ------------------------------------------------------------------
    // Real path test: ZoneManager::switchTo error handling when a Zone's
    // identity references a missing Law root.
    // ------------------------------------------------------------------
    auto sandbox = std::filesystem::temp_directory_path() / "earthcall_zone_manager_error_path";
    std::filesystem::remove_all(sandbox);
    std::filesystem::create_directories(sandbox / "zones" / "BrokenZone");
    SaveSystem::setSaveRoot(sandbox.string());

    ZoneManager realMgr;

    // Active Zone 0: ValidZone
    auto validZone = std::make_shared<Zone>("ValidZone", "strict");
    realMgr.addZone(validZone);
    check(realMgr.switchTo(0), "ValidZone activated successfully");
    check(realMgr.currentIndex() == 0, "ValidZone is at index 0");
    check(realMgr.active().getIdentifier() == "ValidZone", "ValidZone is the active zone");

    // Broken Zone 1: BrokenZone (identity file references non-existent law root)
    nlohmann::json brokenIdentity;
    brokenIdentity["identifier"] = "BrokenZone";
    brokenIdentity["name"] = "BrokenZone";
    brokenIdentity["lawRefs"] = nlohmann::json::array({"law-non-existent-missing"});

    std::ofstream out(sandbox / "zones" / "BrokenZone" / "zone.json");
    out << brokenIdentity.dump(2);
    out.close();

    auto brokenZone = std::make_shared<Zone>("BrokenZone", "strict");
    realMgr.addZone(brokenZone);

    // Activating BrokenZone should hit the real try-catch in switchTo, log the refusal,
    // return false, and keep ValidZone (index 0) active.
    bool switchSuccess = realMgr.switchTo(1);
    check(!switchSuccess, "switchTo(1) refused activation for BrokenZone due to missing Law root");
    check(realMgr.currentIndex() == 0, "active zone index remains 0 (ValidZone) after refused activation");
    check(realMgr.active().getIdentifier() == "ValidZone", "active zone remains ValidZone after refused activation");

    std::filesystem::remove_all(sandbox);
    SaveSystem::setSaveRoot("");

    std::cout << "------------------------------------------------------------\n";
    std::cout << g_checks - g_failures << "/" << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cerr << "zone_manager_error_path_test: FAILED\n";
        return 1;
    }
    std::cout << "zone_manager_error_path_test: ALL OK\n";
    return 0;
}
