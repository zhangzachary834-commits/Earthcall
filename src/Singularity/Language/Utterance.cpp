#include "Singularity/Language/Utterance.hpp"
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"
#include "ConstructedBeing/Singular/Property/PropertyRef.hpp"
#include <atomic>

namespace Singularity {
namespace Language {

namespace {
std::string nextUtteranceId() {
    static std::atomic<unsigned long long> next{1};
    return "utterance-" + std::to_string(next.fetch_add(1));
}
}

Utterance::Utterance(const std::string& rawText, double timestamp)
    : _id(nextUtteranceId()), _rawText(rawText), _timestamp(timestamp) {}

Utterance::Utterance(const std::string& rawText, const std::string& id, double timestamp)
    : _id(id), _rawText(rawText), _timestamp(timestamp) {}

std::string Utterance::getIdentifier() const {
    return _id;
}

void Utterance::addLexeme(Lexeme* lexeme) {
    if (lexeme) {
        _lexemes.push_back(lexeme);
    }
}

std::vector<std::shared_ptr<Relation>> Utterance::createOccurrenceRelations() const {
    std::vector<std::shared_ptr<Relation>> relations;
    for (Lexeme* lex : _lexemes) {
        if (lex) {
            auto rel = std::make_shared<Relation>("occurrence-of", const_cast<Utterance&>(*this), *lex, true);
            relations.push_back(rel);
        }
    }
    return relations;
}

void Utterance::buildProperties() {
    registerProperty(std::make_unique<PropertyRef<Utterance, std::string>>(
        "rawText", this, &Utterance::_rawText));
    registerProperty(std::make_unique<PropertyRef<Utterance, double>>(
        "timestamp", this, &Utterance::_timestamp));
}

} // namespace Language
} // namespace Singularity
