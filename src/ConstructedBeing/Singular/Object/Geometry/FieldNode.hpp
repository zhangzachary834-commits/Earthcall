#pragma once

#include "ConstructedBeing/Singular/Singular.hpp"
#include "ConstructedBeing/Singular/Property/PropertyRef.hpp"
#include "Singularity/OntoMath/Field.hpp"
#include <glm/glm.hpp>
#include "json.hpp"
#include <memory>
#include <string>

namespace geom {

// An OntoMath Piecewise, addressable as the JSON it already serializes to.
// Reading gives the whole tree; writing replaces it, and a document that does
// not parse is refused outright so a field is never left half-rewritten.
class AstBridge : public Property {
public:
    explicit AstBridge(std::string name, OntoMath::Piecewise* ast)
        : _name(std::move(name)), _nameId(Earthcall::StringInterner::intern(_name)), _ast(ast) {}

    std::string name() const override { return _name; }
    Earthcall::StringId nameId() const override { return _nameId; }
    std::string typeName() const override { return "string"; }

    PropertyValue value() const override {
        if (!_ast) return PropertyValue(std::string("{}"));
        return PropertyValue(_ast->toJson().dump());
    }
    bool setValue(const PropertyValue& v) override {
        if (!_ast) return false;
        const std::string* src = std::get_if<std::string>(&v);
        if (!src) return false;
        nlohmann::json parsed = nlohmann::json::parse(*src, nullptr, false);
        if (parsed.is_discarded()) return false;   // malformed: refuse, keep the old AST
        *_ast = OntoMath::Piecewise::fromJson(parsed);
        return true;
    }

private:
    std::string _name;
    Earthcall::StringId _nameId;
    OntoMath::Piecewise* _ast;
};

// A FieldNode represents the spatial placement of an OntoMath Field within the scene.
// By inheriting from Singular, it maps the field's mathematical variables into the 
// PropertyPath system, allowing the Law system to modulate the field dynamically.
class FieldNode : public Singular {
private:
    std::string _id;

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
        registerProperty(std::make_unique<PropertyRef<FieldNode, glm::vec3>>("origin", this, &FieldNode::origin));
        registerProperty(std::make_unique<PropertyRef<FieldNode, glm::vec3>>("scale", this, &FieldNode::scale));

        // Expose mathematical configuration from the underlying OntoMath field
        if (field) {
            registerProperty(std::make_unique<PropertyRef<OntoMath::ScalarField, float>>("field.baseDensity", field.get(), &OntoMath::ScalarField::baseDensity));
            registerProperty(std::make_unique<PropertyRef<OntoMath::ScalarField, float>>("field.frequency", field.get(), &OntoMath::ScalarField::frequency));
            registerProperty(std::make_unique<PropertyRef<OntoMath::ScalarField, float>>("field.amplitude", field.get(), &OntoMath::ScalarField::amplitude));
            
            // The AST itself, as its own serialized form. It used to reach no
            // property path at all -- the note here said the Law system could
            // rewrite it "via specialized OntoMath endpoints or over the
            // network", which is to say: not by law. That is the black box
            // refusal #6 forbids, and the mathematics of a field is exactly
            // the state a Person most needs to reach.
            //
            // A Piecewise is a recursive tree, so it is exposed the way the
            // rest of the engine exposes recursive state -- as the JSON it
            // already round-trips through. Readable in full; writable, with a
            // malformed document REFUSED rather than half-applied.
            registerProperty(std::make_unique<AstBridge>(
                "field.ast", &field->astDefinition));
        }

        if (vectorField) {
            registerProperty(std::make_unique<PropertyRef<OntoMath::VectorField, float>>("vectorField.baseFlowX", vectorField.get(), &OntoMath::VectorField::baseFlowX));
            registerProperty(std::make_unique<PropertyRef<OntoMath::VectorField, float>>("vectorField.baseFlowY", vectorField.get(), &OntoMath::VectorField::baseFlowY));
            registerProperty(std::make_unique<PropertyRef<OntoMath::VectorField, float>>("vectorField.baseFlowZ", vectorField.get(), &OntoMath::VectorField::baseFlowZ));
            registerProperty(std::make_unique<PropertyRef<OntoMath::VectorField, float>>("vectorField.frequency", vectorField.get(), &OntoMath::VectorField::frequency));
            registerProperty(std::make_unique<PropertyRef<OntoMath::VectorField, float>>("vectorField.amplitude", vectorField.get(), &OntoMath::VectorField::amplitude));
            registerProperty(std::make_unique<AstBridge>(
                "vectorField.ast", &vectorField->astDefinition));
        }
    }
};

} // namespace geom
