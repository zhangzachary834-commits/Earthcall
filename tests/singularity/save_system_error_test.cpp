#include "Singularity/Storage/SaveSystem.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>

int main() {
    auto sandbox = std::filesystem::temp_directory_path() / "earthcall_savesystem_error";
    std::filesystem::remove_all(sandbox);
    std::filesystem::create_directories(sandbox / "worlds");
    SaveSystem::setSaveRoot(sandbox.string());

    int checks = 0;
    int failures = 0;

    auto check = [&](bool condition, const std::string& desc) {
        checks++;
        if (!condition) {
            failures++;
            std::cerr << "  FAILED: " << desc << "\n";
        } else {
            std::cout << "  ok: " << desc << "\n";
        }
    };

    // Test 1: JSON parse error in readSaveData (fallback to plain JSON)
    std::string badJsonPath = (sandbox / "worlds" / "bad.json").string();
    std::ofstream out(badJsonPath);
    out << "{ \"malformed\": true, }";
    out.close();

    nlohmann::json parsed = SaveSystem::readSaveData(badJsonPath);
    check(parsed.empty(), "Malformed JSON returns empty json object and is caught by try/catch");

    // Test 2: MSGPack decompression exception in readSaveData (.ecsave)
    std::string badEcsavePath = (sandbox / "worlds" / "bad.ecsave").string();
    std::ofstream eout(badEcsavePath, std::ios::binary);
    size_t smallSize = 10;
    eout.write(reinterpret_cast<const char*>(&smallSize), sizeof(smallSize));
    eout << "Some payload";
    eout.close();

    nlohmann::json parsedEcsave = SaveSystem::readSaveData(badEcsavePath);
    check(parsedEcsave.empty(), "Malformed .ecsave msgpack returns empty json object and is caught by try/catch");

    // Test 3: readMatterData returns the raw bytes (not throwing on malformed matter)
    std::string badMatterPath = (sandbox / "worlds" / "bad.ecmatter").string();
    std::ofstream mout(badMatterPath, std::ios::binary);
    mout << "Not compressed data at all";
    mout.close();

    std::vector<uint8_t> parsedMatter = SaveSystem::readMatterData(badMatterPath);
    check(!parsedMatter.empty(), "Malformed .ecmatter returns raw uncompressed bytes as expected");

    // Test 4: Atomic write-before-commit for writeSaveData (JSON)
    nlohmann::json testWorld = {{"name", "AtomicWorld"}, {"version", 1}};
    std::string writtenPath = SaveSystem::writeSaveData(testWorld, "atomic_world", SaveSystem::SaveType::WORLD);
    check(!writtenPath.empty() && std::filesystem::exists(writtenPath), "writeSaveData creates atomic .ecform file");

    nlohmann::json readWorld = SaveSystem::readSaveData(writtenPath);
    check(readWorld.contains("version") && readWorld["version"] == 1, "writeSaveData contents match");

    // Overwrite with updated version
    testWorld["version"] = 2;
    std::string updatedPath = SaveSystem::writeSaveData(testWorld, "atomic_world", SaveSystem::SaveType::WORLD);
    nlohmann::json reloadedWorld = SaveSystem::readSaveData(updatedPath);
    check(reloadedWorld.contains("version") && reloadedWorld["version"] == 2, "Atomic write cleanly overwrites existing save file");

    // Test 5: Atomic write-before-commit for writeZoneIdentity and writeHomeIdentity
    nlohmann::json zoneDoc = {{"identifier", "AtomicZone"}, {"active", true}};
    check(SaveSystem::writeZoneIdentity("AtomicZone", zoneDoc), "writeZoneIdentity succeeds atomically");
    nlohmann::json readZone = SaveSystem::readZoneIdentity("AtomicZone");
    check(readZone.contains("active") && readZone["active"] == true, "readZoneIdentity matches written atomic zone document");

    nlohmann::json homeDoc = {{"identifier", "AtomicHome"}, {"active", true}};
    check(SaveSystem::writeHomeIdentity("AtomicHome", homeDoc), "writeHomeIdentity succeeds atomically");
    nlohmann::json readHome = SaveSystem::readHomeIdentity("AtomicHome");
    check(readHome.contains("active") && readHome["active"] == true, "readHomeIdentity matches written atomic home document");

    // Test 6: Atomic unpackSaveToDirectory via temporary directory staging
    nlohmann::json unpackWorld = {
        {"name", "UnpackWorld"},
        {"version", 1},
        {"objects", nlohmann::json::array({{{"id", "obj1"}, {"kind", "Cube"}}})},
        {"zones", nlohmann::json::array({{{"name", "Zone1"}}})}
    };
    std::string unpackTarget = (sandbox / "worlds" / "unpacked_test").string();
    SaveSystem::unpackSaveToDirectory(unpackWorld, unpackTarget);
    check(std::filesystem::exists(unpackTarget + "/world_meta.json"), "unpackSaveToDirectory produces world_meta.json");
    check(std::filesystem::exists(unpackTarget + "/objects/object_obj1.json"), "unpackSaveToDirectory produces object JSON");
    check(std::filesystem::exists(unpackTarget + "/zones/zone_Zone1.json"), "unpackSaveToDirectory produces zone JSON");

    // Verify no stray .tmp- or .tmp_unpack- files/directories remain in sandbox
    bool hasTempFiles = false;
    std::error_code ec;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(sandbox, ec)) {
        std::string fname = entry.path().filename().string();
        if (fname.find(".tmp-") != std::string::npos || fname.find(".tmp_unpack-") != std::string::npos) {
            hasTempFiles = true;
            break;
        }
    }
    check(!hasTempFiles, "No temporary .tmp- or .tmp_unpack- files left behind after atomic write commits");

    // Cleanup
    std::filesystem::remove_all(sandbox);
    SaveSystem::setSaveRoot("");

    std::cout << "------------------------------------------------------------\n";
    std::cout << checks - failures << "/" << checks << " checks passed\n";
    if (failures > 0) {
        std::cout << "save_system_error_test: FAILED\n";
        return 1;
    }
    std::cout << "save_system_error_test: ALL OK\n";
    return 0;
}
