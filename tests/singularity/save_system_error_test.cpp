#include "Singularity/Storage/SaveSystem.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <algorithm>
#include <sstream>

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

    auto treeFingerprint = [](const std::filesystem::path& dir) {
        std::vector<std::string> entries;
        std::error_code ec;
        if (!std::filesystem::exists(dir, ec)) return std::string{"<missing>"};
        for (const auto& entry : std::filesystem::recursive_directory_iterator(dir, ec)) {
            if (ec) break;
            const std::string rel =
                entry.path().lexically_relative(dir).generic_string();
            if (entry.is_directory(ec)) {
                entries.push_back("D:" + rel);
            } else if (entry.is_regular_file(ec)) {
                std::ifstream in(entry.path(), std::ios::binary);
                std::string bytes((std::istreambuf_iterator<char>(in)),
                                  std::istreambuf_iterator<char>());
                entries.push_back("F:" + rel + ":" + bytes);
            }
        }
        std::sort(entries.begin(), entries.end());
        std::ostringstream out;
        for (const auto& entry : entries) out << entry << "\n";
        return out.str();
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

    // Test 6: whole-generation staging + canonical Zone identity.
    std::filesystem::path unpackedDir = sandbox / "unpacked_world";
    check(SaveSystem::unpackSaveToDirectory(fullSave, unpackedDir.string()),
          "Transactional unpack succeeds");
    check(std::filesystem::exists(unpackedDir / "objects" / "object_obj_1.json"),
          "Unpacked object exists");
    check(std::filesystem::exists(unpackedDir / "authored_laws" / "law_law_1.json"),
          "Unpacked authored Law exists");
    check(std::filesystem::exists(unpackedDir / "zones" / "zone_zone_1.json"),
          "Zone filename uses canonical identifier rather than display name");
    check(std::filesystem::exists(unpackedDir / ".unpack_manifest.json"),
          "Unpack ownership manifest exists");

    nlohmann::json recompiled = SaveSystem::compileSaveFromDirectory(unpackedDir.string());
    check(recompiled.contains("zones") && recompiled["zones"].size() == 1,
          "Fresh transactional unpack recompiles to exactly one Zone");

    // Test 7: the exact historical name-based alias is retired only when
    // its semantic JSON is identical to the incoming canonical Zone.
    std::filesystem::remove(unpackedDir / ".unpack_manifest.json");
    std::filesystem::path safeAlias =
        unpackedDir / "zones" / "zone_Display Name Zone 1.json";
    {
        std::ofstream out(safeAlias);
        out << std::setw(2) << fullSave["zones"][0] << std::endl;
    }
    check(SaveSystem::unpackSaveToDirectory(fullSave, unpackedDir.string()),
          "Proven legacy name-based alias migrates safely");
    check(!std::filesystem::exists(safeAlias),
          "Equivalent historical alias is retired");
    nlohmann::json afterAlias =
        SaveSystem::compileSaveFromDirectory(unpackedDir.string());
    check(afterAlias.contains("zones") && afterAlias["zones"].size() == 1,
          "Alias migration cannot duplicate the Zone");

    // Test 8: same identity/path shape with Person-visible edits is NOT
    // guessed to be stale. The whole transaction refuses and preserves it.
    std::filesystem::path ambiguousDir = sandbox / "ambiguous_alias_unpack";
    check(SaveSystem::unpackSaveToDirectory(fullSave, ambiguousDir.string()),
          "Fixture unpack for ambiguous alias succeeds");
    std::filesystem::remove(ambiguousDir / ".unpack_manifest.json");
    std::filesystem::path ambiguousAlias =
        ambiguousDir / "zones" / "zone_Display Name Zone 1.json";
    nlohmann::json personAlias = fullSave["zones"][0];
    personAlias["person_note"] = "keep this authored difference";
    {
        std::ofstream out(ambiguousAlias);
        out << std::setw(2) << personAlias << std::endl;
    }
    const std::string ambiguousBefore = treeFingerprint(ambiguousDir);
    check(!SaveSystem::unpackSaveToDirectory(fullSave, ambiguousDir.string()),
          "Ambiguous pre-manifest alias refuses re-unpack");
    check(treeFingerprint(ambiguousDir) == ambiguousBefore,
          "Ambiguous alias refusal leaves the live tree byte-for-byte/logically unchanged");

    // Test 9: a late staged write failure never mutates a prior generation.
    std::filesystem::path failureDir = sandbox / "staged_failure_unpack";
    check(SaveSystem::unpackSaveToDirectory(fullSave, failureDir.string()),
          "Fixture unpack for staged failure succeeds");
    const std::string failureBefore = treeFingerprint(failureDir);
    nlohmann::json blockedMeta = fullSave;
    blockedMeta["__test_block_meta_write"] = true;
    check(!SaveSystem::unpackSaveToDirectory(blockedMeta, failureDir.string()),
          "Meta write failure propagates from unpack transaction");
    check(treeFingerprint(failureDir) == failureBefore,
          "Failed staged generation leaves previous generation unchanged");

    // Test 10: once a generated canonical file diverges from its manifest
    // hash, its Person-authored bytes win over future generated output.
    std::filesystem::path personEditedDir = sandbox / "person_edited_unpack";
    check(SaveSystem::unpackSaveToDirectory(fullSave, personEditedDir.string()),
          "Fixture unpack for Person edit succeeds");
    std::filesystem::path editedZone =
        personEditedDir / "zones" / "zone_zone_1.json";
    nlohmann::json edited = fullSave["zones"][0];
    edited["person_note"] = "important";
    {
        std::ofstream out(editedZone);
        out << std::setw(2) << edited << std::endl;
    }
    nlohmann::json nextGeneration = fullSave;
    nextGeneration["worldTime"] = 100.0;
    check(SaveSystem::unpackSaveToDirectory(nextGeneration, personEditedDir.string()),
          "Re-unpack preserves a Person-modified canonical path");
    nlohmann::json preservedEdit = SaveSystem::readSaveData(editedZone.string());
    check(preservedEdit.value("person_note", std::string{}) == "important",
          "Person-modified canonical bytes are not overwritten");

    // Test 11: arbitrary files and empty directory structure survive the
    // generated-directory swap.
    std::filesystem::path arbitraryDir = sandbox / "arbitrary_preservation_unpack";
    check(SaveSystem::unpackSaveToDirectory(fullSave, arbitraryDir.string()),
          "Fixture unpack for arbitrary preservation succeeds");
    {
        std::ofstream out(arbitraryDir / "notes.md");
        out << "# Person notes\n";
    }
    std::filesystem::create_directories(arbitraryDir / "textures" / "empty_subdir");
    {
        std::ofstream out(arbitraryDir / "textures" / "icon.bin", std::ios::binary);
        out << "PERSON_BYTES";
    }
    check(SaveSystem::unpackSaveToDirectory(fullSave, arbitraryDir.string()),
          "Re-unpack with arbitrary Person files succeeds");
    check(std::filesystem::exists(arbitraryDir / "notes.md"),
          "Top-level Person file survives re-unpack");
    check(std::filesystem::exists(arbitraryDir / "textures" / "icon.bin"),
          "Nested Person file survives re-unpack");
    check(std::filesystem::is_directory(
              arbitraryDir / "textures" / "empty_subdir"),
          "Empty Person-authored directory survives re-unpack");

    // Test 12: preservation-copy failure is fail-closed before live swap.
    std::filesystem::path copyFailDir = sandbox / "copy_failure_unpack";
    check(SaveSystem::unpackSaveToDirectory(fullSave, copyFailDir.string()),
          "Fixture unpack for copy failure succeeds");
    {
        std::ofstream out(copyFailDir / "blocked_user_file.txt");
        out << "critical Person content";
    }
    const std::string copyFailBefore = treeFingerprint(copyFailDir);
    nlohmann::json blockedCopy = fullSave;
    blockedCopy["__test_block_preservation_copy"] = true;
    check(!SaveSystem::unpackSaveToDirectory(blockedCopy, copyFailDir.string()),
          "Injected preservation-copy failure propagates");
    check(treeFingerprint(copyFailDir) == copyFailBefore,
          "Copy failure cannot swap or damage the live authoring tree");

    // Test 13: manifest write is a single checked commit; its failure also
    // leaves the prior tree unchanged.
    std::filesystem::path manifestFailDir = sandbox / "manifest_failure_unpack";
    check(SaveSystem::unpackSaveToDirectory(fullSave, manifestFailDir.string()),
          "Fixture unpack for manifest failure succeeds");
    const std::string manifestBefore = treeFingerprint(manifestFailDir);
    nlohmann::json blockedManifest = fullSave;
    blockedManifest["__test_block_manifest_write"] = true;
    check(!SaveSystem::unpackSaveToDirectory(
              blockedManifest, manifestFailDir.string()),
          "Manifest write failure propagates");
    check(treeFingerprint(manifestFailDir) == manifestBefore,
          "Manifest failure leaves previous generation unchanged");

    // Test 14: an unmodified generated entity absent from the next
    // generation is deleted rather than resurrected by preservation.
    std::filesystem::path deletionDir = sandbox / "non_resurrection_unpack";
    nlohmann::json generationA = {
        {"zones", {
            {{"identifier", "zone_1"}, {"name", "Zone 1"}},
            {{"identifier", "zone_x"}, {"name", "Zone X"}}
        }}
    };
    nlohmann::json generationB = {
        {"zones", {
            {{"identifier", "zone_1"}, {"name", "Zone 1"}}
        }}
    };
    check(SaveSystem::unpackSaveToDirectory(generationA, deletionDir.string()),
          "Generation A unpack succeeds");
    check(std::filesystem::exists(deletionDir / "zones" / "zone_zone_x.json"),
          "Generated Zone X exists in generation A");
    check(SaveSystem::unpackSaveToDirectory(generationB, deletionDir.string()),
          "Generation B unpack succeeds");
    check(!std::filesystem::exists(deletionDir / "zones" / "zone_zone_x.json"),
          "Deleted generated Zone X is not resurrected");

    // Test 15: final manifest hashes generated payloads only; it does not
    // contain a recursively impossible self-hash.
    nlohmann::json manifest =
        SaveSystem::readSaveData((unpackedDir / ".unpack_manifest.json").string());
    check(manifest.contains("fileHashes") &&
              !manifest["fileHashes"].contains(".unpack_manifest.json"),
          "Manifest deliberately has no stale self-hash");

    // Test 16: a malformed prior ownership manifest is not treated as
    // "no manifest"; the transaction refuses rather than guessing ownership.
    std::filesystem::path malformedManifestDir =
        sandbox / "malformed_manifest_unpack";
    check(SaveSystem::unpackSaveToDirectory(
              fullSave, malformedManifestDir.string()),
          "Fixture unpack for malformed manifest succeeds");
    {
        std::ofstream out(malformedManifestDir / ".unpack_manifest.json");
        out << "{ malformed manifest";
    }
    const std::string malformedBefore =
        treeFingerprint(malformedManifestDir);
    check(!SaveSystem::unpackSaveToDirectory(
              fullSave, malformedManifestDir.string()),
          "Malformed prior manifest refuses re-unpack");
    check(treeFingerprint(malformedManifestDir) == malformedBefore,
          "Malformed manifest refusal leaves live tree unchanged");

    // Verify no staging/backup/leaf temporary artifacts remain.
    bool hasTempFiles = false;
    std::error_code ec;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(sandbox, ec)) {
        const std::string name = entry.path().filename().string();
        if (name.find(".tmp-") != std::string::npos ||
            name.find(".tmp_unpack_") != std::string::npos ||
            name.find(".tmp_backup_") != std::string::npos) {
            hasTempFiles = true;
            break;
        }
    }
    check(!hasTempFiles,
          "No temporary save/unpack artifacts remain after all transaction paths");

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
