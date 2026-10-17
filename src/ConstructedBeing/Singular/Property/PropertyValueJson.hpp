#pragma once

#include "PropertyValue.hpp"
#include "json.hpp"

// JSON round-trip for the legible alternatives, encoded as {"t": <tag>, "v": ...}
// so the exact alternative (int vs long vs float vs double) survives.
// OntoMath scalar_field/vector_field tags carry the existing Field::toJson
// payload; legacy tag-only records stay undefined (their math was not saved).
//
// Singular-reference alternatives serialize as {"t":"ref","id":"<identifier>"}
// and deserialize to monostate — world references are resolved by the world
// loader against live identity, never reconstructed from value serialization.
nlohmann::json propertyValueToJson(const PropertyValue& v);
// Structural codec for existing mathematical trees and compiler syntax.
// Arrays remain lists; no vector/matrix heuristic chooses their meaning.
PropertyValue propertyStructureFromJson(const nlohmann::json& value, unsigned depth = 0);
nlohmann::json propertyStructureToJson(const PropertyValue& value, unsigned depth = 0);
PropertyValue propertyValueFromJson(const nlohmann::json& j);
