#pragma once

#include "ConstructedBeing/Singular/Lexeme/Lexeme.hpp"
#include "Relation/Formation/Formation.hpp"
#include "Relation/Relation.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace Singularity {
namespace Language {

// A substrate mechanism for translating one already-meaningful Earthcall graph
// into another graph without pretending that the target manifestation IS the
// source being.
//
// This class deliberately does not decide what a source means, which target
// symbols should exist, or which Relations should connect them. Those choices
// belong above this mechanism in authored Law / Metalaw. GraphTransduction
// only checks a complete requested projection, materializes it detached, and
// records exact source -> target correspondence when the caller supplies an
// authored Relation-kind Lexeme.
//
// The first witness is semantic Formation -> HTML Lexeme Formation, but there
// is nothing HTML-specific here.
class GraphTransduction {
public:
    struct OccurrenceSpec {
        // The source beings whose meaning participates in this target
        // occurrence. Empty is allowed for target-only structural punctuation.
        // Many-to-one is intentional: composition must not throw provenance
        // away merely because several source meanings become one target node.
        std::vector<const Singular*> sources;

        // Exact target identity. Spelling is never identity; two occurrences
        // may intentionally share symbol while retaining distinct targetId.
        std::string targetId;
        std::string symbol;
    };

    struct RelationSpec {
        // Optional source beings (normally one source Relation) whose meaning
        // participates in this target Relation.
        std::vector<const Singular*> sources;

        std::size_t from = 0;
        std::size_t to = 0;

        // Prefer a grounded authored relation-kind Lexeme. legacyType exists
        // only for callers still crossing a string-typed Relation boundary.
        Lexeme* kind = nullptr;
        std::string legacyType;

        bool directed = true;
        float weight = 1.0f;
    };

    struct Plan {
        // Optional source boundary. When present, every non-null source named
        // by an occurrence/relation must belong to it. The target never reuses
        // this Formation.
        const Formation* sourceFormation = nullptr;

        std::string targetFormationId;
        std::vector<OccurrenceSpec> occurrences;
        std::vector<RelationSpec> relations;

        bool hasRoot = false;
        std::size_t rootOccurrence = 0;

        // Exact provenance/correspondence vocabulary. Prefer the authored,
        // grounded Lexeme. The fallback string keeps legacy Relation callers
        // possible without making that spelling an engine-level semantic.
        Lexeme* correspondenceKind = nullptr;
        std::string correspondenceLegacyType;
    };

    // An entirely graph-native authoring surface. The request Formation holds:
    //
    //   source --mapping kind--> target prototype Lexeme
    //
    // Relations among those target prototype Lexemes are the desired target
    // topology. buildFromTemplate clones the prototypes into fresh target
    // occurrences and preserves the mapping as source -> occurrence
    // correspondence. No TransductionRequest C++ being is introduced.
    struct TemplateRequest {
        const Formation* requestFormation = nullptr;

        // This same authored relation kind both identifies source->prototype
        // mapping edges in the request and records source->final-target
        // provenance in the result.
        Lexeme* mappingKind = nullptr;
        std::string mappingLegacyType;

        std::string targetFormationId;
        std::string targetIdPrefix;

        // Optional target prototype that should become the output root.
        const Lexeme* rootPrototype = nullptr;
    };

    struct Result {
        bool ok = false;
        std::string refusal;

        // Declaration order is intentional: targetFormation is destroyed
        // before Relations and Lexemes, so its non-owning member pointers do
        // not outlive the beings they name.
        std::vector<std::shared_ptr<Lexeme>> targetLexemes;
        std::vector<std::shared_ptr<Relation>> targetRelations;
        std::vector<std::shared_ptr<Relation>> correspondenceRelations;
        std::shared_ptr<Formation> targetFormation;

        explicit operator bool() const { return ok; }

        std::shared_ptr<Lexeme> findTarget(const std::string& id) const;
    };

    // Build detached. Nothing is registered into LanguageSystem, Universe, a
    // Zone, or a browser channel here. A refused plan therefore publishes
    // absolutely nothing, and a successful result may be inspected before a
    // caller admits/acts on it.
    static Result build(const Plan& plan);

    // Compile an authored graph template into the same detached Plan/result.
    // This is the first rung where the mapping itself is ordinary Earthcall
    // graph structure rather than a C++ list of target nodes.
    static Result buildFromTemplate(const TemplateRequest& request);

private:
    static std::shared_ptr<Relation> makeRelation(
        Lexeme* kind,
        const std::string& legacyType,
        const Singular& a,
        const Singular& b,
        bool directed,
        float weight,
        std::string& refusal);

    static bool sameKind(
        const Relation& relation,
        const Lexeme* kind,
        const std::string& legacyType);
};

} // namespace Language
} // namespace Singularity
