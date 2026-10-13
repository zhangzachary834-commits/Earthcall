#pragma once

#include "Singularity/Foreign/Web/Designer/WebConcept.hpp"
#include "ConstructedBeing/Singular/Lexeme/Lexeme.hpp"
#include "Relation/Relation.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace Singularity {
namespace Foreign {
namespace Web {

// ---------------------------------------------------------------------------
// Web Designer Lexeme & Metalaw Architecture
//
// In accordance with Earthcall precedent:
//   "Characters / authored input -> LawLine / Lexemes -> Lexeme denotes / relates
//    to Law or Metalaw -> semantic authored consequence."
//
// Meaning belongs in Lexeme <---denotes---> Law / Metalaw relations, NOT in
// a static C++ dictionary or tokenizer switch statement.
// ---------------------------------------------------------------------------

struct WebLexemeDef {
    std::string symbol;      // Natural language spelling ("heading", "large", "centered")
    std::string opcode;      // Opcode tag ("concept.role.heading", "concept.size.large")
    std::string stableId;    // Lexeme ID ("lexeme.web.heading")
    std::string metalawId;   // Denoted Law ID ("law.concept.heading")
    std::string description; // Semantic gloss
};

class WebDesignerMetalaws {
public:
    // Seeds standard authoring Lexemes, Laws, and "denotes" Relations into LawManager & world
    static void syncRegister(LawManager& laws, Singular* author = nullptr);

    // Access live seeded Lexemes and denotation Relations
    static const std::vector<std::shared_ptr<Singularity::Language::Lexeme>>& seededLexemes();
    static const std::vector<std::shared_ptr<Relation>>& seededRelations();

    // Look up vocabulary definition for inspection/completion
    static const WebLexemeDef* findDefinition(const std::string& symbol);
    static const std::vector<WebLexemeDef>& allDefinitions();

    // Compile tokens into a target Singular using in-world Lexemes and their "denotes" Relations
    static WebConcept compileTokens(const std::vector<std::string>& tokens,
                                    LawManager& laws,
                                    Singular& channelTarget,
                                    std::string* outError = nullptr,
                                    const std::vector<Relation*>& extraRelations = {});

    // Parse and compile an authored natural language phrase into a target Singular
    static WebConcept compilePhrase(const std::string& phrase,
                                    LawManager& laws,
                                    Singular& channelTarget,
                                    std::string* outError = nullptr,
                                    const std::vector<Relation*>& extraRelations = {});

    // Create an authored newborn Singular (Object) in the world with real authorship relation
    static std::shared_ptr<Singular> createAuthoredElement(
        const std::string& identifier,
        const WebConcept& concept,
        Singular* author = nullptr,
        std::vector<std::shared_ptr<Relation>>* outRelations = nullptr);
};

} // namespace Web
} // namespace Foreign
} // namespace Singularity
