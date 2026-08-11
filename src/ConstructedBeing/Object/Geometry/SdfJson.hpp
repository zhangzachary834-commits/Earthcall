#pragma once

#include "Sdf.hpp"
#include "json.hpp"

// JSON round-trip for the SDF expression tree. The compiled RPN is derived
// state (recompiled from `expr` on load), so only the mathematical text is
// serialized — same rule as everywhere else: models are primary, compiled
// artifacts are derived.
namespace geom {

nlohmann::json sdfToJson(const SdfNode& n);
SdfNode sdfFromJson(const nlohmann::json& j);

} // namespace geom
