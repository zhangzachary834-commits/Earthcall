#pragma once

#include "Singularity/Terminal/LawSentence.hpp"
#include "Singularity/Language/LanguageSystem.hpp"
#include "Relation/Formation/Formation.hpp"
#include "Relation/Relation.hpp"

#include <memory>
#include <string>
#include <vector>

namespace Singularity {
namespace Terminal {
namespace LawSentenceGraph {

// First explicit graph projection of the Terminal Law Line. It does NOT parse
// natural language a second time. LawSentence remains the modality reader;
// this layer exposes what that reader already knows as ordinary Earthcall
// Lexemes, Relations, and Formations.
//
// The output is intentionally layered:
//   lexical     occurrence Lexemes + exact sequence
//   denotation  those occurrences + the live authored Lexemes they mean
//
// Semantic AST / intended-being Formation is the next rung; this file refuses
// to pretend syntax roles are already full world semantics.
struct Kinds {
    Singularity::Language::Lexeme* next = nullptr;
    Singularity::Language::Lexeme* denotes = nullptr;
    Singularity::Language::Lexeme* candidateDenotation = nullptr;

    // Higher semantic layer. semanticChild is the exact AST / intent-tree
    // topology. expresses is provenance from a lexical occurrence to the
    // semantic node it truthfully helped select; those provenance edges stay
    // outside the semantic Formation so the semantic graph does not absorb
    // its source layer as members.
    Singularity::Language::Lexeme* semanticChild = nullptr;
    Singularity::Language::Lexeme* expresses = nullptr;
};

struct Result {
    bool ok = false;
    std::string refusal;
    LawSentence::Parse parse;

    std::vector<std::shared_ptr<Singularity::Language::Lexeme>> occurrences;
    // Pin every live meaning Lexeme named by a denotation edge so Formations'
    // raw pointers remain valid for the lifetime of this Result.
    std::vector<std::shared_ptr<Singularity::Language::Lexeme>> meaningLexemes;
    std::vector<std::shared_ptr<Relation>> lexicalRelations;
    std::vector<std::shared_ptr<Relation>> denotationRelations;

    // Rung 3b: exact Law semantic tree manifested as Lexeme beings. The
    // semantic tree is only present when the parse itself is semantically
    // admissible (Parse::ok). A refused/open parse still has perfectly valid
    // lexical + denotation layers above.
    std::vector<std::shared_ptr<Singularity::Language::Lexeme>> semanticLexemes;
    std::vector<std::shared_ptr<Relation>> semanticRelations;
    std::vector<std::shared_ptr<Relation>> semanticProvenanceRelations;

    std::shared_ptr<Formation> lexicalFormation;
    std::shared_ptr<Formation> denotationFormation;
    std::shared_ptr<Formation> semanticFormation;

    explicit operator bool() const { return ok; }
};

// Project one utterance. utteranceId is semantic identity supplied by the
// caller; spelling is never used as occurrence identity.
//
// Ambiguity is preserved whether or not a Metalaw resolves it:
// - the occurrence carries language.candidates as a PropertyList;
// - candidateDenotation edges point to every live authored Lexeme candidate;
// - when one meaning was chosen, denotes points to that exact Lexeme too.
//
// For a successful parse, the same result also contains a rooted semantic
// Formation:
//
//   Law intent
//      +-- Condition tree
//      +-- Action tree
//      +-- trigger nodes
//
// Every ConditionNode / ActionNode is manifested as its own Lexeme with its
// exact serialized model attached as semantic.model. semanticChild Relations
// reproduce the model-tree topology. expresses provenance is conservative:
// only parser occurrences whose own opcode proves they selected an action,
// condition/operator, preset, activation/scope, trigger/name clause are linked.
// We never guess that an arbitrary path/value atom belongs to a deep AST leaf.
Result project(const std::string& text,
               const LawSentence::Vocabulary& vocabulary,
               const std::string& utteranceId,
               const Kinds& kinds);

// Same projection, but from the exact Parse a modality already performed.
// This matters for channels whose Vocabulary::resolve invokes Metalaws: graph
// projection must not parse a second time and ask those Metalaws twice.
Result projectParsed(const std::string& text,
                     const LawSentence::Parse& parsed,
                     const std::string& utteranceId,
                     const Kinds& kinds);

} // namespace LawSentenceGraph
} // namespace Terminal
} // namespace Singularity
