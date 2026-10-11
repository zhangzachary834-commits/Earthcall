// Stale sidecar cleanup and identity path resolution test.
//
// Verifies that:
// 1. SaveSystem::resolveZoneIdentityPath and resolveHomeIdentityPath accurately
//    resolve .ecform when present, falling back to legacy .json if present.
// 2. SaveSystem::writeZoneIdentity and writeHomeIdentity automatically remove any
//    legacy fixed-name zone.json or home.json sidecars in the zone/home directory
//    upon successfully writing the .ecform binary identity file.

#include "Singularity/Storage/SaveSystem.hpp"
#include "json.hpp"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
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

struct TempSaveSandbox {
    std::filesystem::path path;
    std::string previousRoot;

    TempSaveSandbox() {
        previousRoot = SaveSystem::saveRoot();
        path = std::filesystem::temp_directory_path() /
               ("earthcall-stale-sidecar-test-" +
                std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(path);
        SaveSystem::setSaveRoot(path.string());
    }

    ~TempSaveSandbox() {
        SaveSystem::setSaveRoot(previousRoot);
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};

} // namespace

int main() {
    std::cout << "============================================================\n";
    std::cout << "Running stale sidecar cleanup & identity path resolution test...\n";
    std::cout << "============================================================\n";

    TempSaveSandbox sandbox;

    const std::string zoneId = "CleanupZone";
    const std::string homeId = "CleanupHome";

    const std::filesystem::path zoneDir = sandbox.path / "zones" / zoneId;
    const std::filesystem::path homeDir = sandbox.path / "homes" / homeId;

    std::filesystem::create_directories(zoneDir);
    std::filesystem::create_directories(homeDir);

    const std::filesystem::path legacyZoneJson = zoneDir / "zone.json";
    const std::filesystem::path legacyHomeJson = homeDir / "home.json";

    // Write legacy .json sidecars
    {
        std::ofstream zOut(legacyZoneJson);
        zOut << nlohmann::json{{"identifier", zoneId}, {"name", "Legacy Zone"}}.dump(2);
    }
    {
        std::ofstream hOut(legacyHomeJson);
        hOut << nlohmann::json{{"identifier", homeId}, {"name", "Legacy Home"}}.dump(2);
    }

    check(std::filesystem::exists(legacyZoneJson), "Legacy zone.json created in sandbox");
    check(std::filesystem::exists(legacyHomeJson), "Legacy home.json created in sandbox");

    // Before .ecform exists, resolveZoneIdentityPath/resolveHomeIdentityPath should resolve to .json
    check(SaveSystem::resolveZoneIdentityPath(zoneId) == legacyZoneJson.string(),
          "resolveZoneIdentityPath resolves legacy zone.json when zone.ecform does not exist");
    check(SaveSystem::resolveHomeIdentityPath(homeId) == legacyHomeJson.string(),
          "resolveHomeIdentityPath resolves legacy home.json when home.ecform does not exist");

    // Verify readZoneIdentity and readHomeIdentity correctly load legacy .json prior to .ecform creation
    nlohmann::json legacyZoneRead = SaveSystem::readZoneIdentity(zoneId);
    nlohmann::json legacyHomeRead = SaveSystem::readHomeIdentity(homeId);
    check(legacyZoneRead.value("name", "") == "Legacy Zone",
          "readZoneIdentity reads legacy zone.json before zone.ecform exists");
    check(legacyHomeRead.value("name", "") == "Legacy Home",
          "readHomeIdentity reads legacy home.json before home.ecform exists");

    // Write primary .ecform identities via writeZoneIdentity and writeHomeIdentity
    nlohmann::json zoneDoc{
        {"identifier", zoneId},
        {"name", "Cleanup Zone"},
        {"qualities", {{"kind", "zone"}}},
        {"world", {{"objects", nlohmann::json::array()}}}
    };
    nlohmann::json homeDoc{
        {"identifier", homeId},
        {"name", "Cleanup Home"},
        {"qualities", {{"kind", "home"}}},
        {"world", {{"objects", nlohmann::json::array()}}}
    };

    check(SaveSystem::writeZoneIdentity(zoneId, zoneDoc), "writeZoneIdentity succeeded");
    check(SaveSystem::writeHomeIdentity(homeId, homeDoc), "writeHomeIdentity succeeded");

    const std::filesystem::path zoneEcform = zoneDir / "zone.ecform";
    const std::filesystem::path homeEcform = homeDir / "home.ecform";

    check(std::filesystem::exists(zoneEcform), "Primary zone.ecform created");
    check(std::filesystem::exists(homeEcform), "Primary home.ecform created");

    // Assert legacy fixed-name .json sidecars were deleted automatically
    check(!std::filesystem::exists(legacyZoneJson), "Stale legacy zone.json sidecar was removed");
    check(!std::filesystem::exists(legacyHomeJson), "Stale legacy home.json sidecar was removed");

    // Assert resolveZoneIdentityPath / resolveHomeIdentityPath now resolve to .ecform
    check(SaveSystem::resolveZoneIdentityPath(zoneId) == zoneEcform.string(),
          "resolveZoneIdentityPath resolves zone.ecform");
    check(SaveSystem::resolveHomeIdentityPath(homeId) == homeEcform.string(),
          "resolveHomeIdentityPath resolves home.ecform");

    // Verify readZoneIdentity and readHomeIdentity read the updated documents cleanly
    nlohmann::json readZone = SaveSystem::readZoneIdentity(zoneId);
    nlohmann::json readHome = SaveSystem::readHomeIdentity(homeId);

    check(readZone.value("name", "") == "Cleanup Zone", "readZoneIdentity accurately reads updated .ecform");
    check(readHome.value("name", "") == "Cleanup Home", "readHomeIdentity accurately reads updated .ecform");

    std::cout << "------------------------------------------------------------\n";
    std::cout << g_checks - g_failures << "/" << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cout << "stale_sidecar_cleanup_test: FAILED\n";
        return 1;
    }
    std::cout << "stale_sidecar_cleanup_test: ALL OK\n";
    return 0;
}
