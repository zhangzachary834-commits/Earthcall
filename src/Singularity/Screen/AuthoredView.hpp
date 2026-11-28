#pragma once

#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValue.hpp"

#include <cmath>

namespace Rendering {

// How far a Zone's sight reaches. Zach chose an authored far plane on 2026-11-03,
// after the app's hard-coded 100 m clipped The Merry Dancers' 960 m sky.
//
// This is the ordinary authored property `view.farPlane` on the Zone's spatial root,
// beside `light.*`, so it is readable and writable by Law and persists with the Zone.
// Because it lives on the Zone, leaving the Zone ends it: a channel-wide value would
// leak into the next Zone and need a reset. Absent means `fallback`, so a Zone that
// says nothing renders exactly as before. A wrongly typed, non-finite or at-or-inside
// near value is ignored rather than guessed at, because an invalid projection makes
// Engine::render return with nothing drawn; the property stays visible to correct.
inline float authoredFarPlane(const geom::FieldNode* zoneRoot, float fallback, float nearPlane) {
    if (!zoneRoot) return fallback;
    PropertyValue value;
    if (!zoneRoot->getDynamicProperty("view.farPlane", value)) return fallback;
    double number = 0.0;
    if (!propertyValueToNumber(value, number)) return fallback;
    const float far = static_cast<float>(number);
    if (!std::isfinite(far) || far <= nearPlane) return fallback;
    return far;
}

} // namespace Rendering
