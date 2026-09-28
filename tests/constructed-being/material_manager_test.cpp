#include "ConstructedBeing/Material/MaterialManager.hpp"
#include <cassert>
#include <iostream>

static void testEnsureDefault() {
    MaterialManager materials;
    auto def = materials.get("material.default");
    assert(def != nullptr);
    assert(def->name() == "default");
    assert(def->getIdentifier() == "material.default");

    std::cout << "  ensure default OK\n";
}

static void testCreateMaterial() {
    MaterialManager materials;
    auto mat = materials.create("material.glass");
    assert(mat != nullptr);
    assert(mat->name() == "glass");
    assert(mat->getIdentifier() == "material.glass");

    auto bare = materials.create("steel");
    assert(bare != nullptr);
    assert(bare->name() == "steel");
    assert(bare->getIdentifier() == "material.steel");

    std::cout << "  create material OK\n";
}

static void testAddAndRemoveMaterial() {
    MaterialManager materials;
    auto custom = std::make_shared<Material>("custom");
    materials.add(custom);

    auto retrieved = materials.get("material.custom");
    assert(retrieved == custom);

    assert(materials.remove("material.custom") == true);
    assert(materials.get("material.custom") == nullptr);

    // Default material cannot be removed
    assert(materials.remove("material.default") == false);
    assert(materials.get("material.default") != nullptr);

    std::cout << "  add and remove material OK\n";
}

static void testResolveOrDefault() {
    MaterialManager materials;
    materials.create("material.wood");

    auto wood = materials.resolveOrDefault("material.wood");
    assert(wood != nullptr);
    assert(wood->name() == "wood");

    auto unknown = materials.resolveOrDefault("material.unknown");
    assert(unknown != nullptr);
    assert(unknown->name() == "default");

    std::cout << "  resolve or default OK\n";
}

static void testSerializationRoundtrip() {
    MaterialManager materials1;
    auto mat = materials1.create("material.plastic");
    mat->shininess = 16.0f;
    mat->opacity = 0.5f;

    nlohmann::json j = materials1.toJson();

    MaterialManager materials2;
    materials2.loadFromJson(j);

    auto retrieved = materials2.get("material.plastic");
    assert(retrieved != nullptr);
    assert(retrieved->name() == "plastic");
    assert(std::abs(retrieved->shininess - 16.0f) < 1e-5f);
    assert(std::abs(retrieved->opacity - 0.5f) < 1e-5f);

    std::cout << "  serialization roundtrip OK\n";
}

static void testMergeFromJson() {
    MaterialManager materials;
    materials.create("material.base");

    nlohmann::json j = nlohmann::json::array();

    Material extra("extra");
    extra.shininess = 64.0f;
    j.push_back(extra.toJson());

    materials.mergeFromJson(j);

    assert(materials.get("material.base") != nullptr);
    auto retrieved = materials.get("material.extra");
    assert(retrieved != nullptr);
    assert(std::abs(retrieved->shininess - 64.0f) < 1e-5f);

    std::cout << "  merge from json OK\n";
}

int main() {
    std::cout << "material_manager_test:\n";
    testEnsureDefault();
    testCreateMaterial();
    testAddAndRemoveMaterial();
    testResolveOrDefault();
    testSerializationRoundtrip();
    testMergeFromJson();
    std::cout << "material_manager_test: ALL OK\n";
    return 0;
}
