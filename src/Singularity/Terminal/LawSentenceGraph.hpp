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

    std::shared_ptr<Formation> lexicalFormation;
    std::shared_ptr<Formation> denotationFormation;

    explicit operator bool() const { return ok; }
};

// Project one utterance. utteranceId is semantic identity supplied by the
// caller; spelling is never used as occurrence identity.
//
// Ambiguity is preserved whether or not a Metalaw resolves it:
// - the occurrence carries language.candidates as a PropertyList;
// - candidateDenotation edges point to every live authored Lexeme candidate;
// - when one meaning was chosen, denotes points to that exact Lexeme too.
Result project(const std::string& text,
               const LawSentence::Vocabulary& vocabulary,
               const std::string& utteranceId,
               const Kinds& kinds);

} // namespace LawSentenceGraph
} // namespace Terminal
} // namespace Singularity
