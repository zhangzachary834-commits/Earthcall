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
        // The source being whose meaning is being manifested by this target
        // occurrence. Null is allowed for target-only structural punctuation.
        const Singular* source = nullptr;

        // Exact target identity. Spelling is never identity; two occurrences
        // may intentionally share symbol while retaining distinct targetId.
        std::string targetId;
        std::string symbol;
    };

    struct RelationSpec {
        // Optional source Relation (or other source being) whose meaning is
        // being manifested by this target Relation.
        const Singular* source = nullptr;

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
        // Optional source boundary. When present, every non-null occurrence
        // source must belong to it. The target never reuses this Formation.
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

private:
    static std::shared_ptr<Relation> makeRelation(
        Lexeme* kind,
        const std::string& legacyType,
        const Singular& a,
        const Singular& b,
        bool directed,
        float weight,
        std::string& refusal);
};

} // namespace Language
} // namespace Singularity
