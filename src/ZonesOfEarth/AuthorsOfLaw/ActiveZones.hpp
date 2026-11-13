#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class Singular;
class Relation;
class Timeline;
class Event;
class Zone;

// The law engine's working set — not a being, not the vessel (Ourverse),
// not the womb that receives newborns (Zone). Continuous laws and quantified
// conditions (ForAny / ForAll) range over the beings this yields.
//
// ActiveZones manages first-class multi-Zone operands (_zones), where _zones[0]
// is the primaryZone() in front of the Person, plus any concurrently held or
// mounted capability Zones.
//
// Do not make this a Singular. Do not register @active-zones or @universe.
// Kernel is not a being (WORLD_UNIVERSE_REFUSALS_AUDIT_2026-08-20).
class ActiveZones {
public:
    static ActiveZones& instance() {
        static ActiveZones activeZones;
        return activeZones;
    }

    // ------------------------------------------------------------------
    // Multi-Zone operand management
    // ------------------------------------------------------------------
    const std::vector<Zone*>& zones() const noexcept { return _zones; }
    Zone* primaryZone() const noexcept { return _zones.empty() ? nullptr : _zones.front(); }
    void setPrimaryZone(Zone* zone);
    void setActiveZones(std::vector<Zone*> zones);
    bool activateZone(Zone* zone);
    bool deactivateZone(Zone* zone);
    void clearActiveZones();
    bool isActive(const Zone* zone) const noexcept;

    // ------------------------------------------------------------------
    // The working set provider (shared registers: LawManager, MaterialManager,
    // CategoryManager, TransferPolicy, Person, WorldTimeline, inactive Zone headers).
    // ------------------------------------------------------------------
    using Provider = std::function<void(std::vector<Singular*>&)>;

    void setProvider(Provider provider) {
        _provider = std::move(provider);
        bumpStructuralRevision();
    }
    void setSharedProvider(Provider provider) {
        setProvider(std::move(provider));
    }
    bool hasProvider() const { return static_cast<bool>(_provider); }

    // Native multi-zone operand harvesting + shared provider beings
    std::vector<Singular*> beings() const;

    // ------------------------------------------------------------------
    // Who is listening. A law publishing an event that no law hears is a
    // fact asserted into the network for nothing.
    // ------------------------------------------------------------------
    using EventInterest = std::function<bool(const std::string&)>;
    void setEventInterest(EventInterest interest) { _eventInterest = std::move(interest); }
    bool anyoneHears(const std::string& eventType) const {
        return !_eventInterest || _eventInterest(eventType);
    }

    // ------------------------------------------------------------------
    // Multi-Zone relation graph aggregation
    // ------------------------------------------------------------------
    using RelationProvider = std::function<void(std::vector<Relation*>&)>;
    void setRelationProvider(RelationProvider provider) {
        _relationProvider = std::move(provider);
        _relationsInvolvingProvider = nullptr;
    }

    using RelationGenerationProvider = std::function<std::size_t()>;
    void setRelationGenerationProvider(RelationGenerationProvider provider) {
        _relationGenerationProvider = std::move(provider);
    }
    std::size_t relationGeneration() const;
    bool hasRelationGeneration() const;

    using RelationsInvolvingProvider =
        std::function<void(const Singular& being, std::vector<Relation*>& out)>;
    void setRelationsInvolvingProvider(RelationsInvolvingProvider provider) {
        _relationsInvolvingProvider = std::move(provider);
    }
    bool relationsInvolving(const Singular& being, std::vector<Relation*>& out) const;
    std::vector<Relation*> relations() const;

    bool isRelationSuperseded(const Relation& r) const;
    bool isRelationCurrent(const Relation& r) const;

    using RelationRegistrar = std::function<void(std::shared_ptr<Relation>)>;
    void setRelationRegistrar(RelationRegistrar registrar) {
        _relationRegistrar = std::move(registrar);
    }
    bool hasRelationRegistrar() const;
    void addRelation(std::shared_ptr<Relation> relation);

    // ------------------------------------------------------------------
    // Temporal authority.
    // ------------------------------------------------------------------
    void setTimeline(Timeline* timeline);
    Timeline* timeline() const { return _timeline; }
    void setClock(double now, double dt);
    bool hasClock() const;
    double now() const;
    double dt() const;

    void setApplicationOnset(double onset) {
        _onset = onset;
        _onsetSet = true;
    }
    void clearApplicationOnset() { _onsetSet = false; }
    bool hasApplicationOnset() const { return _onsetSet; }
    double applicationOnset() const { return _onset; }

    class OnsetScope {
    public:
        explicit OnsetScope(double onset) {
            ActiveZones& u = ActiveZones::instance();
            _hadOnset = u._onsetSet;
            _savedOnset = u._onset;
            u.setApplicationOnset(onset);
        }
        ~OnsetScope() {
            ActiveZones& u = ActiveZones::instance();
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
        EventScope(Singular* subject, Singular* object,
                   const Event* occurrence = nullptr) {
            ActiveZones& u = ActiveZones::instance();
            _hadEvent = u._eventSet;
            _savedSubject = u._eventSubject;
            _savedObject = u._eventObject;
            _savedOccurrence = u._eventOccurrence;
            u.setApplicationEvent(subject, object, occurrence);
        }
        ~EventScope() {
            ActiveZones& u = ActiveZones::instance();
            u._eventSubject = _savedSubject;
            u._eventObject = _savedObject;
            u._eventOccurrence = _savedOccurrence;
            u._eventSet = _hadEvent;
        }
        EventScope(const EventScope&) = delete;
        EventScope& operator=(const EventScope&) = delete;

    private:
        bool _hadEvent = false;
        Singular* _savedSubject = nullptr;
        Singular* _savedObject = nullptr;
        const Event* _savedOccurrence = nullptr;
    };

    void setApplicationEvent(Singular* subject, Singular* object,
                             const Event* occurrence = nullptr) {
        _eventSubject = subject;
        _eventObject = object;
        _eventOccurrence = occurrence;
        _eventSet = true;
    }
    void clearApplicationEvent() {
        _eventSubject = nullptr;
        _eventObject = nullptr;
        _eventOccurrence = nullptr;
        _eventSet = false;
    }
    bool hasApplicationEvent() const { return _eventSet; }
    Singular* applicationEventSubject() const { return _eventSubject; }
    Singular* applicationEventObject() const { return _eventObject; }
    const Event* applicationEvent() const { return _eventOccurrence; }

    // ------------------------------------------------------------------
    // Unmaking is DEFERRED.
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

    uint64_t structuralRevision() const { return _structuralRevision; }
    void bumpStructuralRevision() { ++_structuralRevision; }

private:
    ActiveZones() = default;
    ActiveZones(const ActiveZones&) = delete;
    ActiveZones& operator=(const ActiveZones&) = delete;

    std::vector<Zone*> _zones;

    Provider _provider;
    RelationProvider _relationProvider;
    RelationsInvolvingProvider _relationsInvolvingProvider;
    RelationGenerationProvider _relationGenerationProvider;
    RelationRegistrar _relationRegistrar;
    EventInterest _eventInterest;

    Timeline* _timeline = nullptr;
    double _now = 0.0;
    double _dt = 0.0;
    bool _clockSet = false;

    double _onset = 0.0;
    bool _onsetSet = false;

    Singular* _eventSubject = nullptr;
    Singular* _eventObject = nullptr;
    const Event* _eventOccurrence = nullptr;
    bool _eventSet = false;

    std::vector<Singular*> _unmaking;
    uint64_t _structuralRevision = 0;
};

void reapUnmadeBeings();
