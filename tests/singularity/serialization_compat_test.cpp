#include "Singularity/Storage/SaveSystem.hpp"
#include "Singularity/Storage/Serialization.hpp"
#include "ConstructedBeing/Object/Object.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ConditionModel.hpp"

#include <GLFW/glfw3.h>
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

// A condition kind this build cannot evaluate must survive a load/save round
// trip byte for byte. Kinds 12 and 13 were the pair quantifiers, retired in
// favour of Relations; before Kind::Unsupported existed they cast straight to
// an out-of-range enum that no switch matched, so they compiled to a silent
// constant false AND re-serialized as a husk with their children stripped.
// Merely opening a world in this build destroyed law text written by another.
void test_retired_condition_kind_survives_roundtrip() {
    json pairQuantifier;
    pairQuantifier["kind"] = 12;              // the retired ForAnyPair
    pairQuantifier["beingKind"] = 1;
    pairQuantifier["beingKindB"] = 1;
    pairQuantifier["except"] = json::array({"someBeing"});
    pairQuantifier["children"] = json::array({
        {{"kind", 11}, {"otherId", "@event.object"}}   // Overlaps
    });

    ConditionNode loaded = ConditionNode::fromJson(pairQuantifier);
    assert(loaded.kind == ConditionNode::Kind::Unsupported);

    // It never holds -- and it does not throw or crash getting there.
    ECA::Event probe;
    probe.type = "test";
    Object subject;
    assert(loaded.compile()(probe, subject) == false);

    // The payload rides along untouched, children and all.
    json resaved = loaded.toJson();
    assert(resaved == pairQuantifier);

    // And it stays intact across repeated open/save cycles, which is the
    // case that actually destroys a world: load, save, load, save.
    json twice = ConditionNode::fromJson(resaved).toJson();
    assert(twice == pairQuantifier);

    std::puts("serialization_compat_test: retired condition kind preserved OK");
}

int main() {
    // Object's constructor touches GL, so the probe subject needs a context.
    if (!glfwInit()) {
        std::fprintf(stderr, "serialization_compat_test: glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "serialization_compat_test", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "serialization_compat_test: no GL context\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    std::puts("serialization_compat_test: Starting...");
    test_msgpack_roundtrip();
    test_json_fallback();
    test_retired_condition_kind_survives_roundtrip();

    glfwDestroyWindow(window);
    glfwTerminate();
    std::puts("serialization_compat_test: ALL OK");
    return 0;
}
