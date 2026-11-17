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

inline bool containsDefaultMaterialReference(const nlohmann::json& node) {
    if (node.is_object()) {
        const auto it = node.find("materialId");
        if (it != node.end() && it->is_string() &&
            it->get<std::string>() == "material.default") {
            return true;
        }
        for (auto child = node.begin(); child != node.end(); ++child) {
            if (containsDefaultMaterialReference(child.value())) return true;
        }
        return false;
    }
    if (node.is_array()) {
        for (const auto& child : node) {
            if (containsDefaultMaterialReference(child)) return true;
        }
    }
    return false;
}

inline void rewriteDefaultMaterialReferences(
    nlohmann::json& node, const std::string& scopedId) {
    if (node.is_object()) {
        auto it = node.find("materialId");
        if (it != node.end() && it->is_string() &&
            it->get<std::string>() == "material.default") {
            *it = scopedId;
        }
        for (auto child = node.begin(); child != node.end(); ++child) {
            rewriteDefaultMaterialReferences(child.value(), scopedId);
        }
        return;
    }
    if (node.is_array()) {
        for (auto& child : node) {
            rewriteDefaultMaterialReferences(child, scopedId);
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

    bool rootReferencesDefault = false;
    if (input.contains("world")) {
        rootReferencesDefault =
            containsDefaultMaterialReference(input["world"]);
    }
    if (!rootReferencesDefault && input.contains("objects")) {
        rootReferencesDefault =
            containsDefaultMaterialReference(input["objects"]);
    }

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

    if (out.document.contains("world")) {
        rewriteDefaultMaterialReferences(
            out.document["world"], out.scopedMaterialId);
    }
    if (out.document.contains("objects")) {
        rewriteDefaultMaterialReferences(
            out.document["objects"], out.scopedMaterialId);
    }

    out.migrated = true;
    return out;
}

} // namespace MaterialPersistenceMigration
