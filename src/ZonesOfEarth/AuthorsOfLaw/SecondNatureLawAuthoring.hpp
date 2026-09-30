#pragma once

#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"

class Singular;

// Thin Person-facing adapter from an authored interface event to the universal
// Singular set-to-set creation operation. Law authoring itself is NOT a second
// creation machine: the prototype is an ordinary Law Singular and the newborn
// is derived through SingularSetToSetCreation::derive().
namespace SecondNatureLawAuthoring {
    
inline constexpr const char* kInvokeEvent = "law-authoring-instrument-invoked";
inline constexpr const char* kAuthoredEvent = "law-authored";
inline constexpr const char* kPrototypeProperty = "forgePrototypeId";
inline constexpr const char* kTargetProperty = "forgeTargetId";
inline constexpr const char* kNameProperty = "forgeNewbornName";
inline constexpr const char* kStatusProperty = "forgeStatus";
inline constexpr const char* kLastCreatedProperty = "forgeLastCreated";

void install();
bool instantiate(const ECA::Event& event);

} // namespace SecondNatureLawAuthoring
