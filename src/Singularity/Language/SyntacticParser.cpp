#include "SyntacticParser.hpp"
#include "Singularity/Language/LanguageSystem.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "Relation/RelationManager.hpp"
#include <cctype>
#include <algorithm>

namespace Singularity {
namespace Language {

std::vector<std::string> SyntacticParser::tokenize(const std::string& input) {
    std::vector<std::string> tokens;
    std::string current;
    for (char c : input) {
        if (std::isspace(static_cast<unsigned char>(c)) ||
            std::ispunct(static_cast<unsigned char>(c))) {
            if (!current.empty()) {
                std::transform(current.begin(), current.end(), current.begin(),
                               [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
                tokens.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }
    if (!current.empty()) {
        std::transform(current.begin(), current.end(), current.begin(),
                       [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        tokens.push_back(current);
    }
    return tokens;
}

Lexeme* SyntacticParser::resolvePOS(Lexeme& lexeme, Zone& activeZone) {
    auto rels = activeZone.formation().relations().getRelationsOf(lexeme);
    for (const auto& r : rels) {
        if (r && r->type == "is_pos" && r->a() == &lexeme) {
            return dynamic_cast<Lexeme*>(r->b());
        }
    }
    return nullptr;
}

std::string SyntacticParser::resolveMeaning(Lexeme& verbPhrase, Zone& activeZone) {
    auto rels = activeZone.formation().relations().getRelationsOf(verbPhrase);
    for (const auto& r : rels) {
        if (r && r->type == "resolves_to" && r->a() == &verbPhrase) {
            if (auto* meaning = dynamic_cast<Lexeme*>(r->b())) {
                return meaning->getSymbol();
            }
        }
    }
    return verbPhrase.getSymbol();
}

std::vector<std::shared_ptr<Relation>> SyntacticParser::parse(const std::string& utterance, Zone& activeZone) {
    auto& language = LanguageSystem::instance();
    std::vector<std::string> tokens = tokenize(utterance);
    std::vector<std::shared_ptr<Relation>> results;

    enum State { EXPECTING_SUBJECT, EXPECTING_VERB, EXPECTING_OBJECT };
    State state = EXPECTING_SUBJECT;

    Lexeme* subject = nullptr;
    std::string relationType;

    for (const auto& symbol : tokens) {
        auto lexeme = language.resolve(symbol);
        if (!lexeme) continue;
        activeZone.addToFormation(lexeme.get());

        Lexeme* pos = resolvePOS(*lexeme, activeZone);
        const std::string posSymbol = pos ? pos->getSymbol() : std::string("unknown");

        if (posSymbol == "determiner") {
            continue;
        }

        if (state == EXPECTING_SUBJECT) {
            if (posSymbol == "noun" || posSymbol == "unknown") {
                subject = lexeme.get();
                state = EXPECTING_VERB;
            }
        }
        else if (state == EXPECTING_VERB) {
            if (posSymbol == "verb" || posSymbol == "preposition") {
                if (relationType.empty()) {
                    relationType = lexeme->getSymbol();
                } else {
                    relationType += "_" + lexeme->getSymbol();
                }
            } else if (posSymbol == "noun" || posSymbol == "unknown") {
                if (!relationType.empty() && subject) {
                    auto phrase = language.resolve(relationType);
                    activeZone.addToFormation(phrase.get());
                    const std::string canonical = resolveMeaning(*phrase, activeZone);

                    auto rel = std::make_shared<Relation>(canonical, *subject, *lexeme, true);
                    rel->setWeight(0.5f);
                    rel->setDynamicProperty("decayRate", 0.02f);
                    results.push_back(rel);

                    subject = nullptr;
                    relationType.clear();
                    state = EXPECTING_SUBJECT;
                } else {
                    subject = lexeme.get();
                }
            }
        }
    }

    return results;
}

} // namespace Language
} // namespace Singularity
