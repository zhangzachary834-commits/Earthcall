#pragma once

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class Singular;
class Relation;

// The law engine's working set — not a being, not the vessel (Ourverse),
// not the womb that receives newborns (Zone). Continuous laws and quantified
// conditions (ForAny / ForAll) range over the beings this singleton yields.
//
// The ECA loop is edge-triggered: conditions are checked at the discrete
// moments events fire. But some laws are level-triggered — their condition
// phase must monitor the program at all times ("whenever y sinks below the
// ground", with no event announcing it), and quantified conditions ("if ANY
// object...", "ALL instances except...") need a domain to range over. Both
// need the same thing: the set of beings currently present.
//
// The engine supplies the provider (the active Zone's objects, the
// registered laws, the player); tests supply their own. No provider = empty
// domain: continuous untargeted laws watch nothing, ForAny is false,
// ForAll is vacuously true.
//
// Do not make this a Singular. Do not register @universe. Kernel is not a
// being (WORLD_UNIVERSE_REFUSALS_AUDIT_2026-08-20).
class Universe {
public:
    static Universe& instance() {
        static Universe universe;
        return universe;
    }

    using Provider = std::function<void(std::vector<Singular*>&)>;

    void setProvider(Provider provider) { _provider = std::move(provider); }
    bool hasProvider() const { return static_cast<bool>(_provider); }

    std::vector<Singular*> beings() const {
        std::vector<Singular*> out;
        if (_provider) _provider(out);
        if (!_unmaking.empty()) {
            // A being awaiting the reaper is already gone as far as law is
            // concerned: it must not be swept, quantified over, or addressed
            // by name in the window between the asking and the reaping.
            out.erase(std::remove_if(out.begin(), out.end(),
                                     [this](Singular* being) { return isUnmade(being); }),
                      out.end());
        }
        return out;
    }

    // ------------------------------------------------------------------
    // Who is listening. A law publishing an event that no law hears is a
    // fact asserted into the network for nothing — and because asserting
    // marks the network dirty, it costs a whole extra evaluation round per
    // tick. The LawManager owns the trigger table, so it answers this; with
    // no answerer we assume someone might be listening and publish.
    // ------------------------------------------------------------------
    using EventInterest = std::function<bool(const std::string&)>;
    void setEventInterest(EventInterest interest) { _eventInterest = std::move(interest); }
    bool anyoneHears(const std::string& eventType) const {
        return !_eventInterest || _eventInterest(eventType);
    }

    // ------------------------------------------------------------------
    // The relation graph. Relations are Singulars — discrete beings — but
    // laws also need the EDGE view: "is x related to y (in type t)?".
    // The engine supplies the provider (the active zone's Formation
    // relations); Related conditions query it. No provider = no proven
    // relations: a Related condition never passes.
    // ------------------------------------------------------------------
    using RelationProvider = std::function<void(std::vector<Relation*>&)>;

    void setRelationProvider(RelationProvider provider) {
        _relationProvider = std::move(provider);
    }
    std::vector<Relation*> relations() const {
        std::vector<Relation*> out;
        if (_relationProvider) _relationProvider(out);
        return out;
    }

    // Registering a NEW relation into the world's graph — the write side.
    // The engine wires this to the active zone's Formation; concept
    // instantiation uses it so a captured set's inter-member relations are
    // reborn between the newborns. No registrar = the relation is not made
    // (structure is never silently dropped somewhere unfindable).
    using RelationRegistrar = std::function<void(std::shared_ptr<Relation>)>;

    void setRelationRegistrar(RelationRegistrar registrar) {
        _relationRegistrar = std::move(registrar);
    }
    bool hasRelationRegistrar() const { return static_cast<bool>(_relationRegistrar); }
    void addRelation(std::shared_ptr<Relation> relation) {
        if (_relationRegistrar && relation) _relationRegistrar(std::move(relation));
    }

    // ------------------------------------------------------------------
    // The world clock. Singularity owns time: the engine sets it once per
    // frame (accumulated seconds since the world began + that frame's dt),
    // tests set it by hand, and laws read it through the reserved paths
    // "time" / "time.delta" / "time.sinceApplied" (MathBinding.hpp). No law
    // writes time.
    // ------------------------------------------------------------------
    void setClock(double now, double dt) {
        _now = now;
        _dt = dt;
        _clockSet = true;
    }
    bool hasClock() const { return _clockSet; }
    double now() const { return _now; }
    double dt() const { return _dt; }

    // The application context: while a law's actions execute, this holds the
    // world time at which that law began holding for the current subject, so
    // "time.sinceApplied" (= now - onset) resolves inside compiled closures.
    // Law::applyTo sets and clears it (RAII); outside an application the
    // path is undefined — a law never reads another law's onset.
    void setApplicationOnset(double onset) {
        _onset = onset;
        _onsetSet = true;
    }
    void clearApplicationOnset() { _onsetSet = false; }
    bool hasApplicationOnset() const { return _onsetSet; }
    double applicationOnset() const { return _onset; }

    // RESTORING scopes, not clearing ones. A nested arming (a pair quantifier
    // borrowing the event vocabulary, a law applied from inside another law's
    // action) must hand back exactly what it found — a guard that CLEARS makes
    // the outer context vanish mid-application, and "time.sinceApplied" or
    // "@event.subject" silently becomes undefined for the rest of the outer
    // law's actions. Save/restore is the only shape that composes.
    class OnsetScope {
    public:
        explicit OnsetScope(double onset) {
            Universe& u = Universe::instance();
            _hadOnset = u._onsetSet;
            _savedOnset = u._onset;
            u.setApplicationOnset(onset);
        }
        ~OnsetScope() {
            Universe& u = Universe::instance();
            u._onset = _savedOnset;
            u._onsetSet = _hadOnset;
        }
        OnsetScope(const OnsetScope&) = delete;
        OnsetScope& operator=(const OnsetScope&) = delete;

    private:
        bool _hadOnset = false;
        double _savedOnset = 0.0;
    };

    class EventScope {
    public:
        EventScope(Singular* subject, Singular* object) {
            Universe& u = Universe::instance();
            _hadEvent = u._eventSet;
            _savedSubject = u._eventSubject;
            _savedObject = u._eventObject;
            u.setApplicationEvent(subject, object);
        }
        ~EventScope() {
            Universe& u = Universe::instance();
            u._eventSubject = _savedSubject;
            u._eventObject = _savedObject;
            u._eventSet = _hadEvent;
        }
        EventScope(const EventScope&) = delete;
        EventScope& operator=(const EventScope&) = delete;

    private:
        bool _hadEvent = false;
        Singular* _savedSubject = nullptr;
        Singular* _savedObject = nullptr;
    };

    // The event context: while laws respond to an event, this holds the
    // event's PARTICIPANTS so the reserved roots "@event.subject" and
    // "@event.object" resolve — the action phase names its own referents,
    // and the beings involved in the triggering event are available BY
    // CHOICE (a collision has two participants; the law may act on either,
    // both, or someone else entirely). LawManager arms this around event
    // rounds; outside an event response the roots are undefined.
    void setApplicationEvent(Singular* subject, Singular* object) {
        _eventSubject = subject;
        _eventObject = object;
        _eventSet = true;
    }
    void clearApplicationEvent() {
        _eventSubject = nullptr;
        _eventObject = nullptr;
        _eventSet = false;
    }
    bool hasApplicationEvent() const { return _eventSet; }
    Singular* applicationEventSubject() const { return _eventSubject; }
    Singular* applicationEventObject() const { return _eventObject; }

    // ------------------------------------------------------------------
    // Unmaking is DEFERRED. A law that destroys a being does so in the
    // middle of its own application: the action returns into applyTo, which
    // still has to write the record, log the outcome, and publish the
    // applied event — all through a reference to the being just freed. And
    // the sweeps above hold snapshot vectors of raw pointers that a
    // mid-sweep free turns into a minefield.
    //
    // So Destroy does not destroy. It ASKS, and the LawManager reaps once
    // the tick's every pass is done and no pointer to the victim is still
    // live. Between the asking and the reaping the being is `unmade`: still
    // in memory, but skipped by every sweep, so no law acts on a corpse.
    // ------------------------------------------------------------------
    void requestUnmaking(Singular* being) {
        if (!being) return;
        for (Singular* pending : _unmaking) {
            if (pending == being) return;
        }
        _unmaking.push_back(being);
    }
    bool isUnmade(Singular* being) const {
        for (Singular* pending : _unmaking) {
            if (pending == being) return true;
        }
        return false;
    }
    bool hasUnmakings() const { return !_unmaking.empty(); }
    const std::vector<Singular*>& unmakings() const { return _unmaking; }
    std::vector<Singular*> takeUnmakings() {
        std::vector<Singular*> out;
        out.swap(_unmaking);
        return out;
    }

private:
    Universe() = default;
    Universe(const Universe&) = delete;
    Universe& operator=(const Universe&) = delete;

    Provider _provider;
    RelationProvider _relationProvider;
    RelationRegistrar _relationRegistrar;
    EventInterest _eventInterest;

    double _now = 0.0;
    double _dt = 0.0;
    bool _clockSet = false;

    double _onset = 0.0;
    bool _onsetSet = false;

    Singular* _eventSubject = nullptr;
    Singular* _eventObject = nullptr;
    bool _eventSet = false;

    std::vector<Singular*> _unmaking;
};

// Free every being whose unmaking has been requested, releasing it from the
// Zone that owns it (which announces "object-destroyed" while it still
// exists, so listeners can read who it was).
//
// LawManager::tick() calls this once its every pass is done. Anyone who fires
// a compiled Destroy node OUTSIDE a tick — a tool, a test — owns the same
// responsibility: until this runs, the being is hidden from Universe::beings()
// but still allocated.
void reapUnmadeBeings();
