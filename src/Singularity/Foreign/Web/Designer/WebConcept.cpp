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
        default:                       return "unknown";
    }
}

SemanticRole WebConcept::stringToRole(const std::string& s) {
    std::string lower = toLower(s);
    if (lower == "page") return SemanticRole::Page;
    if (lower == "section") return SemanticRole::Section;
    if (lower == "navigation" || lower == "nav" || lower == "navbar") return SemanticRole::Navigation;
    if (lower == "heading" || lower == "title" || lower == "subtitle" || lower == "headline") return SemanticRole::Heading;
    if (lower == "paragraph" || lower == "text" || lower == "p" || lower == "body") return SemanticRole::Paragraph;
    if (lower == "action" || lower == "button" || lower == "btn" || lower == "link" || lower == "cta") return SemanticRole::Action;
    if (lower == "container" || lower == "card" || lower == "box" || lower == "panel") return SemanticRole::Container;
    if (lower == "list") return SemanticRole::List;
    if (lower == "item" || lower == "list-item") return SemanticRole::Item;
    if (lower == "media" || lower == "image" || lower == "img" || lower == "icon") return SemanticRole::Media;
    if (lower == "quote" || lower == "blockquote" || lower == "callout") return SemanticRole::Quote;
    if (lower == "divider" || lower == "hr" || lower == "separator") return SemanticRole::Divider;
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
    if (lower == "horizontal" || lower == "row" || lower == "inline") return LayoutFlow::Horizontal;
    if (lower == "grid") return LayoutFlow::Grid;
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
    if (lower == "center" || lower == "centered" || lower == "middle") return LayoutAlign::Center;
    if (lower == "end" || lower == "right") return LayoutAlign::End;
    if (lower == "stretch" || lower == "fill") return LayoutAlign::Stretch;
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
    if (lower == "between" || lower == "space-between") return LayoutJustify::SpaceBetween;
    if (lower == "around" || lower == "space-around") return LayoutJustify::SpaceAround;
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
    if (lower == "none" || lower == "0") return SpacingScale::None;
    if (lower == "tight" || lower == "compact") return SpacingScale::Tight;
    if (lower == "relaxed" || lower == "medium") return SpacingScale::Relaxed;
    if (lower == "generous" || lower == "spacious" || lower == "loose" || lower == "large") return SpacingScale::Generous;
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
    if (lower == "small" || lower == "sm") return ContentScale::Small;
    if (lower == "large" || lower == "lg" || lower == "big") return ContentScale::Large;
    if (lower == "huge" || lower == "xlarge" || lower == "xl" || lower == "hero") return ContentScale::Huge;
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
    if (lower == "light" || lower == "thin") return TypographyWeight::Light;
    if (lower == "medium" || lower == "semibold" || lower == "semi-bold") return TypographyWeight::Medium;
    if (lower == "bold" || lower == "heavy" || lower == "strong") return TypographyWeight::Bold;
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
    if (lower == "muted" || lower == "subtle" || lower == "secondary") return TypographyTone::Muted;
    if (lower == "accent" || lower == "brand") return TypographyTone::Accent;
    if (lower == "prominent" || lower == "lead" || lower == "highlight") return TypographyTone::Prominent;
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
    if (lower == "primary" || lower == "cta") return SurfaceVariant::Primary;
    if (lower == "secondary") return SurfaceVariant::Secondary;
    if (lower == "surface") return SurfaceVariant::Surface;
    if (lower == "card") return SurfaceVariant::Card;
    if (lower == "hero") return SurfaceVariant::Hero;
    if (lower == "subtle") return SurfaceVariant::Subtle;
    if (lower == "outline" || lower == "bordered") return SurfaceVariant::Outline;
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
    if (lower == "sharp" || lower == "square") return SurfaceShape::Sharp;
    if (lower == "pill" || lower == "capsule") return SurfaceShape::Pill;
    if (lower == "circle" || lower == "round") return SurfaceShape::Circle;
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
    if (lower == "raised" || lower == "shadow") return SurfaceElevation::Raised;
    if (lower == "floating" || lower == "high") return SurfaceElevation::Floating;
    return SurfaceElevation::Flat;
}

void WebConcept::applyToSingular(Singular& being) const {
    being.setDynamicProperty("web.role", PropertyValue(roleToString(role)));
    if (!text.empty()) being.setDynamicProperty("web.text", PropertyValue(text));
    being.setDynamicProperty("web.level", PropertyValue(static_cast<double>(headingLevel)));
    being.setDynamicProperty("web.layout.flow", PropertyValue(flowToString(flow)));
    being.setDynamicProperty("web.layout.align", PropertyValue(alignToString(align)));
    being.setDynamicProperty("web.layout.justify", PropertyValue(justifyToString(justify)));
    being.setDynamicProperty("web.layout.wrap", PropertyValue(wrap));
    being.setDynamicProperty("web.layout.spacing", PropertyValue(spacingToString(spacing)));
    being.setDynamicProperty("web.layout.columns", PropertyValue(static_cast<double>(gridColumns)));
    being.setDynamicProperty("web.scale.size", PropertyValue(scaleToString(scale)));
    being.setDynamicProperty("web.typography.weight", PropertyValue(weightToString(weight)));
    being.setDynamicProperty("web.typography.tone", PropertyValue(toneToString(tone)));
    being.setDynamicProperty("web.surface.variant", PropertyValue(variantToString(variant)));
    being.setDynamicProperty("web.surface.shape", PropertyValue(shapeToString(shape)));
    being.setDynamicProperty("web.surface.elevation", PropertyValue(elevationToString(elevation)));
    if (!background.empty()) being.setDynamicProperty("web.surface.background", PropertyValue(background));
    if (!color.empty()) being.setDynamicProperty("web.surface.color", PropertyValue(color));
    if (!actionHref.empty()) being.setDynamicProperty("web.action.href", PropertyValue(actionHref));
    if (!actionEvent.empty()) being.setDynamicProperty("web.action.event", PropertyValue(actionEvent));
    if (!mediaSrc.empty()) being.setDynamicProperty("web.media.src", PropertyValue(mediaSrc));
    if (!mediaAlt.empty()) being.setDynamicProperty("web.media.alt", PropertyValue(mediaAlt));
    being.setDynamicProperty("web.order", PropertyValue(static_cast<double>(order)));
}

WebConcept WebConcept::readFromSingular(const Singular& being) {
    WebConcept concept;
    PropertyValue v;

    if (being.getDynamicProperty("web.role", v)) {
        if (auto s = std::get_if<std::string>(&v)) concept.role = stringToRole(*s);
    }
    if (being.getDynamicProperty("web.text", v)) {
        if (auto s = std::get_if<std::string>(&v)) concept.text = *s;
    }
    if (being.getDynamicProperty("web.level", v)) {
        if (auto d = std::get_if<double>(&v)) concept.headingLevel = static_cast<int>(*d);
    }
    if (being.getDynamicProperty("web.layout.flow", v)) {
        if (auto s = std::get_if<std::string>(&v)) concept.flow = stringToFlow(*s);
    }
    if (being.getDynamicProperty("web.layout.align", v)) {
        if (auto s = std::get_if<std::string>(&v)) concept.align = stringToAlign(*s);
    }
    if (being.getDynamicProperty("web.layout.justify", v)) {
        if (auto s = std::get_if<std::string>(&v)) concept.justify = stringToJustify(*s);
    }
    if (being.getDynamicProperty("web.layout.wrap", v)) {
        if (auto b = std::get_if<bool>(&v)) concept.wrap = *b;
    }
    if (being.getDynamicProperty("web.layout.spacing", v)) {
        if (auto s = std::get_if<std::string>(&v)) concept.spacing = stringToSpacing(*s);
    }
    if (being.getDynamicProperty("web.layout.columns", v)) {
        if (auto d = std::get_if<double>(&v)) concept.gridColumns = static_cast<int>(*d);
    }
    if (being.getDynamicProperty("web.scale.size", v)) {
        if (auto s = std::get_if<std::string>(&v)) concept.scale = stringToScale(*s);
    }
    if (being.getDynamicProperty("web.typography.weight", v)) {
        if (auto s = std::get_if<std::string>(&v)) concept.weight = stringToWeight(*s);
    }
    if (being.getDynamicProperty("web.typography.tone", v)) {
        if (auto s = std::get_if<std::string>(&v)) concept.tone = stringToTone(*s);
    }
    if (being.getDynamicProperty("web.surface.variant", v)) {
        if (auto s = std::get_if<std::string>(&v)) concept.variant = stringToVariant(*s);
    }
    if (being.getDynamicProperty("web.surface.shape", v)) {
        if (auto s = std::get_if<std::string>(&v)) concept.shape = stringToShape(*s);
    }
    if (being.getDynamicProperty("web.surface.elevation", v)) {
        if (auto s = std::get_if<std::string>(&v)) concept.elevation = stringToElevation(*s);
    }
    if (being.getDynamicProperty("web.surface.background", v)) {
        if (auto s = std::get_if<std::string>(&v)) concept.background = *s;
    }
    if (being.getDynamicProperty("web.surface.color", v)) {
        if (auto s = std::get_if<std::string>(&v)) concept.color = *s;
    }
    if (being.getDynamicProperty("web.action.href", v)) {
        if (auto s = std::get_if<std::string>(&v)) concept.actionHref = *s;
    }
    if (being.getDynamicProperty("web.action.event", v)) {
        if (auto s = std::get_if<std::string>(&v)) concept.actionEvent = *s;
    }
    if (being.getDynamicProperty("web.media.src", v)) {
        if (auto s = std::get_if<std::string>(&v)) concept.mediaSrc = *s;
    }
    if (being.getDynamicProperty("web.media.alt", v)) {
        if (auto s = std::get_if<std::string>(&v)) concept.mediaAlt = *s;
    }
    if (being.getDynamicProperty("web.order", v)) {
        if (auto d = std::get_if<double>(&v)) concept.order = static_cast<int>(*d);
    }

    return concept;
}

} // namespace Web
} // namespace Foreign
} // namespace Singularity
