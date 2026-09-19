#pragma once

#include "Relation/Relation.hpp"
#include "../../ConstructedBeing/Singular/Lexeme/Lexeme.hpp"

#include <memory>
#include <string>
#include <vector>

class Zone;

namespace Singularity {
namespace Language {

// Surface-form split of an utterance into Lexemes, then into Relations
// between those Lexemes. Token is not a being — the occurrence/type cut
// belongs to utterance vs Lexeme, and this parser does not model occurrence.
// ParsedRelation is not a being — the committed edge is Relation.
class SyntacticParser {
public:
    static std::vector<std::shared_ptr<Relation>> parse(const std::string& utterance, Zone& activeZone);

private:
    static std::vector<std::string> tokenize(const std::string& input);

    static Lexeme* resolvePOS(Lexeme& lexeme, Zone& activeZone);
    static std::string resolveMeaning(Lexeme& verbPhrase, Zone& activeZone);
};

} // namespace Language
} // namespace Singularity
