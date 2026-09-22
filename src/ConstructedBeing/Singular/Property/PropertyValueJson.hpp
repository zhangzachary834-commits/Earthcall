#pragma once

#include "PropertyValue.hpp"
#include "json.hpp"

// JSON round-trip for the legible alternatives, encoded as {"t": <tag>, "v": ...}
// so the exact alternative (int vs long vs float vs double) survives.
//
// Singular-reference alternatives serialize as {"t":"ref","id":"<identifier>"}
// and deserialize to monostate — world references are resolved by the world
// loader against live identity, never reconstructed from value serialization.
nlohmann::json propertyValueToJson(const PropertyValue& v);
PropertyValue propertyValueFromJson(const nlohmann::json& j);
