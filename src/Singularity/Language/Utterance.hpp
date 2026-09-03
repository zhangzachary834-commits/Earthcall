#pragma once

#include "ConstructedBeing/Singular/Singular.hpp"
#include "../../ConstructedBeing/Singular/Lexeme/Lexeme.hpp"
#include "Relation/Relation.hpp"

#include <string>
#include <vector>
#include <memory>

namespace Singularity {
namespace Language {

// An Utterance represents an occurrence of language in time and space (the tokening),
// distinct from the Lexeme (the type). The occurrence-of relation binds an Utterance
// occurrence to its Lexeme types.
class Utterance : public Singular {
public:
    explicit Utterance(const std::string& rawText, double timestamp = 0.0);
    Utterance(const std::string& rawText, const std::string& id, double timestamp = 0.0);

    std::string getIdentifier() const override;

    const std::string& getRawText() const { return _rawText; }
    double getTimestamp() const { return _timestamp; }

    void addLexeme(Lexeme* lexeme);
    const std::vector<Lexeme*>& getLexemes() const { return _lexemes; }

    // Generates occurrence-of Relations connecting this Utterance occurrence
    // to each contained Lexeme type.
    std::vector<std::shared_ptr<Relation>> createOccurrenceRelations() const;

protected:
    void buildProperties() override;

private:
    std::string _id;
    std::string _rawText;
    double _timestamp = 0.0;
    std::vector<Lexeme*> _lexemes;
};

} // namespace Language
} // namespace Singularity
