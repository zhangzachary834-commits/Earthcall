#pragma once

#include "json.hpp"
#include "ConstructedBeing/Object/Object.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "Person/Body/BodyPart/BodyPart.hpp"
#include "Person/Body/Body.hpp"
#include <glm/gtc/type_ptr.hpp>

// Free functions enabling nlohmann::json (ADL) serialization

void to_json(nlohmann::json& j, const Object& obj);
void from_json(const nlohmann::json& j, Object& obj);

// The on-disk bag is still `{"objects":[...]}` under `zones[].world`.
// That key is not the ontology; it is the shape existing Person saves carry.
nlohmann::json zoneObjectsToJson(const Zone& zone);
void zoneObjectsFromJson(const nlohmann::json& j, Zone& zone);

// BodyPart serialization (includes faceTextures from Object)
nlohmann::json bodyPartToJson(const BodyPart& part);
void bodyPartFromJson(const nlohmann::json& j, BodyPart& part);

// Convenience: serialize/deserialize all body parts of a Body
nlohmann::json bodyToJson(const Body& body);
void bodyFromJson(const nlohmann::json& j, Body& body);

// ---------------------------------------------------------
// Frontier Serialization Wrappers
// ---------------------------------------------------------
#include "Singularity/Storage/Frontier.hpp"

// The Root payload version 1 wrapper
struct EarthcallSaveState_V1 {
    nlohmann::json payload;
};

namespace Frontier {
    template <>
    struct VersionInfo<EarthcallSaveState_V1> {
        static constexpr uint32_t VERSION = 1;
        using Previous = void;
    };

    template <>
    struct IsFrontier<EarthcallSaveState_V1> : std::true_type {};
}

// An adapter that reads from a JSON object and acts as a Reader for Frontier.hpp
class JsonReader {
    const nlohmann::json& root;
public:
    JsonReader(const nlohmann::json& j) : root(j) {}
    
    uint32_t readVersion() const {
        // If the json doesn't have a version, we assume it's V1 (legacy)
        return root.value("schema_version", 1);
    }
    
    void read(EarthcallSaveState_V1& out) {
        out.payload = root;
    }
};