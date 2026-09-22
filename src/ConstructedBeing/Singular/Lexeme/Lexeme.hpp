#pragma once

#include "ConstructedBeing/Singular/Singular.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValue.hpp"
#include <string>
#include <memory>

namespace Singularity {
namespace Language {

// A Lexeme represents a linguistic-symbolic unit (a word, a phrase, a JSON key)
// natively instantiated as a physical entity within the Earthcall graph.
// This allows Laws to target words mathematically and probabilistically using OntoMath.
class Lexeme : public Singular {
public:
    explicit Lexeme(const std::string& symbol);
    // First-mover seed: a stable slug law-text can name (`lexeme.christ`).
    // Ordinary resolve() still mints a uuid so authored words do not collide.
    Lexeme(const std::string& symbol, const std::string& stableId);
    ~Lexeme() override = default;

    // Singular interface
    std::string getIdentifier() const override;

    // Hardcoded linguistic string: the characters this Lexeme is. Authored
    // vocabulary lives on the being as this property, never as a Relation
    // endpoint string.
    const std::string& getSymbol() const { return _symbol; }

    // Fast numeric view for channels that only multiply. The authored value
    // is `_conceptualWeightValue` (int/float/double, or an OntoMath field).
    float getConceptualWeight() const { return _conceptualWeight; }
    void setConceptualWeight(float weight);

    const PropertyValue& conceptualWeightValue() const { return _conceptualWeightValue; }
    bool setConceptualWeightValue(const PropertyValue& v);

protected:
    void buildProperties() override;

private:
    std::string _id;
    std::string _symbol;
    // Type-flexible authored weight (doubles, OntoMath fields). The float is
    // the cheap cache when the value is a number — it does not replace this.
    PropertyValue _conceptualWeightValue = 1.0;
    float _conceptualWeight = 1.0f;
};

} // namespace Language
} // namespace Singularity
