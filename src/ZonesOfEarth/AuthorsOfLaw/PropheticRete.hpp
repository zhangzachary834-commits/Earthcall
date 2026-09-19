#pragma once

#include "ActionModel.hpp"
#include "ConditionModel.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"
#include "json.hpp"

#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Law;

// ============================================================================
// Prophetic Rete (B-Time Rete) — the ahead-of-time abstract interpreter over
// the authored law set.  Zach's realization, 2026-09-01 (`law/B-time Rete.md`,
// structured by ChatGPT 5.6 Luna as `Rete_Truth_Seeking_Focus.md`):
//
//   > In Earthcall, changes are not arbitrary. Changes are caused by Laws and
//   > First Movers.
//
// An ordinary Rete asks, after the fact, "what changed?".  Earthcall can ask
// the earlier question, because the authored Laws are structured data that
// exist before they fire: "given everything that is lawfully capable of
// changing, where could that change possibly matter?"  A Law's text already
// names the properties it reads, the properties it writes, the Beta
// constraints on the way in and the OntoMath transformation on the way out.
// Composing those constraints is an ordinary abstract interpretation, and its
// result is a set of filters that only change when the Laws do.
//
// THIS FILE IS THE FOUNDATION LAYER, and deliberately not the whole design.
// It builds the possibility-space index; it does not yet own the runtime
// ActionNode -> Beta back-pointers (§9), and it does not attempt the
// Rete-as-Singulars / First Mover stratification (§16-19), which is Zach's
// architectural call to make.  See `docs/architecture/law/PROPHETIC_RETE.md`
// for what is built, what is next, and which decisions are marked AUTHOR.
//
// ----------------------------------------------------------------------------
// THE ONE RULE, and everything else is detail: **the analysis is an
// OVER-approximation, and only ever concludes IMPOSSIBLE.**
//
// Every answer here is allowed to be too generous and never too narrow.  A
// range that is wider than the truth costs a little performance.  A range that
// is narrower than the truth makes a Law go deaf — it stops firing, silently,
// and no test that does not happen to exercise that exact Law will notice.
// So: anything this analysis cannot read answers Top, and any Law carrying
// something it cannot read marks the whole index INCOMPLETE, at which point
// every filter derived from it fails open.  "Possibly" is always a safe
// answer here; "no" never is unless it is proved.
// ============================================================================

namespace Prophetic {

// --------------------------------------------------------------------------
// The abstract value lattice: what a property could hold, as opposed to what
// it does hold.  NOT serialized — like OntoMath::ValueKind this is a
// judgement ABOUT authored text, never written into a save — so unlike the
// enums a save file carries, Kind may be extended.
// --------------------------------------------------------------------------
struct Range {
    enum class Kind { Bottom, Number, Boolean, Text, Top };

    Kind kind = Kind::Top;

    OntoMath::Interval number{};                 // Kind::Number
    bool maybeTrue = true, maybeFalse = true;    // Kind::Boolean
    std::set<std::string> texts;                 // Kind::Text — the exact possibilities
    bool textMaybeOther = false;                 // Kind::Text — ...plus something unlisted

    static Range bottom() { Range r; r.kind = Kind::Bottom; return r; }
    static Range top()    { Range r; r.kind = Kind::Top;    return r; }
    static Range fromInterval(OntoMath::Interval i);
    static Range boolean(bool canBeTrue, bool canBeFalse);
    static Range text(std::set<std::string> possibilities);
    // The singleton range of one concrete value. A value this lattice cannot
    // represent (a vec3, a mat4, a being pointer, monostate) answers Top.
    static Range ofValue(const PropertyValue& v);

    bool isBottom() const { return kind == Kind::Bottom; }
    bool isTop() const { return kind == Kind::Top; }

    // THE question Pass 3 asks. True unless the two are PROVABLY disjoint —
    // mismatched lattice kinds, anything involving Top, and every case this
    // does not know how to compare all answer true.
    bool mayIntersect(const Range& other) const;

    Range joined(const Range& other) const;   // union / widening (Any, Σ of writers)
    Range met(const Range& other) const;      // overlap (All); may be Bottom

    std::string print() const;
    nlohmann::json toJson() const;
};

// --------------------------------------------------------------------------
// One write a Law's action tree can perform: where, and what could land there.
// --------------------------------------------------------------------------
struct WriteEffect {
    std::string lawId;
    std::string path;          // dotted, exactly as authored (referent prefix kept)
    Range range;               // what this action can put there
    std::string via;           // the ActionNode::Kind that writes it, for the report
};

// --------------------------------------------------------------------------
// One demand a Law's condition tree makes: where it reads, and which values
// there could satisfy it.  `satisfying` is an over-approximation of the
// satisfying set — Top whenever the condition's shape is not an interval
// (Ne, Not, a live right-hand path, a region, a guard).
// --------------------------------------------------------------------------
struct ReadDemand {
    std::string lawId;
    std::string path;
    Range satisfying;
    bool aboutInstances = false;   // read inside a ForAny/ForAll: about the
                                   // quantified being, not the law's subject
};

// --------------------------------------------------------------------------
// What one Law's text says, read structurally. `opaque*` are the honesty
// flags: a Law that creates, destroys, spawns, or publishes changes the world
// in ways a path walk does not enumerate, and a Law whose conditions reach
// through a region or a guard reads more than a path walk can name.
// --------------------------------------------------------------------------
struct LawFacts {
    std::string lawId;
    std::vector<WriteEffect> writes;
    std::vector<ReadDemand> reads;
    // Every property NAME (not path) this law's conditions could cause to be
    // read, including every contiguous sub-run of each authored path's
    // segments — because a path resolves through nested Singulars and the
    // dirty callback names the LEAF owner's property, not the whole path.
    std::unordered_set<std::string> readNames;
    bool opaqueWrites = false;     // structural action: the write set is not enumerable
    bool opaqueReads = false;      // structural condition: the read set is not enumerable
    std::vector<std::string> notes;   // why, in tree order — this is the audit trail
};

// Read one action tree / condition tree structurally. Free functions so the
// analysis stays testable without a Law, a Universe, or a world.
void analyzeAction(const ActionNode& node, LawFacts& out);
void analyzeCondition(const ConditionNode& node, LawFacts& out,
                      bool insideQuantifier = false);
LawFacts analyzeLaw(const Law& law);

// The possibility space of one authored OntoMath model, with every variable
// unbound (Top). This is Pass 3's engine: it is what proves "x -> 2x -> x+5
// over x in [0,10] can never exceed 25", and what bounds `0.5*sin(t)+0.5` to
// [0,1] no matter what t does.
Range rangeOfPiecewise(const OntoMath::Piecewise& model,
                       const std::map<std::string, OntoMath::Interval>& bound = {});
Range rangeOfCurve(const CurveModel& curve);

// --------------------------------------------------------------------------
// The index itself: the possibility-space filters, rebuilt only when the Laws
// change (§11).  Everything it answers is derived — the Laws are the truth,
// this is a cache of conclusions about them — which is why it is not
// registered as property paths on anything: there is no state here a Person
// could author that is not already the law text itself.
// --------------------------------------------------------------------------
class Index {
public:
    // Rebuild from the whole law register. Cheap enough to call per frame
    // (it is a walk of the law TEXT, not of the world), but the caller is
    // expected to gate it on a revision — see LawManager::tick.
    void rebuild(const std::vector<std::shared_ptr<Law>>& laws);
    void clear();

    // --- Pass 1 / Pass 2: the relevance filter. -------------------------
    // False ONLY when no authored condition anywhere could read a property of
    // this name. When the index is incomplete this always answers true, which
    // is what makes it safe to consult on the hot path.
    bool anyConditionReads(const std::string& propertyName) const;

    // True when every Law in the register was read structurally. False means
    // at least one carried something the analysis refuses to guess about, and
    // every filter derived from this index must fail open.
    bool complete() const { return _complete; }

    // --- Pass 3: what the abstract interpretation concluded. -------------
    // A condition no authored Law can drive into its satisfying range.
    //
    // READ THE CLAIM EXACTLY. This is NOT "this law can never fire". Properties
    // are also moved by First Movers and by foreign channels, whose transforms
    // this analysis cannot see — Zach's §20/§21 edge case, and the reason the
    // claim is scoped to *authored Law*. What it says is: nothing in this
    // world's law text can carry that property into that range, so if it ever
    // gets there, a First Mover or an external channel put it there. That is
    // exactly the question an author asks when a law they wrote never fires.
    //
    // Two reasons produce an entry, and only these two:
    //   SelfImpossible  the law's own mathematics can never satisfy its own
    //                   condition (an authored Zone function whose range never
    //                   meets its satisfaction interval). Proved from one
    //                   law's text alone; true unconditionally.
    //   NoLawfulDriver  some law writes the path, but the union of every
    //                   authored write to it is disjoint from the demand.
    //                   Reported only when the whole index is complete and
    //                   nothing in it writes opaquely.
    struct Unreachable {
        std::string lawId;
        std::string path;
        std::string why;
        bool selfImpossible = false;   // true = proved from this law's text alone
    };
    const std::vector<Unreachable>& unreachable() const { return _unreachable; }

    // The union of everything any Law can write to this path — the answer
    // Pass 3 intersects a condition's demand against. Top when unknown.
    Range writeRangeOf(const std::string& path) const;

    const std::vector<LawFacts>& facts() const { return _facts; }

    // Counters, for the report and for the tests that guard the filter.
    std::size_t readNameCount() const { return _readNames.size(); }
    std::size_t lawCount() const { return _facts.size(); }

    nlohmann::json toJson() const;

private:
    std::vector<LawFacts> _facts;
    std::unordered_set<std::string> _readNames;
    std::unordered_map<std::string, Range> _writeRanges;
    std::vector<Unreachable> _unreachable;
    bool _complete = true;
};

// Every contiguous sub-run of a dotted path's segments, joined by dots.
// "body.head.position" yields body, head, position, body.head, head.position,
// body.head.position — because PropertyPath resolution descends through
// nested Singulars and the property-change callback reports the name on
// WHICHEVER Singular ends up owning the leaf, not the path the author wrote.
// Anything narrower than this is unsound; see the header comment.
void collectPathNames(const PropertyPath& path, std::unordered_set<std::string>& out);

} // namespace Prophetic
