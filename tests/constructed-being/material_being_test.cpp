// Material-as-a-being test (OPENGL_MIGRATION_PLAN.md, Milestone 2).
//
// The decision was that a Material is a Singular being, not a render-layer struct.
// That claim only means something if two things hold:
//   1. A Material is LAW-ADDRESSABLE — its fields resolve through PropertyPath and
//      can be read and driven exactly like an Object's position. This is the whole
//      reason to make it a being: a Law can change a material's colour.
//   2. MaterialManager owns the beings the way RelationManager owns relations:
//      a default always resolves, references resolve by identifier, and the set
//      survives a JSON round-trip.
// A behaviour-preserving detail is pinned too: the default material must carry the
// old global ShadingSystem constants, so nothing looks different until a Person
// authors a new material.

#include "ConstructedBeing/Material/Material.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>

static bool nearf(float a, float b, float eps = 1e-5f) { return std::fabs(a - b) < eps; }

int main() {
    // --- 1. A Material is a legible, drivable being --------------------------
    {
        Material m("clay");
        assert(m.getIdentifier() == "material.clay");

        // The vocabulary is discoverable (authoring UIs list these).
        assert(!m.listProperties().empty());

        // A "Law" reads a field.
        PropertyValue out;
        assert(PropertyPath::parse("shininess").getValue(m, out) == PropertyPath::PathResult::Ok);
        assert(nearf(std::get<float>(out), 32.0f));

        // A "Law" drives a field — the payoff of material-as-being.
        assert(PropertyPath::parse("shininess").setValue(m, PropertyValue(8.0f)) == PropertyPath::PathResult::Ok);
        assert(nearf(m.shininess, 8.0f));

        assert(PropertyPath::parse("baseColor").setValue(
            m, PropertyValue(glm::vec3(0.6f, 0.3f, 0.1f))) == PropertyPath::PathResult::Ok);
        assert(nearf(m.baseColor.r, 0.6f) && nearf(m.baseColor.g, 0.3f) && nearf(m.baseColor.b, 0.1f));
        std::printf("  being:   material.clay is law-addressable (shininess, baseColor driven)\n");
    }

    // --- 2. The default material preserves the old global shading ------------
    {
        MaterialManager mm;
        auto def = mm.defaultMaterial();
        assert(def);
        assert(def->getIdentifier() == "material.default");
        // These are the exact constants ShadingSystem used to set globally.
        assert(nearf(def->ambient, 0.2f));
        assert(nearf(def->diffuse, 0.8f));
        assert(nearf(def->specular, 1.0f));
        assert(nearf(def->shininess, 32.0f));
        assert(nearf(def->baseColor.r, 1.0f) && nearf(def->baseColor.g, 1.0f) && nearf(def->baseColor.b, 1.0f));
        std::printf("  default: reproduces old global constants (ambient .2 / diffuse .8 / shininess 32)\n");
    }

    // --- 3. Manager ownership, resolution, and the undeletable default -------
    {
        MaterialManager mm;
        auto clay = mm.create("clay");
        clay->baseColor = glm::vec3(0.6f, 0.3f, 0.1f);

        // create is idempotent by name (no duplicate beings).
        assert(mm.create("clay") == clay);

        // Resolve by full identifier and by bare name.
        assert(mm.get("material.clay") == clay);
        assert(mm.get("clay") == clay);

        // A dangling reference resolves to the default, never nullptr.
        assert(mm.get("material.nope") == nullptr);
        assert(mm.resolveOrDefault("material.nope") == mm.defaultMaterial());

        // The default is load-bearing and cannot be removed.
        assert(!mm.remove("material.default"));
        assert(mm.remove("material.clay"));
        assert(mm.get("clay") == nullptr);
        std::printf("  manager: create/get/resolveOrDefault work; default is undeletable\n");
    }

    // --- 4. Serialization round-trip -----------------------------------------
    {
        MaterialManager mm;
        auto clay = mm.create("clay");
        clay->baseColor = glm::vec3(0.6f, 0.3f, 0.1f);
        clay->shininess = 8.0f;

        MaterialManager loaded;
        loaded.loadFromJson(mm.toJson());

        auto rt = loaded.get("clay");
        assert(rt);
        assert(nearf(rt->baseColor.r, 0.6f) && nearf(rt->shininess, 8.0f));
        assert(loaded.defaultMaterial()); // default survives / is reinstated
        std::printf("  serial:  materials round-trip through JSON, default preserved\n");
    }

    std::printf("material_being_test: ALL OK\n");
    return 0;
}
