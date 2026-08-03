#include "Util/SaveSystem.hpp"
#include "Util/Serialization.hpp"
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <iostream>

using json = nlohmann::json;

void test_msgpack_roundtrip() {
    json j;
    j["schema_version"] = 1;
    j["nodes"] = {
        {"id", "law_gravity"},
        {"type", "physics"},
        {"nested", {
            {"x", 1.0},
            {"y", 2.0},
            {"array", {1, 2, 3}}
        }}
    };
    
    // Save to message pack
    std::string filename = SaveSystem::writeSaveData(j, "test_compat", SaveSystem::SaveType::CUSTOM);
    assert(!filename.empty());
    
    // Load from message pack (fallback logic in readSaveData should detect binary)
    json loaded = SaveSystem::readSaveData(filename);
    
    assert(loaded["schema_version"] == 1);
    assert(loaded["nodes"]["id"] == "law_gravity");
    assert(loaded["nodes"]["nested"]["array"][1] == 2);
    
    // Test Frontier versioning pipeline
    JsonReader reader(loaded);
    auto frontierState = Frontier::load_frontier<EarthcallSaveState_V1>(reader);
    
    assert(frontierState.payload["schema_version"] == 1);
    
    std::filesystem::remove(filename);
    std::puts("serialization_compat_test: MessagePack roundtrip OK");
}

void test_json_fallback() {
    json j;
    j["schema_version"] = 1;
    j["legacy"] = true;
    
    // Write traditional JSON
    std::string filename = SaveSystem::writeSaveData(j, "test_legacy", SaveSystem::SaveType::CUSTOM);
    assert(!filename.empty());
    
    // Load it (fallback logic should detect plain json and parse it properly)
    json loaded = SaveSystem::readSaveData(filename);
    assert(loaded["legacy"] == true);
    
    std::filesystem::remove(filename);
    
    // Remove the parallel .ecsave produced by dry-run
    std::string binaryFile = filename;
    if (binaryFile.length() > 5) {
        binaryFile.replace(binaryFile.length() - 5, 5, ".ecsave");
        std::filesystem::remove(binaryFile);
    }
    
    std::puts("serialization_compat_test: JSON fallback OK");
}

int main() {
    std::puts("serialization_compat_test: Starting...");
    test_msgpack_roundtrip();
    test_json_fallback();
    std::puts("serialization_compat_test: ALL OK");
    return 0;
}
