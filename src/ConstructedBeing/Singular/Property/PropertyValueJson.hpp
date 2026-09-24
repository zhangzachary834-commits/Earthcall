#pragma once

#include "PropertyValue.hpp"
#include "json.hpp"

#include <functional>
#include <optional>
#include <string>

class Singular;

// JSON round-trip for the legible alternatives, encoded as {"t": <tag>, "v": ...}
// so the exact alternative (int vs long vs float vs double) survives.
//
// Identity references are serialized by stable identifier and may be resolved
// only after the referenced Singular exists. Call tryPropertyValueFromJson()
// with a load-scoped resolver for that phase. The legacy convenience reader
// remains for scalar/value-only callers and returns monostate when resolution
// is impossible rather than inventing a pointer.
using PropertyReferenceResolver = std::function<Singular*(const std::string&)>;

nlohmann::json propertyValueToJson(const PropertyValue& v);
std::optional<PropertyValue> tryPropertyValueFromJson(
    const nlohmann::json& j,
    const PropertyReferenceResolver& resolve = {});
PropertyValue propertyValueFromJson(const nlohmann::json& j);
