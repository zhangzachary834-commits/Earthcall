#pragma once

#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "ConstructedBeing/Singular/Property/PropertyCell.hpp"
#include "Singularity/Core/StringId.hpp"
#include "Singularity/TransferPolicy.hpp"
#include <unordered_map>
#include <unordered_set>
#include "ConstructedBeing/Singular/Singular.hpp"
#include "Universe.hpp"
#include "Time/Event/Event.hpp"
#include "json.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"

#include <functional>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <vector>

// The bridge between authored mathematics and the substrate: a binding names
// each free variable of an expression and says WHERE on the subject its value
// lives ("x" -> position.x). Variables are Person-authored primitives like
// everything else in a law's text; this is what makes an OntoMath expression
// legible against real beings.
//
// Reading is strict and preserves the PropertyValue alternative. Each math
// operation decides which types it admits; an identity variable need not
// widen an integer or copy the storage behind a shared container.

enum class BindingMode : uint8_t {
    FollowPath = 0,
    ShareCell  = 1,
    UniqueCell = 2,
    ObserveWeak= 3,
    CopyValue  = 4,
    Derive     = 5
};

inline const char* bindingModeToString(BindingMode mode) {
    switch (mode) {
        case BindingMode::ShareCell:   return "share";
        case BindingMode::UniqueCell:  return "unique";
        case BindingMode::ObserveWeak: return "weak";
        case BindingMode::CopyValue:   return "copy";
        case BindingMode::Derive:      return "derive";
        default:                       return "follow";
    }
}

inline BindingMode bindingModeFromString(const std::string& str) {
    if (str == "share") return BindingMode::ShareCell;
    if (str == "unique") return BindingMode::UniqueCell;
    if (str == "weak") return BindingMode::ObserveWeak;
    if (str == "copy") return BindingMode::CopyValue;
    if (str == "derive") return BindingMode::Derive;
    return BindingMode::FollowPath;
}

struct MathBindingEntry {
    PropertyPath path;
    BindingMode mode = BindingMode::FollowPath;
    mutable PropertyPath::CompiledPath compiledPath;

    MathBindingEntry() = default;
    MathBindingEntry(const PropertyPath& p, BindingMode m = BindingMode::FollowPath)
        : path(p), mode(m), compiledPath(path.compile()) {}
    MathBindingEntry(PropertyPath&& p, BindingMode m = BindingMode::FollowPath)
        : path(std::move(p)), mode(m), compiledPath(path.compile()) {}

    const PropertyPath::CompiledPath& getCompiled() const {
        if (compiledPath.path.fullId() != path.fullId()) {
            compiledPath = path.compile();
        }
        return compiledPath;
    }

    operator const PropertyPath&() const noexcept { return path; }
    operator PropertyPath&() noexcept { return path; }

    const PropertyPath* operator->() const noexcept { return &path; }
    PropertyPath* operator->() noexcept { return &path; }

    std::string toString() const { return path.toString(); }
    bool empty() const { return path.empty(); }
};

using MathBindings = std::map<std::string, MathBindingEntry>;

// ---------------------------------------------------------------------------
// Qualified paths: WHOSE property a path names is the author's choice.
//   position.y                  the law's subject (whoever it applies to)
//   @being-id.position.y        currently resolves by getIdentifier() text in
//                               the live Universe, whoever the subject is.
//                               This is not yet a durable ID-pinned binding.
//                               The text may contain dots
//                               ("@material.clay.baseColor"): the root is
//                               matched LONGEST-FIRST, most specific wins.
//   @event.verb                 the triggering Event Moment's own property
//   @event.start                its temporal coordinate
//   @event.subject.position.y   legacy first-participant path
//   @event.object.position.y    legacy second-participant path
//                               (a collision has two)
//   @world.<reading>            a WORLD READING about the subject, answered
//                               by whichever modality channel registered it
//                               (see registerWorldReading below). Read-only,
//                               and intercepted in lawGetValue before root
//                               resolution — there is no being called
//                               "world" to look up.
// The event roots resolve through the application-event context the
// LawManager arms while laws respond to an event; outside an event response
// they are undefined — a condition never passes and an action never writes
// on an unproven referent.
// ---------------------------------------------------------------------------
inline Singular* resolveLawRoot(Singular& subject, const PropertyPath& path,
                                std::size_t& startIndex) {
    if (path.segments.empty() || path.segments[0].empty() ||
        path.segments[0][0] != '@') {
        startIndex = 0;
        return &subject;
    }
    if (path.segments[0] == "@event" && path.segments.size() >= 2) {
        if (!Universe::instance().hasApplicationEvent()) return nullptr;
        if (path.segments[1] == "subject" || path.segments[1] == "object") {
            startIndex = 2;
            return path.segments[1] == "subject"
                ? Universe::instance().applicationEventSubject()
                : Universe::instance().applicationEventObject();
        }
        startIndex = 1;
        // A Rete fact owns this occurrence for the duration of application.
        // PropertyPath's read API takes a mutable Singular, but the write
        // bridge below refuses mutation of the historical Event snapshot.
        return const_cast<Event*>(Universe::instance().applicationEvent());
    }
    // A being's identifier may itself contain dots: Material namespaces itself
    // as "material.<name>" so it cannot collide with an Object in the same path
    // space, and authored categories follow it with "category.<name>". Taking
    // only the first segment would make every such being unaddressable —
    // @material.clay.baseColor would look for a being called "material".
    //
    // So root resolution does what PropertyPath::resolve already does one level
    // down: LONGEST dotted-name match first, most specific wins. A being named
    // "material.clay" beats one named "material", and the segments it consumed
    // are not offered to the property lookup.
    
    // HOT PATH CACHE: avoid O(N^2) string comparisons and massive vector allocations.
    // The active namespace follows the complete structural fallback. Loaded
    // inactive Zones are deliberately absent: a qualified lookup reaches only
    // the named Zone's retained referent table, so one local edit never sweeps
    // every hydrated Zone. Identifiers are stable authored slugs; changing one
    // after admission must travel through a structural/referent mutation door.
    static uint64_t s_lastRevision = 0;
    static std::size_t s_lastActiveReferentCurrency = 0;
    static ZoneManager* s_lastLiveManager = nullptr;
    static std::unordered_map<Earthcall::StringId, Singular*> s_beingMap;
    static bool s_initialized = false;

    struct ZoneReferentCache {
        uint64_t revision = 0;
        const geom::FieldNode* identityWitness = nullptr;
        std::string zoneIdentifier;
        std::unordered_map<Earthcall::StringId, Singular*> qualified;
        std::unordered_map<Earthcall::StringId, Singular*> unqualified;
        bool initialized = false;
    };
    static std::unordered_map<const Zone*, ZoneReferentCache> s_zoneReferents;

    auto zoneReferents = [&](Zone& zone) -> ZoneReferentCache& {
        ZoneReferentCache& cache = s_zoneReferents[&zone];
        const uint64_t revision = zone.referentRevision();
        const auto* witness = zone.spatialRoot();
        const std::string zoneIdent = zone.getIdentifier();
        if (cache.initialized && cache.revision == revision &&
            cache.identityWitness == witness && cache.zoneIdentifier == zoneIdent) {
            return cache;
        }

        cache.qualified.clear();
        cache.unqualified.clear();
        auto bindIfUnique = [](auto& table, Earthcall::StringId name, Singular* being) {
            auto [it, inserted] = table.emplace(name, being);
            if (!inserted && it->second != being) it->second = nullptr;
        };
        const std::string zonePrefix = "@" + zoneIdent + ".";
        std::unordered_set<Singular*> seen;
        auto add = [&](Singular* being) {
            if (!being || !seen.insert(being).second) return;
            const std::string ident = being->getIdentifier();
            if (ident.empty()) return;
            bindIfUnique(cache.qualified,
                         Earthcall::StringInterner::intern(zonePrefix + ident), being);
            bindIfUnique(cache.unqualified,
                         Earthcall::StringInterner::intern("@" + ident), being);
        };
        add(zone.spatialRoot());
        for (const auto& field : zone.additionalSpatialFields()) add(field.get());
        for (const auto& object : zone.getOwnedObjects()) add(object.get());
        for (Singular* member : zone.formation().getMembers()) add(member);
        for (const auto& stored : zone.storedSingulars()) add(stored.get());

        cache.revision = revision;
        cache.identityWitness = witness;
        cache.zoneIdentifier = zoneIdent;
        cache.initialized = true;
        return cache;
    };
    
    uint64_t currentRevision = Universe::instance().structuralRevision();
    std::size_t activeReferentCurrency = 0;
    auto mixActiveReferent = [&activeReferentCurrency](std::size_t value) {
        activeReferentCurrency ^= value + 0x9e3779b97f4a7c15ULL +
                                   (activeReferentCurrency << 6) +
                                   (activeReferentCurrency >> 2);
    };
    for (const Zone* zone : Universe::instance().zones()) {
        if (!zone) continue;
        mixActiveReferent(reinterpret_cast<std::size_t>(zone));
        mixActiveReferent(static_cast<std::size_t>(zone->referentRevision()));
    }
    ZoneManager* currentLiveManager = ZoneManager::live();
    if (!s_initialized || s_lastRevision != currentRevision ||
        s_lastActiveReferentCurrency != activeReferentCurrency ||
        s_lastLiveManager != currentLiveManager) {
        s_beingMap.clear();
        const auto& beings = Universe::instance().beings();
        auto bindIfUnique = [&](Earthcall::StringId name, Singular* being) {
            auto [it, inserted] = s_beingMap.emplace(name, being);
            if (!inserted && it->second != being) it->second = nullptr;
        };

        // 1. Existing pass over ActiveZones::instance().beings() that binds @<ident> via bindIfUnique
        for (Singular* being : beings) {
            if (being) {
                const std::string ident = being->getIdentifier();
                if (!ident.empty()) {
                    Earthcall::StringId key = Earthcall::StringInterner::intern("@" + ident);
                    bindIfUnique(key, being);
                    if (ident == "Person" || ident == "person") {
                        bindIfUnique(Earthcall::StringInterner::intern("@player"), being);
                        bindIfUnique(Earthcall::StringInterner::intern("@Player"), being);
                    }
                }
            }
        }

        // 2. Zone-qualified keys for ACTIVE operands only. Inactive loaded
        // Zones are resolved lazily below through their own cache.
        for (Zone* z : Universe::instance().zones()) {
            if (!z) continue;
            ZoneReferentCache& cache = zoneReferents(*z);
            for (const auto& [key, being] : cache.qualified) {
                // A null value is an ambiguity proof inside that Zone. Keep
                // it null in the aggregate namespace; skipping it here would
                // turn two equal authored identifiers into load-order choice.
                bindIfUnique(key, being);
            }
        }

        s_lastRevision = currentRevision;
        s_lastActiveReferentCurrency = activeReferentCurrency;
        s_lastLiveManager = currentLiveManager;
        s_initialized = true;
    }

    Singular* best = nullptr;
    std::size_t bestConsumed = 0;
    bool activeNamespaceAnswered = false;
    const auto& jIds = path.joinedIds();
    if (!jIds.empty() && !jIds[0].empty()) {
        const auto& idsFromHere = jIds[0];
        for (std::size_t n = 1; n <= idsFromHere.size(); ++n) {
            auto it = s_beingMap.find(idsFromHere[n - 1]);
            if (it != s_beingMap.end()) {
                best = it->second;
                bestConsumed = n;
                activeNamespaceAnswered = true;
            }
        }
    }

    // Lazy loaded-Zone lookup. A qualified root reaches only that Zone. An
    // unqualified inactive fallback is accepted only when exactly one loaded
    // inactive Zone answers; ambiguity refuses instead of depending on load
    // order. Active ambiguity already answered above and may not be shadowed.
    if (!activeNamespaceAnswered && currentLiveManager && !jIds.empty() && !jIds[0].empty()) {
        const auto& idsFromHere = jIds[0];
        Zone* namedZone = nullptr;
        std::size_t zoneConsumed = 0;
        for (const auto& zonePtr : currentLiveManager->zones()) {
            if (!zonePtr) continue;
            const auto zoneKey = Earthcall::StringInterner::intern(
                "@" + zonePtr->getIdentifier());
            for (std::size_t n = 1; n <= idsFromHere.size(); ++n) {
                if (idsFromHere[n - 1] == zoneKey && n >= zoneConsumed) {
                    namedZone = zonePtr.get();
                    zoneConsumed = n;
                }
            }
        }

        if (namedZone) {
            best = namedZone;
            bestConsumed = zoneConsumed;
            ZoneReferentCache& cache = zoneReferents(*namedZone);
            for (std::size_t n = zoneConsumed + 1; n <= idsFromHere.size(); ++n) {
                auto it = cache.qualified.find(idsFromHere[n - 1]);
                if (it != cache.qualified.end()) {
                    best = it->second;
                    bestConsumed = n;
                }
            }
        } else {
            Singular* inactiveBest = nullptr;
            std::size_t inactiveConsumed = 0;
            bool ambiguous = false;
            for (const auto& zonePtr : currentLiveManager->zones()) {
                if (!zonePtr || Universe::instance().isActive(zonePtr.get())) continue;
                ZoneReferentCache& cache = zoneReferents(*zonePtr);
                for (std::size_t n = 1; n <= idsFromHere.size(); ++n) {
                    auto it = cache.unqualified.find(idsFromHere[n - 1]);
                    if (it == cache.unqualified.end() || !it->second) continue;
                    if (n > inactiveConsumed) {
                        inactiveBest = it->second;
                        inactiveConsumed = n;
                        ambiguous = false;
                    } else if (n == inactiveConsumed && inactiveBest != it->second) {
                        ambiguous = true;
                    }
                }
            }
            if (!ambiguous && inactiveBest) {
                best = inactiveBest;
                bestConsumed = inactiveConsumed;
            }
        }
    }
    
    if (!best && !path.segments.empty() && path.segments[0] == "@transfer-policy") {
        best = &TransferPolicy::instance();
        bestConsumed = 1;
    }

    if (!best) return nullptr;   // the named being is not in the world: no value
    startIndex = bestConsumed;
    return best;
}

// ---------------------------------------------------------------------------
// Reserved time paths — the world clock made legible. These resolve on the
// Universe (Singularity owns time), not on any being, and are read-only:
//   time              seconds since the world began
//   time.delta        the last frame's dt
//   time.sinceApplied seconds since THIS law began holding for THIS subject
//                     (defined only inside a law application; see
//                     Universe::setApplicationOnset)
// This is what lets an authored OntoMath model be a function OF TIME: bind
// t -> time.sinceApplied and position.y := f(t) is change over time.
// ---------------------------------------------------------------------------
inline bool lawGetTime(const PropertyPath& path, PropertyValue& out) {
    if (path.segments.empty() || path.segments[0] != "time") return false;
    const Universe& u = Universe::instance();
    if (!u.hasClock()) return false;
    if (path.segments.size() == 1) {
        out = PropertyValue(u.now());
        return true;
    }
    if (path.segments.size() == 2 && path.segments[1] == "delta") {
        out = PropertyValue(u.dt());
        return true;
    }
    if (path.segments.size() == 2 && path.segments[1] == "sinceApplied") {
        if (!u.hasApplicationOnset()) return false;
        out = PropertyValue(u.now() - u.applicationOnset());
        return true;
    }
    return false;
}

inline bool isTimePath(const PropertyPath& path) {
    return !path.segments.empty() && path.segments[0] == "time";
}

// ---------------------------------------------------------------------------
// World readings — the "@world.*" referent.
//
// A reading is a NAMED, read-only quantity about the subject's SITUATION that
// no property on the subject holds: whether sound from it reaches the
// listener, how much light falls on it. Written as ordinary law text:
//
//   @world.occlusionToCamera     the world, asked about this subject
//
// It joins the qualified-referent vocabulary above (@being-id, @event.subject,
// @event.object) rather than opening a bare `world.` root, for two reasons.
// It reads correctly — a reading is genuinely ABOUT the subject but OWNED by
// the world, which is what the @ prefix has always marked. And
// `Law::rebuildRequiredProperties` deliberately excludes @-rooted paths from
// the vocabulary a sweep filters on ("qualified roots address someone else"),
// which is exactly right here: a bare `world.` root would make every law that
// reads one require a property literally named `world`, and `couldApplyTo`
// would then filter every being in the world out of that law's sweep.
//
// A MODALITY CHANNEL registers what it can answer; the law engine knows only
// that such readings exist and never what any of them MEANS. This matters:
// `lawGetValue` is the funnel every binding read and every Compare condition
// passes through, and it briefly carried a hardcoded branch for
// "world.occlusionToCamera" — a raycast, a camera lookup, and an
// `#include "Singularity/Foreign/API/EarthcallAPI.hpp"` that dragged the foreign-software
// surface into ActionModel, ConditionModel, Universe and the whole set-to-set
// path. A subsystem may not define what a thing is, and the hottest path in
// the engine may not know what occlusion is.
//
// Registration is idempotent-by-replacement and expected once, at channel
// init. Nothing is registered by default: an unregistered reading simply does
// not read, and a law that binds it does not fire — undefined, never guessed.
// ---------------------------------------------------------------------------
using WorldReading = std::function<bool(Singular& subject, PropertyValue& out)>;

inline std::unordered_map<Earthcall::StringId, WorldReading>& worldReadings() {
    static std::unordered_map<Earthcall::StringId, WorldReading> readings;
    return readings;
}

// `dottedName` is the full path INCLUDING the "@world." referent
// (e.g. "@world.occlusionToCamera") — the name a Person writes.
inline void registerWorldReading(const std::string& dottedName, WorldReading reading) {
    if (dottedName.rfind("@world.", 0) != 0) return;   // the referent is reserved
    Earthcall::StringId id = Earthcall::StringInterner::intern(dottedName);
    if (!reading) {
        worldReadings().erase(id);
        return;
    }
    worldReadings()[id] = std::move(reading);
}

inline bool isWorldReadingPath(const PropertyPath& path) {
    return path.segments.size() >= 2 && path.segments[0] == "@world";
}

void resolveSemanticTokenSlowPath(Singular* root, PropertyValue& out);

inline PropertyPath::PathResult lawGetValueResult(Singular& subject, const PropertyPath& path, PropertyValue& out, const AccessContext& ctx = {}) {
    if (isTimePath(path)) {
        return lawGetTime(path, out) ? PropertyPath::PathResult::Ok : PropertyPath::PathResult::NoSuchProperty;
    }
    if (isWorldReadingPath(path)) {
        const auto& readings = worldReadings();
        if (readings.empty()) return PropertyPath::PathResult::NoSuchProperty;
        const auto found = readings.find(path.fullId());
        if (found == readings.end() || !found->second) return PropertyPath::PathResult::NoSuchProperty;
        return found->second(subject, out) ? PropertyPath::PathResult::Ok : PropertyPath::PathResult::NoSuchProperty;
    }
    std::size_t startIndex = 0;
    Singular* root = resolveLawRoot(subject, path, startIndex);
    if (!root) return PropertyPath::PathResult::NoSuchProperty;
    auto res = path.getValue(*root, out, startIndex, ctx);
    if (res == PropertyPath::PathResult::Ok && std::holds_alternative<std::shared_ptr<PropertyDict>>(out)) {
        resolveSemanticTokenSlowPath(root, out);
    }
    return res;
}

inline bool lawGetValue(Singular& subject, const PropertyPath& path, PropertyValue& out, const AccessContext& ctx = {}) {
    return lawGetValueResult(subject, path, out, ctx) == PropertyPath::PathResult::Ok;
}

inline PropertyPath::PathResult lawSetValue(Singular& subject, const PropertyPath& path, const PropertyValue& v, const AccessContext& ctx = {}) {
    if (isTimePath(path)) return PropertyPath::PathResult::ReadOnly;
    if (path.segments.size() >= 2 && path.segments[0] == "@event" &&
        path.segments[1] != "subject" && path.segments[1] != "object") {
        return PropertyPath::PathResult::ReadOnly;
    }
    // A world reading is an observation, not a dial: the world is not written
    // by asserting a measurement of it.
    if (isWorldReadingPath(path) && worldReadings().count(path.fullId())) {
        return PropertyPath::PathResult::ReadOnly;
    }
    std::size_t startIndex = 0;
    Singular* root = resolveLawRoot(subject, path, startIndex);
    if (!root) return PropertyPath::PathResult::NoSuchProperty;
    return path.setValue(*root, v, startIndex, ctx);
}

inline PropertyPath::PathResult lawGetValueResult(Singular& subject, const CompiledPath& compiled, PropertyValue& out, const AccessContext& ctx = {}) {
    if (isTimePath(compiled.path)) {
        return lawGetTime(compiled.path, out) ? PropertyPath::PathResult::Ok : PropertyPath::PathResult::NoSuchProperty;
    }
    if (isWorldReadingPath(compiled.path)) {
        const auto& readings = worldReadings();
        if (readings.empty()) return PropertyPath::PathResult::NoSuchProperty;
        const auto found = readings.find(compiled.path.fullId());
        if (found == readings.end() || !found->second) return PropertyPath::PathResult::NoSuchProperty;
        return found->second(subject, out) ? PropertyPath::PathResult::Ok : PropertyPath::PathResult::NoSuchProperty;
    }
    std::size_t startIndex = 0;
    Singular* root = resolveLawRoot(subject, compiled.path, startIndex);
    if (!root) return PropertyPath::PathResult::NoSuchProperty;
    auto res = compiled.getValue(*root, out, startIndex, ctx);
    if (res == PropertyPath::PathResult::Ok && std::holds_alternative<std::shared_ptr<PropertyDict>>(out)) {
        resolveSemanticTokenSlowPath(root, out);
    }
    return res;
}

inline bool lawGetValue(Singular& subject, const CompiledPath& compiled, PropertyValue& out, const AccessContext& ctx = {}) {
    return lawGetValueResult(subject, compiled, out, ctx) == PropertyPath::PathResult::Ok;
}

inline PropertyPath::PathResult lawSetValue(Singular& subject, const CompiledPath& compiled, const PropertyValue& v, const AccessContext& ctx = {}) {
    if (isTimePath(compiled.path)) return PropertyPath::PathResult::ReadOnly;
    if (compiled.path.segments.size() >= 2 && compiled.path.segments[0] == "@event" &&
        compiled.path.segments[1] != "subject" && compiled.path.segments[1] != "object") {
        return PropertyPath::PathResult::ReadOnly;
    }
    if (isWorldReadingPath(compiled.path) && worldReadings().count(compiled.path.fullId())) {
        return PropertyPath::PathResult::ReadOnly;
    }
    std::size_t startIndex = 0;
    Singular* root = resolveLawRoot(subject, compiled.path, startIndex);
    if (!root) return PropertyPath::PathResult::NoSuchProperty;
    return compiled.setValue(*root, v, startIndex, ctx);
}

inline std::optional<std::map<std::string, PropertyValue>> readMathBindings(
    Singular& subject,
    const MathBindings& bindings,
    const AccessContext& ctx = {},
    PropertyPath::PathResult* outFailureReason = nullptr) {
    std::map<std::string, PropertyValue> vars;
    for (const auto& entry : bindings) {
        PropertyValue value;
        const auto& cp = entry.second.getCompiled();
        PropertyPath::PathResult res = !cp.empty()
            ? lawGetValueResult(subject, cp, value, ctx)
            : lawGetValueResult(subject, entry.second.path, value, ctx);
        if (res != PropertyPath::PathResult::Ok) {
            if (outFailureReason) *outFailureReason = res;
            return std::nullopt;
        }
        vars[entry.first] = std::move(value);
    }
    if (outFailureReason) *outFailureReason = PropertyPath::PathResult::Ok;
    return vars;
}

inline nlohmann::json mathBindingsToJson(const MathBindings& bindings) {
    nlohmann::json j = nlohmann::json::object();
    for (const auto& entry : bindings) {
        if (entry.second.mode == BindingMode::FollowPath) {
            j[entry.first] = entry.second.path.toString();
        } else {
            j[entry.first] = nlohmann::json{
                {"path", entry.second.path.toString()},
                {"mode", bindingModeToString(entry.second.mode)}
            };
        }
    }
    return j;
}

inline MathBindings mathBindingsFromJson(const nlohmann::json& j) {
    MathBindings bindings;
    for (auto it = j.begin(); it != j.end(); ++it) {
        if (it.value().is_string()) {
            bindings[it.key()] = MathBindingEntry{PropertyPath::parse(it.value().get<std::string>()), BindingMode::FollowPath};
        } else if (it.value().is_object()) {
            std::string p = it.value().value("path", "");
            std::string m = it.value().value("mode", "follow");
            bindings[it.key()] = MathBindingEntry{PropertyPath::parse(p), bindingModeFromString(m)};
        }
    }
    return bindings;
}
