#pragma once

#include "Singularity/Foreign/Web/Designer/WebConcept.hpp"
#include "ConstructedBeing/Singular/Lexeme/Lexeme.hpp"
#include "Relation/Relation.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include <string>
#include <vector>
#include <memory>

class Person;

namespace Singularity {
namespace Foreign {
namespace Web {

// ---------------------------------------------------------------------------
// Web Designer Lexeme & Metalaw Architecture
//
// In accordance with Earthcall ontology:
//   "Authorship in Earthcall means human stewardship. It is not efficient causation,
//    runtime creation, initialization, compilation, injection, ownership, or
//    'the thing that happened to make this object exist.'"
//
// Person authorship is Person-exclusive — no arbitrary Singular, Object, or
// First-Mover channel may masquerade as a human author.
// Built-in substrate vocabulary is unauthored infrastructure; runtime
// initialization does not assign a human author.
// Meaning belongs in Lexeme <---denotes---> Law / Metalaw relations.
// ---------------------------------------------------------------------------

class WebDesignerMetalaws {
public:
    // Seeds standard substrate authoring Lexemes, Laws, and "denotes" Relations into LawManager.
    // Built-in infrastructure has NO human author.
    static void syncRegister(LawManager& laws);

    // Access live seeded Lexemes and denotation Relations
    static const std::vector<std::shared_ptr<Singularity::Language::Lexeme>>& seededLexemes();
    static const std::vector<std::shared_ptr<Relation>>& seededRelations();

    // In-World Lexeme Relation Scanner:
    // Compiles tokens into a target Singular using in-world Lexemes and their "denotes" Relations.
    // Resolution occurs exclusively via live Relation("denotes", *lexeme, *law) edges.
    //
    // Architectural Note: This is an in-world Lexeme scanner operating over the active
    // denotation relation graph (Relation("denotes", *lexeme, *law)). It does not invoke
    // the Mac Terminal's LawSentence parser (which is explicitly scoped only to the Mac
    // Terminal command-line grammar per Zach 2026-09-25) nor SyntacticParser (which requires
    // an active Zone context and resolves parts of speech).
    static WebConcept compileTokens(const std::vector<std::string>& tokens,
                                    LawManager& laws,
                                    Singular& channelTarget,
                                    std::string* outError = nullptr,
                                    const std::vector<Relation*>& extraRelations = {});

    // Parse and compile an authored natural language phrase into a target Singular.
    // Scans character sequences against in-world Lexemes and resolves via denotes Relations.
    static WebConcept compilePhrase(const std::string& phrase,
                                    LawManager& laws,
                                    Singular& channelTarget,
                                    std::string* outError = nullptr,
                                    const std::vector<Relation*>& extraRelations = {});

    // Create an authored newborn Singular (Object) in the world with real Person authorship relation.
    // If an author (a living Person) is provided, connects Relation("authored-by", *obj, *author).
    // Person authorship is Person-exclusive — no arbitrary Singular or Object may satisfy it.
    static std::shared_ptr<Singular> createAuthoredElement(
        const std::string& identifier,
        const WebConcept& concept,
        Person* author = nullptr,
        std::vector<std::shared_ptr<Relation>>* outRelations = nullptr);
};

} // namespace Web
} // namespace Foreign
} // namespace Singularity
