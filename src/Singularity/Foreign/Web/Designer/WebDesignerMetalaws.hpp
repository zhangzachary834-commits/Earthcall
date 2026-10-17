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
    // Authors standard web authoring vocabulary into the world with explicit Person stewardship.
    // In accordance with Earthcall ontology, Laws expressing meaning require human stewardship.
    // Each Law is authored by `author`, and each `denotes` Relation is admitted to the Formation / Universe.
    static void authorVocabulary(Person& author,
                                 LawManager& laws,
                                 Formation* formation = nullptr,
                                 std::vector<std::shared_ptr<Relation>>* outRelations = nullptr);

    // Compatibility registration for modality infrastructure (does NOT manufacture fake author)
    static void syncRegister(LawManager& laws);

    // In-World Lexeme Relation Scanner:
    // Compiles tokens into a target Singular using in-world Lexemes and their "denotes" Relations.
    // Resolution occurs exclusively via live Relation("denotes", *lexeme, *law) edges.
    static WebConcept compileTokens(const std::vector<std::string>& tokens,
                                    LawManager& laws,
                                    Singular& channelTarget,
                                    std::string* outError = nullptr,
                                    const std::vector<Relation*>& extraRelations = {});

    // Parse and compile an authored natural language phrase into a target Singular.
    // Scans character sequences against in-world Lexemes and resolves via denotes Relations.
    // All Laws execute strictly via Law::applyTo(); unauthored Laws are refused (no backdoor).
    static WebConcept compilePhrase(const std::string& phrase,
                                    LawManager& laws,
                                    Singular& channelTarget,
                                    std::string* outError = nullptr,
                                    const std::vector<Relation*>& extraRelations = {});

    // Create an authored newborn Singular in the world with real Person authorship relation.
    // Person authorship is Person-exclusive — no arbitrary Singular or Object may satisfy it.
    static std::shared_ptr<Singular> createAuthoredElement(
        const std::string& identifier,
        const WebConcept& concept,
        Person* author = nullptr,
        std::vector<std::shared_ptr<Relation>>* outRelations = nullptr);

    // Authors concept onto any existing Singular bearer (Object, Formation, Lexeme, etc.).
    // The Web adapter reads meaning from a being; it does not force every bearer to be an Object.
    static Singular& applyAuthoredConcept(
        Singular& bearer,
        const WebConcept& concept,
        Person* author = nullptr,
        std::vector<std::shared_ptr<Relation>>* outRelations = nullptr);
};

} // namespace Web
} // namespace Foreign
} // namespace Singularity
