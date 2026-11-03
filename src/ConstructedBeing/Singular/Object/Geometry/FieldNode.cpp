#include "FieldNode.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/SdfJson.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValueJson.hpp"

#include <algorithm>
#include <array>
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

std::string FieldNode::channelContent(std::size_t channel) const {
    const OntoMath::Piecewise* expr = nullptr;
    switch (channel) {
    case kMathFieldAst:
        // Mode joins the content: switching to AST evaluation changes what
        // Screen reads even when the tree is unchanged.
        return field ? std::to_string(static_cast<int>(field->mode)) + ":" +
                           field->astDefinition.toJson().dump()
                     : std::string();
    case kMathVolumeDensity: expr = volumeDensity.get(); break;
    case kMathVolumeExtinction: expr = volumeExtinction.get(); break;
    case kMathVolumeScattering: expr = volumeScattering.get(); break;
    case kMathVolumeChroma: expr = volumeChroma.get(); break;
    case kMathVolumePhase: expr = volumePhase.get(); break;
    case kMathVolumeEmission: expr = volumeEmission.get(); break;
    case kMathVolumeOccluder:
        return geom::isSdfActive(volumeOccluder.get()) ? geom::sdfToJson(*volumeOccluder).dump()
                                                       : std::string();
    case kMathLightChroma: expr = lightChroma.get(); break;
    case kMathLightAngular: expr = lightAngular.get(); break;
    default: break;
    }
    return expr ? expr->toJson().dump() : std::string();
}

void FieldNode::noteAuthoredChannelWritten(std::size_t channel) {
    if (channel < kAuthoredMathChannels)
        _channelRevision[channel] = gAuthoredMathSequence.fetch_add(1) + 1;
}

void FieldNode::noteAuthoredMathWritten() {
    for (std::size_t c = 0; c < kAuthoredMathChannels; ++c) noteAuthoredChannelWritten(c);
}

uint64_t FieldNode::authoredMathRevision() const {
    uint64_t newest = 0;
    for (const uint64_t r : _channelRevision) newest = std::max(newest, r);
    return newest;
}

void FieldNode::verifyAuthoredMath() const {
    static_assert(sizeof(kAuthoredMathChannelNames) / sizeof(kAuthoredMathChannelNames[0]) ==
                  kAuthoredMathChannels, "one name per verified channel");
    const std::size_t channel = _verifyCursor;
    _verifyCursor = (_verifyCursor + 1) % kAuthoredMathChannels;
    const uint64_t hash = static_cast<uint64_t>(std::hash<std::string>{}(channelContent(channel)));

    if (_verifiedAtRevision[channel] == _channelRevision[channel] &&
        _verifiedContentHash[channel] != hash) {
        std::fprintf(stderr,
                     "[FieldNode] %s.%s changed without noteAuthoredChannelWritten(); "
                     "that channel's revision was bumped so Screen re-reads it. Find the "
                     "writer and make it note the channel it changed.\n",
                     _id.c_str(), kAuthoredMathChannelNames[channel]);
        _channelRevision[channel] = gAuthoredMathSequence.fetch_add(1) + 1;
    }
    _verifiedContentHash[channel] = hash;
    _verifiedAtRevision[channel] = _channelRevision[channel];
}

uint64_t FieldNode::verifiedAuthoredMathRevision() const {
    verifyAuthoredMath();
    return authoredMathRevision();
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
    // Per-channel delta: only channels whose content actually changes move.
    std::array<std::string, kAuthoredMathChannels> before;
    for (std::size_t c = 0; c < kAuthoredMathChannels; ++c) before[c] = channelContent(c);
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
    for (std::size_t c = 0; c < kAuthoredMathChannels; ++c) {
        if (channelContent(c) != before[c]) noteAuthoredChannelWritten(c);
    }

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
