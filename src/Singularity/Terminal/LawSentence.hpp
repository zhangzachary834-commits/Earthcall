#pragma once

#include "ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ConditionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// The Law Line: the Mac Terminal's command line, read as law text.
//
// Zach, "Natural Language Law Authoring.md" (2026-09-25): author a Law as one
// sentence, slash-command style, with Tab auto-fill and First-Mover-
// PRECONFIGURED presets that fix some clauses and leave only the remaining
// degrees of freedom open. This grammar belongs to the Terminal modality
// channel only (Zach, 2026-09-25: "We only want a LawSentence to parse the
// modality channel of the Mac Terminal's command line. We don't want it
// in-world.") — it is how that one channel reads its own line, not an
// in-world language system.
//
// WHERE MEANING LIVES. "Lexeme-as-opcode really should mean Lexeme
// <--Relation--> Law (the law is where the executable opcodes live)" — Zach.
// A word's meaning is the Law its Lexeme DENOTES. The Law's own models are
// the opcode; this file only reads their shape (classify()):
//
//   an action with an OPEN slot   (Set with no path, Publish with no event…)
//       -> an action word: the sentence fills the slot    "set", "grant"
//   a condition with an OPEN slot (Compare with no path, Related with no
//   type or other, IsKind(AnyBeing), Not with no child…)
//       -> an operator / condition word                   "greater than", "is"
//   a Law with no open slot
//       -> a PRESET: it fixes its activation, scope, triggers, and any
//          condition (an empty All() fixes "no condition") or action
//                                   "my event-triggered law", "always"
//
// What this file does hold are invariants: the opcode names the engine
// already spells (ActionNode::kindName, ConditionNode kinds and Ops,
// BeingKind, Law::Activation / Scope), and the structural words a sentence
// needs to have clauses at all (called / on / fires / if / when / then / and
// / or / to / by / about / within / true / false). They are the bootstrap:
// the line works before anyone authors an alias, because nobody can type the
// laws that would create the line's own words.
//
// SHARED NAMES ARE LEGITIMATE. Several Lexemes may share a symbol, each
// individuated by its Lexeme ID, and one Lexeme may denote several Laws.
// Grammar position filters meanings first; when two meanings are still
// admissible, WHICH one the sentence uses is decided by a Metalaw (Zach,
// 2026-09-25), reached through Vocabulary::resolve. With no resolving Metalaw
// the sentence is refused, naming the candidates. Nothing is guessed.
//
// This file is pure: no Universe, no LawManager, no EventBus. TerminalChannel
// assembles the live Vocabulary and performs the act.
// ---------------------------------------------------------------------------

namespace Singularity {
namespace Terminal {
namespace LawSentence {

// One spelling the line understands.
struct Word {
    std::string symbol;    // one or more space-separated words, case-insensitive
    std::string opcode;    // "op.Gt", "clause.trigger", "action.Set", "preset", ...
    std::string lexemeId;  // the authored Lexeme; "" for a structural / canonical spelling
    std::string lawId;     // the Law this Lexeme denotes; "" for a canonical spelling
    std::string description;   // what the menu says it means (a denoted Law's name)

    // How an ambiguity names this candidate. A Lexeme that denotes several
    // Laws is several candidates, so the denoted Law is part of the name.
    std::string individual() const {
        if (lexemeId.empty()) return "canonical." + opcode;
        return lawId.empty() ? lexemeId : lexemeId + "->" + lawId;
    }
};

// What a preset Law fixes.
struct Preset {
    std::string lawId;
    Law::Activation activation = Law::Activation::OnEvent;
    Law::Scope scope = Law::Scope::Subject;
    std::vector<std::string> triggers;
    std::optional<ConditionNode> condition;   // empty All() = fixes "no condition"
    std::optional<ActionNode> action;
    // A VALUE word ("red"): a Set with no path but a value — the Law holds the
    // value the word stands for.
    std::optional<PropertyValue> value;
};

// Read a denoted Law's models: which opcode is it? Returns "" when the Law
// carries nothing a sentence can use (e.g. an open slot in a kind that has no
// sentence form). `preset` is filled when the answer is "preset" or "value".
std::string classify(const Law& law, const std::vector<std::string>& triggers, Preset& preset);

// Two or more admissible meanings for the same spelling at the same place.
struct Ambiguity {
    std::string symbol;               // as written in the sentence
    std::string slot;                 // "clause", "operator", "condition", "action", ...
    std::vector<Word> candidates;     // distinct meanings, individuated
};

struct Resolution {
    std::string chosen;   // a candidate's individual(); "" = unresolved
    std::string reason;   // who resolved it, or why nothing did
};

struct Vocabulary {
    std::vector<Word> words;                 // structural + canonical + denoting Lexemes
    std::vector<Preset> presets;             // looked up by Word::lawId
    std::vector<std::string> events;         // event types the world knows
    std::vector<std::string> beings;         // identifiers, for @-completion
    std::string scopeBeing;                  // bare paths complete against this being
    std::function<std::vector<std::string>(const std::string& beingId)> propertiesOf;
    std::function<Resolution(const Ambiguity&)> resolve;   // the Metalaw seam

    // What the menu says beside a candidate. All optional; absent = blank.
    std::function<std::string(const std::string& beingId, const std::string& property)> describeProperty;
    std::function<std::string(const std::string& beingId)> describeBeing;
    std::function<std::string(const std::string& eventType)> describeEvent;
};

// A stretch of the sentence and what it was read as — for colouring the line
// as it is typed. Roles: clause, logic, filler, activation, scope, preset,
// action, operator, condition, kind, value, path, being, event, name, error.
struct Span {
    std::size_t start = 0;
    std::size_t end = 0;
    std::string role;
};

// The structural words and the engine's own opcode spellings.
std::vector<Word> canonicalWords();

struct Parse {
    bool ok = false;
    bool previewOnly = false;   // sentence ended with '?'
    bool search = false;        // sentence began with '??'

    std::string name;
    std::vector<std::string> presetLawIds;
    Law::Activation activation = Law::Activation::OnEvent;
    Law::Scope scope = Law::Scope::Subject;
    std::vector<std::string> triggers;
    std::optional<ConditionNode> condition;   // composed: presets ∧ spoken
    std::optional<ActionNode> action;         // composed: presets ; spoken
    std::vector<std::string> openClauses;     // what the sentence still needs
    std::vector<std::string> notes;           // e.g. which Metalaw resolved an ambiguity

    std::string error;                        // "" when ok
    std::size_t errorOffset = 0;              // byte offset into the sentence
    std::vector<std::string> candidates;      // ambiguity candidates / search hits
    std::vector<Span> spans;                  // offsets into the raw text given to parse()

    std::string preview() const;              // WHEN … -> IF … -> THEN …
};

Parse parse(const std::string& text, const Vocabulary& vocab);

// Tab. `beforeCursor` is the whole line up to the cursor; each result is the
// full replacement for the word being typed (readline's contract).
std::vector<std::string> complete(const std::string& beforeCursor, const Vocabulary& vocab);

// The live menu. Each suggestion REPLACES beforeCursor[from, end) with
// `text`; matching is fuzzy (prefix, then word-start, then subsequence), best
// first. `description` says what the candidate means; `role` colours it.
struct Suggestion {
    std::size_t from = 0;
    std::string text;
    std::string description;
    std::string role;
    int score = 0;
};
std::vector<Suggestion> suggest(const std::string& beforeCursor, const Vocabulary& vocab);

// "?? color" — every spelling, event, being, and scoped property whose text
// contains the query, each labelled with what it is.
std::vector<std::string> search(const std::string& query, const Vocabulary& vocab);

} // namespace LawSentence
} // namespace Terminal
} // namespace Singularity
