#pragma once

#include "Singularity/Core/EventBus.hpp"
#include "Singularity/Execution/ExecutionChannel.hpp"
#include "Relation/Formation/Formation.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Relation/Relation.hpp"
#include "Relation/RelationManager.hpp"
#include "../Physics/Physics.hpp"
#include "ConstructedBeing/Singular/Singular.hpp"
#include "../Zone/Zone.hpp"
#include "ECA.hpp"
#include "ConditionModel.hpp"
#include "PropheticRete.hpp"
#include "ActionModel.hpp"
#include "json.hpp"

#include <ctime>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// This class should eventually govern Physics. Physics should be extended from
// this class.
//
// Law is the identity of process (an Singular); its condition/action models are its essence; the
// compiled ECA closures are its manifestation. Hard-coded closures are for first
// movers only — Person-authored laws arrive as serializable models (see
// LAW_AND_CREATION_SYSTEM.md, Stage 2). The relational aspect of a Law (its
// provenance, its Formation of conditions, and its ontological instances 
// as interactions between the enforcer/authors and the targets/recipients) 
// is carried by composition rather than
// by also inheriting Relation: Object and Relation are both Singulars, and
// Earthcall models Relation-Objects with member formations, not a diamond.
class Law : public Singular {
public:
    // A Law's authors, condition subjects and targets are FORMATIONS — beings
    // in their own right, with an identifier, legible properties, a place in
    // the Universe and provenance. They were briefly replaced by a bare
    // `struct NodeGroup { vector<Singular*>; vector<shared_ptr<Relation>>; }`,
    // which re-implemented the mechanical half of Formation and dropped
    // everything ontological: `law->authors()` stopped being a being and
    // became a field. Refusal #1 — no new C++ class for a domain noun — and
    // this one had a name in the ontology already.
    // Condition/action vocabulary is the shared ECA language (ECA.hpp), so a
    // Law's pieces are interchangeable with any other event-condition-action
    // carrier in the system.
    using ConditionMode = ECA::ConditionMode;
    using ConditionPredicate = ECA::ConditionPredicate;
    using Condition = ECA::Condition;
    using Action = ECA::Action;

    // WHETHER THE ACTION BRANCH WAS REACHED — nothing more. `Applied` means
    // control got past the gates (enabled, authored, authority, conditions)
    // and began traversing the action tree. It does NOT claim the actions
    // landed: that is a finer truth, it lives per-node, and it is carried in
    // the record's `trace`. Conflating the two is what let a law whose every
    // write failed report SUCCESS and open a drive session that never ended.
    enum class ApplicationResult {
        Applied,
        Disabled,
        Unauthored,
        NoTarget,
        ConditionsFailed,
        NoAction,
        AuthorityDenied   // metalaw ceiling: lower authority may not govern higher
    };

    struct ApplicationRecord {
        std::time_t timestamp{0};
        std::string lawId;
        std::string targetId;
        ApplicationResult result{ApplicationResult::NoTarget};
        std::vector<std::string> conditionDescriptions;
        std::vector<std::string> actionDescriptions;

        // Which action nodes fired, and which of them actually wrote. Empty
        // whenever the action branch was never reached.
        ActionNode::Trace trace;

        // Did anything at all change? This — not `result` — is the question
        // "did the law do something", and it is what gates a drive session:
        // a law that wrote nothing must not be handed a process that
        // re-applies it forever.
        bool changedSomething() const { return trace.anyWrote(); }

        nlohmann::json toJson() const;
    };

    struct AppliedEvent {
        const Law* law{nullptr};
        Singular* target{nullptr};
        ApplicationResult result{ApplicationResult::NoTarget};
        std::time_t timestamp{0};
    };

    explicit Law(const std::string& name = "Law");
    Law(const std::string& name, const std::vector<Singular*>& authors);

    std::string getIdentifier() const override { return _lawId; }

    // Give an engine-instantiated law the stable slug law text names it by.
    //
    // setObjectID() is NOT this. Law inherits Object but overrides
    // getIdentifier() to return _lawId, so setObjectID("physics-gravity")
    // writes a field nothing reads back and leaves the law answering to the
    // generated "law-N" -- an id that changes between runs, which is exactly
    // what the Stable Identifiers rule forbids for anything a save file or a
    // law text names. Every first mover built in C++ called setObjectID and
    // got that silent no-op.
    //
    // Renames the three group formations too, for the reason
    // nameGroupFormations() exists: an id decided anywhere has to carry its
    // formations' names with it.
    void setLawIdentifier(const std::string& id);

    const std::string& name() const { return _name; }
    void setName(const std::string& name);

    std::shared_ptr<Zone> jurisdiction() const { return _jurisdiction; }
    void setJurisdiction(std::shared_ptr<Zone> zone) { _jurisdiction = std::move(zone); }

    bool isEnabled() const { return _enabled; }
    void setEnabled(bool enabled) { _enabled = enabled; }

    // ------------------------------------------------------------------
    // Edge vs level. OnEvent laws hear discrete moments (the ECA loop).
    // But some condition-phases must MONITOR the program at all times:
    //   WhileTrue    — checked every tick; fires each tick it holds
    //                  (continuously running actions);
    //   OnBecomeTrue — checked every tick; fires once at the false->true
    //                  transition and re-arms when it stops holding.
    // Continuous laws watch their targets Formation when present, otherwise
    // the whole Universe of beings. Serialized as int — APPEND-ONLY.
    // ------------------------------------------------------------------
    enum class Activation { OnEvent = 0, WhileTrue = 1, OnBecomeTrue = 2 };
    Activation activation() const { return _activation; }
    void setActivation(Activation a) { _activation = a; }

    // ------------------------------------------------------------------
    // WHOM an event application reaches — the "whose position?" question.
    //   Subject  — the event's subject only (the being the event is about);
    //   Everyone — sweep the law's targets, or the whole Universe when
    //              untargeted, applying to EVERY being that satisfies the
    //              conditions. This is the "every instance of the category"
    //              law; combine with IsKind/Identity conditions to carve
    //              the category. Serialized as int — APPEND-ONLY.
    // ------------------------------------------------------------------
    enum class Scope { Subject = 0, Everyone = 1 };
    Scope scope() const { return _scope; }
    void setScope(Scope s) { _scope = s; }

    // ------------------------------------------------------------------
    // Drive: an AUTHORED choice that this law keeps applying after its
    // trigger — every tick, with t=0 at the moment it took hold — until its
    // function becomes undefined for the subject. The authored bounds end
    // it, and ANY bound variable may cut those bounds: time, another
    // being's position, the subject's own state. (Meaningful for OnEvent /
    // OnBecomeTrue; WhileTrue already re-applies on its own.)
    // ------------------------------------------------------------------
    bool drives() const { return _drives; }
    void setDrives(bool drives) { _drives = drives; }

    // What a re-trigger means while this law's drive is still running — an
    // AUTHORED choice, never an accident (serialized as int, APPEND-ONLY):
    //   Absorb  — the running process owns its clock; new triggers are
    //             absorbed (a block resting in constant collision cannot
    //             stack or reset the process)
    //   Restart — the new trigger is a new t=0: re-kick mid-arc, re-arc
    enum class Retrigger { Absorb = 0, Restart = 1 };
    Retrigger retrigger() const { return _retrigger; }
    void setRetrigger(Retrigger mode) { _retrigger = mode; }

    // Per-subject condition memory for edge detection (OnBecomeTrue).
    bool lastConditionState(const Singular* subject) const {
        auto it = _conditionMemory.find(subject);
        return it != _conditionMemory.end() && it->second;
    }
    void rememberConditionState(const Singular* subject, bool state) {
        _conditionMemory[subject] = state;
    }
    // Who the law believes it currently holds for. The reactive path learns
    // who ENTERED the match set from the network, but nothing tells it who
    // LEFT — so it reads its own memory and takes the difference. Release is
    // what re-arms the onset clock, and a release nobody notices is an onset
    // that never re-arms.
    const std::unordered_map<const Singular*, bool>& conditionMemory() const {
        return _conditionMemory;
    }

    // Per-subject onset memory: the world time at which this law's condition
    // last went false->true for that subject. This is the t=0 of
    // "time.sinceApplied" — the authored change-over-time clock. Runtime
    // state like _conditionMemory: never serialized; release re-arms it.
    bool hasOnset(const Singular* subject) const {
        return _onsetMemory.count(subject) != 0;
    }
    double onsetFor(const Singular* subject) const {
        auto it = _onsetMemory.find(subject);
        return it != _onsetMemory.end() ? it->second : 0.0;
    }
    void rememberOnset(const Singular* subject, double worldTime) {
        _onsetMemory[subject] = worldTime;
    }
    void forgetOnset(const Singular* subject) { _onsetMemory.erase(subject); }
    void forgetSubject(const Singular* subject) {
        _conditionMemory.erase(subject);
        _onsetMemory.erase(subject);
    }

    // First movers (engine-backed bridge laws) live in the register for
    // LEGIBILITY and GOVERNANCE, but their truth lives in the engine:
    // serialization skips them and world loads preserve them.
    virtual bool isFirstMover() const { return false; }

    // ------------------------------------------------------------------
    // The Singularity-grounded hierarchy of authored authority. A law may
    // govern (apply to) another law only when its authority is >= the
    // target's. Deliberately NOT registered as a legible property: the
    // ceiling is granted at the Singularity/first-mover level, never
    // law-modifiable — that is the anti-tyranny (and anti-Babel) guarantee.
    // Lower scopes may govern laws within their jurisdiction but cannot
    // override higher-order metalaws, kernel laws, or substrate order.
    //
    // Keeping it out of the property registry sealed one door and left
    // another wide open: authority came straight off the save file, so
    // hand-editing one integer let an authored law outrank every kernel
    // metalaw. Authority is GRANTED, never claimed —
    //
    //   setAuthorityLevel  the ordinary path. Clamped to kAuthoredCeiling:
    //                      this is what loading, the UI, and any authored
    //                      route may ask for.
    //   grantAuthority     the first-mover path, unclamped. Today the only
    //                      legitimate caller is the engine's own dev tooling
    //                      (the ImGui panels), which is the first-mover
    //                      surface in intention but not yet an explicit
    //                      framework — when that framework arrives, this is
    //                      the seam it plugs into.
    // ------------------------------------------------------------------
    static constexpr int kAuthoredCeiling = 0;
    int authorityLevel() const { return _authorityLevel; }
    void setAuthorityLevel(int level) {
        _authorityLevel = level < kAuthoredCeiling ? level : kAuthoredCeiling;
    }
    void grantAuthority(int level) { _authorityLevel = level; }

    ConditionMode conditionMode() const { return _conditionMode; }
    void setConditionMode(ConditionMode mode) { _conditionMode = mode; }

    Formation& authors() { return _authors; }
    const Formation& authors() const { return _authors; }
    void addAuthor(Singular& author);
    void setAuthors(const std::vector<Singular*>& authors);
    bool isAuthored() const { return !_authors.getMembers().empty(); }

    Formation& conditions() { return _conditions; }
    const Formation& conditions() const { return _conditions; }
    void addConditionSubject(Singular& subject);
    void addConditionRelation(const std::shared_ptr<Relation>& relation);
    void clearConditionFormation();

    Formation& targets() { return _targets; }
    const Formation& targets() const { return _targets; }
    void addTarget(Singular& target);
    void clearTargets();

    RelationManager& provenance() { return _provenance; }
    const RelationManager& provenance() const { return _provenance; }
    std::shared_ptr<Relation> recordProvenance(const std::string& type,
                                               const Singular& a,
                                               const Singular& b,
                                               bool directed = true,
                                               float weight = 1.0f);

    // First-mover lane: register a hard-coded condition/action closure directly.
    // Person-authored laws will instead carry ConditionModel/ActionModel trees
    // that compile into these same slots (Stage 2).
    void addCondition(const std::string& description,
                      ConditionPredicate predicate,
                      bool required = true);
    void clearConditions();
    bool conditionsSatisfied(const Singular& target) const;

    void addAction(const std::string& description, ECA::ActionExecutor action);
    void clearActions();

    // ------------------------------------------------------------------
    // The law's text (Stage 2): serializable condition/action models.
    // When present, recompile() derives the executable ECA slots from them —
    // the tree is primary, the closure is derived. This is what lets a
    // Person-authored law survive save/load, be synthesized, and be governed.
    // ------------------------------------------------------------------
    bool hasConditionModel() const { return _conditionModel.has_value(); }
    bool hasActionModel() const { return _actionModel.has_value(); }
    // How many compiled condition predicates this law carries. With an
    // authored model behind them they are derived from it and introspectable;
    // WITHOUT one they came from addCondition() as arbitrary closures, and a
    // closure cannot be read. The Prophetic index asks this to know whether it
    // may claim to have seen everything this law reads.
    std::size_t conditionPredicateCount() const { return _conditionPredicates.size(); }
    const ConditionModel* conditionModel() const {
        return _conditionModel ? &*_conditionModel : nullptr;
    }
    const ActionModel* actionModel() const {
        return _actionModel ? &*_actionModel : nullptr;
    }
    // ------------------------------------------------------------------
    // What a subject must HAVE for this law to be about it.
    //
    // "Apply to everyone" was never the intent — a law that writes
    // `position.y` has nothing to say to a Relation, a Zone, or another Law,
    // and sweeping them cost a full condition evaluation, an application
    // record, and an audit line each, per being, per tick. The honest scope
    // is "everything WITH THIS PROPERTY": the law's own text already names
    // the vocabulary it needs, so the sweep reads that vocabulary off the
    // text and never visits a being that could not possibly satisfy it.
    //
    // These are ROOT names (the first segment of each authored path), which
    // is the granularity a registry lookup answers in one step. Paths rooted
    // on someone else (@being-id, @event.*) and the reserved time paths are
    // excluded: they say nothing about the subject. A law whose every path
    // is qualified that way has no requirements and sweeps everything, which
    // is correct — it really is about all of them.
    // ------------------------------------------------------------------
    const std::vector<std::string>& requiredProperties() const { return _requiredProperties; }
    const std::vector<ConditionPredicate>& compiledGates() const { return _compiledGates; }
    bool couldApplyTo(Singular& being) const;

    void setConditionModel(ConditionModel model);
    void setActionModel(ActionModel model);
    void clearConditionModel() {
        _conditionModel.reset();
        _conditionPredicates.clear();
        ++_conditionRevision;
        bumpTextRevision();
    }

    // Bumped by every change to the condition tree. The compiled Rete
    // terminals are DERIVED from that tree, and only two paths ever built
    // them (registration and world load) — so a law edited in the graph
    // window kept firing on its old conditions through the network while
    // conditionsSatisfied() answered with the new ones. The two evaluation
    // paths disagreed by construction. LawManager watches this number and
    // recompiles, so nobody has to remember to ask.
    std::uint64_t conditionRevision() const { return _conditionRevision; }

    // ------------------------------------------------------------------
    // "The law text changed, SOMEWHERE." One counter for the whole register,
    // bumped by every edit to any law's condition or action model and by
    // every law entering or leaving the register.
    //
    // conditionRevision() is per-law and per-condition, which is right for
    // recompiling one law's Rete terminals. The Prophetic index is a
    // conclusion about the register as a WHOLE, and it is consulted from the
    // property-change callback — a path that runs between ticks and cannot
    // afford to walk every law to ask whether it is still current. Comparing
    // one integer is what lets that path fail open the instant anything moves.
    // ------------------------------------------------------------------
    static std::uint64_t textRevision() { return s_textRevision; }
    static void bumpTextRevision() { ++s_textRevision; }
    void clearActionModel() {
        _actionModel.reset();
        _actions.clear();
        bumpTextRevision();
    }
    void recompile();

    // Restore identity + behavior from toJson() output. Authors and targets
    // are world references (identities) — the loader reattaches them; a law
    // without authors stays Unauthored and cannot fire.
    static std::shared_ptr<Law> fromJson(const nlohmann::json& j);

    // Event-typed carrier for this law's ECA loop (which event kind wakes it).
    // Wired to the bus/Rete loop in Stage 3.
    const ECA::Loop& ecaLoop() const { return _ecaLoop; }
    ECA::Loop& ecaLoop() { return _ecaLoop; }

    // Create algorithm to use ordinary tools to create conditions (projection
    // mode — the shape tools emit ConditionNodes instead of world objects).

    ApplicationResult applyTo(Singular& target);
    std::vector<ApplicationRecord> applyToTargets();

    const std::vector<ApplicationRecord>& applicationLog() const { return _applicationLog; }
    nlohmann::json toJson() const;

    static const char* resultName(ApplicationResult result);

protected:
    // Subclasses that override buildProperties replace the registry. Call this
    // so `@<id>.enabled` still resolves — otherwise the Law Graph checkbox
    // writes Law::_enabled and law-text cannot see it. First-mover channels
    // (Creation, Locomotion) must call this; their actuation also reads
    // isEnabled() so setting the first mover down actually stops it.
    void registerEnabledProperty();

private:
    void initializeLawIdentity();
    // Stable, law-derived names for the three group Formations (see the .cpp).
    void nameGroupFormations();
    ApplicationRecord makeRecord(Singular* target, ApplicationResult result) const;
    void publishAppliedEvent(Singular* target, ApplicationResult result) const;

    // Metalaws need zero new machinery: registering the law's own governable
    // state (enabled / conditionMode / name) makes a Law a legible Singular —
    // a law whose TARGET is another law simply IS a metalaw. (Overrides
    // Object::buildProperties: laws are extra-spatial, so the spatial
    // properties are not registered.) Bridges adapt to Property signatures.
    void buildProperties() override;
    void rebuildRequiredProperties();
    bool propEnabled() const { return _enabled; }
    void propSetEnabled(const bool& v) { _enabled = v; }
    int propConditionMode() const { return static_cast<int>(_conditionMode); }
    void propSetConditionMode(const int& v) {
        _conditionMode = v == 1 ? ConditionMode::Any : ConditionMode::All;
    }
    std::string propName() const { return _name; }
    void propSetName(const std::string& v) { setName(v); }
    bool propDrives() const { return _drives; }
    void propSetDrives(const bool& v) { _drives = v; }

    std::string _lawId;
    std::string _name;
    bool _enabled = true;
    int _authorityLevel = 0;
    Activation _activation = Activation::OnEvent;
    Scope _scope = Scope::Subject;
    std::shared_ptr<Zone> _jurisdiction;
    bool _drives = false;
    Retrigger _retrigger = Retrigger::Absorb;
    std::unordered_map<const Singular*, bool> _conditionMemory;   // edge detection
    std::unordered_map<const Singular*, double> _onsetMemory;     // t=0 per subject
    std::uint64_t _conditionRevision{0};                      // see conditionRevision()
    static std::uint64_t s_textRevision;                      // see textRevision()
    ConditionMode _conditionMode = ConditionMode::All;


    Formation _authors;
    Formation _conditions;
    Formation _targets;
    RelationManager _provenance;

    ECA::Loop _ecaLoop;
    std::optional<ConditionModel> _conditionModel;
    std::optional<ActionModel> _actionModel;
    std::vector<Condition> _conditionPredicates;
    std::vector<Action> _actions;
    std::vector<ApplicationRecord> _applicationLog;
    std::vector<std::string> _requiredProperties;   // derived at recompile()
    std::vector<ConditionPredicate> _compiledGates;
};

struct LawRegisteredEvent {
    std::shared_ptr<const Law> law;
    std::time_t timestamp{0};
};

struct ReteFact {
    std::string id;
    std::string type;
    std::string subjectId;
    std::string attribute;
    nlohmann::json value;
    Singular* subject{nullptr};
    Singular* object{nullptr};   // the event's OTHER participant (collision has two)
    bool isState{false};
    bool dirty{true};
};
using FactPtr = std::shared_ptr<ReteFact>;

struct ReteToken {
    std::vector<FactPtr> facts;
    std::unordered_map<std::string, std::string> bindings;
};

struct ReteActivation {
    std::string lawId;
    ReteToken token;
    std::time_t timestamp{0};
};

class ReteNetwork {
public:
    using AlphaPredicate = std::function<bool(const FactPtr&)>;
    using BetaJoin = std::function<bool(const ReteToken&, const FactPtr&)>;

    // Where an alpha node's predicate came from. A predicate is a closure and
    // cannot be read back, so this records what is known about it AT THE
    // MOMENT IT IS MADE — the only moment anyone knows.
    //   Interned  the one-line "type == x" node internTypeAlpha builds. Its
    //             whole behaviour is the event type it names.
    //   Authored  compiled from a ConditionNode by compileToRete. Opaque as a
    //             closure, but its TEXT is right there in the law, and the
    //             Prophetic index has already read exactly what it matches on.
    //   Foreign   a hand-written closure (the graph editor, a test, a channel).
    //             Nothing can be known about what it reads.
    // Serialized nowhere — this is a fact about a runtime node, not law text.
    enum class AlphaSource { Interned, Authored, Foreign };

    struct AlphaNode {
        std::size_t id{0};
        std::string description;
        AlphaPredicate predicate;
        AlphaSource source{AlphaSource::Foreign};
        std::vector<FactPtr> memory;
    };

    struct BetaNode {
        std::size_t id{0};
        std::string description;
        bool leftIsBeta{false};
        std::size_t leftId{0};
        std::size_t rightAlphaId{0};
        BetaJoin join;
        std::vector<ReteToken> memory;
    };

    // State facts are inserted and persistent across ticks.
    std::string assertFact(FactPtr fact);
    bool retractFact(const std::string& factId);
    void retractStateFactsBySubject(const std::string& subjectId);
    // Returns whether any fact was actually marked. FALSE means the network
    // holds no state fact for that (subject, attribute) at all — which is not
    // "nothing changed", it is "the network has never heard of this property
    // on this being". seedStateFacts snapshots a being's properties ONCE per
    // being ever, so a property GRANTED at runtime had no fact to dirty and
    // never got one: a WhileTrue law reading it stayed permanently deaf to
    // that being, silently. Same family as the relation deafness rung 0 fixed.
    // The scan was already linear over the fact list, so the answer is free.
    bool markFactDirty(const std::string& subjectId, const std::string& attribute);
    void evaluateDirty();
    bool hasDirtyFacts() const { return !_dirtyFacts.empty(); }
    // Drop every fact naming this being. Called when it is actually freed:
    // facts hold raw participant pointers and outlive the round that
    // asserted them, so a fact about a dead being is a dangling read waiting
    // for the next tick.
    // Returns the subject identifiers whose state facts were dropped, so a
    // caller can forget it ever seeded them. Matching is by POINTER only:
    // this is called from ~Singular, where the being is no longer anything
    // more than a Singular and no virtual call on it is valid.
    std::vector<std::string> retractFactsAbout(const Singular* being);
    // Retract the oldest `count` facts — the consumption step of the tick
    // loop: facts asserted before a round are consumed by it, facts asserted
    // during it (laws firing events) survive into the next round.
    void retractFirst(std::size_t count);
    void clearFacts();
    const std::vector<FactPtr>& facts() const { return _facts; }

    // Is a live relation-state fact already keyed on this being and type?
    //
    // assertFact does NOT deduplicate — it pushes a fact and an id every call —
    // and three paths now assert edge facts (the first-tick seed, the
    // relation-formed handler, and the back-seed when a relation type first
    // enters play). Without this they stack duplicates into every alpha memory
    // that matches, which is a standing per-tick propagation tax and, over a
    // session of relations forming and dissolving, unbounded.
    //
    // Compares the SUBJECT POINTER, never dereferencing it: this is called on
    // paths where a relation's far endpoint may already be destroyed.
    bool hasRelationStateFact(const Singular* subject, const std::string& relationType) const;

    // `source` defaults to Foreign deliberately: a caller that has not said
    // where its predicate came from has not earned the assumption that it can
    // be reasoned about.
    std::size_t addAlphaNode(const std::string& description, AlphaPredicate predicate,
                             AlphaSource source = AlphaSource::Foreign);
    std::size_t addBetaNode(const std::string& description,
                            bool leftIsBeta,
                            std::size_t leftId,
                            std::size_t rightAlphaId,
                            BetaJoin join = {});

    // Binding is retroactive: facts already live become this law's backlog,
    // queued onto the agenda for the NEXT drain. Order of setup therefore does
    // not matter — a law bound after its events were published still hears
    // them, as it did back when the agenda was rebuilt from scratch each frame.
    // Only the newly bound law is queued, so laws already on the node do not
    // fire a second time.
    void bindLawToAlpha(const std::string& lawId, std::size_t alphaNodeId);
    void bindLawToBeta(const std::string& lawId, std::size_t betaNodeId);
    // Remove every binding of this law from the network. Nodes left with no
    // bindings are DROPPED, not kept: "an unbound node is inert" was wrong
    // about the cost — a node is predicate-tested against every fact before
    // anything looks at its bindings, so an accumulating pile of dead nodes is
    // a standing tax. Every trigger rebind, and every world load, used to add
    // another pile. (Propagation now skips unread nodes too, but dropping them
    // is still right: they also cost the scan in alphaFeedsAnyBeta and get
    // refilled on every bind.)
    void unbindLaw(const std::string& lawId);
    void unbindLawFromAlpha(const std::string& lawId, std::size_t alphaNodeId);
    // Drop any activations already queued for a law, so unbinding actually
    // silences it instead of leaving a backlog to fire on the next tick.
    void purgeAgendaOf(const std::string& lawId);

    // One alpha node per event type, shared by every law that listens for it.
    // Fifty laws on "collision" is one predicate over the fact stream, not
    // fifty identical ones.
    std::size_t internTypeAlpha(const std::string& eventType);

    // ------------------------------------------------------------------
    // One node per DISTINCT authored condition, shared by every law that
    // states it — the same bargain internTypeAlpha already makes for event
    // types, extended to the path every authored leaf actually takes.
    //
    // Measured in the saved worlds: 70% of all condition leaves are textual
    // duplicates (791 of 1124). chess_app.json states
    // `instance-of category.chess.piece` in 76 separate laws and
    // `onBoard == true` in 74. Each compiled its own node, and a node is not
    // just a closure — it keeps a vector of every fact it has matched, and
    // assertFact runs EVERY node's predicate against EVERY fact. So the
    // duplication was paid twice: once in memory, and once per fact assertion
    // in time.
    //
    // THE KEY IS THE WHOLE SERIALIZED LEAF, and that is not incidental.
    // An analysis proposed keying on (path, op, const), which is incomplete —
    // it omits operandPath, tolerance, lo/hi, relationType, otherId, probe,
    // region and beingKind. Two different conditions sharing a key would bind
    // one law to another's predicate, and if that predicate is the stricter of
    // the two the law is NARROWED — silently deaf, the failure
    // PROPHETIC_RETE.md §2 exists to forbid. The saves already contain the
    // collision: chess states both `chessColor == <literal>` and
    // `chessColor == @state.chess.turn`, identical under (path, op, const) and
    // emphatically not the same condition.
    //
    // toJson() is complete by construction — it is the serialization contract,
    // so two leaves that serialize identically ARE the same condition — and it
    // fails safe: a kind that ever serialized incompletely would break saving
    // long before it broke sharing.
    //
    // Only AUTHORED leaves. A Foreign closure cannot be reasoned about at all,
    // and Interned type nodes already share through their own index.
    std::size_t internAuthoredAlpha(const std::string& conditionKey,
                                    const std::string& description,
                                    AlphaPredicate predicate);
    std::size_t alphaNodeCount() const { return _alphaNodes.size(); }
    std::size_t betaNodeCount() const { return _betaNodes.size(); }

    // How many fact references all node memories are holding right now.
    //
    // The number the α-node-sharing question actually turns on. Node COUNT is
    // cheap — a node is a closure and a description; what costs is that every
    // bound node keeps a vector of every fact it has matched, so two laws with
    // the same condition text keep two identical copies of the same match set.
    //
    // Exposed because it was not: alphaNodeCount(), PropheticCounters and
    // TickTiming all existed with ZERO call sites anywhere in src/ — the same
    // shape as Universe::structuralRevision(), which sat wrong and unnoticed
    // until something finally read it. A counter nobody reads cannot be
    // observed to be wrong.
    std::size_t nodeMemoryFootprint() const {
        std::size_t total = 0;
        for (const auto& alpha : _alphaNodes) total += alpha.memory.size();
        for (const auto& beta : _betaNodes) total += beta.memory.size();
        return total;
    }

    // ------------------------------------------------------------------
    // "Would anything act on a fact of this type?" — asked before an event
    // is published, so an echo nobody hears costs nothing.
    //
    // Interned type nodes answer exactly. Nodes added through addAlphaNode
    // carry an ARBITRARY predicate closure, which cannot be introspected —
    // so if any such node is bound, the honest answer is "possibly", and we
    // publish. Guessing "no" about a predicate we cannot read would silently
    // drop events a law really was listening for.
    // ------------------------------------------------------------------
    bool hearsType(const std::string& eventType) const;
    bool hasOpaqueBoundAlpha() const;

    // Is any node bound to a law reading through a predicate NOBODY can
    // account for — a Foreign closure? hasOpaqueBoundAlpha() answers the
    // event-interest question and counts every non-interned node, authored
    // conditions included; this asks the narrower question the Prophetic
    // index needs, because an authored condition's reads ARE enumerable (they
    // are the law's own text) and treating them as unknowable would switch
    // the possibility-space filter off in every world that has laws in it.
    bool hasForeignBoundAlpha() const;

    // There is no evaluation phase. Propagation happens as it arrives —
    // assertFact queues activations for the facts it matches, and the bind
    // paths backfill from the live fact list — so the agenda is complete at
    // every instant and there is never anything to bring up to date.
    //
    // (An evaluate() lived here through the incremental rewrite, having become
    // a no-op returning _agenda. It was removed rather than renamed: agenda()
    // already says exactly that, and a second name for one accessor is how the
    // ordering bug in bindLawToAlpha stayed invisible — callers read the name
    // as "recompute now" and assumed setup order did not matter.)
    //
    // agenda() peeks; drainAgenda() takes and clears. Conflict resolution —
    // ordering the pending set by recency/specificity/salience before firing —
    // is the one piece a classic Rete has that this does not, and it belongs
    // in drainAgenda when it lands: it orders what is pending, it does not
    // compute what is pending.
    std::vector<ReteActivation> drainAgenda();
    const std::vector<ReteActivation>& agenda() const { return _agenda; }

    // Collect all unique subjects that currently match a set of terminal nodes.
    // Used by WhileTrue laws to replace sweepSubjects with O(Matching) iteration.
    std::vector<Singular*> collectTerminalSubjects(
        const std::vector<std::size_t>& terminalIds) const;

    // True when the given ID belongs to an alpha node (not beta).
    bool isAlphaNode(std::size_t id) const { return findAlpha(id) != nullptr; }

    nlohmann::json toJson() const;

private:
    const AlphaNode* findAlpha(std::size_t id) const;
    AlphaNode* findAlpha(std::size_t id);

    // ------------------------------------------------------------------
    // Backfill. Propagation is incremental — assertFact maintains the
    // memories — and it SKIPS any alpha node that no law is bound to and no
    // beta reads, because predicate-testing a node nothing looks at is pure
    // waste. That skip is only safe if a node can be brought current the
    // moment something does start reading it, which is what these do.
    //
    // A node's memory is a pure function of (its predicate, _facts), so
    // rebuilding it by rescanning _facts is always correct and idempotent —
    // and it preserves assertion order, so a refilled memory is
    // indistinguishable from one built fact by fact. Without this, a law
    // bound after a fact was asserted stayed deaf to it forever while
    // facts() still reported the fact as live.
    // ------------------------------------------------------------------
    void refillAlphaMemory(AlphaNode& alpha);
    void refillBetaMemory(BetaNode& beta);
    // The skip condition in assertFact, named once so backfill and
    // propagation cannot drift apart.
    bool alphaFeedsAnyBeta(std::size_t alphaId) const;
    bool alphaIsRead(std::size_t alphaId) const;

    // Token shapes, shared by incremental propagation and backfill so the two
    // produce byte-identical memories and agendas.
    static ReteToken alphaToken(const FactPtr& fact);
    static ReteToken joinSeed(const FactPtr& left);
    static ReteToken joinedToken(const FactPtr& left, const FactPtr& right);
    static ReteToken joinedToken(const ReteToken& left, const FactPtr& right);

    std::vector<FactPtr> _facts;
    // Every being that has ever been a fact's subject or object, so
    // retractFactsAbout can answer "this one has no facts" in O(1) instead of
    // scanning the table. It fires on EVERY Singular destructor — including
    // the `Moment` inside every transient `ECA::Event` — so that scan was the
    // engine's real quadratic. A SUPERSET on purpose: see retractFactsAbout.
    std::unordered_set<const Singular*> _factParticipants;
    // Which relation types each being already has a relation-state fact for.
    //
    // DERIVED STATE, and its invalidation is declared here because the last
    // time it was not, the answer was a linear scan that turned seeding into
    // O(beings x relations x facts):
    //   depends on  : the relation-state facts in _facts
    //   maintained  : assertFact inserts; retractFactsAbout erases that being;
    //                 every bulk retraction path clears outright
    //   rebuilt     : never — maintained, not rebuilt
    //
    // It may UNDER-report ("no fact" when there is one), which costs one
    // duplicate fact and nothing else. It must never OVER-report: that would
    // skip asserting a fact that does not exist, and a missing relation-state
    // fact is a deaf `Related` law — the defect rung 0 exists to fix. That
    // asymmetry is why the bulk paths clear rather than try to be precise.
    std::unordered_map<const Singular*, std::unordered_set<std::string>> _relationStateIndex;
    std::vector<FactPtr> _dirtyFacts;
    std::vector<AlphaNode> _alphaNodes;
    std::vector<BetaNode> _betaNodes;
    std::vector<ReteActivation> _agenda;
    void dropUnboundAlphaNodes();

    std::unordered_map<std::size_t, std::vector<std::string>> _alphaLawBindings;
    std::unordered_map<std::size_t, std::vector<std::string>> _betaLawBindings;
    std::unordered_map<std::string, std::size_t> _typeAlphaIndex;   // event type -> node
    // serialized condition leaf -> node. Entries may go stale when
    // dropUnboundAlphaNodes removes a node nobody reads; node ids are never
    // reused, so a stale entry resolves to nothing and is simply rebuilt.
    std::unordered_map<std::string, std::size_t> _authoredAlphaIndex;
    // ONE counter for both tables. Alpha and beta ids are handed to callers as
    // bare `std::size_t` and are told apart afterwards by isAlphaNode(), which
    // answers by looking the id up in the alpha table — so two independent
    // counters made "beta 1" indistinguishable from "alpha 1", and since beta
    // ids run behind alpha ids in every real compile, every beta terminal was
    // misread as the alpha of the same number. `All(a<1, b<1)` bound the law to
    // its FIRST clause alone. Sharing the counter is what makes the id itself
    // carry the distinction the lookup was already assuming it had.
    std::size_t _nextNodeId{1};
};

class FirstMoverLaw : public Law {
public:
    FirstMoverLaw(const std::string& name) : Law(name) {
        setLawIdentifier(name);
    }
    bool isFirstMover() const override { return true; }
};

class LawManager {
public:
    std::shared_ptr<Law> createLaw(const std::string& name,
                                   const std::vector<Singular*>& authors = {});
    void add(const std::shared_ptr<Law>& law);
    bool remove(const std::string& lawId);

    Law* find(const std::string& lawId) const;
    std::vector<std::shared_ptr<Law>> getByAuthor(const std::string& authorId) const;
    const std::vector<std::shared_ptr<Law>>& getAll() const { return _laws; }

    Formation& formation() { return _lawFormation; }
    const Formation& formation() const { return _lawFormation; }

    std::vector<Law::ApplicationRecord> applyAllTo(Singular& target);
    std::vector<Law::ApplicationRecord> applyAllToTargets();

    ReteNetwork& rete() { return _rete; }
    const ReteNetwork& rete() const { return _rete; }

    // ------------------------------------------------------------------
    // Prophetic Rete (B-Time Rete) — what the authored law set makes
    // POSSIBLE, computed from the law text before anything fires. Rebuilt
    // only when that text changes; see PropheticRete.hpp for the analysis and
    // `docs/architecture/law/PROPHETIC_RETE.md` for what it is for.
    // ------------------------------------------------------------------
    const Prophetic::Index& prophetic() const { return _prophetic; }
    // Bring the index up to date with the law register if the text moved.
    // Called at the top of tick(); safe (and cheap) to call at any time.
    void syncProphetic();

    // Pass 1/2 of the four-pass model, asked on the hot path: could a change
    // to a property of this name reach ANY authored condition?
    //
    // FAILS OPEN, three ways: a stale index, an incomplete index, or an
    // opaque alpha node in the network all answer true. The only "no" this
    // returns is one the abstract interpretation proved, and a wrong "no"
    // here is a law that silently stops hearing — so the gate is written to
    // make that the hard case to reach, not the easy one.
    bool propheticHears(const std::string& propertyName) const;

    // How many property-change notifications the filter has answered, and how
    // many of those it ruled irrelevant. Read by the perf window and by the
    // test that guards the filter; reset with resetPropheticCounters().
    struct PropheticCounters {
        std::uint64_t asked = 0;
        std::uint64_t filtered = 0;
    };
    const PropheticCounters& propheticCounters() const { return _propheticCounters; }
    void resetPropheticCounters() { _propheticCounters = {}; }
    // (evaluateRete() lived here, wrapping the removed ReteNetwork::evaluate().
    // It had no callers. Reach through rete().agenda() for a peek.)

    // ------------------------------------------------------------------
    // Hearing (Stage 3): laws listen. connectToEventBus() turns every
    // published ECA::Event into a ReteFact; tick() — once per frame —
    // evaluates the network and drains the agenda into applyTo. Event facts
    // are transient: consumed by the round that evaluates them.
    //
    // NOTE: the EventBus has no unsubscribe, so a connected LawManager must
    // outlive all publishing (engine-lifetime object). Handlers run on the
    // publishing thread; keep publishing on the main thread for now.
    // ------------------------------------------------------------------
    void connectToEventBus();
    bool isConnected() const { return _connected; }
    // Releases the static Singular hooks this manager installed, if it is the
    // one that installed them. Without this, a block-scoped LawManager dies
    // before the beings declared above it and their destructors call back into
    // freed memory — the ordinary shape of every test in this tree.
    ~LawManager();
    std::vector<Law::ApplicationRecord> tick();

    // Per-frame timing breakdown, written by tick(), read by the perf window.
    struct TickTiming {
        float syncMs    = 0.0f;   // syncReteCompilation over all laws
        float seedMs    = 0.0f;   // seedStateFacts over all beings
        float evalMs    = 0.0f;   // Rete evaluation + agenda drain + continuous sweep
        float driveMs   = 0.0f;   // runDriveSessions
        float reapMs    = 0.0f;   // reapUnmade
        float totalMs   = 0.0f;   // wall time of the entire tick()
    };
    const TickTiming& lastTickTiming() const { return _tickTiming; }

    // ------------------------------------------------------------------
    // Triggers — the serializable truth of WHAT WAKES each law. The Rete
    // alpha bindings are derived from this map (closures cannot serialize);
    // bind/unbind through here so saved worlds keep their laws listening.
    // ------------------------------------------------------------------
    void bindTrigger(const std::string& lawId, const std::string& eventType);
    void unbindTrigger(const std::string& lawId, const std::string& eventType);
    const std::vector<std::string>& triggersOf(const std::string& lawId) const;

    // Restore a saved register: replaces the current laws, reattaches
    // authors/targets as world references BY IDENTIFIER (authorship is a
    // covenant, not a copy — an author absent from the world leaves the law
    // Unauthored and it cannot fire), and rebinds triggers. Call after the
    // world is loaded so the Universe can resolve the identifiers.
    void loadFromJson(const nlohmann::json& j);

    // ------------------------------------------------------------------
    // Drive sessions: change over time that outlives its event. When an
    // OnEvent law whose action reads "time.sinceApplied" applies, a session
    // begins; every tick re-applies the law to that subject with the
    // session's onset as t=0, until the authored Piecewise bounds no longer
    // contain t (the bounds ARE the duration) — then "law-drive-finished"
    // is published. Runtime state only; never serialized.
    // ------------------------------------------------------------------
    struct DriveSession {
        std::string lawId;
        std::string subjectId;
        double onset = 0.0;
        // The launching event's participants, so "@event.subject" /
        // "@event.object" stay addressable for the drive's whole life
        // (resolved fresh each tick; a participant that left the world
        // resolves to nothing).
        std::string eventSubjectId;
        std::string eventObjectId;
    };
    const std::vector<DriveSession>& driveSessions() const { return _driveSessions; }
    bool hasDriveSession(const std::string& lawId, const std::string& subjectId) const {
        for (const auto& session : _driveSessions) {
            if (session.lawId == lawId && session.subjectId == subjectId) return true;
        }
        return false;
    }

    // Bounded law chaining per tick: a law firing an event that wakes another
    // law resolves within the same tick, but never unboundedly.
    // The ceiling is now authorable rather than hardcoded.
    int maxChainRounds() const { return _maxChainRounds; }
    void setMaxChainRounds(int rounds) { _maxChainRounds = rounds; }

    nlohmann::json toJson() const;

    // Compile a law's condition model into Rete nodes and track terminals.
    void compileConditionsToRete(Law& law);

private:
    void maybeStartDriveSession(Law& law, Singular& subject);
    void restartDriveSession(Law& law, const std::string& subjectId);
    void runDriveSessions(std::vector<Law::ApplicationRecord>& records);
    // Apply, record, and start a drive session only if the law CHANGED
    // something (not merely if the action branch was reached).
    void applyAndMaybeDrive(Law& law, Singular& subject,
                            std::vector<Law::ApplicationRecord>& records);
    // Whom an untargeted law sweeps: the beings carrying its vocabulary.
    std::vector<Singular*> sweepSubjects(const Law& law) const;

    // ------------------------------------------------------------------
    // The vocabulary index — FORMATION_RETE.md §3.0, §8 rung 2.
    //
    // sweepSubjects used to answer "whom is this law about" by rebuilding
    // Universe::beings() — the provider allocates a fresh vector every call —
    // and testing couldApplyTo against every being, once per sweeping law per
    // tick. Measured at 1000 beings and 8 laws where only 8 beings could ever
    // match: 7.6 ms/tick, fitting k = 0.82 against POPULATION with the
    // matching set held constant. The sweep cost the whole world to find eight
    // beings; ~99% of it could not have matched.
    //
    // §3.0 names what that filter really is: "a degenerate, implicit,
    // unauthored category". This is that category made explicit — one entry per
    // property NAME some law requires, holding the beings that carry it. Only
    // names laws actually ask for are indexed, which is Magic Sets in miniature
    // (§4C): the goal restricts what the engine bothers to know.
    //
    // NOT yet a Formation, and that is deliberate rather than unfinished.
    // Reifying it as a rooted Category Formation is the other half of rung 2
    // and it is blocked: per Zach's revised definition (§3.4) a purely
    // branching taxonomy is not a Formation at all, and closing the loop needs
    // concept-Singulars. Formation::addMember also walks the relation graph per
    // member, which a per-structural-change rebuild cannot afford.
    //
    // Refusal 6: this is Kernel-tier DERIVED state — a pure function of
    // (the world, the law set), reconstructible at any moment, holding no truth
    // of its own — and it is named here rather than merely omitted.
    // ------------------------------------------------------------------
    // const, and the state below is mutable, ON PURPOSE. sweepSubjects is
    // const and calls this itself, so the index cannot be read stale by a
    // caller that forgot to refresh first — and a stale index does not merely
    // give an old answer, it returns {} for a name it has not indexed yet,
    // which is a law reaching nobody. Correctness must not depend on call
    // order; tick() still calls it once up front so the per-law call is an
    // integer compare rather than N passes over the world.
    void refreshVocabularyIndex() const;

    // Do this law's subject-independent gates hold right now?
    //
    // False means the law's candidate set is EMPTY this tick, whatever the
    // population — so the subject loop can be skipped entirely instead of
    // walking every being to be refused by each one. See
    // ConditionNode::isHoistableGate for what qualifies and what deliberately
    // does not.
    //
    // Returns true whenever it cannot prove otherwise: no gates, an unreadable
    // referent, or an action that could move the gate mid-sweep. Widening is
    // the safe direction; a wrong `false` here silences a law completely.
    bool gatesHold(const Law& law) const;
    // Members are RAW pointers, so this must be rebuilt whenever the world's
    // shape changes. Universe::structuralRevision() is that signal, and
    // Zone::removeObject had to be taught to bump it before this could be
    // trusted — see the comment there.
    mutable std::unordered_map<std::string, std::vector<Singular*>> _vocabularyIndex;
    mutable std::unordered_set<std::string> _indexedNames;
    // Deliberately a sentinel no real revision can equal, so the first tick
    // always builds rather than trusting an empty index.
    mutable uint64_t _vocabularyBuiltAt = std::numeric_limits<uint64_t>::max();
    // End-of-tick unmaking, once no pointer to a victim is still live.
    void reapUnmade();
    void releaseFromLaws(Singular* being);

    std::vector<std::shared_ptr<Law>> _laws;
    Formation _lawFormation;
    ReteNetwork _rete;
    std::vector<DriveSession> _driveSessions;
    std::unordered_map<std::string, std::vector<std::string>> _triggers;
    // Terminal node IDs for each law's compiled condition DAG.
    // Key: lawId, Value: {nodeId, isBeta} pairs for the DAG's terminal nodes.
    struct TerminalInfo {
        std::size_t nodeId;
        bool isBeta;  // true = BetaNode, false = AlphaNode
    };
    std::unordered_map<std::string, std::vector<TerminalInfo>> _reteTerminals;
    // The Law::conditionRevision() each entry in _reteTerminals was built
    // from. Compiled terminals are derived state; this is what lets the tick
    // notice they have gone stale rather than trusting that whoever edited
    // the condition remembered to say so.
    std::unordered_map<std::string, std::uint64_t> _compiledConditionRevision;
    // Rebuild this law's terminals if its condition changed (or drop them if
    // it no longer wants any). Cheap when nothing moved: one map lookup.
    void syncReteCompilation(Law& law);

    // ------------------------------------------------------------------
    // Give the network the `property-state` facts for a being it has not met.
    //
    // Alpha predicates compiled from conditions all begin `if (!fact->isState)
    // return false;` — state facts are the ONLY thing a condition can match.
    // They used to be asserted from exactly one place: the "object-created"
    // echo, whose sole publisher is an ActionNode Create/Spawn. So a being
    // that entered the world any other way — a loaded save, the object tool,
    // a Universe provider — was invisible to every compiled condition, and a
    // WhileTrue law loaded from disk matched nobody while the identical law
    // authored in-session worked. Same law text, two behaviors, decided by
    // where the being came from.
    //
    // Seeding is once per being: the property-change callback installed by
    // connectToEventBus() is what keeps the facts current afterwards.
    // ------------------------------------------------------------------
    void seedStateFacts(Singular* being);
    // One edge fact, asserted WITHOUT consulting _seededSubjects.
    //
    // That set guards the property snapshot above, which is genuinely
    // once-per-being. Edge seeding was made to share the gate and should never
    // have: a being's properties are a snapshot, its edges are a stream. Once
    // the being was known, every relation formed afterwards was silently
    // dropped — FORMATION_RETE.md §1.2(a), the defect that made every
    // continuous `Related` law permanently deaf. Two concerns, now separated.
    //
    // `endpoint` is the being the fact is keyed on; the far end is never
    // dereferenced (see seedStateFacts for why that matters).
    void assertRelationStateFact(Singular* endpoint, const std::string& relationType);
    // Catch up the edges that already existed when a relation type first
    // enters the seeding vocabulary — the second shape of the same defect.
    void backSeedRelationStateFacts(const std::unordered_set<std::string>& types);
    std::unordered_set<std::string> _seededSubjects;
    // Relation types any registered law's condition names. Maintained by
    // compileConditionsToRete; see seedStateFacts for why the narrowing is
    // sound and why it is worth doing.
    std::unordered_set<std::string> _relationTypesInPlay;
    bool _connected = false;
    bool _dirty = false;
    TickTiming _tickTiming;

    Prophetic::Index _prophetic;
    // The Law::textRevision() the index was built from. Deliberately started
    // one BELOW any real revision so a manager that has never synced reads as
    // stale and fails open.
    std::uint64_t _propheticRevision = static_cast<std::uint64_t>(-1);
    mutable PropheticCounters _propheticCounters;
    int _maxChainRounds = 5;

    Earthcall::Execution::ExecutionChannel _executionChannel;
};
