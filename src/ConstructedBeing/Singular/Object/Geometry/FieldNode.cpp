#include "FieldNode.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValueJson.hpp"

namespace geom {

nlohmann::json FieldNode::toJson() const {
    nlohmann::json j;
    j["id"] = _id;
    j["origin"] = {origin.x, origin.y, origin.z};
    j["scale"] = {scale.x, scale.y, scale.z};
    j["field"] = field->toJson();
    j["vectorField"] = vectorField->toJson();

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
