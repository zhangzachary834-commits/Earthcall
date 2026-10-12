#pragma once

#include "Singularity/Foreign/Web/Designer/WebConcept.hpp"
#include "ConstructedBeing/Singular/Lexeme/Lexeme.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace Singularity {
namespace Foreign {
namespace Web {

// ---------------------------------------------------------------------------
// Web Designer Metalaws & Lexeme Architecture
//
// In accordance with Zach's LawLine + Metalaw precedent:
//   "Design the Metalaws so multiple natural-language constructions can
//    converge onto stable generalized web concepts, and those concepts can
//    then be realized as HTML/CSS/DOM behavior.
//    Prefer:
//    Natural language -> Lexemes -> Metalaw-derived semantic web concepts
//    -> website representation -> HTML/CSS/DOM realization"
// ---------------------------------------------------------------------------

struct WebLexemeDef {
    std::string symbol;      // Natural language spelling ("heading", "large", "centered")
    std::string opcode;      // Opcode tag ("web.concept.role.heading", "web.concept.size.large")
    std::string stableId;    // Lexeme ID ("lexeme.web.heading")
    std::string metalawId;   // Denoted Metalaw ID ("metalaw.web.heading")
    std::string description; // Semantic gloss
};

class WebDesignerMetalaws {
public:
    // Sync-registers all standard Web Designer Lexemes and Metalaws into LawManager
    static void syncRegister(LawManager& laws, Singular* author = nullptr);

    // Look up known vocabulary definition by word/phrase (case-insensitive)
    static const WebLexemeDef* findDefinition(const std::string& symbol);

    // List all registered vocabulary definitions
    static const std::vector<WebLexemeDef>& allDefinitions();

    // Compile a list of natural language tokens or lexemes into a composed WebConcept
    // using the registered Metalaws applied against channelTarget.
    static WebConcept compileTokens(const std::vector<std::string>& tokens,
                                    LawManager& laws,
                                    Singular& channelTarget,
                                    std::string* outError = nullptr);

    // Parse and compile a natural language phrase (e.g. "a large centered heading")
    static WebConcept compilePhrase(const std::string& phrase,
                                    LawManager& laws,
                                    Singular& channelTarget,
                                    std::string* outError = nullptr);

    // Create an authored newborn Singular (Object) in the world with the compiled WebConcept
    static std::shared_ptr<Singular> createAuthoredElement(
        const std::string& identifier,
        const WebConcept& concept,
        Singular* author = nullptr);
};

} // namespace Web
} // namespace Foreign
} // namespace Singularity
