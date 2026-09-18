#include "ConstructedBeing/Material/Material.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>

static bool nearf(float a, float b, float eps = 1e-5f) { return std::fabs(a - b) < eps; }

int main() {
    // 1. Construction and Default Material
    {
        MaterialManager mm;
        assert(mm.defaultMaterial() != nullptr);
        assert(mm.defaultMaterial()->name() == "default");
        assert(mm.defaultMaterial()->getIdentifier() == "material.default");
        assert(mm.getAll().size() == 1);
        assert(mm.getAll()[0] == mm.defaultMaterial());
    }

    // 2. Creation and Idempotency
    {
        MaterialManager mm;
        auto m1 = mm.create("stone");
        assert(m1 != nullptr);
        assert(m1->name() == "stone");
        assert(m1->getIdentifier() == "material.stone");

        // Normalization on create ("material.stone" vs "stone")
        auto m1_dup = mm.create("material.stone");
        assert(m1 == m1_dup);

        auto m2 = mm.create("wood");
        assert(m2 != nullptr);
        assert(m2 != m1);
        assert(mm.getAll().size() == 3); // default, stone, wood
    }

    // 3. Get, ResolveOrDefault, and Null handling
    {
        MaterialManager mm;
        auto clay = mm.create("clay");

        // get with bare name and full identifier
        assert(mm.get("clay") == clay);
        assert(mm.get("material.clay") == clay);

        // get non-existent returns nullptr
        assert(mm.get("iron") == nullptr);
        assert(mm.get("material.iron") == nullptr);

        // resolveOrDefault returns found material or fallback to default
        assert(mm.resolveOrDefault("clay") == clay);
        assert(mm.resolveOrDefault("material.clay") == clay);
        assert(mm.resolveOrDefault("iron") == mm.defaultMaterial());
        assert(mm.resolveOrDefault("material.iron") == mm.defaultMaterial());
    }

    // 4. Add and Replacement by Identity
    {
        MaterialManager mm;
        auto matA = std::make_shared<Material>("glass");
        matA->shininess = 100.0f;
        mm.add(matA);

        assert(mm.get("glass") == matA);
        assert(nearf(mm.get("glass")->shininess, 100.0f));

        // Adding nullptr should be safe and do nothing
        mm.add(nullptr);
        assert(mm.get("glass") == matA);

        // Replacing existing material identity
        auto matB = std::make_shared<Material>("glass");
        matB->shininess = 200.0f;
        mm.add(matB);

        assert(mm.get("glass") == matB);
        assert(nearf(mm.get("glass")->shininess, 200.0f));
    }

    // 5. Remove operations
    {
        MaterialManager mm;
        auto m = mm.create("gold");
        assert(mm.get("gold") != nullptr);

        // Cannot remove default material
        assert(!mm.remove("default"));
        assert(!mm.remove("material.default"));
        assert(mm.defaultMaterial() != nullptr);

        // Removing non-existent material returns false
        assert(!mm.remove("nonexistent"));

        // Removing existing material (bare name or identifier)
        assert(mm.remove("gold"));
        assert(mm.get("gold") == nullptr);

        auto m2 = mm.create("silver");
        assert(mm.remove("material.silver"));
        assert(mm.get("silver") == nullptr);
    }

    // 6. JSON Serialization (toJson and loadFromJson)
    {
        MaterialManager mm;
        auto mat = mm.create("copper");
        mat->shininess = 50.0f;
        mat->baseColor = glm::vec3(0.9f, 0.5f, 0.3f);

        nlohmann::json jsonVal = mm.toJson();
        assert(jsonVal.is_array());

        MaterialManager loadedMM;
        loadedMM.loadFromJson(jsonVal);

        auto loadedCopper = loadedMM.get("copper");
        assert(loadedCopper != nullptr);
        assert(nearf(loadedCopper->shininess, 50.0f));
        assert(nearf(loadedCopper->baseColor.r, 0.9f));
        assert(loadedMM.defaultMaterial() != nullptr);

        // loadFromJson with non-array JSON handles gracefully and ensures default
        MaterialManager emptyMM;
        emptyMM.loadFromJson(nlohmann::json::object());
        assert(emptyMM.defaultMaterial() != nullptr);
    }

    // 7. JSON Merge (mergeFromJson)
    {
        MaterialManager mm;
        auto existing = mm.create("existing_mat");
        existing->shininess = 10.0f;

        MaterialManager externalMM;
        auto ext = externalMM.create("existing_mat");
        ext->shininess = 80.0f;
        auto brandNew = externalMM.create("new_mat");
        brandNew->shininess = 40.0f;

        mm.mergeFromJson(externalMM.toJson());

        // existing_mat should be updated by upsert
        assert(nearf(mm.get("existing_mat")->shininess, 80.0f));
        // new_mat should be added
        assert(mm.get("new_mat") != nullptr);
        assert(nearf(mm.get("new_mat")->shininess, 40.0f));
        // default material present
        assert(mm.defaultMaterial() != nullptr);

        // mergeFromJson with non-array JSON
        mm.mergeFromJson(nlohmann::json::object());
        assert(mm.defaultMaterial() != nullptr);
    }

    std::printf("material_manager_test: ALL OK\n");
    return 0;
}
