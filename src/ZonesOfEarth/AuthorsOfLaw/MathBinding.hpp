#pragma once

#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "Singularity/Core/StringId.hpp"
#include <unordered_map>
#include "ConstructedBeing/Singular/Singular.hpp"
#include "Universe.hpp"
#include "json.hpp"

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
// Reading is strict: if any bound path fails to resolve to a number, the
// whole read fails — a law must never evaluate mathematics on missing values.
using MathBindings = std::map<std::string, PropertyPath>;

// ---------------------------------------------------------------------------
// Qualified paths: WHOSE property a path names is the author's choice.
//   position.y                  the law's subject (whoever it applies to)
//   @being-id.position.y        that NAMED being (Universe lookup), whoever
//                               the subject is. The id may contain dots
//                               ("@material.clay.baseColor"): the root is
//                               matched LONGEST-FIRST, most specific wins.
//   @event.subject.position.y   the triggering event's subject
//   @event.object.position.y    the triggering event's OTHER participant
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
        startIndex = 2;
        if (!Universe::instance().hasApplicationEvent()) return nullptr;
        if (path.segments[1] == "subject") {
            return Universe::instance().applicationEventSubject();
        }
        if (path.segments[1] == "object") {
            return Universe::instance().applicationEventObject();
        }
        return nullptr;
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
    const std::vector<Singular*> beings = Universe::instance().beings();
    std::string candidate = path.segments[0].substr(1);
    Singular* best = nullptr;
    std::size_t bestConsumed = 0;
    for (std::size_t n = 1; n <= path.segments.size(); ++n) {
        if (n > 1) candidate += "." + path.segments[n - 1];
        for (Singular* being : beings) {
            if (being && being->getIdentifier() == candidate) {
                best = being;
                bestConsumed = n;
                break;
            }
        }
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

inline bool lawGetValue(Singular& subject, const PropertyPath& path, PropertyValue& out) {
    if (isTimePath(path)) return lawGetTime(path, out);
    if (isWorldReadingPath(path)) {
        const auto& readings = worldReadings();
        if (readings.empty()) return false;          // no channel answers "@world.*"
        const auto found = readings.find(path.fullId());
        if (found == readings.end() || !found->second) return false;
        return found->second(subject, out);
    }
    std::size_t startIndex = 0;
    Singular* root = resolveLawRoot(subject, path, startIndex);
    return root && (path.getValue(*root, out, startIndex) == PropertyPath::PathResult::Ok);
}

inline PropertyPath::PathResult lawSetValue(Singular& subject, const PropertyPath& path, const PropertyValue& v) {
    if (isTimePath(path)) return PropertyPath::PathResult::ReadOnly;
    // A world reading is an observation, not a dial: the world is not written
    // by asserting a measurement of it.
    if (isWorldReadingPath(path) && worldReadings().count(path.fullId())) {
        return PropertyPath::PathResult::ReadOnly;
    }
    std::size_t startIndex = 0;
    Singular* root = resolveLawRoot(subject, path, startIndex);
    if (!root) return PropertyPath::PathResult::NoSuchProperty;
    return path.setValue(*root, v, startIndex);
}

inline std::optional<std::map<std::string, double>> readMathBindings(
    Singular& subject, const MathBindings& bindings) {
    std::map<std::string, double> vars;
    for (const auto& entry : bindings) {
        PropertyValue value;
        double x = 0.0;
        if (!lawGetValue(subject, entry.second, value) || !propertyValueToNumber(value, x)) {
            return std::nullopt;
        }
        vars[entry.first] = x;
    }
    return vars;
}

inline nlohmann::json mathBindingsToJson(const MathBindings& bindings) {
    nlohmann::json j = nlohmann::json::object();
    for (const auto& entry : bindings) j[entry.first] = entry.second.toString();
    return j;
}

inline MathBindings mathBindingsFromJson(const nlohmann::json& j) {
    MathBindings bindings;
    for (auto it = j.begin(); it != j.end(); ++it) {
        bindings[it.key()] = PropertyPath::parse(it.value().get<std::string>());
    }
    return bindings;
}
