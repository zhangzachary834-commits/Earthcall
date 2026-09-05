#pragma once

#include "json.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "Person/Body/BodyPart/BodyPart.hpp"
#include "Person/Body/Body.hpp"
#include "Singularity/Storage/Serialization/ConstructedBeing/ObjectSerialization.hpp"
#include "Singularity/Storage/Serialization/Person/BodySerialization.hpp"
#include "Singularity/Storage/Serialization/Person/PersonSerialization.hpp"
#include "Singularity/Storage/Serialization/Relation/RelationSerialization.hpp"
#include "Singularity/Storage/Serialization/Relation/FormationSerialization.hpp"
#include "Singularity/Storage/Serialization/SessionSemanticRoots.hpp"
#include "Singularity/Storage/Serialization/ZonesOfEarth/HomeSerialization.hpp"
#include "Singularity/Storage/Serialization/ZonesOfEarth/OurverseSerialization.hpp"
#include "Singularity/Storage/Serialization/ZonesOfEarth/ZoneSerialization.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <memory>

// Compatibility declarations for the current save schema.  These are grouped by
// persistence root in the sibling headers; this file remains the single public
// umbrella so legacy callers do not invent a second serialization API.

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
