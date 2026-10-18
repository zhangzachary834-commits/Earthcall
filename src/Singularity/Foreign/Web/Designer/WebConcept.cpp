#include "Singularity/Foreign/Web/Designer/WebConcept.hpp"
#include <algorithm>
#include <cctype>

namespace Singularity {
namespace Foreign {
namespace Web {

namespace {

std::string pickValue(const std::string& primary, const std::string& fallback) {
    return primary.empty() ? fallback : primary;
}

} // namespace

void WebConcept::applyToSingular(Singular& being) const {
    // 1. Universal Earthcall Properties: ONLY write explicitly authored facets!
    std::string effRole = pickValue(role, roleName);
    if (!effRole.empty() && effRole != "unknown") {
        being.setDynamicProperty("role", PropertyValue(effRole));
        being.setDynamicProperty("web.role", PropertyValue(effRole));
    }
    if (!text.empty()) {
        being.setDynamicProperty("text", PropertyValue(text));
        being.setDynamicProperty("web.text", PropertyValue(text));
    }
    if (headingLevel.has_value()) {
        being.setDynamicProperty("level", PropertyValue(static_cast<double>(*headingLevel)));
        being.setDynamicProperty("web.level", PropertyValue(static_cast<double>(*headingLevel)));
    }
    std::string effFlow = pickValue(flow, flowName);
    if (!effFlow.empty()) {
        being.setDynamicProperty("layout.flow", PropertyValue(effFlow));
        being.setDynamicProperty("web.layout.flow", PropertyValue(effFlow));
    }
    std::string effAlign = pickValue(align, alignName);
    if (!effAlign.empty()) {
        being.setDynamicProperty("layout.align", PropertyValue(effAlign));
        being.setDynamicProperty("web.layout.align", PropertyValue(effAlign));
    }
    std::string effJustify = pickValue(justify, justifyName);
    if (!effJustify.empty()) {
        being.setDynamicProperty("layout.justify", PropertyValue(effJustify));
        being.setDynamicProperty("web.layout.justify", PropertyValue(effJustify));
    }
    if (wrap.has_value()) {
        being.setDynamicProperty("layout.wrap", PropertyValue(*wrap));
        being.setDynamicProperty("web.layout.wrap", PropertyValue(*wrap));
    }
    std::string effSpacing = pickValue(spacing, spacingName);
    if (!effSpacing.empty()) {
        being.setDynamicProperty("layout.spacing", PropertyValue(effSpacing));
        being.setDynamicProperty("web.layout.spacing", PropertyValue(effSpacing));
    }
    if (gridColumns.has_value()) {
        being.setDynamicProperty("layout.columns", PropertyValue(static_cast<double>(*gridColumns)));
        being.setDynamicProperty("web.layout.columns", PropertyValue(static_cast<double>(*gridColumns)));
    }
    std::string effScale = pickValue(scale, scaleName);
    if (!effScale.empty()) {
        being.setDynamicProperty("scale", PropertyValue(effScale));
        being.setDynamicProperty("web.scale.size", PropertyValue(effScale));
    }
    std::string effWeight = pickValue(weight, weightName);
    if (!effWeight.empty()) {
        being.setDynamicProperty("typography.weight", PropertyValue(effWeight));
        being.setDynamicProperty("web.typography.weight", PropertyValue(effWeight));
    }
    std::string effTone = pickValue(tone, toneName);
    if (!effTone.empty()) {
        being.setDynamicProperty("typography.tone", PropertyValue(effTone));
        being.setDynamicProperty("web.typography.tone", PropertyValue(effTone));
    }
    std::string effVariant = pickValue(variant, variantName);
    if (!effVariant.empty()) {
        being.setDynamicProperty("surface.variant", PropertyValue(effVariant));
        being.setDynamicProperty("web.surface.variant", PropertyValue(effVariant));
    }
    std::string effShape = pickValue(shape, shapeName);
    if (!effShape.empty()) {
        being.setDynamicProperty("surface.shape", PropertyValue(effShape));
        being.setDynamicProperty("web.surface.shape", PropertyValue(effShape));
    }
    std::string effElevation = pickValue(elevation, elevationName);
    if (!effElevation.empty()) {
        being.setDynamicProperty("surface.elevation", PropertyValue(effElevation));
        being.setDynamicProperty("web.surface.elevation", PropertyValue(effElevation));
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
            concept.role = *s;
            concept.roleName = *s;
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
            concept.flow = *s;
            concept.flowName = *s;
        }
    }

    // Layout align
    if (getProp("layout.align", "web.layout.align", v) || being.getDynamicProperty("align", v)) {
        if (auto s = std::get_if<std::string>(&v)) {
            concept.align = *s;
            concept.alignName = *s;
        }
    }

    // Layout justify
    if (getProp("layout.justify", "web.layout.justify", v) || being.getDynamicProperty("justify", v)) {
        if (auto s = std::get_if<std::string>(&v)) {
            concept.justify = *s;
            concept.justifyName = *s;
        }
    }

    // Layout wrap
    if (getProp("layout.wrap", "web.layout.wrap", v) || being.getDynamicProperty("wrap", v)) {
        if (auto b = std::get_if<bool>(&v)) concept.wrap = *b;
    }

    // Spacing
    if (getProp("layout.spacing", "web.layout.spacing", v) || being.getDynamicProperty("spacing", v)) {
        if (auto s = std::get_if<std::string>(&v)) {
            concept.spacing = *s;
            concept.spacingName = *s;
        }
    }

    // Columns
    if (getProp("layout.columns", "web.layout.columns", v) || being.getDynamicProperty("columns", v)) {
        if (auto d = std::get_if<double>(&v)) concept.gridColumns = static_cast<int>(*d);
    }

    // Scale
    if (getProp("scale", "web.scale.size", v) || being.getDynamicProperty("size", v)) {
        if (auto s = std::get_if<std::string>(&v)) {
            concept.scale = *s;
            concept.scaleName = *s;
        }
    }

    // Typography weight
    if (getProp("typography.weight", "web.typography.weight", v) || being.getDynamicProperty("weight", v)) {
        if (auto s = std::get_if<std::string>(&v)) {
            concept.weight = *s;
            concept.weightName = *s;
        }
    }

    // Typography tone
    if (getProp("typography.tone", "web.typography.tone", v) || being.getDynamicProperty("tone", v)) {
        if (auto s = std::get_if<std::string>(&v)) {
            concept.tone = *s;
            concept.toneName = *s;
        }
    }

    // Surface variant
    if (getProp("surface.variant", "web.surface.variant", v) || being.getDynamicProperty("variant", v)) {
        if (auto s = std::get_if<std::string>(&v)) {
            concept.variant = *s;
            concept.variantName = *s;
        }
    }

    // Surface shape
    if (getProp("surface.shape", "web.surface.shape", v) || being.getDynamicProperty("shape", v)) {
        if (auto s = std::get_if<std::string>(&v)) {
            concept.shape = *s;
            concept.shapeName = *s;
        }
    }

    // Surface elevation
    if (getProp("surface.elevation", "web.surface.elevation", v) || being.getDynamicProperty("elevation", v)) {
        if (auto s = std::get_if<std::string>(&v)) {
            concept.elevation = *s;
            concept.elevationName = *s;
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
