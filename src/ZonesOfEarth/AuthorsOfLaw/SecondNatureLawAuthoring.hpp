#pragma once

#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"

#include <string>

class Law;
class Singular;

// The Person-facing Law authoring instrument is ordinary authored world state:
// Objects, Properties, Relations, and Laws. This file supplies only the one
// irreducible birth seam the current action vocabulary still lacks: turning an
// already-authored Law shape into another ordinary Law.
//
// Protocol:
//   publish ECA::Event{"law-concept-invoked", request, optionalTarget}
//
// `request` may itself be the template Law, or any Singular carrying:
//   lawConcept.templateId : string (required)
//   lawConcept.targetId   : string (optional; event.object wins)
//   lawConcept.name       : string (optional newborn display name)
//
// Template Law text may contain the readable token `$TARGET` in string-valued
// model leaves. At birth it is structurally rebound to the selected target's
// stable identifier. The template stays untouched and normally disabled; the
// newborn is enabled, serializable Law text with the same authorship.
namespace SecondNatureLawAuthoring {

inline constexpr const char* kInvokeEvent = "law-concept-invoked";
inline constexpr const char* kAuthoredEvent = "law-authored";
inline constexpr const char* kTemplateProperty = "lawConcept.templateId";
inline constexpr const char* kTargetProperty = "lawConcept.targetId";
inline constexpr const char* kNameProperty = "lawConcept.name";
inline constexpr const char* kStatusProperty = "lawConcept.status";
inline constexpr const char* kLastCreatedProperty = "lawConcept.lastCreated";

// Idempotent for a process. Called by the translation unit's bootstrap and
// exposed so focused tests/tools can state the dependency explicitly.
void install();

// The listener body is public for deterministic tests. Returns true only when
// a Law was actually born and registered.
bool instantiate(const ECA::Event& event);

} // namespace SecondNatureLawAuthoring
