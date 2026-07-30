#pragma once

#include "Form/Singular/Property/PropertyPath.hpp"
#include "Form/Singular/Singular.hpp"
#include "Universe.hpp"
#include "json.hpp"

#include <iostream>
#include <map>
#include <optional>
#include <string>

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
//                               the subject is
//   @event.subject.position.y   the triggering event's subject
//   @event.object.position.y    the triggering event's OTHER participant
//                               (a collision has two)
// The event roots resolve through the application-event context the
// LawManager arms while laws respond to an event; outside an event response
// they are undefined — a condition never passes and an action never writes
// on an unproven referent.
// ---------------------------------------------------------------------------
inline Singular* resolveLawRoot(Singular& subject, const PropertyPath& path,
                                PropertyPath& remainder) {
    if (path.segments.empty() || path.segments[0].empty() ||
        path.segments[0][0] != '@') {
        remainder = path;
        return &subject;
    }
    if (path.segments[0] == "@event" && path.segments.size() >= 2) {
        remainder.segments.assign(path.segments.begin() + 2, path.segments.end());
        if (!Universe::instance().hasApplicationEvent()) return nullptr;
        if (path.segments[1] == "subject") {
            return Universe::instance().applicationEventSubject();
        }
        if (path.segments[1] == "object") {
            return Universe::instance().applicationEventObject();
        }
        return nullptr;
    }
    const std::string beingId = path.segments[0].substr(1);
    remainder.segments.assign(path.segments.begin() + 1, path.segments.end());
    for (Singular* being : Universe::instance().beings()) {
        if (being && being->getIdentifier() == beingId) return being;
    }
    return nullptr;   // the named being is not in the world: no value
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

inline bool lawGetValue(Singular& subject, const PropertyPath& path, PropertyValue& out) {
    if (isTimePath(path)) return lawGetTime(path, out);
    PropertyPath remainder;
    Singular* root = resolveLawRoot(subject, path, remainder);
    return root && (remainder.getValue(*root, out) == PropertyPath::PathResult::Ok);
}

inline PropertyPath::PathResult lawSetValue(Singular& subject, const PropertyPath& path, const PropertyValue& v) {
    if (isTimePath(path)) return PropertyPath::PathResult::ReadOnly;
    PropertyPath remainder;
    Singular* root = resolveLawRoot(subject, path, remainder);
    if (!root) return PropertyPath::PathResult::NoSuchProperty;
    return remainder.setValue(*root, v);
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
