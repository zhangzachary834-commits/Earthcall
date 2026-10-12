#include "Singularity/Foreign/Web/Designer/WebDesignerMetalaws.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ConditionModel.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace Singularity {
namespace Foreign {
namespace Web {

namespace {

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
}

std::string trim(const std::string& s) {
    size_t first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, (last - first + 1));
}

const std::vector<WebLexemeDef> s_definitions = {
    // Structural / Semantic Roles
    {"page",        "web.concept.role.page",        "lexeme.web.page",        "metalaw.web.role.page",        "Whole web page or document root"},
    {"section",     "web.concept.role.section",     "lexeme.web.section",     "metalaw.web.role.section",     "Document section landmark"},
    {"navigation",  "web.concept.role.navigation",  "lexeme.web.navigation",  "metalaw.web.role.navigation",  "Navigation region landmark"},
    {"nav",         "web.concept.role.navigation",  "lexeme.web.nav",         "metalaw.web.role.navigation",  "Navigation bar"},
    {"heading",     "web.concept.role.heading",     "lexeme.web.heading",     "metalaw.web.role.heading",     "Semantic heading element"},
    {"title",       "web.concept.role.heading",     "lexeme.web.title",       "metalaw.web.role.heading",     "Semantic heading title"},
    {"subtitle",    "web.concept.role.subtitle",    "lexeme.web.subtitle",    "metalaw.web.role.subtitle",    "Secondary heading or subtitle"},
    {"paragraph",   "web.concept.role.paragraph",   "lexeme.web.paragraph",   "metalaw.web.role.paragraph",   "Paragraph body copy"},
    {"text",        "web.concept.role.paragraph",   "lexeme.web.text",        "metalaw.web.role.paragraph",   "Text content run"},
    {"button",      "web.concept.role.action",      "lexeme.web.button",      "metalaw.web.role.action",      "Interactive button action"},
    {"action",      "web.concept.role.action",      "lexeme.web.action",      "metalaw.web.role.action",      "Interactive action target"},
    {"link",        "web.concept.role.link",        "lexeme.web.link",        "metalaw.web.role.link",        "Hyperlink action"},
    {"container",   "web.concept.role.container",   "lexeme.web.container",   "metalaw.web.role.container",   "Layout container grouping"},
    {"card",        "web.concept.role.card",        "lexeme.web.card",        "metalaw.web.role.card",        "Elevated card container"},
    {"list",        "web.concept.role.list",        "lexeme.web.list",        "metalaw.web.role.list",        "Semantic list element"},
    {"item",        "web.concept.role.item",        "lexeme.web.item",        "metalaw.web.role.item",        "List or grid item"},
    {"media",       "web.concept.role.media",       "lexeme.web.media",       "metalaw.web.role.media",       "Visual media or graphic"},
    {"image",       "web.concept.role.media",       "lexeme.web.image",       "metalaw.web.role.media",       "Image element"},
    {"quote",       "web.concept.role.quote",       "lexeme.web.quote",       "metalaw.web.role.quote",       "Blockquote or testimonial"},
    {"divider",     "web.concept.role.divider",     "lexeme.web.divider",     "metalaw.web.role.divider",     "Visual separation divider"},

    // Flow & Layout
    {"vertical",    "web.concept.flow.vertical",    "lexeme.web.vertical",    "metalaw.web.flow.vertical",    "Vertical stack layout"},
    {"column",      "web.concept.flow.vertical",    "lexeme.web.column",      "metalaw.web.flow.vertical",    "Column flow layout"},
    {"horizontal",  "web.concept.flow.horizontal",  "lexeme.web.horizontal",  "metalaw.web.flow.horizontal",  "Horizontal row layout"},
    {"row",         "web.concept.flow.horizontal",  "lexeme.web.row",         "metalaw.web.flow.horizontal",  "Row flow layout"},
    {"grid",        "web.concept.flow.grid",        "lexeme.web.grid",        "metalaw.web.flow.grid",        "Multi-column responsive grid"},
    {"centered",    "web.concept.align.center",     "lexeme.web.centered",    "metalaw.web.align.center",     "Center alignment"},
    {"center",      "web.concept.align.center",     "lexeme.web.center",      "metalaw.web.align.center",     "Center alignment"},
    {"start",       "web.concept.align.start",      "lexeme.web.start",       "metalaw.web.align.start",      "Start alignment"},
    {"between",     "web.concept.justify.between",  "lexeme.web.between",     "metalaw.web.justify.between",  "Space between distribution"},

    // Spacing
    {"tight",       "web.concept.spacing.tight",    "lexeme.web.tight",       "metalaw.web.spacing.tight",    "Tight spacing scale"},
    {"compact",     "web.concept.spacing.tight",    "lexeme.web.compact",     "metalaw.web.spacing.tight",    "Compact spacing scale"},
    {"normal",      "web.concept.spacing.normal",   "lexeme.web.normal",      "metalaw.web.spacing.normal",   "Normal spacing scale"},
    {"relaxed",     "web.concept.spacing.relaxed",  "lexeme.web.relaxed",     "metalaw.web.spacing.relaxed",  "Relaxed spacing scale"},
    {"generous",    "web.concept.spacing.generous", "lexeme.web.generous",    "metalaw.web.spacing.generous", "Generous layout spacing"},
    {"spacious",    "web.concept.spacing.generous", "lexeme.web.spacious",    "metalaw.web.spacing.generous", "Spacious layout spacing"},

    // Scale
    {"small",       "web.concept.size.small",       "lexeme.web.small",       "metalaw.web.size.small",       "Small scale sizing"},
    {"large",       "web.concept.size.large",       "lexeme.web.large",       "metalaw.web.size.large",       "Large scale sizing"},
    {"huge",        "web.concept.size.huge",        "lexeme.web.huge",        "metalaw.web.size.huge",        "Huge hero scale sizing"},

    // Typography
    {"bold",        "web.concept.weight.bold",      "lexeme.web.bold",        "metalaw.web.weight.bold",      "Bold font weight"},
    {"heavy",       "web.concept.weight.bold",      "lexeme.web.heavy",       "metalaw.web.weight.bold",      "Heavy font weight"},
    {"light",       "web.concept.weight.light",     "lexeme.web.light",       "metalaw.web.weight.light",     "Light font weight"},
    {"muted",       "web.concept.tone.muted",       "lexeme.web.muted",       "metalaw.web.tone.muted",       "Muted secondary tone"},
    {"lead",        "web.concept.tone.prominent",   "lexeme.web.lead",        "metalaw.web.tone.prominent",   "Prominent lead copy"},

    // Surface & Presentation
    {"rounded",     "web.concept.shape.rounded",    "lexeme.web.rounded",     "metalaw.web.shape.rounded",    "Rounded corners"},
    {"pill",        "web.concept.shape.pill",       "lexeme.web.pill",        "metalaw.web.shape.pill",       "Pill capsule shape"},
    {"sharp",       "web.concept.shape.sharp",      "lexeme.web.sharp",       "metalaw.web.shape.sharp",      "Sharp square corners"},
    {"primary",     "web.concept.variant.primary",  "lexeme.web.primary",     "metalaw.web.variant.primary",  "Primary brand styling"},
    {"secondary",   "web.concept.variant.secondary","lexeme.web.secondary",   "metalaw.web.variant.secondary","Secondary styling"},
    {"hero",        "web.concept.variant.hero",     "lexeme.web.hero",        "metalaw.web.variant.hero",     "Hero emphasis styling"},
    {"outline",     "web.concept.variant.outline",  "lexeme.web.outline",     "metalaw.web.variant.outline",  "Outlined border styling"},
    {"subtle",      "web.concept.elevation.subtle", "lexeme.web.subtle",      "metalaw.web.elevation.subtle", "Subtle elevation shadow"},
    {"elevated",    "web.concept.elevation.raised", "lexeme.web.elevated",    "metalaw.web.elevation.raised", "Raised elevation shadow"}
};

void addMetalaw(LawManager& laws,
                const std::string& metalawId,
                const std::string& opcode,
                ActionNode action,
                Singular* author) {
    if (laws.find(metalawId)) return;

    std::vector<Singular*> authors;
    if (author) authors.push_back(author);

    auto metalaw = std::make_shared<Law>(metalawId, authors);
    metalaw->setLawIdentifier(metalawId);
    metalaw->grantAuthority(10); // First Mover / Singularity modality authority

    // Trigger on compilation request
    metalaw->setActivation(Law::Activation::OnEvent);
    metalaw->setScope(Law::Scope::Subject);

    // Condition: compilation.input.opcode == opcode
    metalaw->setConditionModel(ConditionNode::compare(
        "compilation.input.opcode",
        ConditionNode::Op::Eq,
        PropertyValue(opcode)));

    metalaw->setActionModel(std::move(action));

    laws.add(metalaw);
    laws.bindTrigger(metalawId, "web-designer-compilation-requested");
}

} // namespace

const WebLexemeDef* WebDesignerMetalaws::findDefinition(const std::string& symbol) {
    std::string lower = toLower(trim(symbol));
    for (const auto& def : s_definitions) {
        if (def.symbol == lower) return &def;
    }
    return nullptr;
}

const std::vector<WebLexemeDef>& WebDesignerMetalaws::allDefinitions() {
    return s_definitions;
}

void WebDesignerMetalaws::syncRegister(LawManager& laws, Singular* author) {
    // 1. Roles
    addMetalaw(laws, "metalaw.web.role.page", "web.concept.role.page",
               ActionNode::set("compilation.output.web.role", PropertyValue(std::string("page"))), author);
    addMetalaw(laws, "metalaw.web.role.section", "web.concept.role.section",
               ActionNode::sequence({
                   ActionNode::set("compilation.output.web.role", PropertyValue(std::string("section"))),
                   ActionNode::set("compilation.output.web.layout.flow", PropertyValue(std::string("vertical")))
               }), author);
    addMetalaw(laws, "metalaw.web.role.navigation", "web.concept.role.navigation",
               ActionNode::sequence({
                   ActionNode::set("compilation.output.web.role", PropertyValue(std::string("navigation"))),
                   ActionNode::set("compilation.output.web.layout.flow", PropertyValue(std::string("horizontal")))
               }), author);
    addMetalaw(laws, "metalaw.web.role.heading", "web.concept.role.heading",
               ActionNode::sequence({
                   ActionNode::set("compilation.output.web.role", PropertyValue(std::string("heading"))),
                   ActionNode::set("compilation.output.web.level", PropertyValue(2.0))
               }), author);
    addMetalaw(laws, "metalaw.web.role.subtitle", "web.concept.role.subtitle",
               ActionNode::sequence({
                   ActionNode::set("compilation.output.web.role", PropertyValue(std::string("heading"))),
                   ActionNode::set("compilation.output.web.level", PropertyValue(3.0)),
                   ActionNode::set("compilation.output.web.typography.tone", PropertyValue(std::string("muted")))
               }), author);
    addMetalaw(laws, "metalaw.web.role.paragraph", "web.concept.role.paragraph",
               ActionNode::set("compilation.output.web.role", PropertyValue(std::string("paragraph"))), author);
    addMetalaw(laws, "metalaw.web.role.action", "web.concept.role.action",
               ActionNode::set("compilation.output.web.role", PropertyValue(std::string("action"))), author);
    addMetalaw(laws, "metalaw.web.role.link", "web.concept.role.link",
               ActionNode::sequence({
                   ActionNode::set("compilation.output.web.role", PropertyValue(std::string("action"))),
                   ActionNode::set("compilation.output.web.surface.variant", PropertyValue(std::string("outline")))
               }), author);
    addMetalaw(laws, "metalaw.web.role.container", "web.concept.role.container",
               ActionNode::set("compilation.output.web.role", PropertyValue(std::string("container"))), author);
    addMetalaw(laws, "metalaw.web.role.card", "web.concept.role.card",
               ActionNode::sequence({
                   ActionNode::set("compilation.output.web.role", PropertyValue(std::string("container"))),
                   ActionNode::set("compilation.output.web.surface.variant", PropertyValue(std::string("card"))),
                   ActionNode::set("compilation.output.web.surface.shape", PropertyValue(std::string("rounded"))),
                   ActionNode::set("compilation.output.web.surface.elevation", PropertyValue(std::string("subtle")))
               }), author);
    addMetalaw(laws, "metalaw.web.role.list", "web.concept.role.list",
               ActionNode::sequence({
                   ActionNode::set("compilation.output.web.role", PropertyValue(std::string("list"))),
                   ActionNode::set("compilation.output.web.layout.flow", PropertyValue(std::string("vertical")))
               }), author);
    addMetalaw(laws, "metalaw.web.role.item", "web.concept.role.item",
               ActionNode::set("compilation.output.web.role", PropertyValue(std::string("item"))), author);
    addMetalaw(laws, "metalaw.web.role.media", "web.concept.role.media",
               ActionNode::set("compilation.output.web.role", PropertyValue(std::string("media"))), author);
    addMetalaw(laws, "metalaw.web.role.quote", "web.concept.role.quote",
               ActionNode::sequence({
                   ActionNode::set("compilation.output.web.role", PropertyValue(std::string("quote"))),
                   ActionNode::set("compilation.output.web.layout.align", PropertyValue(std::string("center"))),
                   ActionNode::set("compilation.output.web.typography.tone", PropertyValue(std::string("prominent")))
               }), author);
    addMetalaw(laws, "metalaw.web.role.divider", "web.concept.role.divider",
               ActionNode::set("compilation.output.web.role", PropertyValue(std::string("divider"))), author);

    // 2. Flow & Alignment
    addMetalaw(laws, "metalaw.web.flow.vertical", "web.concept.flow.vertical",
               ActionNode::set("compilation.output.web.layout.flow", PropertyValue(std::string("vertical"))), author);
    addMetalaw(laws, "metalaw.web.flow.horizontal", "web.concept.flow.horizontal",
               ActionNode::set("compilation.output.web.layout.flow", PropertyValue(std::string("horizontal"))), author);
    addMetalaw(laws, "metalaw.web.flow.grid", "web.concept.flow.grid",
               ActionNode::set("compilation.output.web.layout.flow", PropertyValue(std::string("grid"))), author);
    addMetalaw(laws, "metalaw.web.align.center", "web.concept.align.center",
               ActionNode::set("compilation.output.web.layout.align", PropertyValue(std::string("center"))), author);
    addMetalaw(laws, "metalaw.web.align.start", "web.concept.align.start",
               ActionNode::set("compilation.output.web.layout.align", PropertyValue(std::string("start"))), author);
    addMetalaw(laws, "metalaw.web.justify.between", "web.concept.justify.between",
               ActionNode::set("compilation.output.web.layout.justify", PropertyValue(std::string("between"))), author);

    // 3. Spacing
    addMetalaw(laws, "metalaw.web.spacing.tight", "web.concept.spacing.tight",
               ActionNode::set("compilation.output.web.layout.spacing", PropertyValue(std::string("tight"))), author);
    addMetalaw(laws, "metalaw.web.spacing.normal", "web.concept.spacing.normal",
               ActionNode::set("compilation.output.web.layout.spacing", PropertyValue(std::string("normal"))), author);
    addMetalaw(laws, "metalaw.web.spacing.relaxed", "web.concept.spacing.relaxed",
               ActionNode::set("compilation.output.web.layout.spacing", PropertyValue(std::string("relaxed"))), author);
    addMetalaw(laws, "metalaw.web.spacing.generous", "web.concept.spacing.generous",
               ActionNode::set("compilation.output.web.layout.spacing", PropertyValue(std::string("generous"))), author);

    // 4. Scale
    addMetalaw(laws, "metalaw.web.size.small", "web.concept.size.small",
               ActionNode::set("compilation.output.web.scale.size", PropertyValue(std::string("small"))), author);
    addMetalaw(laws, "metalaw.web.size.large", "web.concept.size.large",
               ActionNode::set("compilation.output.web.scale.size", PropertyValue(std::string("large"))), author);
    addMetalaw(laws, "metalaw.web.size.huge", "web.concept.size.huge",
               ActionNode::set("compilation.output.web.scale.size", PropertyValue(std::string("huge"))), author);

    // 5. Typography
    addMetalaw(laws, "metalaw.web.weight.bold", "web.concept.weight.bold",
               ActionNode::set("compilation.output.web.typography.weight", PropertyValue(std::string("bold"))), author);
    addMetalaw(laws, "metalaw.web.weight.light", "web.concept.weight.light",
               ActionNode::set("compilation.output.web.typography.weight", PropertyValue(std::string("light"))), author);
    addMetalaw(laws, "metalaw.web.tone.muted", "web.concept.tone.muted",
               ActionNode::set("compilation.output.web.typography.tone", PropertyValue(std::string("muted"))), author);
    addMetalaw(laws, "metalaw.web.tone.prominent", "web.concept.tone.prominent",
               ActionNode::set("compilation.output.web.typography.tone", PropertyValue(std::string("prominent"))), author);

    // 6. Surface
    addMetalaw(laws, "metalaw.web.shape.rounded", "web.concept.shape.rounded",
               ActionNode::set("compilation.output.web.surface.shape", PropertyValue(std::string("rounded"))), author);
    addMetalaw(laws, "metalaw.web.shape.pill", "web.concept.shape.pill",
               ActionNode::set("compilation.output.web.surface.shape", PropertyValue(std::string("pill"))), author);
    addMetalaw(laws, "metalaw.web.shape.sharp", "web.concept.shape.sharp",
               ActionNode::set("compilation.output.web.surface.shape", PropertyValue(std::string("sharp"))), author);
    addMetalaw(laws, "metalaw.web.variant.primary", "web.concept.variant.primary",
               ActionNode::set("compilation.output.web.surface.variant", PropertyValue(std::string("primary"))), author);
    addMetalaw(laws, "metalaw.web.variant.secondary", "web.concept.variant.secondary",
               ActionNode::set("compilation.output.web.surface.variant", PropertyValue(std::string("secondary"))), author);
    addMetalaw(laws, "metalaw.web.variant.hero", "web.concept.variant.hero",
               ActionNode::set("compilation.output.web.surface.variant", PropertyValue(std::string("hero"))), author);
    addMetalaw(laws, "metalaw.web.variant.outline", "web.concept.variant.outline",
               ActionNode::set("compilation.output.web.surface.variant", PropertyValue(std::string("outline"))), author);
    addMetalaw(laws, "metalaw.web.elevation.subtle", "web.concept.elevation.subtle",
               ActionNode::set("compilation.output.web.surface.elevation", PropertyValue(std::string("subtle"))), author);
    addMetalaw(laws, "metalaw.web.elevation.raised", "web.concept.elevation.raised",
               ActionNode::set("compilation.output.web.surface.elevation", PropertyValue(std::string("raised"))), author);
}

WebConcept WebDesignerMetalaws::compileTokens(const std::vector<std::string>& tokens,
                                              LawManager& laws,
                                              Singular& channelTarget,
                                              std::string* outError) {
    WebConcept concept;

    // Pre-initialize compilation slots on channelTarget so PropertyPath resolves dynamic slots
    channelTarget.setDynamicProperty("compilation.input.opcode", PropertyValue(std::string("")));
    channelTarget.setDynamicProperty("compilation.input.lexeme", PropertyValue(std::string("")));
    channelTarget.setDynamicProperty("compilation.output.web.role", PropertyValue(std::string("")));
    channelTarget.setDynamicProperty("compilation.output.web.level", PropertyValue(0.0));
    channelTarget.setDynamicProperty("compilation.output.web.layout.flow", PropertyValue(std::string("")));
    channelTarget.setDynamicProperty("compilation.output.web.layout.align", PropertyValue(std::string("")));
    channelTarget.setDynamicProperty("compilation.output.web.layout.justify", PropertyValue(std::string("")));
    channelTarget.setDynamicProperty("compilation.output.web.layout.spacing", PropertyValue(std::string("")));
    channelTarget.setDynamicProperty("compilation.output.web.scale.size", PropertyValue(std::string("")));
    channelTarget.setDynamicProperty("compilation.output.web.typography.weight", PropertyValue(std::string("")));
    channelTarget.setDynamicProperty("compilation.output.web.typography.tone", PropertyValue(std::string("")));
    channelTarget.setDynamicProperty("compilation.output.web.surface.variant", PropertyValue(std::string("")));
    channelTarget.setDynamicProperty("compilation.output.web.surface.shape", PropertyValue(std::string("")));
    channelTarget.setDynamicProperty("compilation.output.web.surface.elevation", PropertyValue(std::string("")));

    // Track facets to detect conflicts
    std::string assignedFlow;
    std::string assignedAlign;
    std::string assignedScale;

    for (const auto& token : tokens) {
        std::string cleaned = toLower(trim(token));
        if (cleaned.empty()) continue;

        // Skip non-semantic filler words
        if (cleaned == "a" || cleaned == "an" || cleaned == "the" || cleaned == "with") continue;

        const auto* def = findDefinition(cleaned);
        if (!def) {
            // Unrecognized token - if outError requested, note it, otherwise ignore
            continue;
        }

        // Prepare the channel compilation slot
        channelTarget.setDynamicProperty("compilation.input.opcode", PropertyValue(def->opcode));
        channelTarget.setDynamicProperty("compilation.input.lexeme", PropertyValue(def->stableId));

        auto* metalaw = laws.find(def->metalawId);
        if (metalaw) {
            auto res = metalaw->applyTo(channelTarget);
            if (res != Law::ApplicationResult::Applied) {
                if (outError) *outError = "Metalaw " + def->metalawId + " failed to apply";
            }
        }

        // Check for semantic conflicts
        if (def->opcode.find("web.concept.flow.") != std::string::npos) {
            if (!assignedFlow.empty() && assignedFlow != def->opcode) {
                if (outError) *outError = "Conflicting layout flow concepts in phrase: " + assignedFlow + " vs " + def->opcode;
            }
            assignedFlow = def->opcode;
        }
        if (def->opcode.find("web.concept.align.") != std::string::npos) {
            if (!assignedAlign.empty() && assignedAlign != def->opcode) {
                if (outError) *outError = "Conflicting alignment concepts in phrase: " + assignedAlign + " vs " + def->opcode;
            }
            assignedAlign = def->opcode;
        }
        if (def->opcode.find("web.concept.size.") != std::string::npos) {
            if (!assignedScale.empty() && assignedScale != def->opcode) {
                if (outError) *outError = "Conflicting scale concepts in phrase: " + assignedScale + " vs " + def->opcode;
            }
            assignedScale = def->opcode;
        }
    }

    // Read the compiled output from the channel target into our WebConcept
    PropertyValue v;
    if (channelTarget.getDynamicProperty("compilation.output.web.role", v)) {
        if (auto s = std::get_if<std::string>(&v)) {
            if (!s->empty()) concept.role = WebConcept::stringToRole(*s);
        }
    }
    if (channelTarget.getDynamicProperty("compilation.output.web.level", v)) {
        if (auto d = std::get_if<double>(&v)) {
            if (*d > 0.0) concept.headingLevel = static_cast<int>(*d);
        }
    }
    if (channelTarget.getDynamicProperty("compilation.output.web.layout.flow", v)) {
        if (auto s = std::get_if<std::string>(&v)) {
            if (!s->empty()) concept.flow = WebConcept::stringToFlow(*s);
        }
    }
    if (channelTarget.getDynamicProperty("compilation.output.web.layout.align", v)) {
        if (auto s = std::get_if<std::string>(&v)) {
            if (!s->empty()) concept.align = WebConcept::stringToAlign(*s);
        }
    }
    if (channelTarget.getDynamicProperty("compilation.output.web.layout.justify", v)) {
        if (auto s = std::get_if<std::string>(&v)) {
            if (!s->empty()) concept.justify = WebConcept::stringToJustify(*s);
        }
    }
    if (channelTarget.getDynamicProperty("compilation.output.web.layout.spacing", v)) {
        if (auto s = std::get_if<std::string>(&v)) {
            if (!s->empty()) concept.spacing = WebConcept::stringToSpacing(*s);
        }
    }
    if (channelTarget.getDynamicProperty("compilation.output.web.scale.size", v)) {
        if (auto s = std::get_if<std::string>(&v)) {
            if (!s->empty()) concept.scale = WebConcept::stringToScale(*s);
        }
    }
    if (channelTarget.getDynamicProperty("compilation.output.web.typography.weight", v)) {
        if (auto s = std::get_if<std::string>(&v)) {
            if (!s->empty()) concept.weight = WebConcept::stringToWeight(*s);
        }
    }
    if (channelTarget.getDynamicProperty("compilation.output.web.typography.tone", v)) {
        if (auto s = std::get_if<std::string>(&v)) {
            if (!s->empty()) concept.tone = WebConcept::stringToTone(*s);
        }
    }
    if (channelTarget.getDynamicProperty("compilation.output.web.surface.variant", v)) {
        if (auto s = std::get_if<std::string>(&v)) {
            if (!s->empty()) concept.variant = WebConcept::stringToVariant(*s);
        }
    }
    if (channelTarget.getDynamicProperty("compilation.output.web.surface.shape", v)) {
        if (auto s = std::get_if<std::string>(&v)) {
            if (!s->empty()) concept.shape = WebConcept::stringToShape(*s);
        }
    }
    if (channelTarget.getDynamicProperty("compilation.output.web.surface.elevation", v)) {
        if (auto s = std::get_if<std::string>(&v)) {
            if (!s->empty()) concept.elevation = WebConcept::stringToElevation(*s);
        }
    }

    return concept;
}

WebConcept WebDesignerMetalaws::compilePhrase(const std::string& phrase,
                                             LawManager& laws,
                                             Singular& channelTarget,
                                             std::string* outError) {
    // Split phrase by whitespace or punctuation
    std::vector<std::string> tokens;
    std::string current;
    for (char c : phrase) {
        if (std::isspace(static_cast<unsigned char>(c)) || c == ',' || c == ';') {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        } else {
            current.push_back(c);
        }
    }
    if (!current.empty()) tokens.push_back(current);

    return compileTokens(tokens, laws, channelTarget, outError);
}

std::shared_ptr<Singular> WebDesignerMetalaws::createAuthoredElement(
    const std::string& identifier,
    const WebConcept& concept,
    Singular* author) {
    auto obj = std::make_shared<Object>();
    obj->setObjectID(identifier);
    if (author) {
        // Record authorship
        obj->setDynamicProperty("authored.by", PropertyValue(author->getIdentifier()));
    }
    concept.applyToSingular(*obj);
    return obj;
}

} // namespace Web
} // namespace Foreign
} // namespace Singularity
