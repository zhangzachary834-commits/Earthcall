#pragma once

#include "Form/Singular/Property/PropertyPath.hpp"
#include "Form/Singular/Singular.hpp"
#include "Universe.hpp"
#include "json.hpp"

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
// Qualified paths: "@being-id.position.y" resolves on that NAMED being
// (looked up in the Universe by identifier) instead of the law's subject.
// This is how a condition or action addresses ONE specific object's property
// while the law itself ranges over many. An unqualified path stays what it
// always was: the subject's own property.
// ---------------------------------------------------------------------------
inline Singular* resolveLawRoot(Singular& subject, const PropertyPath& path,
                                PropertyPath& remainder) {
    if (path.segments.empty() || path.segments[0].empty() ||
        path.segments[0][0] != '@') {
        remainder = path;
        return &subject;
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
    return root && remainder.getValue(*root, out);
}

inline bool lawSetValue(Singular& subject, const PropertyPath& path, const PropertyValue& v) {
    if (isTimePath(path)) return false;   // no law writes time
    PropertyPath remainder;
    Singular* root = resolveLawRoot(subject, path, remainder);
    return root && remainder.setValue(*root, v);
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
