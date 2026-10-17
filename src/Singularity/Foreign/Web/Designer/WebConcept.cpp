#include "Singularity/Foreign/Web/Designer/WebConcept.hpp"
#include <algorithm>
#include <cctype>

namespace Singularity {
namespace Foreign {
namespace Web {

namespace {

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
}

} // namespace

std::string WebConcept::roleToString(SemanticRole r) {
    switch (r) {
        case SemanticRole::Page:       return "page";
        case SemanticRole::Section:    return "section";
        case SemanticRole::Navigation: return "navigation";
        case SemanticRole::Heading:    return "heading";
        case SemanticRole::Paragraph:  return "paragraph";
        case SemanticRole::Action:     return "action";
        case SemanticRole::Container:  return "container";
        case SemanticRole::List:       return "list";
        case SemanticRole::Item:       return "item";
        case SemanticRole::Media:      return "media";
        case SemanticRole::Quote:      return "quote";
        case SemanticRole::Divider:    return "divider";
        case SemanticRole::Unknown:    return "unknown";
    }
    return "unknown";
}

SemanticRole WebConcept::stringToRole(const std::string& s) {
    std::string lower = toLower(s);
    if (lower == "page") return SemanticRole::Page;
    if (lower == "section") return SemanticRole::Section;
    if (lower == "navigation") return SemanticRole::Navigation;
    if (lower == "heading") return SemanticRole::Heading;
    if (lower == "paragraph") return SemanticRole::Paragraph;
    if (lower == "action") return SemanticRole::Action;
    if (lower == "container") return SemanticRole::Container;
    if (lower == "list") return SemanticRole::List;
    if (lower == "item") return SemanticRole::Item;
    if (lower == "media") return SemanticRole::Media;
    if (lower == "quote") return SemanticRole::Quote;
    if (lower == "divider") return SemanticRole::Divider;
    return SemanticRole::Unknown;
}

std::string WebConcept::flowToString(LayoutFlow f) {
    switch (f) {
        case LayoutFlow::Vertical:   return "vertical";
        case LayoutFlow::Horizontal: return "horizontal";
        case LayoutFlow::Grid:       return "grid";
    }
    return "vertical";
}

LayoutFlow WebConcept::stringToFlow(const std::string& s) {
    std::string lower = toLower(s);
    if (lower == "horizontal") return LayoutFlow::Horizontal;
    if (lower == "grid") return LayoutFlow::Grid;
    if (lower == "vertical") return LayoutFlow::Vertical;
    return LayoutFlow::Vertical;
}

std::string WebConcept::alignToString(LayoutAlign a) {
    switch (a) {
        case LayoutAlign::Start:   return "start";
        case LayoutAlign::Center:  return "center";
        case LayoutAlign::End:     return "end";
        case LayoutAlign::Stretch: return "stretch";
    }
    return "start";
}

LayoutAlign WebConcept::stringToAlign(const std::string& s) {
    std::string lower = toLower(s);
    if (lower == "center") return LayoutAlign::Center;
    if (lower == "end") return LayoutAlign::End;
    if (lower == "stretch") return LayoutAlign::Stretch;
    return LayoutAlign::Start;
}

std::string WebConcept::justifyToString(LayoutJustify j) {
    switch (j) {
        case LayoutJustify::Start:        return "start";
        case LayoutJustify::Center:       return "center";
        case LayoutJustify::End:          return "end";
        case LayoutJustify::SpaceBetween: return "between";
        case LayoutJustify::SpaceAround:  return "around";
    }
    return "start";
}

LayoutJustify WebConcept::stringToJustify(const std::string& s) {
    std::string lower = toLower(s);
    if (lower == "center") return LayoutJustify::Center;
    if (lower == "end") return LayoutJustify::End;
    if (lower == "between") return LayoutJustify::SpaceBetween;
    if (lower == "around") return LayoutJustify::SpaceAround;
    return LayoutJustify::Start;
}

std::string WebConcept::spacingToString(SpacingScale sp) {
    switch (sp) {
        case SpacingScale::None:     return "none";
        case SpacingScale::Tight:    return "tight";
        case SpacingScale::Normal:   return "normal";
        case SpacingScale::Relaxed:  return "relaxed";
        case SpacingScale::Generous: return "generous";
    }
    return "normal";
}

SpacingScale WebConcept::stringToSpacing(const std::string& s) {
    std::string lower = toLower(s);
    if (lower == "none") return SpacingScale::None;
    if (lower == "tight") return SpacingScale::Tight;
    if (lower == "relaxed") return SpacingScale::Relaxed;
    if (lower == "generous") return SpacingScale::Generous;
    return SpacingScale::Normal;
}

std::string WebConcept::scaleToString(ContentScale sc) {
    switch (sc) {
        case ContentScale::Small:  return "small";
        case ContentScale::Normal: return "normal";
        case ContentScale::Large:  return "large";
        case ContentScale::Huge:   return "huge";
    }
    return "normal";
}

ContentScale WebConcept::stringToScale(const std::string& s) {
    std::string lower = toLower(s);
    if (lower == "small") return ContentScale::Small;
    if (lower == "large") return ContentScale::Large;
    if (lower == "huge") return ContentScale::Huge;
    return ContentScale::Normal;
}

std::string WebConcept::weightToString(TypographyWeight w) {
    switch (w) {
        case TypographyWeight::Normal: return "normal";
        case TypographyWeight::Light:  return "light";
        case TypographyWeight::Medium: return "medium";
        case TypographyWeight::Bold:   return "bold";
    }
    return "normal";
}

TypographyWeight WebConcept::stringToWeight(const std::string& s) {
    std::string lower = toLower(s);
    if (lower == "light") return TypographyWeight::Light;
    if (lower == "medium") return TypographyWeight::Medium;
    if (lower == "bold") return TypographyWeight::Bold;
    return TypographyWeight::Normal;
}

std::string WebConcept::toneToString(TypographyTone t) {
    switch (t) {
        case TypographyTone::Default:   return "default";
        case TypographyTone::Muted:     return "muted";
        case TypographyTone::Accent:    return "accent";
        case TypographyTone::Prominent: return "prominent";
    }
    return "default";
}

TypographyTone WebConcept::stringToTone(const std::string& s) {
    std::string lower = toLower(s);
    if (lower == "muted") return TypographyTone::Muted;
    if (lower == "accent") return TypographyTone::Accent;
    if (lower == "prominent") return TypographyTone::Prominent;
    return TypographyTone::Default;
}

std::string WebConcept::variantToString(SurfaceVariant v) {
    switch (v) {
        case SurfaceVariant::Default:   return "default";
        case SurfaceVariant::Primary:   return "primary";
        case SurfaceVariant::Secondary: return "secondary";
        case SurfaceVariant::Surface:   return "surface";
        case SurfaceVariant::Card:      return "card";
        case SurfaceVariant::Hero:      return "hero";
        case SurfaceVariant::Subtle:    return "subtle";
        case SurfaceVariant::Outline:   return "outline";
        case SurfaceVariant::Ghost:     return "ghost";
    }
    return "default";
}

SurfaceVariant WebConcept::stringToVariant(const std::string& s) {
    std::string lower = toLower(s);
    if (lower == "primary") return SurfaceVariant::Primary;
    if (lower == "secondary") return SurfaceVariant::Secondary;
    if (lower == "surface") return SurfaceVariant::Surface;
    if (lower == "card") return SurfaceVariant::Card;
    if (lower == "hero") return SurfaceVariant::Hero;
    if (lower == "subtle") return SurfaceVariant::Subtle;
    if (lower == "outline") return SurfaceVariant::Outline;
    if (lower == "ghost") return SurfaceVariant::Ghost;
    return SurfaceVariant::Default;
}

std::string WebConcept::shapeToString(SurfaceShape sh) {
    switch (sh) {
        case SurfaceShape::Sharp:   return "sharp";
        case SurfaceShape::Rounded: return "rounded";
        case SurfaceShape::Pill:    return "pill";
        case SurfaceShape::Circle:  return "circle";
    }
    return "rounded";
}

SurfaceShape WebConcept::stringToShape(const std::string& s) {
    std::string lower = toLower(s);
    if (lower == "sharp") return SurfaceShape::Sharp;
    if (lower == "pill") return SurfaceShape::Pill;
    if (lower == "circle") return SurfaceShape::Circle;
    if (lower == "rounded") return SurfaceShape::Rounded;
    return SurfaceShape::Rounded;
}

std::string WebConcept::elevationToString(SurfaceElevation el) {
    switch (el) {
        case SurfaceElevation::Flat:     return "flat";
        case SurfaceElevation::Subtle:   return "subtle";
        case SurfaceElevation::Raised:   return "raised";
        case SurfaceElevation::Floating: return "floating";
    }
    return "flat";
}

SurfaceElevation WebConcept::stringToElevation(const std::string& s) {
    std::string lower = toLower(s);
    if (lower == "subtle") return SurfaceElevation::Subtle;
    if (lower == "raised") return SurfaceElevation::Raised;
    if (lower == "floating") return SurfaceElevation::Floating;
    return SurfaceElevation::Flat;
}

void WebConcept::applyToSingular(Singular& being) const {
    // 1. Universal Earthcall Properties: ONLY write explicitly authored facets!
    std::string effectiveRole = roleName.empty() ? (role != SemanticRole::Unknown ? roleToString(role) : "") : roleName;
    if (!effectiveRole.empty() && effectiveRole != "unknown") {
        being.setDynamicProperty("role", PropertyValue(effectiveRole));
        being.setDynamicProperty("web.role", PropertyValue(effectiveRole));
    }
    if (!text.empty()) {
        being.setDynamicProperty("text", PropertyValue(text));
        being.setDynamicProperty("web.text", PropertyValue(text));
    }
    if (headingLevel.has_value()) {
        being.setDynamicProperty("level", PropertyValue(static_cast<double>(*headingLevel)));
        being.setDynamicProperty("web.level", PropertyValue(static_cast<double>(*headingLevel)));
    }
    if (!flowName.empty()) {
        being.setDynamicProperty("layout.flow", PropertyValue(flowName));
        being.setDynamicProperty("web.layout.flow", PropertyValue(flowName));
    }
    if (!alignName.empty()) {
        being.setDynamicProperty("layout.align", PropertyValue(alignName));
        being.setDynamicProperty("web.layout.align", PropertyValue(alignName));
    }
    if (!justifyName.empty()) {
        being.setDynamicProperty("layout.justify", PropertyValue(justifyName));
        being.setDynamicProperty("web.layout.justify", PropertyValue(justifyName));
    }
    if (wrap.has_value()) {
        being.setDynamicProperty("layout.wrap", PropertyValue(*wrap));
        being.setDynamicProperty("web.layout.wrap", PropertyValue(*wrap));
    }
    if (!spacingName.empty()) {
        being.setDynamicProperty("layout.spacing", PropertyValue(spacingName));
        being.setDynamicProperty("web.layout.spacing", PropertyValue(spacingName));
    }
    if (gridColumns.has_value()) {
        being.setDynamicProperty("layout.columns", PropertyValue(static_cast<double>(*gridColumns)));
        being.setDynamicProperty("web.layout.columns", PropertyValue(static_cast<double>(*gridColumns)));
    }
    if (!scaleName.empty()) {
        being.setDynamicProperty("scale", PropertyValue(scaleName));
        being.setDynamicProperty("web.scale.size", PropertyValue(scaleName));
    }
    if (!weightName.empty()) {
        being.setDynamicProperty("typography.weight", PropertyValue(weightName));
        being.setDynamicProperty("web.typography.weight", PropertyValue(weightName));
    }
    if (!toneName.empty()) {
        being.setDynamicProperty("typography.tone", PropertyValue(toneName));
        being.setDynamicProperty("web.typography.tone", PropertyValue(toneName));
    }
    if (!variantName.empty()) {
        being.setDynamicProperty("surface.variant", PropertyValue(variantName));
        being.setDynamicProperty("web.surface.variant", PropertyValue(variantName));
    }
    if (!shapeName.empty()) {
        being.setDynamicProperty("surface.shape", PropertyValue(shapeName));
        being.setDynamicProperty("web.surface.shape", PropertyValue(shapeName));
    }
    if (!elevationName.empty()) {
        being.setDynamicProperty("surface.elevation", PropertyValue(elevationName));
        being.setDynamicProperty("web.surface.elevation", PropertyValue(elevationName));
    }
    if (!background.empty()) being.setDynamicProperty("surface.background", PropertyValue(background));
    if (!color.empty()) being.setDynamicProperty("surface.color", PropertyValue(color));
    if (subordinate) being.setDynamicProperty("subordinate", PropertyValue(true));
    if (!prominence.empty()) being.setDynamicProperty("prominence", PropertyValue(prominence));

    // 2. Medium-Specific Bindings (web.*)
    if (!actionHref.empty()) {
        being.setDynamicProperty("web.href", PropertyValue(actionHref));
        being.setDynamicProperty("web.action.href", PropertyValue(actionHref));
    }
    if (!actionEvent.empty()) being.setDynamicProperty("web.action.event", PropertyValue(actionEvent));
    if (!mediaSrc.empty()) being.setDynamicProperty("web.media.src", PropertyValue(mediaSrc));
    if (!mediaAlt.empty()) being.setDynamicProperty("web.media.alt", PropertyValue(mediaAlt));
    if (!elementTag.empty()) being.setDynamicProperty("web.element.tag", PropertyValue(elementTag));

    // Refusal 1 & Sun decree: web.order write path is eliminated.
    // Relational topology (precedes Relation) governs sequence; no numeric order is written.
}

WebConcept WebConcept::readFromSingular(const Singular& being) {
    WebConcept concept;
    PropertyValue v;

    // Helper lambda to check universal key first, then web.* fallback
    auto getProp = [&](const std::string& universalKey, const std::string& webKey, PropertyValue& out) -> bool {
        if (being.getDynamicProperty(universalKey, out)) return true;
        if (!webKey.empty() && being.getDynamicProperty(webKey, out)) return true;
        return false;
    };

    // Role
    if (getProp("role", "web.role", v)) {
        if (auto s = std::get_if<std::string>(&v)) {
            concept.roleName = *s;
            concept.role = stringToRole(*s);
        }
    }

    // Text / Content
    if (getProp("text", "web.text", v) || being.getDynamicProperty("content", v)) {
        if (auto s = std::get_if<std::string>(&v)) concept.text = *s;
    }

    // Heading level
    if (getProp("level", "web.level", v) || being.getDynamicProperty("heading.level", v)) {
        if (auto d = std::get_if<double>(&v)) concept.headingLevel = static_cast<int>(*d);
    }

    // Layout flow
    if (getProp("layout.flow", "web.layout.flow", v) || being.getDynamicProperty("flow", v)) {
        if (auto s = std::get_if<std::string>(&v)) {
            concept.flowName = *s;
            concept.flow = stringToFlow(*s);
        }
    }

    // Layout align
    if (getProp("layout.align", "web.layout.align", v) || being.getDynamicProperty("align", v)) {
        if (auto s = std::get_if<std::string>(&v)) {
            concept.alignName = *s;
            concept.align = stringToAlign(*s);
        }
    }

    // Layout justify
    if (getProp("layout.justify", "web.layout.justify", v) || being.getDynamicProperty("justify", v)) {
        if (auto s = std::get_if<std::string>(&v)) {
            concept.justifyName = *s;
            concept.justify = stringToJustify(*s);
        }
    }

    // Layout wrap
    if (getProp("layout.wrap", "web.layout.wrap", v) || being.getDynamicProperty("wrap", v)) {
        if (auto b = std::get_if<bool>(&v)) concept.wrap = *b;
    }

    // Spacing
    if (getProp("layout.spacing", "web.layout.spacing", v) || being.getDynamicProperty("spacing", v)) {
        if (auto s = std::get_if<std::string>(&v)) {
            concept.spacingName = *s;
            concept.spacing = stringToSpacing(*s);
        }
    }

    // Columns
    if (getProp("layout.columns", "web.layout.columns", v) || being.getDynamicProperty("columns", v)) {
        if (auto d = std::get_if<double>(&v)) concept.gridColumns = static_cast<int>(*d);
    }

    // Scale
    if (getProp("scale", "web.scale.size", v) || being.getDynamicProperty("size", v)) {
        if (auto s = std::get_if<std::string>(&v)) {
            concept.scaleName = *s;
            concept.scale = stringToScale(*s);
        }
    }

    // Typography weight
    if (getProp("typography.weight", "web.typography.weight", v) || being.getDynamicProperty("weight", v)) {
        if (auto s = std::get_if<std::string>(&v)) {
            concept.weightName = *s;
            concept.weight = stringToWeight(*s);
        }
    }

    // Typography tone
    if (getProp("typography.tone", "web.typography.tone", v) || being.getDynamicProperty("tone", v)) {
        if (auto s = std::get_if<std::string>(&v)) {
            concept.toneName = *s;
            concept.tone = stringToTone(*s);
        }
    }

    // Surface variant
    if (getProp("surface.variant", "web.surface.variant", v) || being.getDynamicProperty("variant", v)) {
        if (auto s = std::get_if<std::string>(&v)) {
            concept.variantName = *s;
            concept.variant = stringToVariant(*s);
        }
    }

    // Surface shape
    if (getProp("surface.shape", "web.surface.shape", v) || being.getDynamicProperty("shape", v)) {
        if (auto s = std::get_if<std::string>(&v)) {
            concept.shapeName = *s;
            concept.shape = stringToShape(*s);
        }
    }

    // Surface elevation
    if (getProp("surface.elevation", "web.surface.elevation", v) || being.getDynamicProperty("elevation", v)) {
        if (auto s = std::get_if<std::string>(&v)) {
            concept.elevationName = *s;
            concept.elevation = stringToElevation(*s);
        }
    }

    // Colors
    if (getProp("surface.background", "web.surface.background", v) || being.getDynamicProperty("background", v)) {
        if (auto s = std::get_if<std::string>(&v)) concept.background = *s;
    }
    if (getProp("surface.color", "web.surface.color", v) || being.getDynamicProperty("color", v)) {
        if (auto s = std::get_if<std::string>(&v)) concept.color = *s;
    }

    // Relational modifiers
    if (being.getDynamicProperty("prominence", v)) {
        if (auto s = std::get_if<std::string>(&v)) concept.prominence = *s;
    }
    if (being.getDynamicProperty("subordinate", v)) {
        if (auto b = std::get_if<bool>(&v)) concept.subordinate = *b;
    }

    // Interactive & Media
    if (getProp("action.href", "web.action.href", v) || getProp("href", "web.href", v)) {
        if (auto s = std::get_if<std::string>(&v)) concept.actionHref = *s;
    }
    if (getProp("action.event", "web.action.event", v)) {
        if (auto s = std::get_if<std::string>(&v)) concept.actionEvent = *s;
    }
    if (getProp("media.src", "web.media.src", v) || being.getDynamicProperty("src", v)) {
        if (auto s = std::get_if<std::string>(&v)) concept.mediaSrc = *s;
    }
    if (getProp("media.alt", "web.media.alt", v) || being.getDynamicProperty("alt", v)) {
        if (auto s = std::get_if<std::string>(&v)) concept.mediaAlt = *s;
    }
    if (getProp("element.tag", "web.element.tag", v)) {
        if (auto s = std::get_if<std::string>(&v)) concept.elementTag = *s;
    }

    // Optional realization order cache (if present)
    if (being.getDynamicProperty("web.order", v) || being.getDynamicProperty("order", v)) {
        if (auto d = std::get_if<double>(&v)) concept.order = static_cast<int>(*d);
    }

    return concept;
}

} // namespace Web
} // namespace Foreign
} // namespace Singularity
