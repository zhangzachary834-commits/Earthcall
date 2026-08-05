#pragma once

#include "Form/Singular/Singular.hpp"
#include "Form/Singular/Property/PropertyRef.hpp"
#include "Singularity/OntoMath/Field.hpp"
#include <glm/glm.hpp>
#include "json.hpp"
#include <memory>
#include <string>

namespace geom {

// A FieldNode represents the spatial placement of an OntoMath Field within the scene.
// By inheriting from Singular, it maps the field's mathematical variables into the 
// PropertyPath system, allowing the Law system to modulate the field dynamically.
class FieldNode : public Singular {
public:
    FieldNode(std::string id = "field_node") 
        : _id(std::move(id)), 
          field(std::make_shared<OntoMath::ScalarField>()),
          vectorField(std::make_shared<OntoMath::VectorField>()) {}

    std::string getIdentifier() const override { return _id; }

    // Spatial transform
    glm::vec3 origin{0.0f};
    glm::vec3 scale{1.0f};

    // The pure mathematical field definition
    // Const pointer ensures the property registry doesn't dangle
    const std::shared_ptr<OntoMath::ScalarField> field;
    const std::shared_ptr<OntoMath::VectorField> vectorField;

    nlohmann::json toJson() const;
    static std::shared_ptr<FieldNode> fromJson(const nlohmann::json& j);

protected:
    void buildProperties() override {
        // Expose spatial transform
        _propertyRegistry.push_back(std::make_unique<PropertyRef<FieldNode, glm::vec3>>("origin", this, &FieldNode::origin));
        _propertyRegistry.push_back(std::make_unique<PropertyRef<FieldNode, glm::vec3>>("scale", this, &FieldNode::scale));

        // Expose mathematical configuration from the underlying OntoMath field
        if (field) {
            _propertyRegistry.push_back(std::make_unique<PropertyRef<OntoMath::ScalarField, float>>("field.baseDensity", field.get(), &OntoMath::ScalarField::baseDensity));
            _propertyRegistry.push_back(std::make_unique<PropertyRef<OntoMath::ScalarField, float>>("field.frequency", field.get(), &OntoMath::ScalarField::frequency));
            _propertyRegistry.push_back(std::make_unique<PropertyRef<OntoMath::ScalarField, float>>("field.amplitude", field.get(), &OntoMath::ScalarField::amplitude));
            
            // NOTE: The AST definition (field->astDefinition) is structurally more complex, 
            // but the Law system can rewrite it via specialized OntoMath endpoints or by 
            // replacing the AST entirely over the network.
        }

        if (vectorField) {
            _propertyRegistry.push_back(std::make_unique<PropertyRef<OntoMath::VectorField, float>>("vectorField.baseFlowX", vectorField.get(), &OntoMath::VectorField::baseFlowX));
            _propertyRegistry.push_back(std::make_unique<PropertyRef<OntoMath::VectorField, float>>("vectorField.baseFlowY", vectorField.get(), &OntoMath::VectorField::baseFlowY));
            _propertyRegistry.push_back(std::make_unique<PropertyRef<OntoMath::VectorField, float>>("vectorField.baseFlowZ", vectorField.get(), &OntoMath::VectorField::baseFlowZ));
            _propertyRegistry.push_back(std::make_unique<PropertyRef<OntoMath::VectorField, float>>("vectorField.frequency", vectorField.get(), &OntoMath::VectorField::frequency));
            _propertyRegistry.push_back(std::make_unique<PropertyRef<OntoMath::VectorField, float>>("vectorField.amplitude", vectorField.get(), &OntoMath::VectorField::amplitude));
        }
    }

private:
    std::string _id;
};

} // namespace geom
