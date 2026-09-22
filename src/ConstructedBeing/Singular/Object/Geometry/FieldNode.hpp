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
//
// Writing an AST also selects AST evaluation. Without this transition a Person
// could successfully author field.ast while the Field remained Procedural; the
// authored tree would neither govern evaluation nor be emitted by Field::toJson.
// Keep those two pieces of state coherent at the authoring boundary.
template <typename FieldT>
class AstBridge : public Property {
public:
    explicit AstBridge(std::string name, FieldT* field)
        : _name(std::move(name)), _nameId(Earthcall::StringInterner::intern(_name)), _field(field) {}

    std::string name() const override { return _name; }
    Earthcall::StringId nameId() const override { return _nameId; }
    std::string typeName() const override { return "string"; }

    PropertyValue value() const override {
        if (!_field) return PropertyValue(std::string("{}"));
        return PropertyValue(_field->astDefinition.toJson().dump());
    }
    bool setValue(const PropertyValue& v) override {
        if (!_field) return false;
        const std::string* src = std::get_if<std::string>(&v);
        if (!src) return false;
        nlohmann::json parsed = nlohmann::json::parse(*src, nullptr, false);
        if (parsed.is_discarded()) return false;   // malformed: refuse, keep the old AST and mode

        _field->astDefinition = OntoMath::Piecewise::fromJson(parsed);
        _field->mode = FieldT::EvaluationMode::AST;
        return true;
    }

private:
    std::string _name;
    Earthcall::StringId _nameId;
    FieldT* _field;
};

// An authored Piecewise that is semantically its own channel rather than a
// ScalarField/VectorField container. Rung 5 uses this for source chroma chi(p,t):
// vec3. The bridge keeps the recursive mathematics reachable by Law without
// pretending RGB is the existing flow/force VectorField merely because both are vec3.
class PiecewiseAstBridge : public Property {
public:
    PiecewiseAstBridge(std::string name, OntoMath::Piecewise* expr)
        : _name(std::move(name)), _nameId(Earthcall::StringInterner::intern(_name)), _expr(expr) {}

    std::string name() const override { return _name; }
    Earthcall::StringId nameId() const override { return _nameId; }
    std::string typeName() const override { return "string"; }

    PropertyValue value() const override {
        if (!_expr) return PropertyValue(std::string("{}"));
        return PropertyValue(_expr->toJson().dump());
    }
    bool setValue(const PropertyValue& v) override {
        if (!_expr) return false;
        const std::string* src = std::get_if<std::string>(&v);
        if (!src) return false;
        nlohmann::json parsed = nlohmann::json::parse(*src, nullptr, false);
        if (parsed.is_discarded()) return false;
        *_expr = OntoMath::Piecewise::fromJson(parsed);
        return true;
    }

private:
    std::string _name;
    Earthcall::StringId _nameId;
    OntoMath::Piecewise* _expr;
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
          vectorField(std::make_shared<OntoMath::VectorField>()),
          volumeDensity(std::make_shared<OntoMath::Piecewise>()),
          lightChroma(std::make_shared<OntoMath::Piecewise>()),
          lightAngular(std::make_shared<OntoMath::Piecewise>()) {}

    std::string getIdentifier() const override { return _id; }

    // Spatial transform
    glm::vec3 origin{0.0f};
    glm::vec3 scale{1.0f};

    // The pure mathematical field definition
    // Const pointer ensures the property registry doesn't dangle
    const std::shared_ptr<OntoMath::ScalarField> field;
    const std::shared_ptr<OntoMath::VectorField> vectorField;

    // V0 participating-medium density D(p,t) -> scalar. This is deliberately
    // independent from the generic scalar field and from source radiance rho.
    // Empty means no explicitly authored volume-density channel; compatibility
    // migration, where required, is resolved outside this storage boundary.
    const std::shared_ptr<OntoMath::Piecewise> volumeDensity;

    // Optional source-side chroma chi(p,t) -> vec3. Empty means ABSENT, in which
    // case the historical authored light.color remains the constant chroma.
    // This is deliberately not VectorField: that existing vessel means flow/force.
    const std::shared_ptr<OntoMath::Piecewise> lightChroma;

    // Optional source-side angular factor alpha(p,omega,t) -> scalar. Empty is
    // exactly the multiplicative identity alpha=1. Direction is bound by the
    // consuming radiance channel, never stored here as a renderer preset.
    const std::shared_ptr<OntoMath::Piecewise> lightAngular;

    nlohmann::json toJson() const;
    void applyJson(const nlohmann::json& j);
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
            registerProperty(std::make_unique<AstBridge<OntoMath::ScalarField>>(
                "field.ast", field.get()));
        }

        if (vectorField) {
            registerProperty(std::make_unique<PropertyRef<OntoMath::VectorField, float>>("vectorField.baseFlowX", vectorField.get(), &OntoMath::VectorField::baseFlowX));
            registerProperty(std::make_unique<PropertyRef<OntoMath::VectorField, float>>("vectorField.baseFlowY", vectorField.get(), &OntoMath::VectorField::baseFlowY));
            registerProperty(std::make_unique<PropertyRef<OntoMath::VectorField, float>>("vectorField.baseFlowZ", vectorField.get(), &OntoMath::VectorField::baseFlowZ));
            registerProperty(std::make_unique<PropertyRef<OntoMath::VectorField, float>>("vectorField.frequency", vectorField.get(), &OntoMath::VectorField::frequency));
            registerProperty(std::make_unique<PropertyRef<OntoMath::VectorField, float>>("vectorField.amplitude", vectorField.get(), &OntoMath::VectorField::amplitude));
            registerProperty(std::make_unique<AstBridge<OntoMath::VectorField>>(
                "vectorField.ast", vectorField.get()));
        }

        if (volumeDensity) {
            registerProperty(std::make_unique<PiecewiseAstBridge>(
                "volume.density.ast", volumeDensity.get()));
        }

        if (lightChroma) {
            registerProperty(std::make_unique<PiecewiseAstBridge>(
                "light.chroma.ast", lightChroma.get()));
        }
        if (lightAngular) {
            registerProperty(std::make_unique<PiecewiseAstBridge>(
                "light.angular.ast", lightAngular.get()));
        }
    }
};

} // namespace geom
