#pragma once

#include <string>
#include <vector>

class Zone;

namespace Singularity {
namespace Language {

struct Token {
    std::string raw;
    std::string normalized;
};

struct ParsedRelation {
    std::string entityA;
    std::string relationType;
    std::string entityB;
};

class SyntacticParser {
public:
    // Takes the raw utterance and the active Zone to query the semantic graph
    static std::vector<ParsedRelation> parse(const std::string& utterance, Zone& activeZone);
    
private:
    static std::vector<Token> tokenize(const std::string& input);
    
    // Graph resolution helpers
    static std::string resolvePOS(const std::string& tokenStr, Zone& activeZone);
    static std::string resolveMeaning(const std::string& tokenStr, Zone& activeZone);
};

} // namespace Language
} // namespace Singularity
