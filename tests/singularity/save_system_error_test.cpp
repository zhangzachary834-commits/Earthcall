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

    // Verify no stray .tmp- files remain in sandbox
    bool hasTempFiles = false;
    std::error_code ec;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(sandbox, ec)) {
        if (entry.path().filename().string().find(".tmp-") != std::string::npos) {
            hasTempFiles = true;
            break;
        }
    }
    check(!hasTempFiles, "No temporary .tmp- files left behind after atomic write commits");

    // Test 6: Unpacking atomic directory staging and canonical zone resolution
    std::filesystem::path unpackedDir = sandbox / "unpacked_world";
    nlohmann::json fullSave = {
        {"saveFormat", "zone-identity-v1"},
        {"objects", {
            {{"identifier", "obj_1"}, {"type", "cube"}}
        }},
        {"authoredLaws", {
            {"laws", {
                {{"identifier", "law_1"}, {"name", "gravity"}}
            }}
        }},
        {"zones", {
            {{"identifier", "zone_1"}, {"name", "Display Name Zone 1"}}
        }},
        {"worldTime", 42.0}
    };

    check(SaveSystem::unpackSaveToDirectory(fullSave, unpackedDir.string()), "unpackSaveToDirectory succeeds on full save");

    check(std::filesystem::exists(unpackedDir / "objects" / "object_obj_1.json"), "Unpacked object file exists");
    check(std::filesystem::exists(unpackedDir / "authored_laws" / "law_law_1.json"), "Unpacked law file exists");
    check(std::filesystem::exists(unpackedDir / "zones" / "zone_zone_1.json"), "Unpacked zone file uses identifier over display name");
    check(std::filesystem::exists(unpackedDir / "world_meta.json"), "Unpacked world_meta.json exists");

    nlohmann::json recompiled = SaveSystem::compileSaveFromDirectory(unpackedDir.string());
    check(recompiled.contains("objects") && recompiled["objects"].size() == 1, "Compiled save contains objects");
    check(recompiled.contains("zones") && recompiled["zones"].size() == 1, "Compiled save contains zones");

    // Test 7: Stale unpacked file reconciliation prevents Zone duplication
    std::filesystem::path staleZonePath = unpackedDir / "zones" / "zone_Display_Name_Zone_1.json";
    {
        std::ofstream staleFile(staleZonePath);
        staleFile << "{ \"identifier\": \"zone_1\", \"name\": \"Display Name Zone 1\" }";
    }
    std::filesystem::remove(unpackedDir / ".unpack_manifest.json");
    check(std::filesystem::exists(staleZonePath), "Stale zone file manually injected into unpacked directory");

    check(SaveSystem::unpackSaveToDirectory(fullSave, unpackedDir.string()), "Re-unpacking save succeeds");
    check(!std::filesystem::exists(staleZonePath), "Stale zone file was purged during unpack reconciliation");
    check(std::filesystem::exists(unpackedDir / "zones" / "zone_zone_1.json"), "Canonical zone file remains");

    nlohmann::json recompiledClean = SaveSystem::compileSaveFromDirectory(unpackedDir.string());
    check(recompiledClean.contains("zones") && recompiledClean["zones"].size() == 1, "Recompiling unpacked directory produces exactly 1 Zone without duplicates");

    // Test 8: Unpack write failure propagation
    nlohmann::json blockedSave = fullSave;
    blockedSave["__test_block_meta_write"] = true;

    bool unpackSuccess = SaveSystem::unpackSaveToDirectory(blockedSave, (sandbox / "blocked_unpack").string());
    check(!unpackSuccess, "unpackSaveToDirectory returns false when an atomic write in staging fails");

    // Test 9: Preservation witness — late staging failure leaves prior generation byte-for-byte unchanged
    auto hashDirectoryTree = [](const std::filesystem::path& dir) -> std::string {
        std::string composite;
        std::error_code ec;
        std::vector<std::filesystem::path> paths;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(dir, ec)) {
            if (entry.is_regular_file(ec)) {
                paths.push_back(entry.path());
            }
        }
        std::sort(paths.begin(), paths.end());
        for (const auto& p : paths) {
            composite += std::filesystem::relative(p, dir, ec).string() + ":";
            std::ifstream in(p, std::ios::binary);
            composite += std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            composite += "|";
        }
        return composite;
    };

    std::filesystem::path prepopulatedDir = sandbox / "prepopulated_unpack";
    std::filesystem::create_directories(prepopulatedDir / "objects");
    std::filesystem::create_directories(prepopulatedDir / "zones");
    {
        std::ofstream o(prepopulatedDir / "objects" / "object_old_1.json");
        o << "{\"identifier\":\"old_1\",\"type\":\"cube\"}";
    }
    {
        std::ofstream z(prepopulatedDir / "zones" / "zone_Display_Name_Zone_1.json");
        z << "{\"identifier\":\"zone_1\",\"name\":\"Display Name Zone 1\"}";
    }
    {
        std::ofstream u(prepopulatedDir / "zones" / "zone_user_notes.json");
        u << "{\"notes\":\"user note\"}";
    }
    {
        std::ofstream m(prepopulatedDir / "world_meta.json");
        m << "{\"version\":1}";
    }

    std::string beforeHash = hashDirectoryTree(prepopulatedDir);

    bool failedUnpack = SaveSystem::unpackSaveToDirectory(blockedSave, prepopulatedDir.string());
    check(!failedUnpack, "unpackSaveToDirectory returns false on staged write failure");

    std::string afterHash = hashDirectoryTree(prepopulatedDir);
    check(afterHash == beforeHash, "Pre-existing generation remains byte-for-byte identical after a failed unpack");

    // Test 10: Zone id-only fallback resolution
    std::filesystem::path idOnlyDir = sandbox / "id_only_unpack";
    nlohmann::json idOnlySave = {
        {"zones", {
            {{"id", "legacy_zone_42"}, {"name", "Legacy Zone 42 Display Name"}}
        }}
    };
    check(SaveSystem::unpackSaveToDirectory(idOnlySave, idOnlyDir.string()), "id-only Zone unpack succeeds");
    check(std::filesystem::exists(idOnlyDir / "zones" / "zone_legacy_zone_42.json"), "Zone record carrying only 'id' resolves to canonical zone_id.json filename");

    // Test 11: Non-resurrection regression witness — deleted entities stay deleted across generations
    std::filesystem::path nonResurrectDir = sandbox / "non_resurrect_unpack";
    nlohmann::json saveGenA = {
        {"saveFormat", "zone-identity-v1"},
        {"zones", {
            {{"identifier", "zone_1"}, {"name", "Zone 1"}},
            {{"identifier", "zone_x"}, {"name", "Zone X"}}
        }}
    };
    check(SaveSystem::unpackSaveToDirectory(saveGenA, nonResurrectDir.string()), "Unpack generation A succeeds");
    check(std::filesystem::exists(nonResurrectDir / "zones" / "zone_zone_x.json"), "Zone X present in generation A");

    nlohmann::json saveGenB = {
        {"saveFormat", "zone-identity-v1"},
        {"zones", {
            {{"identifier", "zone_1"}, {"name", "Zone 1"}}
        }}
    };
    check(SaveSystem::unpackSaveToDirectory(saveGenB, nonResurrectDir.string()), "Unpack generation B succeeds");
    check(!std::filesystem::exists(nonResurrectDir / "zones" / "zone_zone_x.json"), "Deleted Zone X is NOT resurrected in generation B");

    // Test 12: Person-edited canonical entity file preservation witness
    std::filesystem::path personEditedDir = sandbox / "person_edited_unpack";
    check(SaveSystem::unpackSaveToDirectory(fullSave, personEditedDir.string()), "Initial unpack for Person editing succeeds");

    std::filesystem::path editedZonePath = personEditedDir / "zones" / "zone_zone_1.json";
    {
        std::ofstream editOut(editedZonePath);
        editOut << "{\n  \"identifier\": \"zone_1\",\n  \"name\": \"Display Name Zone 1\",\n  \"user_edit\": \"important_person_notes\"\n}\n";
    }

    nlohmann::json nextGenSave = fullSave;
    nextGenSave["worldTime"] = 100.0;
    check(SaveSystem::unpackSaveToDirectory(nextGenSave, personEditedDir.string()), "Unpack next generation over Person-edited directory succeeds");

    nlohmann::json preservedZoneJson;
    {
        std::ifstream in(editedZonePath);
        in >> preservedZoneJson;
    }
    check(preservedZoneJson.contains("user_edit") && preservedZoneJson["user_edit"] == "important_person_notes",
          "Person-modified canonical entity file is preserved and NOT overwritten by incoming generated version");

    // Test 13: Recursive arbitrary file/directory preservation witness
    std::filesystem::path arbitraryDir = sandbox / "arbitrary_files_unpack";
    check(SaveSystem::unpackSaveToDirectory(fullSave, arbitraryDir.string()), "Initial unpack for arbitrary file test succeeds");

    {
        std::ofstream noteFile(arbitraryDir / "notes.md");
        noteFile << "# My World Notes\nCustom notes written by Person.\n";
    }
    std::filesystem::create_directories(arbitraryDir / "textures" / "sub");
    {
        std::ofstream texFile(arbitraryDir / "textures" / "sub" / "icon.png");
        texFile << "PNG_BINARY_DATA";
    }

    check(SaveSystem::unpackSaveToDirectory(fullSave, arbitraryDir.string()), "Re-unpacking over arbitrary files succeeds");
    check(std::filesystem::exists(arbitraryDir / "notes.md"), "Top-level arbitrary file notes.md is preserved");
    check(std::filesystem::exists(arbitraryDir / "textures" / "sub" / "icon.png"), "Nested arbitrary file textures/sub/icon.png is preserved");

    // Test 14: Injected preservation copy failure aborts unpack and leaves live directory byte-for-byte unchanged
    std::filesystem::path failDir = sandbox / "preservation_copy_fail_unpack";
    std::filesystem::create_directories(failDir);
    {
        std::ofstream u(failDir / "blocked_user_file.txt");
        u << "critical user content";
    }
    std::string beforeFailHash = hashDirectoryTree(failDir);

    nlohmann::json failSave = fullSave;
    failSave["__test_block_preservation_copy"] = true;

    bool failUnpackSuccess = SaveSystem::unpackSaveToDirectory(failSave, failDir.string());
    check(!failUnpackSuccess, "unpackSaveToDirectory returns false when preservation file copy fails");

    std::string afterFailHash = hashDirectoryTree(failDir);
    check(afterFailHash == beforeFailHash, "Live directory tree remains byte-for-byte identical when preservation copy fails");

    // Test 15: Recursive empty user-authored directory preservation witness
    std::filesystem::path emptyDirUnpack = sandbox / "empty_dirs_unpack";
    check(SaveSystem::unpackSaveToDirectory(fullSave, emptyDirUnpack.string()), "Initial unpack for empty directory test succeeds");
    std::filesystem::create_directories(emptyDirUnpack / "user_sidecars" / "empty_sub");
    check(std::filesystem::is_directory(emptyDirUnpack / "user_sidecars" / "empty_sub"), "User created empty subdirectory");

    check(SaveSystem::unpackSaveToDirectory(fullSave, emptyDirUnpack.string()), "Re-unpacking over empty user directory succeeds");
    check(std::filesystem::is_directory(emptyDirUnpack / "user_sidecars" / "empty_sub"), "Empty user-authored directory structure is preserved");

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
