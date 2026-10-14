#pragma once

#include "Singularity/Foreign/Web/Designer/WebConcept.hpp"
#include "ConstructedBeing/Singular/Lexeme/Lexeme.hpp"
#include "Relation/Relation.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include <string>
#include <vector>
#include <memory>

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
// There is NO "Zach Plush" (fake Object pretending to be a Person).
// Live Person authorship is preserved through genuine Person beings.
// ---------------------------------------------------------------------------

class WebDesignerMetalaws {
public:
    // Seeds standard authoring Lexemes, Laws, and "denotes" Relations into LawManager & world.
    // If author is null, the seeding uses the First Mover channel identity, NEVER a fake Person object.
    static void syncRegister(LawManager& laws, Singular* author = nullptr);

    // Access live seeded Lexemes and denotation Relations
    static const std::vector<std::shared_ptr<Singularity::Language::Lexeme>>& seededLexemes();
    static const std::vector<std::shared_ptr<Relation>>& seededRelations();

    // Compile tokens into a target Singular using in-world Lexemes and their "denotes" Relations.
    // Resolution occurs exclusively via live Relation("denotes", *lexeme, *law) edges.
    static WebConcept compileTokens(const std::vector<std::string>& tokens,
                                    LawManager& laws,
                                    Singular& channelTarget,
                                    std::string* outError = nullptr,
                                    const std::vector<Relation*>& extraRelations = {});

    // Parse and compile an authored natural language phrase into a target Singular.
    static WebConcept compilePhrase(const std::string& phrase,
                                    LawManager& laws,
                                    Singular& channelTarget,
                                    std::string* outError = nullptr,
                                    const std::vector<Relation*>& extraRelations = {});

    // Create an authored newborn Singular (Object) in the world with real authorship relation.
    // If an author (such as a living Person) is provided, connects Relation("authored-by", *obj, *author).
    static std::shared_ptr<Singular> createAuthoredElement(
        const std::string& identifier,
        const WebConcept& concept,
        Singular* author = nullptr,
        std::vector<std::shared_ptr<Relation>>* outRelations = nullptr);
};

} // namespace Web
} // namespace Foreign
} // namespace Singularity
