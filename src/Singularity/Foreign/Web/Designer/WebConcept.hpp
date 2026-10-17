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
// In accordance with Earthcall Refusal 1 and "C++ must not decide what the Person means":
//   "No new C++ class for a domain noun. Domain things and their state are
//    authored in-world as data, never carved into the type system."
//
// WebConcept is NOT the authority of what meanings may exist. The authority
// is the in-world Singular properties, Formations, and Relations authored
// by Persons. WebConcept is an ephemeral compiled/adapter view used by the
// thin web realizer for structured translation into HTML/CSS/DOM.
//
// Enums below exist purely as convenience caches for standard browser
// landmarks/mechanics. Custom and newly-authored roles, variants, flows,
// and modifiers are fully preserved as open string values and arbitrary
// dynamic properties without requiring C++ enum expansion.
// ---------------------------------------------------------------------------

enum class SemanticRole {
    Unknown = 0,
    Page,
    Section,
    Navigation,
    Heading,
    Paragraph,
    Action,      // Button, link, clickable CTA
    Container,   // Generic grouping, card, box
    List,        // Ordered or unordered list
    Item,        // List item, card item
    Media,       // Image, graphic, icon
    Quote,       // Blockquote, testimonial callout
    Divider      // Visual/semantic separator
};

enum class LayoutFlow {
    Vertical = 0, // Column
    Horizontal,   // Row
    Grid          // Multi-column grid
};

enum class LayoutAlign {
    Start = 0,
    Center,
    End,
    Stretch
};

enum class LayoutJustify {
    Start = 0,
    Center,
    End,
    SpaceBetween,
    SpaceAround
};

enum class SpacingScale {
    None = 0,
    Tight,      // 0.5rem / 8px
    Normal,     // 1.0rem / 16px
    Relaxed,    // 1.5rem / 24px
    Generous    // 2.5rem / 40px
};

enum class ContentScale {
    Small = 0,
    Normal,
    Large,
    Huge
};

enum class TypographyWeight {
    Normal = 0,
    Light,
    Medium,
    Bold
};

enum class TypographyTone {
    Default = 0,
    Muted,
    Accent,
    Prominent
};

enum class SurfaceVariant {
    Default = 0,
    Primary,
    Secondary,
    Surface,
    Card,
    Hero,
    Subtle,
    Outline,
    Ghost
};

enum class SurfaceShape {
    Sharp = 0,
    Rounded,  // ~0.75rem / 12px
    Pill,     // Fully rounded capsule
    Circle
};

enum class SurfaceElevation {
    Flat = 0,
    Subtle,
    Raised,
    Floating
};

struct WebConcept {
    // Open semantic role: preserves arbitrary authored roles (e.g. "sanctuary", "article", "badge")
    // Empty by default: C++ does not author default role.
    std::string roleName;
    SemanticRole role = SemanticRole::Unknown; // Fast-path cache for known roles

    std::string text;
    std::optional<int> headingLevel; // Unset by default

    // Layout facets (empty by default; enums are ephemeral adapter fallbacks for realization only)
    std::string flowName;
    LayoutFlow flow = LayoutFlow::Vertical;

    std::string alignName;
    LayoutAlign align = LayoutAlign::Start;

    std::string justifyName;
    LayoutJustify justify = LayoutJustify::Start;

    std::optional<bool> wrap;

    std::string spacingName;
    SpacingScale spacing = SpacingScale::Normal;

    std::optional<int> gridColumns;

    // Typography & scale facets (empty by default)
    std::string scaleName;
    ContentScale scale = ContentScale::Normal;

    std::string weightName;
    TypographyWeight weight = TypographyWeight::Normal;

    std::string toneName;
    TypographyTone tone = TypographyTone::Default;

    // Surface & presentation facets (empty by default)
    std::string variantName;
    SurfaceVariant variant = SurfaceVariant::Default;

    std::string shapeName;
    SurfaceShape shape = SurfaceShape::Rounded;

    std::string elevationName;
    SurfaceElevation elevation = SurfaceElevation::Flat;

    std::string background; // Hex, rgba, or CSS color
    std::string color;      // Text color override

    // Universal relational modifiers
    std::string prominence;  // "primary", "subordinate", "prominent", "quiet"
    bool subordinate = false; // Authored subordination hint

    // Interactive & media facets (genuinely medium-specific or action bindings)
    std::string actionHref;   // URL or anchor link
    std::string actionEvent;  // Earthcall ECA event name emitted on activation
    std::string mediaSrc;     // Image/media source path or URL
    std::string mediaAlt;     // Accessible alternative text
    std::string elementTag;   // Explicit lower-level HTML tag escape hatch if authored

    // Preservation of unconstrained authored properties
    std::unordered_map<std::string, PropertyValue> authoredProperties;

    // Derived realization ordering (derived from Relation topology, not an authoring source of truth)
    int order = 0;

    // Converters to/from string tokens
    static std::string roleToString(SemanticRole r);
    static SemanticRole stringToRole(const std::string& s);

    static std::string flowToString(LayoutFlow f);
    static LayoutFlow stringToFlow(const std::string& s);

    static std::string alignToString(LayoutAlign a);
    static LayoutAlign stringToAlign(const std::string& s);

    static std::string justifyToString(LayoutJustify j);
    static LayoutJustify stringToJustify(const std::string& s);

    static std::string spacingToString(SpacingScale sp);
    static SpacingScale stringToSpacing(const std::string& s);

    static std::string scaleToString(ContentScale sc);
    static ContentScale stringToScale(const std::string& s);

    static std::string weightToString(TypographyWeight w);
    static TypographyWeight stringToWeight(const std::string& s);

    static std::string toneToString(TypographyTone t);
    static TypographyTone stringToTone(const std::string& s);

    static std::string variantToString(SurfaceVariant v);
    static SurfaceVariant stringToVariant(const std::string& s);

    static std::string shapeToString(SurfaceShape sh);
    static SurfaceShape stringToShape(const std::string& s);

    static std::string elevationToString(SurfaceElevation el);
    static SurfaceElevation stringToElevation(const std::string& s);

    // Apply this semantic concept onto an Earthcall Singular as authored properties
    // Writes universal properties (role, layout.flow, etc.) and web-specific properties where relevant
    void applyToSingular(Singular& being) const;

    // Read an authored semantic concept from an Earthcall Singular's properties
    // Prioritizes universal properties and gracefully falls back to web.* legacy keys
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
