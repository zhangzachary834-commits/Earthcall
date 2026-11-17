#pragma once

#include "ConstructedBeing/Material/MaterialResponseDefaults.hpp"
#include "json.hpp"

#include <cstddef>
#include <string>
#include <utility>

namespace MaterialPersistenceMigration {

struct ScopedLegacyDefaultMigration {
    nlohmann::json document;
    bool migrated = false;
    bool synthesized = false;
    std::string scopedMaterialId;
};

inline std::string materialIdentity(const nlohmann::json& material) {
    if (!material.is_object()) return {};
    if (material.contains("id") && material["id"].is_string()) {
        std::string id = material["id"].get<std::string>();
        if (id.rfind("material.", 0) == 0) return id;
        if (!id.empty()) return "material." + id;
    }
    if (material.contains("name") && material["name"].is_string()) {
        const std::string name = material["name"].get<std::string>();
        if (name.rfind("material.", 0) == 0) return name;
        if (!name.empty()) return "material." + name;
    }
    return {};
}

inline bool isDefaultMaterialRecord(const nlohmann::json& material) {
    return materialIdentity(material) == "material.default";
}

inline const nlohmann::json& legacyDirectJson() {
    static const nlohmann::json value =
        MaterialResponseDefaults::makeLegacyDirectResponse()->toJson();
    return value;
}

inline bool isLegacyDefaultRecord(const nlohmann::json& material) {
    if (!isDefaultMaterialRecord(material)) return false;

    // Gate-B current omission is explicit persistence metadata. Unknown future
    // markers are not guessed here; Material::fromJson remains responsible for
    // refusing versions this build cannot interpret.
    if (material.contains(MaterialResponseDefaults::kOmissionSemanticsKey)) {
        return false;
    }

    // Pre-authored-response save: omission meant historical shading.
    if (!material.contains("responseExpr")) return true;

    // Gate-A-era save: the old default may already have been materialized as
    // the exact authored compatibility AST. It is still legacy default state
    // and must not occupy the canonical global material.default identity.
    return material["responseExpr"] == legacyDirectJson();
}

inline std::string hexIdentity(const std::string& value) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out;
    out.reserve(value.size() * 2);
    for (unsigned char c : value) {
        out.push_back(kHex[(c >> 4u) & 0x0fu]);
        out.push_back(kHex[c & 0x0fu]);
    }
    return out;
}

inline std::string scopedLegacyDefaultId(const std::string& scopeIdentity) {
    // Full UTF-8 bytes are encoded, not hashed: distinct scope identities
    // cannot collide merely because a finite hash did.
    const std::string source =
        scopeIdentity.empty() ? std::string("<anonymous-persistence-scope>")
                              : scopeIdentity;
    return "material.__legacy_default__." + hexIdentity(source);
}

inline std::string bareMaterialName(const std::string& id) {
    constexpr const char* kPrefix = "material.";
    return id.rfind(kPrefix, 0) == 0 ? id.substr(9) : id;
}

inline const nlohmann::json* documentObjects(
    const nlohmann::json& document) {
    if (!document.is_object()) return nullptr;
    if (document.contains("world") && document["world"].is_object() &&
        document["world"].contains("objects") &&
        document["world"]["objects"].is_array()) {
        return &document["world"]["objects"];
    }
    if (document.contains("objects") && document["objects"].is_array()) {
        return &document["objects"];
    }
    return nullptr;
}

inline nlohmann::json* documentObjects(nlohmann::json& document) {
    if (!document.is_object()) return nullptr;
    if (document.contains("world") && document["world"].is_object() &&
        document["world"].contains("objects") &&
        document["world"]["objects"].is_array()) {
        return &document["world"]["objects"];
    }
    if (document.contains("objects") && document["objects"].is_array()) {
        return &document["objects"];
    }
    return nullptr;
}

inline bool objectUsesLegacyDefault(const nlohmann::json& object) {
    if (!object.is_object()) return false;
    const auto it = object.find("materialId");
    if (it == object.end()) {
        // Object::from_json historically maps omission to material.default.
        return true;
    }
    return it->is_string() &&
           it->get<std::string>() == "material.default";
}

inline bool objectsUseLegacyDefault(const nlohmann::json& document) {
    const auto* objects = documentObjects(document);
    if (!objects) return false;
    for (const auto& object : *objects) {
        if (objectUsesLegacyDefault(object)) return true;
    }
    return false;
}

inline void rewriteLegacyDefaultObjectReferences(
    nlohmann::json& document, const std::string& scopedId) {
    auto* objects = documentObjects(document);
    if (!objects) return;
    for (auto& object : *objects) {
        if (!object.is_object()) continue;
        const auto it = object.find("materialId");
        if (it == object.end()) {
            // Make the old implicit fallback explicit before Object::from_json
            // can reinterpret it under the current canonical default.
            object["materialId"] = scopedId;
        } else if (it->is_string() &&
                   it->get<std::string>() == "material.default") {
            *it = scopedId;
        }
    }
}

inline nlohmann::json synthesizedLegacyDefaultRecord(
    const std::string& scopedId) {
    // These are the historical Material defaults that fed the authored
    // compatibility response. No responseExpr is emitted here on purpose:
    // Material::fromJson performs the same one-time migration as every other
    // old response-less material and the next save persists the explicit AST.
    return nlohmann::json{
        {"id", scopedId},
        {"name", bareMaterialName(scopedId)},
        {"baseColor", {1.0f, 1.0f, 1.0f}},
        {"opacity", 1.0f},
        {"ambient", 0.2f},
        {"diffuse", 0.8f},
        {"specular", 1.0f},
        {"shininess", 32.0f}
    };
}

// Migrates only the ROOT material bag and ROOT world/objects of one persistence
// document. Nested Zone documents deliberately remain untouched so each Zone
// gets its own identity-derived scope when it is admitted.
inline ScopedLegacyDefaultMigration migrateLegacyDefaultDocument(
    const nlohmann::json& input, const std::string& scopeIdentity) {
    ScopedLegacyDefaultMigration out;
    out.document = input;
    if (!input.is_object()) return out;

    const bool rootReferencesDefault =
        objectsUseLegacyDefault(input);

    std::size_t legacyDefaultIndex = static_cast<std::size_t>(-1);
    bool hasAnyDefaultRecord = false;
    if (input.contains("materials") && input["materials"].is_array()) {
        const auto& materials = input["materials"];
        for (std::size_t i = 0; i < materials.size(); ++i) {
            if (!isDefaultMaterialRecord(materials[i])) continue;
            hasAnyDefaultRecord = true;
            if (isLegacyDefaultRecord(materials[i])) {
                legacyDefaultIndex = i;
                break;
            }
        }
    }

    // Current serializer always carries every referenced Material in the
    // Zone identity. Therefore a root object that names material.default while
    // its old document has no default record is itself legacy evidence.
    const bool synthesize =
        legacyDefaultIndex == static_cast<std::size_t>(-1) &&
        !hasAnyDefaultRecord && rootReferencesDefault;

    if (legacyDefaultIndex == static_cast<std::size_t>(-1) && !synthesize) {
        return out;
    }

    out.scopedMaterialId = scopedLegacyDefaultId(scopeIdentity);
    auto scopedRecord = synthesize
        ? synthesizedLegacyDefaultRecord(out.scopedMaterialId)
        : out.document["materials"][legacyDefaultIndex];

    scopedRecord["id"] = out.scopedMaterialId;
    scopedRecord["name"] = bareMaterialName(out.scopedMaterialId);

    if (!out.document.contains("materials") ||
        !out.document["materials"].is_array()) {
        out.document["materials"] = nlohmann::json::array();
    }

    if (legacyDefaultIndex != static_cast<std::size_t>(-1)) {
        out.document["materials"][legacyDefaultIndex] =
            std::move(scopedRecord);
    } else {
        out.document["materials"].push_back(std::move(scopedRecord));
        out.synthesized = true;
    }

    rewriteLegacyDefaultObjectReferences(
        out.document, out.scopedMaterialId);

    out.migrated = true;
    return out;
}

} // namespace MaterialPersistenceMigration
