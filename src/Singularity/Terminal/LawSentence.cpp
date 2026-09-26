#include "Singularity/Terminal/LawSentence.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <map>
#include <set>

namespace Singularity {
namespace Terminal {
namespace LawSentence {

namespace {

// ---------------------------------------------------------------------------
// The lexical layer reads CHARACTERS, not whitespace-split words (Zach,
// 2026-09-25): a Lexeme is any semantic unit — a word, a phrase, a whole
// sentence, a prefix or suffix, a run of symbols, a keyboard smash with an
// assigned meaning. So a spelling is matched against the text itself:
//
//   free spelling     "greater than", "asdfgh", ">="   matched where it
//                     stands; a spelling that begins/ends in a letter or
//                     digit must not be the inside of a longer word, and a
//                     space inside it matches any run of spaces.
//   bound prefix      "re-"   (the dictionary's hyphen convention) matches
//                     "re" glued to what follows: "reset" = re- + set.
//   bound suffix      "-ed"   matches "ed" glued to what precedes; an atom
//                     read from the text gives a trailing suffix back.
//
// What is not a spelling — a property path, a value, an event, a name — is
// read as an ATOM: a quoted run, or a maximal run of path characters
// (letters, digits, . _ - @), or a maximal run of other visible characters.
// ---------------------------------------------------------------------------

bool isSpace(char c) { return std::isspace(static_cast<unsigned char>(c)) != 0; }
bool isAtomChar(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '.' || c == '_' || c == '-' ||
           c == '@';
}
bool isWordChar(char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; }
char fold(char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), fold);
    return s;
}

bool startsWith(const std::string& s, const std::string& prefix) {
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

bool looksLikePath(const std::string& s) {
    if (s.empty()) return false;
    return std::all_of(s.begin(), s.end(), isAtomChar);
}

std::string trim(const std::string& s) {
    std::size_t a = 0, b = s.size();
    while (a < b && isSpace(s[a])) ++a;
    while (b > a && isSpace(s[b - 1])) --b;
    return s.substr(a, b - a);
}

enum class Form { Free, Prefix, Suffix };

struct Spelling {
    Word word;
    std::string stem;   // lower-cased; hyphen of a bound form removed
    Form form = Form::Free;
};

Spelling spell(const Word& w) {
    Spelling s{w, lower(trim(w.symbol)), Form::Free};
    const std::string& t = s.stem;
    if (t.size() > 1 && t.back() == '-' && isWordChar(t[t.size() - 2])) {
        s.form = Form::Prefix;
        s.stem.pop_back();
    } else if (t.size() > 1 && t.front() == '-' && isWordChar(t[1])) {
        s.form = Form::Suffix;
        s.stem.erase(0, 1);
    }
    return s;
}

// Match `stem` at `at`; returns the characters consumed, 0 for no match.
std::size_t matchStem(const std::string& text, std::size_t at, const std::string& stem) {
    std::size_t i = 0, j = at;
    while (i < stem.size()) {
        if (isSpace(stem[i])) {
            if (j >= text.size() || !isSpace(text[j])) return 0;
            while (i < stem.size() && isSpace(stem[i])) ++i;
            while (j < text.size() && isSpace(text[j])) ++j;
            continue;
        }
        if (j >= text.size() || fold(text[j]) != stem[i]) return 0;
        ++i;
        ++j;
    }
    return j - at;
}

std::size_t matchSpelling(const std::string& text, std::size_t at, const Spelling& s) {
    if (s.stem.empty()) return 0;
    const bool glued = at > 0 && isWordChar(text[at - 1]);
    const std::size_t n = matchStem(text, at, s.stem);
    if (n == 0) return 0;
    const std::size_t end = at + n;
    const bool gluedAfter = end < text.size() && isWordChar(text[end]);
    switch (s.form) {
        case Form::Free:
            if (glued && isWordChar(s.stem.front())) return 0;
            if (gluedAfter && isWordChar(s.stem.back())) return 0;
            return n;
        case Form::Prefix:
            if (glued) return 0;
            if (end < text.size() && text[end] == '-') return n + 1;
            return gluedAfter ? n : 0;
        case Form::Suffix:
            if (!glued) return 0;
            return gluedAfter ? 0 : n;
    }
    return 0;
}

bool parseNumber(const std::string& text, double& out) {
    if (text.empty()) return false;
    const char c = text[0];
    if (!(std::isdigit(static_cast<unsigned char>(c)) || c == '-' || c == '+' || c == '.')) {
        return false;
    }
    char* end = nullptr;
    out = std::strtod(text.c_str(), &end);
    return end && *end == '\0';
}

// ---------------------------------------------------------------------------
// Admission: which meanings a grammar position will hear. Opcode prefixes, so
// a Lexeme denoting any opcode is admitted wherever that opcode is.
// ---------------------------------------------------------------------------
using Admit = std::vector<std::string>;

bool admits(const Admit& admit, const std::string& opcode) {
    for (const auto& prefix : admit) {
        if (startsWith(opcode, prefix)) return true;
    }
    return false;
}

const Admit kClauseStart{"clause.", "activation.", "scope.", "action.", "preset"};
const Admit kConditionStart{"condition."};

std::string roleOf(const std::string& opcode) {
    if (startsWith(opcode, "op.")) return "operator";
    const std::size_t dot = opcode.find('.');
    return dot == std::string::npos ? opcode : opcode.substr(0, dot);
}

struct Refusal {
    std::string message;
    std::size_t offset = 0;
    std::vector<std::string> candidates;
};

// Thrown when a completing parse reaches the cursor: what it expected there
// is what Tab offers.
struct Stop {};

struct Expectation {
    Admit admit;
    bool event = false;
    bool path = false;
    bool being = false;
    bool value = false;
};

struct Atom {
    std::string text;
    std::size_t offset = 0;
    bool quoted = false;
};

class Parser {
public:
    Parser(std::string text, const Vocabulary& vocab, bool completing)
        : _text(std::move(text)), _vocab(vocab), _completing(completing) {
        for (const auto& w : _vocab.words) {
            _spellings.push_back(spell(w));
            if (_spellings.back().form == Form::Suffix) _suffixes.push_back(_spellings.back().stem);
        }
    }

    Parse& result() { return _out; }
    const std::vector<Span>& spans() const { return _spans; }
    const Expectation& expectation() const { return _expect; }

    void run() {
        while (true) {
            if (atEnd()) {
                if (_completing) {
                    expect(kClauseStart);
                    throw Stop{};
                }
                break;
            }
            const std::size_t at = _pos;
            auto w = tryMatch(kClauseStart, "clause");
            if (!w) {
                refuse("'" + peekAtomText() +
                           "' does not begin a clause here (expected called / on / if / then, an "
                           "activation, a preset, a scope, or an action)",
                       at);
            }
            clause(*w);
        }
    }

    void finish();

private:
    // --- cursor ----------------------------------------------------------
    void skipSpace() {
        while (_pos < _text.size() && isSpace(_text[_pos])) ++_pos;
    }
    bool atEnd() {
        skipSpace();
        return _pos >= _text.size();
    }

    [[noreturn]] void refuse(const std::string& message, std::size_t offset,
                             std::vector<std::string> candidates = {}) {
        throw Refusal{message, std::min(offset, _text.size()), std::move(candidates)};
    }

    void expect(const Admit& admit) {
        _expect.admit.insert(_expect.admit.end(), admit.begin(), admit.end());
    }

    // Longest spelling admitted here; the Metalaw seam when meanings collide.
    std::optional<Word> tryMatch(const Admit& admit, const std::string& slot) {
        // A bound suffix attaches to what precedes it, so do not skip space
        // before trying one; everything else may follow whitespace.
        const std::size_t glued = _pos;
        const bool canGlue = glued > 0 && glued < _text.size() && isWordChar(_text[glued - 1]);
        if (atEnd()) {
            if (_completing) expect(admit);
            return std::nullopt;
        }
        std::size_t bestLen = 0;
        std::size_t bestAt = _pos;
        std::vector<const Word*> best;
        for (const auto& s : _spellings) {
            if (!admits(admit, s.word.opcode)) continue;
            const std::size_t at = (s.form == Form::Suffix && canGlue) ? glued : _pos;
            const std::size_t n = matchSpelling(_text, at, s);
            if (n == 0 || n < bestLen) continue;
            if (n > bestLen) {
                bestLen = n;
                bestAt = at;
                best.clear();
            }
            best.push_back(&s.word);
        }
        if (best.empty()) return std::nullopt;

        // One meaning spelled twice (a canonical word and an alias of it) is
        // not an ambiguity; two denoted preset Laws are two meanings.
        std::vector<Word> distinct;
        std::set<std::string> meanings;
        for (const Word* w : best) {
            const std::string meaning = w->opcode == "preset" ? "preset:" + w->lawId : w->opcode;
            if (meanings.insert(meaning).second) distinct.push_back(*w);
        }
        const std::string written = _text.substr(bestAt, bestLen);
        _pos = bestAt + bestLen;
        Word chosen = distinct.size() == 1 ? distinct.front() : resolve(written, slot, distinct, bestAt);
        mark(bestAt, bestAt + bestLen, roleOf(chosen.opcode));
        return chosen;
    }

    Word resolve(const std::string& written, const std::string& slot,
                 const std::vector<Word>& candidates, std::size_t at) {
        // Tab only needs to know WHERE the sentence is, not which meaning
        // wins; Metalaws are consulted when the sentence is spoken.
        if (_completing) return candidates.front();
        std::vector<std::string> names;
        for (const auto& c : candidates) names.push_back(c.individual() + " (" + c.opcode + ")");
        Resolution r;
        if (_vocab.resolve) r = _vocab.resolve(Ambiguity{written, slot, candidates});
        for (const auto& c : candidates) {
            if (!r.chosen.empty() && c.individual() == r.chosen) {
                _out.notes.push_back("'" + written + "' read as " + c.opcode + " (" +
                                     c.individual() + ")" +
                                     (r.reason.empty() ? "" : " — " + r.reason));
                return c;
            }
        }
        refuse("'" + written + "' has " + std::to_string(candidates.size()) +
                   " meanings at this " + slot + " and no Metalaw resolves which one" +
                   (r.reason.empty() ? "" : " (" + r.reason + ")") +
                   ". Author a Law targeting @terminal-channel that writes ambiguity.resolved",
               at, names);
    }

    bool peekClauseWord() {
        const std::size_t saved = _pos;
        const std::size_t savedSpans = _spans.size();
        const bool savedCompleting = _completing;
        _completing = false;
        bool hit = false;
        try {
            hit = tryMatch(kClauseStart, "clause").has_value() ||
                  tryMatch({"logic."}, "logic").has_value();
        } catch (const Refusal&) {
            hit = true;   // an ambiguous clause word is still a clause word
        }
        _pos = saved;
        _spans.resize(savedSpans);
        _completing = savedCompleting;
        return hit;
    }

    // Read one atom without consuming it (for messages).
    std::string peekAtomText() {
        const std::size_t saved = _pos;
        std::string t;
        if (!atEnd()) t = readAtomRaw().text;
        _pos = saved;
        return t;
    }

    Atom readAtomRaw() {
        skipSpace();
        Atom a;
        a.offset = _pos;
        if (_text[_pos] == '"') {
            const std::size_t close = _text.find('"', _pos + 1);
            const std::size_t end = close == std::string::npos ? _text.size() : close;
            a.text = _text.substr(_pos + 1, end - _pos - 1);
            a.quoted = true;
            _pos = close == std::string::npos ? _text.size() : close + 1;
            return a;
        }
        std::size_t j = _pos;
        if (isAtomChar(_text[j])) {
            while (j < _text.size() && isAtomChar(_text[j])) ++j;
        } else {
            while (j < _text.size() && !isSpace(_text[j]) && !isAtomChar(_text[j]) &&
                   _text[j] != '"') {
                ++j;
            }
        }
        a.text = _text.substr(_pos, j - _pos);
        // An authored bound suffix at the atom's end is its own Lexeme.
        std::size_t strip = 0;
        const std::string low = lower(a.text);
        for (const auto& suffix : _suffixes) {
            if (low.size() > suffix.size() && suffix.size() > strip &&
                low.compare(low.size() - suffix.size(), suffix.size(), suffix) == 0 &&
                isWordChar(low[low.size() - suffix.size() - 1])) {
                strip = suffix.size();
            }
        }
        a.text.resize(a.text.size() - strip);
        _pos = j - strip;
        return a;
    }

    Atom requireAtom(const std::string& what, bool Expectation::*flag) {
        if (atEnd()) {
            if (_completing) {
                _expect.*flag = true;
                throw Stop{};
            }
            refuse("the sentence ends where " + what + " was expected", _pos);
        }
        return readAtomRaw();
    }

    std::string requirePath() {
        const Atom a = requireAtom("a property path", &Expectation::path);
        mark(a.offset, _pos, "path");
        if (a.quoted || !looksLikePath(a.text)) refuse("'" + a.text + "' is not a property path", a.offset);
        return a.text;
    }

    std::string requireBeing(const std::string& what) {
        const Atom a = requireAtom(what, &Expectation::being);
        mark(a.offset, _pos, "being");
        if (a.quoted || !looksLikePath(a.text)) refuse("'" + a.text + "' does not name a being", a.offset);
        if (startsWith(a.text, "@event.")) return a.text;
        return a.text[0] == '@' ? a.text.substr(1) : a.text;
    }

    // An event a Law will LISTEN for must be one this world knows — heard,
    // bound, or published by some Law — or a Law is born deaf to a name that
    // never fires (Zach, 2026-09-25: "fires when" made a Law waiting for an
    // event called "when"). A new name is minted on purpose by quoting it.
    // `mayMint`: publishing may always name a new event; that is minting.
    std::string requireEvent(const std::string& what, bool mayMint) {
        if (!atEnd() && _text[_pos] != '"' && peekClauseWord()) {
            refuse("'" + peekAtomText() + "' is a clause word, not an event; name the event after it" +
                       (_vocab.events.empty() ? std::string{} : " (e.g. " + _vocab.events.front() + ")"),
                   _pos);
        }
        const Atom a = requireAtom(what, &Expectation::event);
        mark(a.offset, _pos, "event");
        if (a.text.empty() || !looksLikePath(a.text)) refuse("'" + a.text + "' does not name an event", a.offset);
        if (a.quoted || mayMint || _vocab.events.empty()) return a.text;
        if (std::find(_vocab.events.begin(), _vocab.events.end(), a.text) != _vocab.events.end()) return a.text;
        std::vector<std::string> near;
        const std::string want = lower(a.text);
        for (const auto& e : _vocab.events) {
            const std::string have = lower(e);
            if (have.find(want) != std::string::npos || want.find(have) != std::string::npos ||
                (want.size() >= 3 && have.compare(0, 3, want, 0, 3) == 0)) {
                near.push_back(e);
            }
        }
        if (near.size() > 6) near.resize(6);
        refuse("'" + a.text + "' is not an event this world knows, so the Law would never fire. "
               "Pick one from the menu, or quote a new name on purpose: on \"" + a.text + "\"",
               a.offset, near);
    }

    PropertyValue parseValue() {
        if (atEnd()) {
            if (_completing) {
                _expect.value = true;
                expect({"value"});
                throw Stop{};
            }
            refuse("the sentence ends where a value was expected", _pos);
        }
        if (auto w = tryMatch({"value"}, "value")) {
            if (w->opcode == "value.true" || w->opcode == "value.false") {
                return PropertyValue(w->opcode == "value.true");
            }
            for (const auto& p : _vocab.presets) {
                if (p.lawId == w->lawId && p.value) return *p.value;
            }
            refuse("'" + w->symbol + "' denotes Law " + w->lawId + ", which holds no value", _pos);
        }
        const std::size_t start = _pos;
        if (peekClauseWord()) refuse("'" + peekAtomText() + "' is a clause word, not a value", start);
        Atom first = readAtomRaw();
        mark(first.offset, _pos, "value");
        if (first.quoted) return PropertyValue(first.text);
        double numbers[3];
        if (parseNumber(first.text, numbers[0])) {
            std::size_t count = 1;
            while (count < 3) {
                const std::size_t saved = _pos;
                if (atEnd()) break;
                const Atom next = readAtomRaw();
                if (next.quoted || !parseNumber(next.text, numbers[count])) {
                    _pos = saved;
                    break;
                }
                mark(next.offset, _pos, "value");
                ++count;
            }
            if (count == 1) return PropertyValue(numbers[0]);
            if (count == 3) {
                return PropertyValue(glm::vec3(static_cast<float>(numbers[0]),
                                               static_cast<float>(numbers[1]),
                                               static_cast<float>(numbers[2])));
            }
            refuse("two numbers make no value here: a vector takes three", first.offset);
        }
        if (first.text[0] == '@') {
            refuse("reading a value from another path ('" + first.text +
                       "') waits for the PropertyPath binding algebra "
                       "(PROPERTY_STORAGE_AND_ONTOMATH_BINDING.md §2, \"copy value\")",
                   first.offset);
        }
        return PropertyValue(first.text);
    }

    double parseNumberValue(const std::string& what) {
        const std::size_t at = _pos;
        PropertyValue v = parseValue();
        if (const auto* d = std::get_if<double>(&v)) return *d;
        refuse(what + " needs a number", at);
    }

    // --- clauses ----------------------------------------------------------
    void clause(const Word& w) {
        const std::string& op = w.opcode;
        if (op == "clause.name") return nameClause();
        if (op == "clause.trigger") return triggerClause();
        if (op == "clause.condition") return conditionClause();
        if (op == "clause.action") return actionList(std::nullopt);
        if (op == "clause.timeline") timelineRefusal();
        if (startsWith(op, "activation.")) return setActivation(op);
        if (op == "scope.Everyone") { _scope = Law::Scope::Everyone; return; }
        if (op == "scope.Subject") { _scope = Law::Scope::Subject; return; }
        if (op == "preset") return takePreset(w);
        if (startsWith(op, "action.")) return actionList(w);   // an implicit "then"
        refuse("'" + w.symbol + "' (" + op + ") has no place at the start of a clause", _pos);
    }

    void nameClause() {
        if (!atEnd() && _text[_pos] == '"') {
            const std::size_t at = _pos;
            _out.name = readAtomRaw().text;
            mark(at, _pos, "name");
            return;
        }
        const std::size_t start = _pos;
        std::size_t end = start;
        while (!atEnd() && !peekClauseWord()) {
            readAtomRaw();
            end = _pos;
        }
        const std::string name = trim(_text.substr(start, end - start));
        mark(start, end, "name");
        if (name.empty()) {
            if (atEnd() && _completing) throw Stop{};
            refuse("a name was expected after 'called'", _pos);
        }
        _out.name = name;
    }

    void takePreset(const Word& w) {
        for (const auto& p : _vocab.presets) {
            if (p.lawId == w.lawId) {
                _presets.push_back(&p);
                _out.presetLawIds.push_back(p.lawId);
                return;
            }
        }
        refuse("'" + w.symbol + "' denotes Law " + w.lawId + ", which is not present", _pos);
    }

    void setActivation(const std::string& op) {
        _activation = op == "activation.WhileTrue"      ? Law::Activation::WhileTrue
                      : op == "activation.OnBecomeTrue" ? Law::Activation::OnBecomeTrue
                                                        : Law::Activation::OnEvent;
        _activationSpoken = true;
    }

    [[noreturn]] void timelineRefusal() {
        refuse("a Timeline clause waits for the Law/Timeline ontology "
               "(TIME_AND_MOMENT.md); this Law was not authored",
               _pos);
    }

    void triggerClause() {
        (void)tryMatch({"clause.trigger"}, "clause");   // "fires on": the "on" is optional
        if (auto a = tryMatch({"activation."}, "activation")) return setActivation(a->opcode);
        if (auto p = tryMatch({"preset"}, "preset")) return takePreset(*p);   // "fires when clicked"
        if (tryMatch({"clause.timeline"}, "clause")) timelineRefusal();
        while (true) {
            _triggers.push_back(requireEvent("an event", false));
            if (!_activationSpoken) {
                _activation = Law::Activation::OnEvent;
                _activationSpoken = true;
            }
            if (!tryMatch({"logic.Or"}, "logic")) break;
        }
    }

    void conditionClause() {
        ConditionNode spoken = parseOr();
        _condition = _condition ? ConditionNode::all({*_condition, spoken}) : spoken;
    }

    ConditionNode parseOr() {
        std::vector<ConditionNode> terms{parseAnd()};
        while (tryMatch({"logic.Or"}, "logic")) terms.push_back(parseAnd());
        return terms.size() == 1 ? terms.front() : ConditionNode::any(std::move(terms));
    }

    ConditionNode parseAnd() {
        std::vector<ConditionNode> terms{parseUnary()};
        while (tryMatch({"logic.And"}, "logic")) terms.push_back(parseUnary());
        return terms.size() == 1 ? terms.front() : ConditionNode::all(std::move(terms));
    }

    ConditionNode parseUnary() {
        skipSpace();
        const std::size_t at = _pos;
        if (auto w = tryMatch(kConditionStart, "condition")) {
            const std::string& op = w->opcode;
            if (op == "condition.Not") return ConditionNode::negate(parseUnary());
            if (op == "condition.IsKind") {
                if (auto k = tryMatch({"kind."}, "kind")) return ConditionNode::isKind(beingKind(k->opcode));
                return ConditionNode::identity(requireBeing("a kind of being or @being"));
            }
            if (op == "condition.Identity") return ConditionNode::identity(requireBeing("a being"));
            if (op == "condition.Overlaps") return ConditionNode::overlaps(requireBeing("a being"));
            if (op == "condition.Related") {
                std::string type;
                if (!tryMatch({"filler.to"}, "filler")) {
                    type = requireAtom("a relation type", &Expectation::value).text;
                    mark(_pos - type.size(), _pos, "value");
                    if (!tryMatch({"filler.to"}, "filler")) return ConditionNode::related(type, "");
                }
                return ConditionNode::related(type, requireBeing("a being"));
            }
            refuse("the " + op.substr(10) +
                       " condition has no sentence form yet (no OntoMath text parser); "
                       "author it in the Law Graph",
                   at);
        }
        const std::string path = requirePath();
        const std::size_t opAt = _pos;
        auto op = tryMatch({"op."}, "operator");
        if (!op) {
            if (atEnd() && _completing) throw Stop{};
            refuse("a comparison was expected after '" + path + "'", opAt);
        }
        const std::string& code = op->opcode;
        if (code == "op.Near") {
            ConditionNode n = ConditionNode::compare(path, ConditionNode::Op::Near, parseValue());
            if (!tryMatch({"filler.within"}, "filler")) {
                if (atEnd() && _completing) throw Stop{};
                refuse("'near' takes 'within <tolerance>'", _pos);
            }
            n.tolerance = parseNumberValue("'within'");
            return n;
        }
        if (code == "op.InRange") {
            ConditionNode n;
            n.kind = ConditionNode::Kind::Compare;
            n.path = PropertyPath::parse(path);
            n.op = ConditionNode::Op::InRange;
            n.lo = parseValue();
            if (!tryMatch({"logic.And"}, "logic")) {
                if (atEnd() && _completing) throw Stop{};
                refuse("'between' takes '<low> and <high>'", _pos);
            }
            n.hi = parseValue();
            return n;
        }
        const ConditionNode::Op cmp = code == "op.Ne" ? ConditionNode::Op::Ne
                                    : code == "op.Lt" ? ConditionNode::Op::Lt
                                    : code == "op.Le" ? ConditionNode::Op::Le
                                    : code == "op.Gt" ? ConditionNode::Op::Gt
                                    : code == "op.Ge" ? ConditionNode::Op::Ge
                                                      : ConditionNode::Op::Eq;
        // Comparing against another path is an existing invariant
        // (ConditionNode::operandPath), not a new binding.
        if (!atEnd() && _text[_pos] == '@') {
            const std::size_t saved = _pos;
            const Atom a = readAtomRaw();
            if (looksLikePath(a.text)) {
                mark(a.offset, _pos, "path");
                return ConditionNode::comparePaths(path, cmp, a.text);
            }
            _pos = saved;
        }
        return ConditionNode::compare(path, cmp, parseValue());
    }

    static ConditionNode::BeingKind beingKind(const std::string& opcode) {
        using BK = ConditionNode::BeingKind;
        const std::string k = opcode.substr(5);
        if (k == "Object") return BK::Object;
        if (k == "Person") return BK::Person;
        if (k == "Relation") return BK::Relation;
        if (k == "Formation") return BK::Formation;
        if (k == "Law") return BK::Law;
        if (k == "Zone") return BK::Zone;
        if (k == "Lexeme") return BK::Lexeme;
        return BK::AnyBeing;
    }

    bool takeComma() {
        if (!atEnd() && _text[_pos] == ',') {
            ++_pos;
            return true;
        }
        return false;
    }

    void actionList(std::optional<Word> firstVerb) {
        _actions.push_back(parseAction(std::move(firstVerb)));
        while (takeComma() || tryMatch({"clause.action", "logic.And"}, "action separator")) {
            _actions.push_back(parseAction(std::nullopt));
        }
    }

    static std::pair<std::string, std::string> ownerAndName(const std::string& token) {
        if (token[0] == '@') {
            const auto dot = token.rfind('.');
            if (dot != std::string::npos && dot > 1) return {token.substr(0, dot), token.substr(dot + 1)};
        }
        return {"", token};
    }

    bool valueFollows() {
        return !atEnd() && _text[_pos] != ',' && !peekClauseWord();
    }

    ActionNode parseAction(std::optional<Word> verb) {
        const std::size_t at = _pos;
        if (!verb) {
            verb = tryMatch({"action."}, "action");
            if (!verb) {
                if (atEnd() && _completing) throw Stop{};
                refuse(atEnd() ? "the sentence ends where an action was expected"
                               : "'" + peekAtomText() + "' is not an action",
                       _pos);
            }
        }
        const std::string kind = verb->opcode.substr(7);
        if (kind == "Set") {
            const std::string path = requirePath();
            (void)tryMatch({"filler.to"}, "filler");
            return ActionNode::set(path, parseValue());
        }
        if (kind == "Add" || kind == "Scale") {
            const std::string path = requirePath();
            (void)tryMatch({"filler.by"}, "filler");
            const double n = parseNumberValue("'" + verb->symbol + "'");
            return kind == "Add" ? ActionNode::add(path, n) : ActionNode::scale(path, n);
        }
        if (kind == "Publish") {
            const std::string event = requireEvent("an event to publish", true);
            std::string subject;
            if (tryMatch({"filler.about"}, "filler")) subject = requireBeing("whom the event is about");
            return ActionNode::publish(event, subject);
        }
        if (kind == "Destroy") {
            return ActionNode::destroy(valueFollows() ? requireBeing("a being") : std::string{});
        }
        if (kind == "Spawn") {
            const Atom a = requireAtom("a concept", &Expectation::being);
            mark(a.offset, _pos, "being");
            return ActionNode::spawn(!a.text.empty() && a.text[0] == '@' ? a.text.substr(1) : a.text);
        }
        if (kind == "AddProperty" || kind == "RemoveProperty") {
            const auto [owner, name] = ownerAndName(requirePath());
            if (kind == "RemoveProperty") return ActionNode::removeProperty(owner, name);
            PropertyValue initial;
            if (valueFollows()) {
                (void)tryMatch({"filler.to"}, "filler");
                initial = parseValue();
            }
            return ActionNode::addProperty(owner, name, initial);
        }
        if (kind == "AddRelation") {
            const std::string a = requireBeing("the relation's source");
            const std::string type = requireAtom("a relation type", &Expectation::value).text;
                    mark(_pos - type.size(), _pos, "value");
            const std::string b = requireBeing("the relation's target");
            return ActionNode::addRelation(a, b, type);
        }
        refuse("the " + kind + " action has no sentence form yet; author it in the Law Graph", at);
    }

    std::string _text;
    const Vocabulary& _vocab;
    bool _completing = false;
    std::size_t _pos = 0;
    std::vector<Spelling> _spellings;
    std::vector<std::string> _suffixes;
    std::vector<Span> _spans;

    void mark(std::size_t start, std::size_t end, const std::string& role) {
        if (end > start) _spans.push_back({start, end, role});
    }

    Parse _out;
    Expectation _expect;

    std::vector<const Preset*> _presets;
    Law::Activation _activation = Law::Activation::OnEvent;
    bool _activationSpoken = false;
    std::optional<Law::Scope> _scope;
    std::vector<std::string> _triggers;
    std::optional<ConditionNode> _condition;
    std::vector<ActionNode> _actions;
};

void Parser::finish() {
    // Compose the presets: each FIXES clauses; the sentence fills the rest.
    // Several presets compose when they agree ("my event-triggered law" and
    // a scope preset are two fragments of one Law, not a conflict).
    std::vector<std::string> triggers;
    std::optional<ConditionNode> condition;
    std::vector<ActionNode> actions;
    std::optional<Law::Activation> fixed;
    std::optional<Law::Scope> presetScope;
    bool fixedNone = false;
    for (const Preset* p : _presets) {
        if (fixed && *fixed != p->activation) {
            refuse("presets " + _presets.front()->lawId + " and " + p->lawId +
                       " fix different times of firing",
                   0);
        }
        fixed = p->activation;
        presetScope = p->scope;
        for (const auto& t : p->triggers) {
            if (std::find(triggers.begin(), triggers.end(), t) == triggers.end()) triggers.push_back(t);
        }
        if (p->condition) {
            if (p->condition->kind == ConditionNode::Kind::All && p->condition->children.empty()) {
                fixedNone = true;
            } else {
                condition = condition ? ConditionNode::all({*condition, *p->condition}) : *p->condition;
            }
        }
        if (p->action) {
            if (p->action->kind == ActionNode::Kind::Sequence) {
                for (const auto& c : p->action->children) actions.push_back(c);
            } else {
                actions.push_back(*p->action);
            }
        }
    }
    if (fixed && _activationSpoken && *fixed != _activation) {
        refuse("preset " + _out.presetLawIds.front() + " fixes when it fires; this sentence says otherwise",
               0);
    }
    if (fixedNone && (_condition || condition)) {
        refuse("a preset here fixes that the Law has no condition", 0);
    }
    const Law::Activation activation = fixed.value_or(_activation);
    const Law::Scope scope = _scope          ? *_scope
                           : presetScope     ? *presetScope
                           : activation == Law::Activation::OnEvent ? Law::Scope::Subject
                                                                    : Law::Scope::Everyone;
    if (activation != Law::Activation::OnEvent && !_triggers.empty()) {
        refuse("a Law that does not fire on events cannot name one to fire on", 0);
    }
    for (const auto& t : _triggers) {
        if (std::find(triggers.begin(), triggers.end(), t) == triggers.end()) triggers.push_back(t);
    }
    if (_condition) condition = condition ? ConditionNode::all({*condition, *_condition}) : *_condition;
    for (const auto& a : _actions) actions.push_back(a);

    _out.activation = activation;
    _out.scope = scope;
    _out.triggers = triggers;
    _out.condition = condition;
    if (actions.size() == 1) _out.action = actions.front();
    else if (!actions.empty()) _out.action = ActionNode::sequence(actions);

    if (activation == Law::Activation::OnEvent && triggers.empty()) _out.openClauses.push_back("on <event>");
    if (!_out.action) _out.openClauses.push_back("then <action>");
    if (activation != Law::Activation::OnEvent && !condition && !fixedNone) {
        _out.openClauses.push_back("if <condition> (optional)");
    }
}

bool requiredOpen(const std::vector<std::string>& open) {
    for (const auto& c : open) {
        if (c.find("(optional)") == std::string::npos) return true;
    }
    return false;
}

// Normalise runs of whitespace so a phrase candidate compares with what was
// typed however it was spaced.
std::string squash(const std::string& s) {
    std::string out;
    bool space = false;
    for (char c : s) {
        if (isSpace(c)) {
            space = true;
            continue;
        }
        if (space && !out.empty()) out += ' ';
        space = false;
        out += fold(c);
    }
    if (space && !out.empty()) out += ' ';
    return out;
}

// `tail` is what has been typed of the thing being completed (it may span
// several words of a phrase); `word` is readline's current word, the part a
// completion replaces. Both end at the cursor.
void addCandidate(std::set<std::string>& out, const std::string& candidate, const std::string& tail,
                  const std::string& word) {
    const std::string t = squash(tail);
    if (!startsWith(squash(candidate), t)) return;
    // What the Person typed keeps the case they typed it in ("se" -> "set",
    // not the engine's "Set"); only the untyped remainder comes from the
    // candidate.
    if (tail.size() <= word.size()) {
        // The thing began inside the current word ("health>" + ">="): keep
        // the word's head, complete its tail.
        if (tail.size() <= candidate.size()) out.insert(word + candidate.substr(tail.size()));
        return;
    }
    // A phrase begun before the current word ("greater th"): complete from
    // where the current word begins inside the candidate.
    const std::size_t from = t.size() - squash(word).size();
    if (from + word.size() <= candidate.size()) out.insert(word + candidate.substr(from + word.size()));
}

void offerPaths(std::set<std::string>& out, const Vocabulary& vocab, const std::string& tail,
                const std::string& word) {
    if (!tail.empty() && tail[0] == '@') {
        for (const char* root : {"@event.subject.", "@event.object.", "@world."}) {
            addCandidate(out, root, tail, word);
        }
        for (const auto& b : vocab.beings) {
            const std::string root = "@" + b + ".";
            addCandidate(out, root, tail, word);
            if (startsWith(tail, root) && vocab.propertiesOf) {
                for (const auto& p : vocab.propertiesOf(b)) addCandidate(out, root + p, tail, word);
            }
        }
        return;
    }
    if (vocab.propertiesOf && !vocab.scopeBeing.empty()) {
        for (const auto& p : vocab.propertiesOf(vocab.scopeBeing)) addCandidate(out, p, tail, word);
    }
}

} // namespace

// ---------------------------------------------------------------------------
// The structural words and the engine's own opcode spellings.
// ---------------------------------------------------------------------------
// What the menu says beside a structural word or an engine opcode spelling.
static std::string canonicalDescription(const std::string& op) {
    static const std::pair<const char*, const char*> kTable[] = {
        {"clause.name", "names the Law"},
        {"clause.trigger", "fires on an event"},
        {"clause.condition", "the condition follows"},
        {"clause.action", "the action follows"},
        {"clause.timeline", "Timeline clause (not yet)"},
        {"logic.And", "both must hold"},
        {"logic.Or", "either may hold"},
        {"filler.to", ""},
        {"filler.by", ""},
        {"filler.about", "whom the event is about"},
        {"filler.within", "the tolerance of near"},
        {"activation.OnEvent", "fires when its event happens"},
        {"activation.WhileTrue", "applies every moment it holds"},
        {"activation.OnBecomeTrue", "fires the moment it starts holding"},
        {"scope.Subject", "acts on the event's subject"},
        {"scope.Everyone", "acts on every being satisfying the IF"},
        {"value.true", "true"},
        {"value.false", "false"},
        {"op.Eq", "equals"},
        {"op.Ne", "differs from"},
        {"op.Lt", "less than"},
        {"op.Le", "at most"},
        {"op.Gt", "greater than"},
        {"op.Ge", "at least"},
        {"op.Near", "near … within <tolerance>"},
        {"op.InRange", "between <low> and <high>"},
        {"condition.Not", "negates the next condition"},
        {"condition.Related", "related [type] [to @being]"},
        {"condition.IsKind", "is a kind of being"},
        {"condition.Identity", "is exactly @being"},
        {"condition.Overlaps", "touching @being"},
        {"action.Set", "set <path> to <value>"},
        {"action.Add", "add <number> to <path>"},
        {"action.Scale", "multiply <path> by <number>"},
        {"action.Publish", "publish an event"},
        {"action.Destroy", "remove a being"},
        {"action.Spawn", "spawn a concept"},
        {"action.AddProperty", "grant a property"},
        {"action.RemoveProperty", "revoke a property"},
        {"action.AddRelation", "relate <a> <type> <b>"},
    };
    for (const auto& [code, text] : kTable) {
        if (op == code) return text;
    }
    if (startsWith(op, "condition.")) return "condition (Law Graph only for now)";
    if (startsWith(op, "action.")) return "action (Law Graph only for now)";
    if (startsWith(op, "kind.")) return "a kind of being";
    return "";
}

std::vector<Word> canonicalWords() {
    std::vector<Word> w{
        {"called", "clause.name", "", ""},
        {"named", "clause.name", "", ""},
        {"my law called", "clause.name", "", ""},
        {"on", "clause.trigger", "", ""},
        {"fires", "clause.trigger", "", ""},
        {"if", "clause.condition", "", ""},
        {"when", "clause.condition", "", ""},
        {"with condition", "clause.condition", "", ""},   // Zach's own phrasing
        {"then", "clause.action", "", ""},
        {"timeline", "clause.timeline", "", ""},
        {"and", "logic.And", "", ""},
        {"or", "logic.Or", "", ""},
        {"to", "filler.to", "", ""},
        {"by", "filler.by", "", ""},
        {"about", "filler.about", "", ""},
        {"within", "filler.within", "", ""},
        {"OnEvent", "activation.OnEvent", "", ""},
        {"WhileTrue", "activation.WhileTrue", "", ""},
        {"OnBecomeTrue", "activation.OnBecomeTrue", "", ""},
        {"Subject", "scope.Subject", "", ""},
        {"Everyone", "scope.Everyone", "", ""},
        {"true", "value.true", "", ""},
        {"false", "value.false", "", ""},
        {"=", "op.Eq", "", ""},
        {"==", "op.Eq", "", ""},
        {"!=", "op.Ne", "", ""},
        {"<", "op.Lt", "", ""},
        {"<=", "op.Le", "", ""},
        {">", "op.Gt", "", ""},
        {">=", "op.Ge", "", ""},
        {"near", "op.Near", "", ""},
        {"between", "op.InRange", "", ""},
        {"not", "condition.Not", "", ""},
    };
    // ConditionNode kinds a sentence names directly (All/Any are and/or;
    // Compare is "<path> <op> <value>").
    for (const char* kind : {"InRegion", "Related", "Zone", "IsKind", "Identity", "ForAny",
                             "ForAll", "Overlaps"}) {
        w.push_back({kind, std::string("condition.") + kind, "", ""});
    }
    // Every ActionNode kind, spelled as the engine spells it. Kinds without a
    // sentence form are still words, so the refusal can name them.
    for (int k = 0; k <= static_cast<int>(ActionNode::Kind::ElevatePixels); ++k) {
        const char* name = ActionNode::kindName(static_cast<ActionNode::Kind>(k));
        w.push_back({name, std::string("action.") + name, "", ""});
    }
    // BeingKind — World (6) is burned and deliberately absent.
    for (const char* kind : {"AnyBeing", "Object", "Person", "Relation", "Formation", "Law",
                             "Zone", "Lexeme"}) {
        w.push_back({kind, std::string("kind.") + kind, "", ""});
    }
    for (auto& word : w) word.description = canonicalDescription(word.opcode);
    return w;
}

// ---------------------------------------------------------------------------
// A denoted Law's shape IS its opcode. An open slot means "the sentence fills
// this"; no open slot means the Law is a fragment a sentence adopts whole.
// ---------------------------------------------------------------------------
std::string classify(const Law& law, const std::vector<std::string>& triggers, Preset& preset) {
    const bool hasAction = law.hasActionModel();
    const bool hasCondition = law.hasConditionModel();
    if (hasAction && !hasCondition) {
        const ActionNode& a = *law.actionModel();
        using K = ActionNode::Kind;
        // "red": a Set with no path but a value. The Law holds what the word
        // stands for; the sentence decides where it goes.
        if (a.kind == K::Set && a.path.empty() && !std::holds_alternative<std::monostate>(a.operand)) {
            preset = Preset{};
            preset.lawId = law.getIdentifier();
            preset.value = a.operand;
            return "value";
        }
        bool open = false;
        switch (a.kind) {
            case K::Set: case K::Add: case K::Scale: open = a.path.empty(); break;
            case K::Publish: open = a.eventType.empty(); break;
            case K::Spawn: open = a.conceptId.empty(); break;
            case K::AddProperty: case K::RemoveProperty: case K::AddRelation:
                open = a.propertyName.empty();
                break;
            case K::Destroy: open = a.elementToken.empty() && a.containerToken.empty(); break;
            default: break;
        }
        if (open) return std::string("action.") + ActionNode::kindName(a.kind);
    }
    if (hasCondition && !hasAction) {
        const ConditionNode& c = *law.conditionModel();
        using K = ConditionNode::Kind;
        switch (c.kind) {
            case K::Compare:
                if (c.path.empty()) {
                    switch (c.op) {
                        case ConditionNode::Op::Eq: return "op.Eq";
                        case ConditionNode::Op::Ne: return "op.Ne";
                        case ConditionNode::Op::Lt: return "op.Lt";
                        case ConditionNode::Op::Le: return "op.Le";
                        case ConditionNode::Op::Gt: return "op.Gt";
                        case ConditionNode::Op::Ge: return "op.Ge";
                        case ConditionNode::Op::Near: return "op.Near";
                        case ConditionNode::Op::InRange: return "op.InRange";
                    }
                }
                break;
            case K::Related:
                if (c.relationType.empty() && c.otherId.empty()) return "condition.Related";
                break;
            case K::Identity:
                if (c.otherId.empty()) return "condition.Identity";
                break;
            case K::Overlaps:
                if (c.otherId.empty()) return "condition.Overlaps";
                break;
            case K::IsKind:
                if (c.beingKind == ConditionNode::BeingKind::AnyBeing) return "condition.IsKind";
                break;
            case K::Not:
                if (c.children.empty()) return "condition.Not";
                break;
            default:
                break;
        }
    }
    preset = Preset{};
    preset.lawId = law.getIdentifier();
    preset.activation = law.activation();
    preset.scope = law.scope();
    preset.triggers = triggers;
    if (hasCondition) preset.condition = *law.conditionModel();
    if (hasAction) preset.action = *law.actionModel();
    return "preset";
}

std::string Parse::preview() const {
    if (!error.empty()) return "refused: " + error;
    std::string s;
    if (!name.empty()) s += "\"" + name + "\": ";
    if (activation == Law::Activation::WhileTrue) {
        s += "EVERY MOMENT, watching its subjects";
    } else if (activation == Law::Activation::OnBecomeTrue) {
        s += "WHENEVER the condition STARTS holding";
    } else if (triggers.empty()) {
        s += "WHEN <event?> fires";
    } else {
        s += "WHEN ";
        for (std::size_t i = 0; i < triggers.size(); ++i) {
            s += (i ? " or \"" : "\"") + triggers[i] + "\"";
        }
        s += " fires";
    }
    s += "  ->  IF " + (condition ? condition->describe() : std::string("always"));
    s += "  ->  THEN " + (action ? action->describe() : std::string("<action?>"));
    s += scope == Law::Scope::Everyone ? "  — on every being satisfying the IF"
                                       : "  — on the event's subject";
    for (const auto& id : presetLawIds) s += "  (preset " + id + ")";
    if (!openClauses.empty()) {
        s += "  [still open:";
        for (const auto& c : openClauses) s += " " + c + ";";
        s += "]";
    }
    return s;
}

Parse parse(const std::string& raw, const Vocabulary& vocab) {
    std::size_t lead = 0;
    while (lead < raw.size() && isSpace(raw[lead])) ++lead;
    std::string text = trim(raw);
    if (startsWith(text, "??")) {
        Parse p;
        p.search = true;
        p.candidates = search(text.substr(2), vocab);
        p.spans.push_back({lead, lead + 2, "clause"});
        p.ok = true;
        return p;
    }
    bool previewOnly = false;
    if (!text.empty() && text.back() == '?') {
        previewOnly = true;
        text.pop_back();
    }

    Parser parser(text, vocab, false);
    Parse out;
    try {
        parser.run();
        parser.finish();
        out = parser.result();
    } catch (const Refusal& r) {
        out = parser.result();
        out.error = r.message;
        out.errorOffset = r.offset;
        out.candidates = r.candidates;
    }
    out.previewOnly = previewOnly;
    for (const auto& span : parser.spans()) out.spans.push_back({span.start + lead, span.end + lead, span.role});
    if (!out.error.empty() && out.errorOffset < text.size()) {
        std::size_t end = out.errorOffset;
        while (end < text.size() && !isSpace(text[end])) ++end;
        out.spans.push_back({out.errorOffset + lead, end + lead, "error"});
    }
    if (out.error.empty() && !previewOnly && requiredOpen(out.openClauses)) {
        out.error = "still open:";
        for (const auto& c : out.openClauses) out.error += " " + c + ";";
        out.errorOffset = text.size();
    }
    out.ok = out.error.empty();
    return out;
}

std::vector<std::string> complete(const std::string& beforeCursor, const Vocabulary& vocab) {
    const std::size_t n = beforeCursor.size();
    std::size_t wordStart = n;
    while (wordStart > 0 && !isSpace(beforeCursor[wordStart - 1])) --wordStart;
    const std::string word = beforeCursor.substr(wordStart);

    // Try every place the thing being completed could have begun: a word
    // boundary or a change between path characters and symbols, up to a
    // short phrase back. Each split parses the head and asks what it expects.
    std::set<std::size_t> splits{n};
    const std::size_t floor = n > 48 ? n - 48 : 0;
    for (std::size_t p = floor; p < n; ++p) {
        if (p == 0 || isSpace(beforeCursor[p - 1]) ||
            (!isSpace(beforeCursor[p]) && isAtomChar(beforeCursor[p - 1]) != isAtomChar(beforeCursor[p]))) {
            if (!isSpace(beforeCursor[p])) splits.insert(p);
        }
    }

    std::set<std::string> out;
    for (std::size_t split : splits) {
        // An empty tail means "what comes next" — only after a space. Right
        // after a word, what comes next would be glued onto it ("cofalse").
        if (split == n && n > 0 && !isSpace(beforeCursor[n - 1])) continue;
        const std::string head = beforeCursor.substr(0, split);
        const std::string tail = beforeCursor.substr(split);
        Parser parser(head, vocab, true);
        try {
            parser.run();
            continue;   // a completing parse always stops at its end
        } catch (const Stop&) {
        } catch (const Refusal&) {
            continue;   // this split does not parse; another may
        }
        const Expectation& e = parser.expectation();
        for (const auto& w : vocab.words) {
            if (admits(e.admit, w.opcode)) addCandidate(out, w.symbol, tail, word);
        }
        if (tail.find_first_of(" \t") != std::string::npos) continue;   // the rest are atoms
        if (e.event) {
            for (const auto& ev : vocab.events) addCandidate(out, ev, tail, word);
        }
        if (e.path) offerPaths(out, vocab, tail, word);
        if (e.being) {
            for (const auto& b : vocab.beings) addCandidate(out, "@" + b, tail, word);
        }
    }
    return {out.begin(), out.end()};
}

namespace {

// How well `typed` (what was typed of the thing, possibly several words)
// matches `candidate`: 0 = not at all. Prefix beats word-start beats
// subsequence; shorter candidates win ties, so the likeliest word comes first.
int fuzzyScore(const std::string& typed, const std::string& candidate) {
    const std::string t = squash(typed);
    const std::string c = squash(candidate);
    if (t.empty()) return 1;
    const int shortness = std::max(0, 200 - static_cast<int>(c.size()));
    if (startsWith(c, t)) return 3000 + shortness;
    if (t.find(' ') != std::string::npos) return 0;   // phrases match by prefix only
    for (std::size_t i = 1; i < c.size(); ++i) {
        const bool wordStart = !std::isalnum(static_cast<unsigned char>(c[i - 1])) &&
                               std::isalnum(static_cast<unsigned char>(c[i]));
        if (wordStart && c.compare(i, t.size(), t) == 0) return 2000 + shortness;
    }
    if (c.find(t) != std::string::npos) return 1500 + shortness;
    // Subsequence ("gtt" -> "greater than"), rewarding runs.
    std::size_t j = 0;
    int run = 0, bonus = 0;
    for (char ch : c) {
        if (j < t.size() && ch == t[j]) {
            ++j;
            bonus += ++run;
        } else {
            run = 0;
        }
    }
    return j == t.size() ? 500 + bonus * 10 + shortness / 4 : 0;
}

} // namespace

std::vector<Suggestion> suggest(const std::string& beforeCursor, const Vocabulary& vocab) {
    const std::size_t n = beforeCursor.size();
    std::set<std::size_t> splits{n};
    const std::size_t floor = n > 48 ? n - 48 : 0;
    for (std::size_t p = floor; p < n; ++p) {
        if (isSpace(beforeCursor[p])) continue;
        if (p == 0 || isSpace(beforeCursor[p - 1]) ||
            isAtomChar(beforeCursor[p - 1]) != isAtomChar(beforeCursor[p])) {
            splits.insert(p);
        }
    }

    std::map<std::string, Suggestion> best;   // by text: keep the best reading of it
    const auto offer = [&](std::size_t from, const std::string& tail, const std::string& text,
                           const std::string& description, const std::string& role) {
        // A word that could only be refused ("author it in the Law Graph")
        // is never offered: the menu only holds words that can work here.
        if (description.find("Law Graph only") != std::string::npos) return;
        const int score = fuzzyScore(tail, text);
        if (score == 0) return;
        const std::string key = lower(text);
        auto it = best.find(key);
        if (it == best.end() || it->second.score < score) {
            best[key] = Suggestion{from, text, description, role, score};
        }
    };

    for (std::size_t split : splits) {
        // An empty tail means "what comes next" — only after a space. Right
        // after a word, what comes next would be glued onto it ("cofalse").
        if (split == n && n > 0 && !isSpace(beforeCursor[n - 1])) continue;
        const std::string head = beforeCursor.substr(0, split);
        const std::string tail = beforeCursor.substr(split);
        Parser parser(head, vocab, true);
        try {
            parser.run();
            continue;
        } catch (const Stop&) {
        } catch (const Refusal&) {
            continue;
        }
        const Expectation& e = parser.expectation();
        for (const auto& w : vocab.words) {
            if (admits(e.admit, w.opcode)) offer(split, tail, w.symbol, w.description, roleOf(w.opcode));
        }
        if (tail.find_first_of(" \t") != std::string::npos) continue;   // atoms are single words
        if (e.event) {
            for (const auto& ev : vocab.events) {
                offer(split, tail, ev, vocab.describeEvent ? vocab.describeEvent(ev) : "event", "event");
            }
        }
        if (e.being) {
            for (const auto& b : vocab.beings) {
                offer(split, tail, "@" + b, vocab.describeBeing ? vocab.describeBeing(b) : "", "being");
            }
        }
        if (e.path) {
            const auto prop = [&](const std::string& being, const std::string& name) {
                return vocab.describeProperty ? vocab.describeProperty(being, name) : std::string{};
            };
            if (!tail.empty() && tail[0] == '@') {
                bool inside = false;
                for (const auto& b : vocab.beings) {
                    const std::string root = "@" + b + ".";
                    if (startsWith(tail, root) && vocab.propertiesOf) {
                        inside = true;
                        const std::string rest = tail.substr(root.size());
                        for (const auto& p : vocab.propertiesOf(b)) {
                            const int score = fuzzyScore(rest, p);
                            if (score == 0) continue;
                            const std::string text = root + p;
                            auto it = best.find(lower(text));
                            if (it == best.end() || it->second.score < score) {
                                best[lower(text)] = Suggestion{split, text, prop(b, p), "path", score};
                            }
                        }
                    }
                }
                if (!inside) {
                    for (const char* root : {"@event.subject.", "@event.object.", "@world."}) {
                        offer(split, tail, root, "whoever the event is about", "path");
                    }
                    for (const auto& b : vocab.beings) {
                        offer(split, tail, "@" + b + ".",
                              vocab.describeBeing ? vocab.describeBeing(b) : "", "being");
                    }
                }
            } else if (vocab.propertiesOf && !vocab.scopeBeing.empty()) {
                for (const auto& p : vocab.propertiesOf(vocab.scopeBeing)) {
                    offer(split, tail, p, prop(vocab.scopeBeing, p), "path");
                }
            }
        }
    }

    std::vector<Suggestion> ranked;
    for (auto& [text, s] : best) ranked.push_back(std::move(s));
    std::sort(ranked.begin(), ranked.end(), [](const Suggestion& a, const Suggestion& b) {
        if (a.score != b.score) return a.score > b.score;
        return lower(a.text) < lower(b.text);
    });

    // Nor a word that would contradict what the sentence already says —
    // "always" after "when they collide" (two presets fixing different
    // times of firing). Each word is tried in place; a refusal that is not
    // merely "the sentence is unfinished" or "a Metalaw decides" drops it.
    std::vector<Suggestion> out;
    for (auto& s : ranked) {
        if (out.size() >= 60) break;   // the menu shows 8; keep typing narrows
        if (s.role != "path" && s.role != "event" && s.role != "being") {
            const Parse trial = parse(beforeCursor.substr(0, s.from) + s.text + " ?", vocab);
            const std::string& e = trial.error;
            if (!e.empty() && e.rfind("the sentence ends where", 0) != 0 &&
                e.find("Metalaw") == std::string::npos && e.find("does not begin a clause") == std::string::npos) {
                continue;
            }
        }
        out.push_back(std::move(s));
    }
    return out;
}

std::vector<std::string> search(const std::string& rawQuery, const Vocabulary& vocab) {
    const std::string q = lower(trim(rawQuery));
    const auto hit = [&](const std::string& s) { return lower(s).find(q) != std::string::npos; };

    std::vector<std::string> out;
    for (const auto& w : vocab.words) {
        if (hit(w.symbol) || hit(w.opcode) || (!w.description.empty() && hit(w.description))) {
            out.push_back("word     " + w.symbol + "  ->  " + w.opcode +
                          (w.description.empty() ? "" : "  · " + w.description) + "  (" + w.individual() + ")");
        }
    }
    for (const auto& e : vocab.events) {
        if (hit(e)) out.push_back("event    " + e);
    }
    for (const auto& b : vocab.beings) {
        if (hit(b)) out.push_back("being    @" + b);
    }
    if (vocab.propertiesOf && !vocab.scopeBeing.empty()) {
        for (const auto& p : vocab.propertiesOf(vocab.scopeBeing)) {
            if (hit(p)) out.push_back("property " + p + "  (on @" + vocab.scopeBeing + ")");
        }
    }
    return out;
}

} // namespace LawSentence
} // namespace Terminal
} // namespace Singularity
