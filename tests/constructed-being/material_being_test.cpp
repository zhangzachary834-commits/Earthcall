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

    // --- 1b. Rung 9 response is independent authored Material truth -----------
    {
        Material m("response_clay");
        assert(m.findProperty("responseExpr") != nullptr);
        assert(!m.responseExpr);
        const uint32_t colorRevisionBefore = m.getRevision();
        const uint32_t responseRevisionBefore = m.getResponseRevision();

        auto root = std::make_shared<OntoMath::MathNode>();
        root->op = OntoMath::MathNode::Op::VectorConstruct;
        for (double value : {0.25, 0.5, 0.75}) {
            auto c = std::make_unique<OntoMath::MathNode>();
            c->op = OntoMath::MathNode::Op::ScalarLeaf;
            c->scalarForm = OntoMath::ScalarForm::constant(value);
            root->children.push_back(std::move(c));
        }
        OntoMath::Piecewise response = OntoMath::Piecewise::continuous(root);
        const std::string serialized = response.toJson().dump();

        assert(PropertyPath::parse("responseExpr").setValue(
                   m, PropertyValue(serialized)) == PropertyPath::PathResult::Ok);
        assert(m.responseExpr);
        assert(m.getResponseRevision() == responseRevisionBefore + 1);
        assert(m.getRevision() == colorRevisionBefore);
        assert(m.responseExpr->toJson() == response.toJson());

        const nlohmann::json saved = m.toJson();
        assert(saved.contains("responseExpr"));
        Material restored = Material::fromJson(saved);
        assert(restored.responseExpr);
        assert(restored.responseExpr->toJson() == response.toJson());

        const uint32_t responseRevisionAfter = m.getResponseRevision();
        assert(PropertyPath::parse("responseExpr").setValue(
                   m, PropertyValue(std::string("{not-json"))) != PropertyPath::PathResult::Ok);
        assert(m.getResponseRevision() == responseRevisionAfter);
        assert(m.responseExpr->toJson() == response.toJson());

        std::printf("  rung9:    responseExpr is Law-addressable, persisted, and revision-independent\n");
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

    // --- 5. Direct Material methods and faceTextures unit tests --------------
    {
        // Default constructor & getters/setters
        Material m_def;
        assert(m_def.name() == "default");
        assert(m_def.getIdentifier() == "material.default");
        assert(nearf(m_def.baseColor.r, 1.0f) && nearf(m_def.baseColor.g, 1.0f) && nearf(m_def.baseColor.b, 1.0f));
        assert(nearf(m_def.opacity, 1.0f));
        assert(nearf(m_def.shininess, 32.0f));
        assert(nearf(m_def.specular, 1.0f));
        assert(nearf(m_def.ambient, 0.2f));
        assert(nearf(m_def.diffuse, 0.8f));
        assert(m_def.faceTextures.empty());

        m_def.setName("stone");
        assert(m_def.name() == "stone");
        assert(m_def.getIdentifier() == "material.stone");

        // initFaceTextures
        m_def.initFaceTextures(6, 32, 32);
        assert(m_def.faceTextures.size() == 6);
        assert(m_def.faceTextures[0].width == 32);
        assert(m_def.faceTextures[0].height == 32);
        assert(m_def.faceTextures[0].pixels.size() == 32 * 32 * 4);

        // Idempotency check: calling initFaceTextures with same count leaves existing textures untouched
        m_def.faceTextures[0].pixels[0] = 0xAA;
        m_def.initFaceTextures(6, 64, 64);
        assert(m_def.faceTextures[0].pixels[0] == 0xAA);
        assert(m_def.faceTextures[0].width == 32);

        // Changing count re-initialises
        m_def.initFaceTextures(4, 16, 16);
        assert(m_def.faceTextures.size() == 4);
        assert(m_def.faceTextures[0].width == 16);

        std::printf("  material: default state, name setter, initFaceTextures tested\n");
    }

    // --- 6. Property registration completeness --------------------------------
    {
        Material m("prop_test");
        assert(m.findProperty("baseColor") != nullptr);
        assert(m.findProperty("opacity") != nullptr);
        assert(m.findProperty("shininess") != nullptr);
        assert(m.findProperty("specular") != nullptr);
        assert(m.findProperty("ambient") != nullptr);
        assert(m.findProperty("diffuse") != nullptr);
        assert(m.findProperty("colorExpr") != nullptr);
        assert(m.findProperty("responseExpr") != nullptr);
        assert(m.findProperty("nonexistent") == nullptr);
        std::printf("  properties: all 6 properties registered and accessible\n");
    }

    // --- 7. Material serialization edge cases and faceTextures ----------------
    {
        Material m("painted");
        m.initFaceTextures(2, 8, 8);
        m.faceTextures[0].pixels[0] = 128; // modified pixel

        nlohmann::json j = m.toJson();
        assert(j["name"] == "painted");
        assert(j.contains("faceTextures"));
        assert(j["faceTextures"].is_array());
        assert(j["faceTextures"].size() == 2);

        Material restored = Material::fromJson(j);
        assert(restored.name() == "painted");
        assert(restored.faceTextures.size() == 2);
        assert(restored.faceTextures[0].width == 8);
        assert(restored.faceTextures[0].height == 8);
        assert(restored.faceTextures[0].pixels.size() == 8 * 8 * 4);
        assert(restored.faceTextures[0].pixels[0] == 128);

        // JSON missing optional fields falls back to defaults
        nlohmann::json empty_j = nlohmann::json::object();
        Material fallback = Material::fromJson(empty_j);
        assert(fallback.name() == "default");
        assert(nearf(fallback.opacity, 1.0f));
        assert(nearf(fallback.shininess, 32.0f));

        // Invalid texture size handling
        nlohmann::json invalid_tex_j = {
            {"name", "invalid_tex"},
            {"faceTextures", {{{"width", -10}, {"height", 64}}}}
        };
        Material inv_m = Material::fromJson(invalid_tex_j);
        assert(inv_m.faceTextures.empty());

        std::printf("  serialization: faceTextures round-trip and fallback defaults tested\n");
    }

    std::printf("material_being_test: ALL OK\n");
    return 0;
}
