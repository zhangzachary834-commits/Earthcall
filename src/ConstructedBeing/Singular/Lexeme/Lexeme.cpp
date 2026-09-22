#include "ConstructedBeing/Singular/Lexeme/Lexeme.hpp"
#include "ConstructedBeing/Singular/Property/Property.hpp"
#include "ConstructedBeing/Singular/Property/PropertyRef.hpp"

#include <uuid/uuid.h>

namespace Singularity {
namespace Language {

namespace {

class ConceptualWeightProperty : public Property {
public:
    explicit ConceptualWeightProperty(Lexeme* owner) : _owner(owner) {}

    std::string name() const override { return "conceptualWeight"; }
    Earthcall::StringId nameId() const override {
        static Earthcall::StringId id = Earthcall::StringInterner::intern("conceptualWeight");
        return id;
    }
    std::string typeName() const override { return "PropertyValue"; }

    PropertyValue value() const override { return _owner->conceptualWeightValue(); }
    bool setValue(const PropertyValue& v) override {
        return _owner->setConceptualWeightValue(v);
    }

    Singular* asSingular() const override { return nullptr; }

private:
    Lexeme* _owner;
};

} // namespace

Lexeme::Lexeme(const std::string& symbol) : _symbol(symbol) {
    uuid_t uuid;
    uuid_generate(uuid);
    char uuid_str[37];
    uuid_unparse(uuid, uuid_str);
    _id = "lexeme_" + std::string(uuid_str);
}

Lexeme::Lexeme(const std::string& symbol, const std::string& stableId)
    : _id(stableId), _symbol(symbol) {}

std::string Lexeme::getIdentifier() const {
    return _id;
}

void Lexeme::setConceptualWeight(float weight) {
    setConceptualWeightValue(PropertyValue(static_cast<double>(weight)));
}

bool Lexeme::setConceptualWeightValue(const PropertyValue& v) {
    double n = 0.0;
    if (propertyValueToNumber(v, n)) {
        _conceptualWeightValue = v;
        _conceptualWeight = static_cast<float>(n);
        return true;
    }
    if (std::holds_alternative<std::shared_ptr<OntoMath::ScalarField>>(v) ||
        std::holds_alternative<std::shared_ptr<OntoMath::VectorField>>(v)) {
        _conceptualWeightValue = v;
        return true;
    }
    return false;
}

void Lexeme::buildProperties() {
    registerProperty(
        std::make_unique<PropertyRef<Lexeme, std::string>>("symbol", this, &Lexeme::_symbol));
    registerProperty(std::make_unique<ConceptualWeightProperty>(this));
    _propertiesBuilt = true;
}

} // namespace Language
} // namespace Singularity
