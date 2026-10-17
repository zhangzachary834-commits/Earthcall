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
    // Authors a single vocabulary word into the world with explicit Person stewardship.
    // In accordance with Earthcall ontology, Laws expressing meaning require human stewardship.
    // Ordinary authored Laws receive authority 1 (least privilege), NOT kernel authority 10.
    static void authorWord(Person& author,
                           LawManager& laws,
                           const std::string& symbol,
                           const std::string& stableLexemeId,
                           const std::string& lawId,
                           ActionNode action,
                           Formation* formation = nullptr,
                           std::vector<std::shared_ptr<Relation>>* outRelations = nullptr);

    // Loads vocabulary definitions from an authored Earthcall data fixture (JSON) into the world.
    // Meaning lives in authored data, not in C++ tables.
    // If fixturePath is empty, loads from "saves/fixtures/web_vocabulary_seed.json".
    static bool loadAuthoredVocabulary(const std::string& fixturePath,
                                       Person& author,
                                       LawManager& laws,
                                       Formation* formation = nullptr,
                                       std::vector<std::shared_ptr<Relation>>* outRelations = nullptr);

    // Loads vocabulary from a JSON string directly (e.g. for dynamic in-memory test redefinitions).
    static bool loadAuthoredVocabularyFromJsonString(const std::string& jsonString,
                                                     Person& author,
                                                     LawManager& laws,
                                                     Formation* formation = nullptr,
                                                     std::vector<std::shared_ptr<Relation>>* outRelations = nullptr);

    // Authors standard web authoring vocabulary into the world with explicit Person stewardship
    // by loading the authored fixture "saves/fixtures/web_vocabulary_seed.json".
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
