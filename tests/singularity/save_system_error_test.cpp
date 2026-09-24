#include "Singularity/Storage/SaveSystem.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <algorithm>

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

    auto snapshotTree = [](const std::filesystem::path& dir) {
        std::vector<std::string> records;
        std::error_code ec;
        if (!std::filesystem::exists(dir, ec)) return records;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(dir, ec)) {
            if (ec) break;
            const auto rel = std::filesystem::relative(entry.path(), dir, ec).generic_string();
            if (ec) break;
            if (entry.is_directory(ec)) {
                records.push_back("D:" + rel);
            } else if (entry.is_regular_file(ec)) {
                std::ifstream in(entry.path(), std::ios::binary);
                std::string bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
                records.push_back("F:" + rel + ":" + bytes);
            }
        }
        std::sort(records.begin(), records.end());
        return records;
    };

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

    // Test 6: transactional unpack + stable Zone identity filename.
    std::filesystem::path unpackedDir = sandbox / "unpacked_world";
    check(SaveSystem::unpackSaveToDirectory(fullSave, unpackedDir.string()),
          "unpackSaveToDirectory succeeds on a full save");
    check(std::filesystem::exists(unpackedDir / "objects" / "object_obj_1.json"),
          "Unpacked object file exists");
    check(std::filesystem::exists(unpackedDir / "authored_laws" / "law_law_1.json"),
          "Unpacked law file exists");
    check(std::filesystem::exists(unpackedDir / "zones" / "zone_zone_1.json"),
          "Unpacked Zone filename uses the canonical Zone identity");
    check(std::filesystem::exists(unpackedDir / ".unpack_manifest.json"),
          "Unpacked authoring tree carries an ownership manifest");

    nlohmann::json recompiled = SaveSystem::compileSaveFromDirectory(unpackedDir.string());
    check(recompiled.contains("zones") && recompiled["zones"].size() == 1,
          "Compiled unpacked save contains exactly one Zone");

    // Test 7: migrate the historical name-based Zone filename only when its
    // meaning exactly matches the incoming canonical Zone.
    std::filesystem::path legacyDir = sandbox / "legacy_alias_unpack";
    check(SaveSystem::unpackSaveToDirectory(fullSave, legacyDir.string()),
          "Initial unpack for legacy alias fixture succeeds");
    std::filesystem::remove(legacyDir / ".unpack_manifest.json");
    const auto canonicalZone = legacyDir / "zones" / "zone_zone_1.json";
    const auto legacyZone = legacyDir / "zones" / "zone_Display_Name_Zone_1.json";
    std::filesystem::rename(canonicalZone, legacyZone);
    check(SaveSystem::unpackSaveToDirectory(fullSave, legacyDir.string()),
          "Re-unpack reconciles an exact legacy name-based alias");
    check(!std::filesystem::exists(legacyZone) && std::filesystem::exists(canonicalZone),
          "Legacy alias is replaced by exactly one canonical Zone file");
    nlohmann::json legacyRecompiled = SaveSystem::compileSaveFromDirectory(legacyDir.string());
    check(legacyRecompiled.contains("zones") && legacyRecompiled["zones"].size() == 1,
          "Legacy alias migration cannot duplicate the Zone on recompile");

    // Test 8: a late generated-file failure never publishes a partial directory.
    std::filesystem::path blockedDir = sandbox / "blocked_unpack";
    nlohmann::json blockedSave = fullSave;
    blockedSave["__test_block_meta_write"] = true;
    check(!SaveSystem::unpackSaveToDirectory(blockedSave, blockedDir.string()),
          "Unpack reports failure when staged world_meta cannot commit");
    check(!std::filesystem::exists(blockedDir),
          "A failed fresh unpack does not publish its staged child files");

    // Test 9: failure against a pre-existing generation leaves every byte and
    // empty-directory marker unchanged.
    std::filesystem::path priorDir = sandbox / "prior_generation_unpack";
    check(SaveSystem::unpackSaveToDirectory(fullSave, priorDir.string()),
          "Initial generation for rollback witness succeeds");
    {
        std::ofstream note(priorDir / "notes.md");
        note << "Person note before failed generation\n";
    }
    std::filesystem::create_directories(priorDir / "empty-person-dir");
    const auto beforeFailedGeneration = snapshotTree(priorDir);
    nlohmann::json changedButBlocked = fullSave;
    changedButBlocked["worldTime"] = 99.0;
    changedButBlocked["__test_block_meta_write"] = true;
    check(!SaveSystem::unpackSaveToDirectory(changedButBlocked, priorDir.string()),
          "Failed replacement generation reports false");
    check(snapshotTree(priorDir) == beforeFailedGeneration,
          "Failed replacement leaves the prior authoring generation byte-for-byte unchanged");

    // Test 10: generated entities intentionally absent from the next generation
    // stay deleted, while never-owned user content survives.
    std::filesystem::path deletionDir = sandbox / "deletion_unpack";
    nlohmann::json generationA = {
        {"saveFormat", "zone-identity-v1"},
        {"zones", {
            {{"identifier", "zone_1"}, {"name", "Zone 1"}},
            {{"identifier", "zone_x"}, {"name", "Zone X"}}
        }}
    };
    check(SaveSystem::unpackSaveToDirectory(generationA, deletionDir.string()),
          "Generation A with Zone X unpacks");
    {
        std::ofstream note(deletionDir / "notes.md");
        note << "keep me\n";
    }
    nlohmann::json generationB = {
        {"saveFormat", "zone-identity-v1"},
        {"zones", {
            {{"identifier", "zone_1"}, {"name", "Zone 1"}}
        }}
    };
    check(SaveSystem::unpackSaveToDirectory(generationB, deletionDir.string()),
          "Generation B without Zone X unpacks");
    check(!std::filesystem::exists(deletionDir / "zones" / "zone_zone_x.json"),
          "Previously generated Zone X is not resurrected");
    check(std::filesystem::exists(deletionDir / "notes.md"),
          "Never-owned user note survives generated-entity deletion");

    // Test 11: once a Person changes bytes at a previously generated canonical
    // path, the unpacker preserves those bytes and relinquishes ownership.
    std::filesystem::path personDir = sandbox / "person_edited_unpack";
    check(SaveSystem::unpackSaveToDirectory(fullSave, personDir.string()),
          "Initial unpack for Person-edit witness succeeds");
    const auto personZonePath = personDir / "zones" / "zone_zone_1.json";
    {
        std::ofstream edit(personZonePath);
        edit << "{\n"
             << "  \"identifier\": \"zone_1\",\n"
             << "  \"name\": \"Display Name Zone 1\",\n"
             << "  \"user_edit\": \"important_person_bytes\"\n"
             << "}\n";
    }
    nlohmann::json incomingChanged = fullSave;
    incomingChanged["zones"][0]["name"] = "Incoming Changed Display";
    check(SaveSystem::unpackSaveToDirectory(incomingChanged, personDir.string()),
          "Re-unpack preserves a Person-modified canonical Zone file");
    nlohmann::json preservedZone;
    {
        std::ifstream in(personZonePath);
        in >> preservedZone;
    }
    check(preservedZone.value("user_edit", std::string{}) == "important_person_bytes",
          "Person-modified canonical bytes are not overwritten by incoming generated bytes");
    nlohmann::json personManifest;
    {
        std::ifstream in(personDir / ".unpack_manifest.json");
        in >> personManifest;
    }
    bool stillClaimsPersonZone = false;
    if (personManifest.contains("ownedFiles")) {
        for (const auto& item : personManifest["ownedFiles"]) {
            if (item.is_string() && item.get<std::string>() == "zones/zone_zone_1.json") {
                stillClaimsPersonZone = true;
            }
        }
    }
    check(!stillClaimsPersonZone,
          "Manifest relinquishes ownership of a canonical file after Person modification");

    // Test 12: preservation is recursive, including arbitrary files and empty dirs.
    std::filesystem::path recursiveDir = sandbox / "recursive_preservation_unpack";
    check(SaveSystem::unpackSaveToDirectory(fullSave, recursiveDir.string()),
          "Initial unpack for recursive preservation succeeds");
    {
        std::ofstream note(recursiveDir / "notes.md");
        note << "# authored notes\n";
    }
    std::filesystem::create_directories(recursiveDir / "textures" / "sub");
    {
        std::ofstream blob(recursiveDir / "textures" / "sub" / "icon.bin", std::ios::binary);
        blob << "BINARY_PERSON_DATA";
    }
    std::filesystem::create_directories(recursiveDir / "empty-authored-folder");
    check(SaveSystem::unpackSaveToDirectory(fullSave, recursiveDir.string()),
          "Re-unpack over recursive user content succeeds");
    check(std::filesystem::exists(recursiveDir / "notes.md"),
          "Top-level arbitrary authoring file survives");
    check(std::filesystem::exists(recursiveDir / "textures" / "sub" / "icon.bin"),
          "Nested arbitrary authoring file survives");
    check(std::filesystem::is_directory(recursiveDir / "empty-authored-folder"),
          "Empty user-authored directory survives the generation swap");

    // Test 13: preservation-copy failure is fail-closed before the live swap.
    std::filesystem::path copyFailureDir = sandbox / "copy_failure_unpack";
    check(SaveSystem::unpackSaveToDirectory(fullSave, copyFailureDir.string()),
          "Initial unpack for preservation-copy failure witness succeeds");
    {
        std::ofstream note(copyFailureDir / "notes.md");
        note << "do not lose this\n";
    }
    const auto beforeCopyFailure = snapshotTree(copyFailureDir);
    nlohmann::json copyBlocked = fullSave;
    copyBlocked["worldTime"] = 123.0;
    copyBlocked["__test_fail_preserve_copy"] = "notes.md";
    check(!SaveSystem::unpackSaveToDirectory(copyBlocked, copyFailureDir.string()),
          "Injected preservation-copy failure refuses the generation");
    check(snapshotTree(copyFailureDir) == beforeCopyFailure,
          "Preservation-copy failure leaves the live authoring tree exactly unchanged");

    // Test 14: deleting a previously generated file is also Person authorship.
    // Re-unpacking the same monolith must not silently resurrect it.
    std::filesystem::path personDeletedDir = sandbox / "person_deleted_unpack";
    check(SaveSystem::unpackSaveToDirectory(fullSave, personDeletedDir.string()),
          "Initial unpack for Person-deletion witness succeeds");
    const auto deletedZonePath = personDeletedDir / "zones" / "zone_zone_1.json";
    check(std::filesystem::remove(deletedZonePath),
          "Person deletion removes the previously generated canonical Zone file");
    check(SaveSystem::unpackSaveToDirectory(fullSave, personDeletedDir.string()),
          "Re-unpack after Person deletion succeeds without resurrecting the file");
    check(!std::filesystem::exists(deletedZonePath),
          "Person-deleted canonical Zone file stays absent after re-unpack");

    nlohmann::json deletedManifest;
    {
        std::ifstream in(personDeletedDir / ".unpack_manifest.json");
        in >> deletedManifest;
    }
    bool claimsDeletedZone = false;
    if (deletedManifest.contains("ownedFiles")) {
        for (const auto& item : deletedManifest["ownedFiles"]) {
            if (item.is_string() && item.get<std::string>() == "zones/zone_zone_1.json") {
                claimsDeletedZone = true;
            }
        }
    }
    check(!claimsDeletedZone,
          "Manifest relinquishes ownership of a Person-deleted canonical file");

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
