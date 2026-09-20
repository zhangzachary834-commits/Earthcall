#include "FieldNode.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValueJson.hpp"
#include "Singularity/Storage/Serialization/Common/SingularPropertySerialization.hpp"

namespace geom {

nlohmann::json FieldNode::toJson() const {
    nlohmann::json j;
    j["id"] = _id;
    j["origin"] = {origin.x, origin.y, origin.z};
    j["scale"] = {scale.x, scale.y, scale.z};
    j["field"] = field->toJson();
    j["vectorField"] = vectorField->toJson();

    // FieldNode participates in the same universal Singular envelope as every
    // other persistence root; its mathematical payload above stays canonical.
    Singularity::Storage::writeSingularProperties(j, *this);
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

    Singularity::Storage::readSingularProperties(j, *this);
}

std::shared_ptr<FieldNode> FieldNode::fromJson(const nlohmann::json& j) {
    auto node = std::make_shared<FieldNode>(j.value("id", "field_node"));
    node->applyJson(j);
    return node;
}

} // namespace geom
