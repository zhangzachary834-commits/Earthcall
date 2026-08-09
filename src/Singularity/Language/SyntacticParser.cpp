#include "SyntacticParser.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "Relation/RelationManager.hpp"
#include <cctype>
#include <sstream>
#include <algorithm>
#include <iostream>

namespace Singularity {
namespace Language {

std::vector<Token> SyntacticParser::tokenize(const std::string& input) {
    std::vector<Token> tokens;
    std::string current;
    for (char c : input) {
        if (std::isspace(c) || std::ispunct(c)) {
            if (!current.empty()) {
                std::string norm = current;
                std::transform(norm.begin(), norm.end(), norm.begin(), ::tolower);
                tokens.push_back({current, norm});
                current.clear();
            }
        } else {
            current += c;
        }
    }
    if (!current.empty()) {
        std::string norm = current;
        std::transform(norm.begin(), norm.end(), norm.begin(), ::tolower);
        tokens.push_back({current, norm});
    }
    return tokens;
}

std::string SyntacticParser::resolvePOS(const std::string& tokenStr, Zone& activeZone) {
    auto rels = activeZone.formation().relations().getRelationsOf(tokenStr);
    for (const auto& r : rels) {
        if (r && r->type == "is_pos" && r->entityA == tokenStr) {
            return r->entityB; // e.g. "verb", "noun", "determiner"
        }
    }
    return "unknown"; // Default if not explicitly defined
}

std::string SyntacticParser::resolveMeaning(const std::string& tokenStr, Zone& activeZone) {
    auto rels = activeZone.formation().relations().getRelationsOf(tokenStr);
    for (const auto& r : rels) {
        if (r && r->type == "resolves_to" && r->entityA == tokenStr) {
            return r->entityB; // e.g. "has_inventory"
        }
    }
    return tokenStr; // Defaults to itself
}

std::vector<ParsedRelation> SyntacticParser::parse(const std::string& utterance, Zone& activeZone) {
    std::vector<Token> tokens = tokenize(utterance);
    std::vector<ParsedRelation> results;
    
    enum State { EXPECTING_SUBJECT, EXPECTING_VERB, EXPECTING_OBJECT };
    State state = EXPECTING_SUBJECT;
    
    ParsedRelation currentRel;
    
    for (const auto& token : tokens) {
        std::string pos = resolvePOS(token.normalized, activeZone);
        
        if (pos == "determiner") {
            continue; // Skip words like "the", "a", "an"
        }
        
        if (state == EXPECTING_SUBJECT) {
            if (pos == "noun" || pos == "unknown") {
                currentRel.entityA = token.raw;
                state = EXPECTING_VERB;
            }
        } 
        else if (state == EXPECTING_VERB) {
            if (pos == "verb" || pos == "preposition") {
                if (currentRel.relationType.empty()) {
                    currentRel.relationType = token.normalized;
                } else {
                    currentRel.relationType += "_" + token.normalized;
                }
            } else if (pos == "noun" || pos == "unknown") {
                if (!currentRel.relationType.empty()) {
                    currentRel.entityB = token.raw;
                    
                    // Resolve synonym/canonical verb mapping from the graph
                    currentRel.relationType = resolveMeaning(currentRel.relationType, activeZone);
                    
                    results.push_back(currentRel);
                    // Reset for next potential relation in compound sentences
                    currentRel = ParsedRelation();
                    state = EXPECTING_SUBJECT;
                } else {
                    // Two subjects in a row? Overwrite current subject.
                    currentRel.entityA = token.raw;
                }
            }
        }
    }
    
    return results;
}

} // namespace Language
} // namespace Singularity
