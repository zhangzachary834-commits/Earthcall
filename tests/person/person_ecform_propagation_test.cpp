#include "Singularity/Storage/Serialization/Person/PersonSerialization.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Person/Body/Body.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {

struct SandboxEnv {
    std::filesystem::path tempDir;
    std::string prevSaveRoot;

    SandboxEnv() {
        tempDir = std::filesystem::temp_directory_path() / "earthcall_ecform_prop_test";
        std::filesystem::remove_all(tempDir);
        std::filesystem::create_directories(tempDir / "worlds");

        prevSaveRoot = SaveSystem::saveRoot();
        SaveSystem::setSaveRoot(tempDir.string());
    }

    ~SandboxEnv() {
        SaveSystem::setSaveRoot(prevSaveRoot);
        std::filesystem::remove_all(tempDir);
    }
};

bool isBinaryMsgpack(const std::filesystem::path& p) {
    std::ifstream in(p, std::ios::binary);
    if (!in.is_open()) return false;
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (bytes.empty()) return false;
    try {
        nlohmann::json wrapper = nlohmann::json::from_msgpack(bytes);
        return wrapper.is_object() && wrapper.contains("MigrationRoot");
    } catch (...) {
        return false;
    }
}

} // namespace

int main() {
    std::cout << "person_ecform_propagation_test:\n";

    SandboxEnv env;

    // 1. Create a binary .ecform save fixture with OldName owner/person references
    nlohmann::json worldData = {
        {"saveFormat", "zone-identity-v1"},
        {"person", {{"displayName", "OldName"}, {"soulName", "OldName"}}},
        {"owner", "OldName"},
        {"zones", nlohmann::json::array({{{"identifier", "Zone1"}, {"owner", "OldName"}}})}
    };
    std::string ecformPath = (env.tempDir / "worlds" / "world1.ecform").string();
    SaveSystem::writeSaveData(worldData, "world1", SaveSystem::SaveType::WORLD);

    assert(std::filesystem::exists(ecformPath));
    assert(isBinaryMsgpack(ecformPath));

    // 2. Create a plain .json save fixture
    nlohmann::json legacyWorldData = {
        {"person", {{"displayName", "OldName"}, {"soulName", "OldName"}}},
        {"owner", "OldName"}
    };
    std::string jsonPath = (env.tempDir / "worlds" / "world_legacy.json").string();
    {
        std::ofstream jsonOut(jsonPath);
        jsonOut << legacyWorldData.dump(2);
    }
    assert(std::filesystem::exists(jsonPath));

    // 3. Construct updated Person "NewName"
    Soul soul("NewName");
    Body body("Humanoid", "Voxel");
    Person person(soul, std::move(body), "");
    person.setDisplayName("NewName");

    // 4. Run updatePriorPersonSerializations
    updatePriorPersonSerializations(person, "OldName");

    // 5. Verify the binary .ecform file was updated and preserved as valid msgpack
    assert(isBinaryMsgpack(ecformPath));
    nlohmann::json readEcform = SaveSystem::readSaveData(ecformPath);
    assert(!readEcform.is_null());
    assert(readEcform.contains("person"));
    assert(readEcform["person"]["displayName"] == "NewName");
    assert(readEcform["owner"] == person.getIdentifier());
    assert(readEcform["zones"][0]["owner"] == person.getIdentifier());
    std::cout << "  .ecform msgpack save file correctly updated and preserved OK\n";

    // 6. Verify the legacy .json file was also updated
    nlohmann::json readJson = SaveSystem::readSaveData(jsonPath);
    assert(!readJson.is_null());
    assert(readJson["person"]["displayName"] == "NewName");
    assert(readJson["owner"] == person.getIdentifier());
    std::cout << "  legacy .json save file correctly updated OK\n";

    std::cout << "person_ecform_propagation_test: ALL OK\n";
    return 0;
}
