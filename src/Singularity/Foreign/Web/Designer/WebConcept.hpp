#pragma once

#include "ConstructedBeing/Singular/Singular.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValue.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

namespace Singularity {
namespace Foreign {
namespace Web {

// ---------------------------------------------------------------------------
// Semantic Concept Model (Ephemeral View / Adapter for Realization)
//
// In accordance with Earthcall Refusal 1, Refusal 3, and Sun's decree:
//   "No new C++ class for a domain noun... Categories are authored beings,
//    not enum members. Before merge, justify every remaining enum as
//    irreducible foreign-medium substrate. If it is merely a cache for authored
//    semantic categories, delete it and let the adapter consume open authored
//    properties directly."
//
// WebConcept contains zero closed domain-noun enums. Roles, flows, alignments,
// surface variants, shapes, elevations, spacing scales, and typography tones
// are preserved purely as open string properties authored in-world as data.
// ---------------------------------------------------------------------------

struct WebConcept {
    // Open semantic role: preserves arbitrary authored roles (e.g. "page", "section", "heading", "action", "sanctuary", etc.)
    std::string role;
    std::string roleName; // compatibility alias

    std::string text;
    std::optional<int> headingLevel;

    // Layout facets
    std::string flow;       // "vertical", "horizontal", "grid"
    std::string flowName;   // compatibility alias
    std::string align;      // "start", "center", "end", "stretch"
    std::string alignName;  // compatibility alias
    std::string justify;    // "start", "center", "end", "between", "around"
    std::string justifyName;// compatibility alias
    std::optional<bool> wrap;
    std::string spacing;    // "none", "tight", "normal", "relaxed", "generous", or CSS length
    std::string spacingName;// compatibility alias
    std::optional<int> gridColumns;

    // Typography & scale facets
    std::string scale;      // "small", "normal", "large", "huge", or CSS size
    std::string scaleName;  // compatibility alias
    std::string weight;     // "normal", "light", "medium", "bold", "700"
    std::string weightName; // compatibility alias
    std::string tone;       // "default", "muted", "accent", "prominent"
    std::string toneName;   // compatibility alias

    // Surface & presentation facets
    std::string variant;    // "default", "primary", "secondary", "card", "hero", "subtle", "outline"
    std::string variantName;// compatibility alias
    std::string shape;      // "sharp", "rounded", "pill", "circle"
    std::string shapeName;  // compatibility alias
    std::string elevation;  // "flat", "subtle", "raised", "floating"
    std::string elevationName;// compatibility alias

    std::string background; // Hex, rgba, or CSS color/gradient
    std::string color;      // Text color override

    // Universal relational modifiers
    std::string prominence;  // "primary", "subordinate", "prominent", "quiet"
    bool subordinate = false;

    // Interactive & media facets
    std::string actionHref;
    std::string actionEvent;
    std::string mediaSrc;
    std::string mediaAlt;
    std::string elementTag;

    // Preservation of unconstrained authored properties
    std::unordered_map<std::string, PropertyValue> authoredProperties;

    // Derived realization ordering (derived from Relation topology, not an authoring source of truth)
    int order = 0;

    // Apply this semantic concept onto an Earthcall Singular as authored properties
    void applyToSingular(Singular& being) const;

    // Read an authored semantic concept from an Earthcall Singular's properties
    static WebConcept readFromSingular(const Singular& being);
};

// Relation types used in Web Designer ontology (authored strings)
namespace WebRelationType {
    constexpr const char* kContains      = "contains";
    constexpr const char* kPrecedes      = "precedes";
    constexpr const char* kAuthoredBy    = "authored-by";
    constexpr const char* kEchoes        = "echoes";
    constexpr const char* kSubordinateTo = "subordinate-to";
    constexpr const char* kAnnotatedBy   = "annotated-by";
    constexpr const char* kGovernedBy    = "governed-by";
    constexpr const char* kInstantiatedBy = "instantiated-by";
}

} // namespace Web
} // namespace Foreign
} // namespace Singularity
