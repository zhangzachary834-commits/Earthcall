#include "FieldNode.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/SdfJson.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValueJson.hpp"

#include <atomic>
#include <cstdio>

namespace geom {

namespace {
// One sequence for every FieldNode in the process. Starts at 1 so 0 keeps
// meaning "channel absent" to the renderer, and 64 bits never wrap.
std::atomic<uint64_t> gAuthoredMathSequence{0};

const char* const kAuthoredMathChannelNames[] = {
    "field.ast", "volume.density.ast", "volume.extinction.ast",
    "volume.scattering.ast", "volume.chroma.ast", "volume.phase.ast",
    "volume.emission.ast", "volume.occluder.sdf", "light.chroma.ast",
    "light.angular.ast"};
}

void FieldNode::noteAuthoredMathWritten() {
    _authoredMathRevision = gAuthoredMathSequence.fetch_add(1) + 1;
}

uint64_t FieldNode::verifiedAuthoredMathRevision() const {
    static_assert(sizeof(kAuthoredMathChannelNames) / sizeof(kAuthoredMathChannelNames[0]) ==
                  kAuthoredMathChannels, "one name per verified channel");
    const std::size_t channel = _verifyCursor;
    _verifyCursor = (_verifyCursor + 1) % kAuthoredMathChannels;

    const OntoMath::Piecewise* expr = nullptr;
    switch (channel) {
    case 0: expr = field ? &field->astDefinition : nullptr; break;
    case 1: expr = volumeDensity.get(); break;
    case 2: expr = volumeExtinction.get(); break;
    case 3: expr = volumeScattering.get(); break;
    case 4: expr = volumeChroma.get(); break;
    case 5: expr = volumePhase.get(); break;
    case 6: expr = volumeEmission.get(); break;
    case 8: expr = lightChroma.get(); break;
    case 9: expr = lightAngular.get(); break;
    default: break;
    }
    std::string content;
    if (channel == 7) {
        if (geom::isSdfActive(volumeOccluder.get())) content = geom::sdfToJson(*volumeOccluder).dump();
    } else if (expr) {
        content = expr->toJson().dump();
    }
    const uint64_t hash = static_cast<uint64_t>(std::hash<std::string>{}(content));

    if (_verifiedAtRevision[channel] == _authoredMathRevision &&
        _verifiedContentHash[channel] != hash) {
        std::fprintf(stderr,
                     "[FieldNode] %s.%s changed without noteAuthoredMathWritten(); "
                     "revision bumped so Screen re-reads it. Find the writer and "
                     "make it call noteAuthoredMathWritten().\n",
                     _id.c_str(), kAuthoredMathChannelNames[channel]);
        _authoredMathRevision = gAuthoredMathSequence.fetch_add(1) + 1;
    }
    _verifiedContentHash[channel] = hash;
    _verifiedAtRevision[channel] = _authoredMathRevision;
    return _authoredMathRevision;
}

nlohmann::json FieldNode::toJson() const {
    nlohmann::json j;
    j["id"] = _id;
    j["origin"] = {origin.x, origin.y, origin.z};
    j["scale"] = {scale.x, scale.y, scale.z};
    j["field"] = field->toJson();
    j["vectorField"] = vectorField->toJson();
    if (volumeDensity && !volumeDensity->pieces.empty()) {
        j["volumeDensity"] = volumeDensity->toJson();
    }
    if (volumeExtinction && !volumeExtinction->pieces.empty()) {
        j["volumeExtinction"] = volumeExtinction->toJson();
    }
    if (volumeScattering && !volumeScattering->pieces.empty()) {
        j["volumeScattering"] = volumeScattering->toJson();
    }
    if (volumeChroma && !volumeChroma->pieces.empty()) {
        j["volumeChroma"] = volumeChroma->toJson();
    }
    if (volumePhase && !volumePhase->pieces.empty()) {
        j["volumePhase"] = volumePhase->toJson();
    }
    if (volumeEmission && !volumeEmission->pieces.empty()) {
        j["volumeEmission"] = volumeEmission->toJson();
    }
    if (geom::isSdfActive(volumeOccluder.get())) {
        j["volumeOccluder"] = geom::sdfToJson(*volumeOccluder);
    }
    if (lightChroma && !lightChroma->pieces.empty()) {
        j["lightChroma"] = lightChroma->toJson();
    }
    if (lightAngular && !lightAngular->pieces.empty()) {
        j["lightAngular"] = lightAngular->toJson();
    }

    // A FieldNode is a Singular, so properties a Person/Law grants it are
    // first-order authored state just like authored Object properties. Keep
    // them beside the mathematical ASTs instead of silently dropping them at
    // the save boundary (the temporal form of Refusal #6's black box).
    if (!dynamicProperties().empty()) {
        nlohmann::json dyn = nlohmann::json::object();
        for (const auto& entry : dynamicProperties()) {
            PropertyValue live = entry.second;
            getDynamicProperty(entry.first, live);
            dyn[Earthcall::StringInterner::resolve(entry.first)] = propertyValueToJson(live);
        }
        j["authoredProperties"] = std::move(dyn);
    }
    return j;
}

void FieldNode::applyJson(const nlohmann::json& j) {
    if (j.contains("origin") && j["origin"].is_array() && j["origin"].size() == 3) {
        origin = glm::vec3(j["origin"][0], j["origin"][1], j["origin"][2]);
    }
    if (j.contains("scale") && j["scale"].is_array() && j["scale"].size() == 3) {
        scale = glm::vec3(j["scale"][0], j["scale"][1], j["scale"][2]);
    }

    if (j.contains("field")) {
        // Keep the shared object stable: PropertyRefs may already point into
        // it. Restore values, not the pointer.
        auto newField = OntoMath::ScalarField::fromJson(j["field"]);
        if (newField) {
            auto* mutField = const_cast<OntoMath::ScalarField*>(field.get());
            mutField->mode = newField->mode;
            mutField->baseDensity = newField->baseDensity;
            mutField->frequency = newField->frequency;
            mutField->amplitude = newField->amplitude;
            mutField->astDefinition = newField->astDefinition;
        }
    }

    if (j.contains("vectorField")) {
        auto newVec = OntoMath::VectorField::fromJson(j["vectorField"]);
        if (newVec) {
            auto* mutVec = const_cast<OntoMath::VectorField*>(vectorField.get());
            mutVec->mode = newVec->mode;
            mutVec->baseFlowX = newVec->baseFlowX;
            mutVec->baseFlowY = newVec->baseFlowY;
            mutVec->baseFlowZ = newVec->baseFlowZ;
            mutVec->frequency = newVec->frequency;
            mutVec->amplitude = newVec->amplitude;
            mutVec->astDefinition = newVec->astDefinition;
        }
    }

    if (volumeDensity) {
        if (j.contains("volumeDensity")) {
            *volumeDensity = OntoMath::Piecewise::fromJson(j["volumeDensity"]);
        } else {
            *volumeDensity = OntoMath::Piecewise{};
        }
    }

    if (volumeExtinction) {
        if (j.contains("volumeExtinction")) {
            *volumeExtinction = OntoMath::Piecewise::fromJson(j["volumeExtinction"]);
        } else {
            *volumeExtinction = OntoMath::Piecewise{};
        }
    }

    if (volumeScattering) {
        if (j.contains("volumeScattering")) {
            *volumeScattering = OntoMath::Piecewise::fromJson(j["volumeScattering"]);
        } else {
            *volumeScattering = OntoMath::Piecewise{};
        }
    }
    if (volumeChroma) {
        if (j.contains("volumeChroma")) {
            *volumeChroma = OntoMath::Piecewise::fromJson(j["volumeChroma"]);
        } else {
            *volumeChroma = OntoMath::Piecewise{};
        }
    }
    if (volumePhase) {
        if (j.contains("volumePhase")) {
            *volumePhase = OntoMath::Piecewise::fromJson(j["volumePhase"]);
        } else {
            *volumePhase = OntoMath::Piecewise{};
        }
    }
    if (volumeEmission) {
        if (j.contains("volumeEmission")) {
            *volumeEmission = OntoMath::Piecewise::fromJson(j["volumeEmission"]);
        } else {
            *volumeEmission = OntoMath::Piecewise{};
        }
    }
    if (volumeOccluder) {
        if (j.contains("volumeOccluder")) {
            *volumeOccluder = geom::sdfFromJson(j["volumeOccluder"]);
        } else {
            *volumeOccluder = geom::SdfNode{};
            volumeOccluder->dims = glm::vec3(0.0f);
        }
    }

    if (lightChroma) {
        if (j.contains("lightChroma")) {
            *lightChroma = OntoMath::Piecewise::fromJson(j["lightChroma"]);
        } else {
            *lightChroma = OntoMath::Piecewise{};
        }
    }
    if (lightAngular) {
        if (j.contains("lightAngular")) {
            *lightAngular = OntoMath::Piecewise::fromJson(j["lightAngular"]);
        } else {
            *lightAngular = OntoMath::Piecewise{};
        }
    }
    noteAuthoredMathWritten();

    if (j.contains("authoredProperties") && j["authoredProperties"].is_object()) {
        for (auto it = j["authoredProperties"].begin();
             it != j["authoredProperties"].end(); ++it) {
            setDynamicProperty(it.key(), propertyValueFromJson(it.value()));
        }
    }
}

std::shared_ptr<FieldNode> FieldNode::fromJson(const nlohmann::json& j) {
    auto node = std::make_shared<FieldNode>(j.value("id", "field_node"));
    node->applyJson(j);
    return node;
}

} // namespace geom
