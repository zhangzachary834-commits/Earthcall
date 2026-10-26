// Stale sidecar cleanup and identity path resolution test.
//
// Verifies that:
// 1. SaveSystem::resolveZoneIdentityPath and resolveHomeIdentityPath accurately
//    resolve .ecsave, .ecform, then legacy .json, including catalog discovery.
// 2. SaveSystem::writeZoneIdentity and writeHomeIdentity automatically remove any
//    legacy fixed-name zone.json, home.json, zone.ecsave and home.ecsave sidecars
//    upon successfully writing the .ecform binary identity file.

#include "Singularity/Storage/SaveSystem.hpp"
#include "json.hpp"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

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
    auto catalogNames = [](bool homes) {
        std::vector<std::string> names;
        const auto records = homes ? SaveSystem::listHomeIdentityRecords()
                                   : SaveSystem::listZoneIdentityRecords();
        for (const auto& rec : records)
            names.push_back(rec.document.is_object() ? rec.document.value("name", "") : "");
        return names;
    };
    auto identityName = [](const nlohmann::json& doc) {
        return doc.is_object() ? doc.value("name", "") : "";
    };

    const std::string zoneId = "CleanupZone";
    const std::string homeId = "CleanupHome";

    const std::filesystem::path zoneDir = sandbox.path / "zones" / zoneId;
    const std::filesystem::path homeDir = sandbox.path / "homes" / homeId;

    std::filesystem::create_directories(zoneDir);
    std::filesystem::create_directories(homeDir);

    const std::filesystem::path legacyZoneJson = zoneDir / "zone.json";
    const std::filesystem::path legacyHomeJson = homeDir / "home.json";
    const std::filesystem::path legacyZoneEcsave = zoneDir / "zone.ecsave";
    const std::filesystem::path legacyHomeEcsave = homeDir / "home.ecsave";

    auto writeMsgpack = [](const std::filesystem::path& path, const nlohmann::json& doc) {
        const auto bytes = nlohmann::json::to_msgpack(doc);
        std::ofstream out(path, std::ios::binary);
        out.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    };

    // First verify the original JSON-only fallback before introducing .ecsave.
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
    check(SaveSystem::resolveZoneIdentityPath(zoneId) == legacyZoneJson.string(),
          "Zone JSON-only fallback resolves before binary identity creation");
    check(SaveSystem::resolveHomeIdentityPath(homeId) == legacyHomeJson.string(),
          "Home JSON-only fallback resolves before binary identity creation");
    check(identityName(SaveSystem::readZoneIdentity(zoneId)) == "Legacy Zone",
          "Zone direct read uses JSON-only fallback");
    check(identityName(SaveSystem::readHomeIdentity(homeId)) == "Legacy Home",
          "Home direct read uses JSON-only fallback");
    check(catalogNames(false) == std::vector<std::string>{"Legacy Zone"},
          "Zone catalog discovers JSON-only identity");
    check(catalogNames(true) == std::vector<std::string>{"Legacy Home"},
          "Home catalog discovers JSON-only identity");

    // Now introduce legacy binary sidecars and verify they take precedence.
    writeMsgpack(legacyZoneEcsave, nlohmann::json{{"identifier", zoneId}, {"name", "Legacy Zone"}});
    writeMsgpack(legacyHomeEcsave, nlohmann::json{{"identifier", homeId}, {"name", "Legacy Home"}});
    check(std::filesystem::exists(legacyZoneEcsave), "Legacy zone.ecsave created in sandbox");
    check(std::filesystem::exists(legacyHomeEcsave), "Legacy home.ecsave created in sandbox");

    // Before .ecform exists, a nonempty .ecsave takes precedence over .json.
    check(SaveSystem::resolveZoneIdentityPath(zoneId) == legacyZoneEcsave.string(),
          "resolveZoneIdentityPath resolves legacy zone.ecsave when zone.ecform does not exist");
    check(SaveSystem::resolveHomeIdentityPath(homeId) == legacyHomeEcsave.string(),
          "resolveHomeIdentityPath resolves legacy home.ecsave when home.ecform does not exist");
    // Direct identity reads and catalog discovery must agree on binary sidecar precedence.
    check(identityName(SaveSystem::readZoneIdentity(zoneId)) == "Legacy Zone",
          "Zone direct read resolves legacy zone.ecsave before .ecform creation");
    check(identityName(SaveSystem::readHomeIdentity(homeId)) == "Legacy Home",
          "Home direct read resolves legacy home.ecsave before .ecform creation");

    check(catalogNames(false) == std::vector<std::string>{"Legacy Zone"},
          "Zone catalog discovers legacy .ecsave identity");
    check(catalogNames(true) == std::vector<std::string>{"Legacy Home"},
          "Home catalog discovers legacy .ecsave identity");

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

    // Assert legacy fixed-name .json and .ecsave sidecars were deleted automatically
    check(!std::filesystem::exists(legacyZoneJson), "Stale legacy zone.json sidecar was removed");
    check(!std::filesystem::exists(legacyHomeJson), "Stale legacy home.json sidecar was removed");
    check(!std::filesystem::exists(legacyZoneEcsave), "Stale legacy zone.ecsave sidecar was removed");
    check(!std::filesystem::exists(legacyHomeEcsave), "Stale legacy home.ecsave sidecar was removed");

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

    check(catalogNames(false) == std::vector<std::string>{"Cleanup Zone"},
          "Zone catalog discovers an existing .ecform-only identity");
    check(catalogNames(true) == std::vector<std::string>{"Cleanup Home"},
          "Home catalog discovers an existing .ecform-only identity");

    // Model the mixed-format store during migration, without rewriting its old
    // identities. A newer file wins, but an empty newer file must not hide them.
    const auto zoneEcsave = zoneDir / "zone.ecsave";
    const auto homeEcsave = homeDir / "home.ecsave";
    auto fileBytes = [](const std::filesystem::path& path) {
        std::ifstream in(path, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    };
    const auto oldZoneBytes = fileBytes(zoneEcform);
    const auto oldHomeBytes = fileBytes(homeEcform);
    zoneDoc["name"] = "New Zone";
    homeDoc["name"] = "New Home";
    writeMsgpack(zoneEcsave, zoneDoc);
    writeMsgpack(homeEcsave, homeDoc);
    check(SaveSystem::resolveZoneIdentityPath(zoneId) == zoneEcsave.string(),
          "Zone path prefers a nonempty .ecsave over .ecform");
    check(SaveSystem::resolveHomeIdentityPath(homeId) == homeEcsave.string(),
          "Home path prefers a nonempty .ecsave over .ecform");
    check(catalogNames(false) == std::vector<std::string>{"New Zone"},
          "Zone catalog uses the newer .ecsave identity once");
    check(catalogNames(true) == std::vector<std::string>{"New Home"},
          "Home catalog uses the newer .ecsave identity once");
    check(identityName(SaveSystem::readZoneIdentity(zoneId)) == "New Zone",
          "Zone direct read agrees with catalog precedence");
    check(identityName(SaveSystem::readHomeIdentity(homeId)) == "New Home",
          "Home direct read agrees with catalog precedence");

    std::ofstream(zoneEcsave, std::ios::binary).close();
    std::ofstream(homeEcsave, std::ios::binary).close();
    check(SaveSystem::resolveZoneIdentityPath(zoneId) == zoneEcform.string(),
          "Empty .ecsave does not hide the existing Zone path");
    check(SaveSystem::resolveHomeIdentityPath(homeId) == homeEcform.string(),
          "Empty .ecsave does not hide the existing Home path");
    check(catalogNames(false) == std::vector<std::string>{"Cleanup Zone"},
          "Zone catalog falls back to .ecform when .ecsave is empty");
    check(catalogNames(true) == std::vector<std::string>{"Cleanup Home"},
          "Home catalog falls back to .ecform when .ecsave is empty");
    check(identityName(SaveSystem::readZoneIdentity(zoneId)) == "Cleanup Zone",
          "Zone direct read falls back when .ecsave is empty");
    check(identityName(SaveSystem::readHomeIdentity(homeId)) == "Cleanup Home",
          "Home direct read falls back when .ecsave is empty");
    check(fileBytes(zoneEcform) == oldZoneBytes && fileBytes(homeEcform) == oldHomeBytes,
          "Discovery and reads preserve both old binary files byte for byte");

    std::cout << "------------------------------------------------------------\n";
    std::cout << g_checks - g_failures << "/" << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cout << "stale_sidecar_cleanup_test: FAILED\n";
        return 1;
    }
    std::cout << "stale_sidecar_cleanup_test: ALL OK\n";
    return 0;
}
