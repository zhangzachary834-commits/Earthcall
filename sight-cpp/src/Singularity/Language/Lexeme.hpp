#pragma once

#include "Form/Singular/Singular.hpp"
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
    ~Lexeme() override = default;

    // Singular interface
    std::string getIdentifier() const override;

    // The symbolic value of this lexeme (e.g., "Joy", "Tree")
    const std::string& getSymbol() const { return _symbol; }

    // Frequency or conceptual weight of this lexeme in the current context
    float getConceptualWeight() const { return _conceptualWeight; }
    void setConceptualWeight(float weight) { _conceptualWeight = weight; }

protected:
    void buildProperties() override;

private:
    std::string _id;
    std::string _symbol;
    float _conceptualWeight = 1.0f;
};

} // namespace Language
} // namespace Singularity
