#include "ConstructedBeing/Singular/Lexeme/Lexeme.hpp"
#include "ConstructedBeing/Singular/Property/PropertyRef.hpp"

#include <uuid/uuid.h>

namespace Singularity {
namespace Language {

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

void Lexeme::buildProperties() {
    _propertyRegistry.push_back(
        std::make_unique<PropertyRef<Lexeme, std::string>>("symbol", this, &Lexeme::_symbol));
    _propertyRegistry.push_back(
        std::make_unique<PropertyRef<Lexeme, float>>("conceptualWeight", this, &Lexeme::_conceptualWeight));
    _propertiesBuilt = true;
}

} // namespace Language
} // namespace Singularity
