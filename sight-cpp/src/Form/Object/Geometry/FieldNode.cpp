#include "FieldNode.hpp"

namespace geom {

nlohmann::json FieldNode::toJson() const {
    nlohmann::json j;
    j["id"] = _id;
    j["origin"] = {origin.x, origin.y, origin.z};
    j["scale"] = {scale.x, scale.y, scale.z};
    j["field"] = field->toJson();
    return j;
}

std::shared_ptr<FieldNode> FieldNode::fromJson(const nlohmann::json& j) {
    auto node = std::make_shared<FieldNode>(j.value("id", "field_node"));
    
    if (j.contains("origin") && j["origin"].is_array() && j["origin"].size() == 3) {
        node->origin = glm::vec3(j["origin"][0], j["origin"][1], j["origin"][2]);
    }
    if (j.contains("scale") && j["scale"].is_array() && j["scale"].size() == 3) {
        node->scale = glm::vec3(j["scale"][0], j["scale"][1], j["scale"][2]);
    }
    
    if (j.contains("field")) {
        // Since node->field is const shared_ptr, we can't reassign the pointer itself.
        // We can either deserialize a new ScalarField and copy its values into node->field,
        // or change the constructor to take the field.
        // Copying values is safer to preserve the already-registered pointer in properties.
        auto newField = OntoMath::ScalarField::fromJson(j["field"]);
        if (newField) {
            auto mutField = const_cast<OntoMath::ScalarField*>(node->field.get());
            mutField->mode = newField->mode;
            mutField->baseDensity = newField->baseDensity;
            mutField->frequency = newField->frequency;
            mutField->amplitude = newField->amplitude;
            mutField->astDefinition = newField->astDefinition;
        }
    }
    return node;
}

} // namespace geom
