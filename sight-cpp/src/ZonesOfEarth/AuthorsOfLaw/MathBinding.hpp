#pragma once

#include "Form/Singular/Property/PropertyPath.hpp"
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

inline std::optional<std::map<std::string, double>> readMathBindings(
    Singular& subject, const MathBindings& bindings) {
    std::map<std::string, double> vars;
    for (const auto& entry : bindings) {
        PropertyValue value;
        double x = 0.0;
        if (!entry.second.getValue(subject, value) || !propertyValueToNumber(value, x)) {
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
