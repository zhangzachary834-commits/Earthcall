#include "Singularity/Foreign/Web/Designer/WebDesignerMetalaws.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ConditionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
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

// Global storage for seeded authoring vocabulary
std::vector<std::shared_ptr<Singularity::Language::Lexeme>> s_seededLexemes;
std::vector<std::shared_ptr<Law>> s_seededLaws;
std::vector<std::shared_ptr<Relation>> s_seededRelations;
std::shared_ptr<Object> s_modalityFirstMover;

void seedLexemeAndLaw(LawManager& laws,
                      const std::string& symbol,
                      const std::string& stableId,
                      const std::string& lawId,
                      ActionNode action,
                      Singular* author) {
    // If no human author is provided, record provenance to the modality's First Mover channel.
    // In accordance with Refusal 5, NEVER construct an Object masquerading as a Person named "Zach".
    if (!author) {
        if (!s_modalityFirstMover) {
            s_modalityFirstMover = std::make_shared<Object>();
            s_modalityFirstMover->setObjectID("first_mover.web_designer_modality");
        }
        author = s_modalityFirstMover.get();
    }

    // 1. Instantiate the real Lexeme
    auto lexeme = std::make_shared<Singularity::Language::Lexeme>(symbol, stableId);
    s_seededLexemes.push_back(lexeme);

    // 2. Instantiate the denoted Law with an enduring author so isAuthored() holds
    auto law = std::make_shared<Law>(lawId, std::vector<Singular*>{author});
    law->setLawIdentifier(lawId);
    law->grantAuthority(10);
    law->setEnabled(true);
    law->setActionModel(std::move(action));
    s_seededLaws.push_back(law);
    laws.add(law);

    // 3. Connect them with a first-class Relation("denotes", *lexeme, *law, true, 1.0f)
    auto rel = std::make_shared<Relation>("denotes", *lexeme, *law, true, 1.0f);
    s_seededRelations.push_back(rel);

    if (Universe::instance().hasRelationRegistrar()) {
        Universe::instance().addRelation(rel);
    }
}

} // namespace

const std::vector<std::shared_ptr<Singularity::Language::Lexeme>>& WebDesignerMetalaws::seededLexemes() {
    return s_seededLexemes;
}

const std::vector<std::shared_ptr<Relation>>& WebDesignerMetalaws::seededRelations() {
    return s_seededRelations;
}

void WebDesignerMetalaws::syncRegister(LawManager& laws, Singular* author) {
    if (!s_seededLaws.empty()) {
        // Already seeded in this process; ensure laws are attached to this manager
        for (const auto& law : s_seededLaws) {
            if (!laws.find(law->getIdentifier())) {
                laws.add(law);
            }
        }
        return;
    }

    // 1. Roles
    seedLexemeAndLaw(laws, "page", "lexeme.web.page", "law.concept.role.page",
                     ActionNode::sequence({
                         ActionNode::set("role", PropertyValue(std::string("page"))),
                         ActionNode::set("compilation.output.web.role", PropertyValue(std::string("page")))
                     }), author);

    seedLexemeAndLaw(laws, "section", "lexeme.web.section", "law.concept.role.section",
                     ActionNode::sequence({
                         ActionNode::set("role", PropertyValue(std::string("section"))),
                         ActionNode::set("compilation.output.web.role", PropertyValue(std::string("section")))
                     }), author);

    seedLexemeAndLaw(laws, "navigation", "lexeme.web.navigation", "law.concept.role.navigation",
                     ActionNode::sequence({
                         ActionNode::set("role", PropertyValue(std::string("navigation"))),
                         ActionNode::set("layout.flow", PropertyValue(std::string("horizontal"))),
                         ActionNode::set("compilation.output.web.role", PropertyValue(std::string("navigation"))),
                         ActionNode::set("compilation.output.web.layout.flow", PropertyValue(std::string("horizontal")))
                     }), author);

    seedLexemeAndLaw(laws, "nav", "lexeme.web.nav", "law.concept.role.navigation",
                     ActionNode::sequence({
                         ActionNode::set("role", PropertyValue(std::string("navigation"))),
                         ActionNode::set("layout.flow", PropertyValue(std::string("horizontal"))),
                         ActionNode::set("compilation.output.web.role", PropertyValue(std::string("navigation"))),
                         ActionNode::set("compilation.output.web.layout.flow", PropertyValue(std::string("horizontal")))
                     }), author);

    seedLexemeAndLaw(laws, "heading", "lexeme.web.heading", "law.concept.role.heading",
                     ActionNode::sequence({
                         ActionNode::set("role", PropertyValue(std::string("heading"))),
                         ActionNode::set("level", PropertyValue(2.0)),
                         ActionNode::set("compilation.output.web.role", PropertyValue(std::string("heading"))),
                         ActionNode::set("compilation.output.web.level", PropertyValue(2.0))
                     }), author);

    seedLexemeAndLaw(laws, "title", "lexeme.web.title", "law.concept.role.heading",
                     ActionNode::sequence({
                         ActionNode::set("role", PropertyValue(std::string("heading"))),
                         ActionNode::set("level", PropertyValue(1.0)),
                         ActionNode::set("compilation.output.web.role", PropertyValue(std::string("heading"))),
                         ActionNode::set("compilation.output.web.level", PropertyValue(1.0))
                     }), author);

    seedLexemeAndLaw(laws, "subtitle", "lexeme.web.subtitle", "law.concept.role.subtitle",
                     ActionNode::sequence({
                         ActionNode::set("role", PropertyValue(std::string("heading"))),
                         ActionNode::set("level", PropertyValue(3.0)),
                         ActionNode::set("typography.tone", PropertyValue(std::string("muted"))),
                         ActionNode::set("compilation.output.web.role", PropertyValue(std::string("heading"))),
                         ActionNode::set("compilation.output.web.level", PropertyValue(3.0)),
                         ActionNode::set("compilation.output.web.typography.tone", PropertyValue(std::string("muted")))
                     }), author);

    seedLexemeAndLaw(laws, "paragraph", "lexeme.web.paragraph", "law.concept.role.paragraph",
                     ActionNode::sequence({
                         ActionNode::set("role", PropertyValue(std::string("paragraph"))),
                         ActionNode::set("compilation.output.web.role", PropertyValue(std::string("paragraph")))
                     }), author);

    seedLexemeAndLaw(laws, "text", "lexeme.web.text", "law.concept.role.paragraph",
                     ActionNode::sequence({
                         ActionNode::set("role", PropertyValue(std::string("paragraph"))),
                         ActionNode::set("compilation.output.web.role", PropertyValue(std::string("paragraph")))
                     }), author);

    seedLexemeAndLaw(laws, "button", "lexeme.web.button", "law.concept.role.action",
                     ActionNode::sequence({
                         ActionNode::set("role", PropertyValue(std::string("action"))),
                         ActionNode::set("surface.shape", PropertyValue(std::string("rounded"))),
                         ActionNode::set("compilation.output.web.role", PropertyValue(std::string("action"))),
                         ActionNode::set("compilation.output.web.surface.shape", PropertyValue(std::string("rounded")))
                     }), author);

    seedLexemeAndLaw(laws, "action", "lexeme.web.action", "law.concept.role.action",
                     ActionNode::sequence({
                         ActionNode::set("role", PropertyValue(std::string("action"))),
                         ActionNode::set("compilation.output.web.role", PropertyValue(std::string("action")))
                     }), author);

    seedLexemeAndLaw(laws, "link", "lexeme.web.link", "law.concept.role.link",
                     ActionNode::sequence({
                         ActionNode::set("role", PropertyValue(std::string("action"))),
                         ActionNode::set("compilation.output.web.role", PropertyValue(std::string("action")))
                     }), author);

    seedLexemeAndLaw(laws, "container", "lexeme.web.container", "law.concept.role.container",
                     ActionNode::sequence({
                         ActionNode::set("role", PropertyValue(std::string("container"))),
                         ActionNode::set("compilation.output.web.role", PropertyValue(std::string("container")))
                     }), author);

    seedLexemeAndLaw(laws, "card", "lexeme.web.card", "law.concept.role.card",
                     ActionNode::sequence({
                         ActionNode::set("role", PropertyValue(std::string("container"))),
                         ActionNode::set("surface.variant", PropertyValue(std::string("card"))),
                         ActionNode::set("surface.shape", PropertyValue(std::string("rounded"))),
                         ActionNode::set("surface.elevation", PropertyValue(std::string("subtle"))),
                         ActionNode::set("compilation.output.web.role", PropertyValue(std::string("container"))),
                         ActionNode::set("compilation.output.web.surface.variant", PropertyValue(std::string("card"))),
                         ActionNode::set("compilation.output.web.surface.shape", PropertyValue(std::string("rounded"))),
                         ActionNode::set("compilation.output.web.surface.elevation", PropertyValue(std::string("subtle")))
                     }), author);

    seedLexemeAndLaw(laws, "list", "lexeme.web.list", "law.concept.role.list",
                     ActionNode::sequence({
                         ActionNode::set("role", PropertyValue(std::string("list"))),
                         ActionNode::set("layout.flow", PropertyValue(std::string("vertical"))),
                         ActionNode::set("compilation.output.web.role", PropertyValue(std::string("list"))),
                         ActionNode::set("compilation.output.web.layout.flow", PropertyValue(std::string("vertical")))
                     }), author);

    seedLexemeAndLaw(laws, "item", "lexeme.web.item", "law.concept.role.item",
                     ActionNode::sequence({
                         ActionNode::set("role", PropertyValue(std::string("item"))),
                         ActionNode::set("compilation.output.web.role", PropertyValue(std::string("item")))
                     }), author);

    seedLexemeAndLaw(laws, "media", "lexeme.web.media", "law.concept.role.media",
                     ActionNode::sequence({
                         ActionNode::set("role", PropertyValue(std::string("media"))),
                         ActionNode::set("compilation.output.web.role", PropertyValue(std::string("media")))
                     }), author);

    seedLexemeAndLaw(laws, "image", "lexeme.web.image", "law.concept.role.media",
                     ActionNode::sequence({
                         ActionNode::set("role", PropertyValue(std::string("media"))),
                         ActionNode::set("compilation.output.web.role", PropertyValue(std::string("media")))
                     }), author);

    seedLexemeAndLaw(laws, "quote", "lexeme.web.quote", "law.concept.role.quote",
                     ActionNode::sequence({
                         ActionNode::set("role", PropertyValue(std::string("quote"))),
                         ActionNode::set("compilation.output.web.role", PropertyValue(std::string("quote")))
                     }), author);

    seedLexemeAndLaw(laws, "divider", "lexeme.web.divider", "law.concept.role.divider",
                     ActionNode::sequence({
                         ActionNode::set("role", PropertyValue(std::string("divider"))),
                         ActionNode::set("compilation.output.web.role", PropertyValue(std::string("divider")))
                     }), author);

    // 2. Flows & Layout
    seedLexemeAndLaw(laws, "vertical", "lexeme.web.vertical", "law.concept.flow.vertical",
                     ActionNode::sequence({
                         ActionNode::set("layout.flow", PropertyValue(std::string("vertical"))),
                         ActionNode::set("compilation.output.web.layout.flow", PropertyValue(std::string("vertical")))
                     }), author);

    seedLexemeAndLaw(laws, "column", "lexeme.web.column", "law.concept.flow.vertical",
                     ActionNode::sequence({
                         ActionNode::set("layout.flow", PropertyValue(std::string("vertical"))),
                         ActionNode::set("compilation.output.web.layout.flow", PropertyValue(std::string("vertical")))
                     }), author);

    seedLexemeAndLaw(laws, "horizontal", "lexeme.web.horizontal", "law.concept.flow.horizontal",
                     ActionNode::sequence({
                         ActionNode::set("layout.flow", PropertyValue(std::string("horizontal"))),
                         ActionNode::set("compilation.output.web.layout.flow", PropertyValue(std::string("horizontal")))
                     }), author);

    seedLexemeAndLaw(laws, "row", "lexeme.web.row", "law.concept.flow.horizontal",
                     ActionNode::sequence({
                         ActionNode::set("layout.flow", PropertyValue(std::string("horizontal"))),
                         ActionNode::set("compilation.output.web.layout.flow", PropertyValue(std::string("horizontal")))
                     }), author);

    seedLexemeAndLaw(laws, "grid", "lexeme.web.grid", "law.concept.flow.grid",
                     ActionNode::sequence({
                         ActionNode::set("layout.flow", PropertyValue(std::string("grid"))),
                         ActionNode::set("compilation.output.web.layout.flow", PropertyValue(std::string("grid")))
                     }), author);

    // 3. Alignments
    seedLexemeAndLaw(laws, "start", "lexeme.web.align.start", "law.concept.align.start",
                     ActionNode::sequence({
                         ActionNode::set("layout.align", PropertyValue(std::string("start"))),
                         ActionNode::set("compilation.output.web.layout.align", PropertyValue(std::string("start")))
                     }), author);

    seedLexemeAndLaw(laws, "center", "lexeme.web.align.center", "law.concept.align.center",
                     ActionNode::sequence({
                         ActionNode::set("layout.align", PropertyValue(std::string("center"))),
                         ActionNode::set("compilation.output.web.layout.align", PropertyValue(std::string("center")))
                     }), author);

    seedLexemeAndLaw(laws, "centered", "lexeme.web.align.centered", "law.concept.align.center",
                     ActionNode::sequence({
                         ActionNode::set("layout.align", PropertyValue(std::string("center"))),
                         ActionNode::set("compilation.output.web.layout.align", PropertyValue(std::string("center")))
                     }), author);

    seedLexemeAndLaw(laws, "end", "lexeme.web.align.end", "law.concept.align.end",
                     ActionNode::sequence({
                         ActionNode::set("layout.align", PropertyValue(std::string("end"))),
                         ActionNode::set("compilation.output.web.layout.align", PropertyValue(std::string("end")))
                     }), author);

    seedLexemeAndLaw(laws, "stretch", "lexeme.web.align.stretch", "law.concept.align.stretch",
                     ActionNode::sequence({
                         ActionNode::set("layout.align", PropertyValue(std::string("stretch"))),
                         ActionNode::set("compilation.output.web.layout.align", PropertyValue(std::string("stretch")))
                     }), author);

    // 4. Spacing
    seedLexemeAndLaw(laws, "tight", "lexeme.web.spacing.tight", "law.concept.spacing.tight",
                     ActionNode::sequence({
                         ActionNode::set("layout.spacing", PropertyValue(std::string("tight"))),
                         ActionNode::set("compilation.output.web.layout.spacing", PropertyValue(std::string("tight")))
                     }), author);

    seedLexemeAndLaw(laws, "normal", "lexeme.web.spacing.normal", "law.concept.spacing.normal",
                     ActionNode::sequence({
                         ActionNode::set("layout.spacing", PropertyValue(std::string("normal"))),
                         ActionNode::set("compilation.output.web.layout.spacing", PropertyValue(std::string("normal")))
                     }), author);

    seedLexemeAndLaw(laws, "relaxed", "lexeme.web.spacing.relaxed", "law.concept.spacing.relaxed",
                     ActionNode::sequence({
                         ActionNode::set("layout.spacing", PropertyValue(std::string("relaxed"))),
                         ActionNode::set("compilation.output.web.layout.spacing", PropertyValue(std::string("relaxed")))
                     }), author);

    seedLexemeAndLaw(laws, "generous", "lexeme.web.spacing.generous", "law.concept.spacing.generous",
                     ActionNode::sequence({
                         ActionNode::set("layout.spacing", PropertyValue(std::string("generous"))),
                         ActionNode::set("compilation.output.web.layout.spacing", PropertyValue(std::string("generous")))
                     }), author);

    // 5. Scales
    seedLexemeAndLaw(laws, "small", "lexeme.web.small", "law.concept.size.small",
                     ActionNode::sequence({
                         ActionNode::set("scale", PropertyValue(std::string("small"))),
                         ActionNode::set("compilation.output.web.scale.size", PropertyValue(std::string("small")))
                     }), author);

    seedLexemeAndLaw(laws, "large", "lexeme.web.large", "law.concept.size.large",
                     ActionNode::sequence({
                         ActionNode::set("scale", PropertyValue(std::string("large"))),
                         ActionNode::set("compilation.output.web.scale.size", PropertyValue(std::string("large")))
                     }), author);

    seedLexemeAndLaw(laws, "huge", "lexeme.web.huge", "law.concept.size.huge",
                     ActionNode::sequence({
                         ActionNode::set("scale", PropertyValue(std::string("huge"))),
                         ActionNode::set("compilation.output.web.scale.size", PropertyValue(std::string("huge")))
                     }), author);

    // 6. Typography
    seedLexemeAndLaw(laws, "bold", "lexeme.web.bold", "law.concept.weight.bold",
                     ActionNode::sequence({
                         ActionNode::set("typography.weight", PropertyValue(std::string("bold"))),
                         ActionNode::set("compilation.output.web.typography.weight", PropertyValue(std::string("bold")))
                     }), author);

    seedLexemeAndLaw(laws, "light", "lexeme.web.light", "law.concept.weight.light",
                     ActionNode::sequence({
                         ActionNode::set("typography.weight", PropertyValue(std::string("light"))),
                         ActionNode::set("compilation.output.web.typography.weight", PropertyValue(std::string("light")))
                     }), author);

    seedLexemeAndLaw(laws, "muted", "lexeme.web.muted", "law.concept.tone.muted",
                     ActionNode::sequence({
                         ActionNode::set("typography.tone", PropertyValue(std::string("muted"))),
                         ActionNode::set("compilation.output.web.typography.tone", PropertyValue(std::string("muted")))
                     }), author);

    seedLexemeAndLaw(laws, "lead", "lexeme.web.lead", "law.concept.tone.prominent",
                     ActionNode::sequence({
                         ActionNode::set("scale", PropertyValue(std::string("large"))),
                         ActionNode::set("typography.tone", PropertyValue(std::string("prominent"))),
                         ActionNode::set("compilation.output.web.scale.size", PropertyValue(std::string("large"))),
                         ActionNode::set("compilation.output.web.typography.tone", PropertyValue(std::string("prominent")))
                     }), author);

    // 7. Surface & Presentation
    seedLexemeAndLaw(laws, "rounded", "lexeme.web.rounded", "law.concept.shape.rounded",
                     ActionNode::sequence({
                         ActionNode::set("surface.shape", PropertyValue(std::string("rounded"))),
                         ActionNode::set("compilation.output.web.surface.shape", PropertyValue(std::string("rounded")))
                     }), author);

    seedLexemeAndLaw(laws, "pill", "lexeme.web.pill", "law.concept.shape.pill",
                     ActionNode::sequence({
                         ActionNode::set("surface.shape", PropertyValue(std::string("pill"))),
                         ActionNode::set("compilation.output.web.surface.shape", PropertyValue(std::string("pill")))
                     }), author);

    seedLexemeAndLaw(laws, "sharp", "lexeme.web.sharp", "law.concept.shape.sharp",
                     ActionNode::sequence({
                         ActionNode::set("surface.shape", PropertyValue(std::string("sharp"))),
                         ActionNode::set("compilation.output.web.surface.shape", PropertyValue(std::string("sharp")))
                     }), author);

    seedLexemeAndLaw(laws, "primary", "lexeme.web.primary", "law.concept.variant.primary",
                     ActionNode::sequence({
                         ActionNode::set("surface.variant", PropertyValue(std::string("primary"))),
                         ActionNode::set("compilation.output.web.surface.variant", PropertyValue(std::string("primary")))
                     }), author);

    seedLexemeAndLaw(laws, "secondary", "lexeme.web.secondary", "law.concept.variant.secondary",
                     ActionNode::sequence({
                         ActionNode::set("surface.variant", PropertyValue(std::string("secondary"))),
                         ActionNode::set("compilation.output.web.surface.variant", PropertyValue(std::string("secondary")))
                     }), author);

    seedLexemeAndLaw(laws, "hero", "lexeme.web.hero", "law.concept.variant.hero",
                     ActionNode::sequence({
                         ActionNode::set("surface.variant", PropertyValue(std::string("hero"))),
                         ActionNode::set("layout.align", PropertyValue(std::string("center"))),
                         ActionNode::set("compilation.output.web.surface.variant", PropertyValue(std::string("hero"))),
                         ActionNode::set("compilation.output.web.layout.align", PropertyValue(std::string("center")))
                     }), author);

    seedLexemeAndLaw(laws, "outline", "lexeme.web.outline", "law.concept.variant.outline",
                     ActionNode::sequence({
                         ActionNode::set("surface.variant", PropertyValue(std::string("outline"))),
                         ActionNode::set("compilation.output.web.surface.variant", PropertyValue(std::string("outline")))
                     }), author);

    seedLexemeAndLaw(laws, "subtle", "lexeme.web.subtle", "law.concept.elevation.subtle",
                     ActionNode::sequence({
                         ActionNode::set("surface.elevation", PropertyValue(std::string("subtle"))),
                         ActionNode::set("compilation.output.web.surface.elevation", PropertyValue(std::string("subtle")))
                     }), author);

    seedLexemeAndLaw(laws, "elevated", "lexeme.web.elevated", "law.concept.elevation.raised",
                     ActionNode::sequence({
                         ActionNode::set("surface.elevation", PropertyValue(std::string("raised"))),
                         ActionNode::set("compilation.output.web.surface.elevation", PropertyValue(std::string("raised")))
                     }), author);

    // 8. Relational Modifiers
    seedLexemeAndLaw(laws, "subordinate", "lexeme.web.subordinate", "law.concept.rel.subordinate",
                     ActionNode::sequence({
                         ActionNode::set("subordinate", PropertyValue(true)),
                         ActionNode::set("compilation.output.web.subordinate", PropertyValue(true))
                     }), author);

    seedLexemeAndLaw(laws, "prominent", "lexeme.web.prominent", "law.concept.rel.prominent",
                     ActionNode::sequence({
                         ActionNode::set("prominence", PropertyValue(std::string("prominent"))),
                         ActionNode::set("compilation.output.web.prominence", PropertyValue(std::string("prominent")))
                     }), author);

    seedLexemeAndLaw(laws, "quiet", "lexeme.web.quiet", "law.concept.rel.quiet",
                     ActionNode::sequence({
                         ActionNode::set("prominence", PropertyValue(std::string("quiet"))),
                         ActionNode::set("compilation.output.web.prominence", PropertyValue(std::string("quiet")))
                     }), author);
}

WebConcept WebDesignerMetalaws::compileTokens(const std::vector<std::string>& tokens,
                                             LawManager& laws,
                                             Singular& channelTarget,
                                             std::string* outError,
                                             const std::vector<Relation*>& extraRelations) {
    if (outError) outError->clear();

    for (const auto& rawToken : tokens) {
        std::string cleaned = toLower(trim(rawToken));
        if (cleaned.empty() || cleaned == "a" || cleaned == "an" || cleaned == "the") continue;

        Law* denotedLaw = nullptr;

        // 1. Search extra relations passed in (caller-supplied)
        for (const auto* rel : extraRelations) {
            if (!rel || rel->type != "denotes") continue;
            auto* lexeme = dynamic_cast<Singularity::Language::Lexeme*>(rel->a());
            if (lexeme && toLower(lexeme->getSymbol()) == cleaned) {
                if (auto* law = dynamic_cast<Law*>(rel->b())) {
                    denotedLaw = law;
                    break;
                }
            }
        }

        // 2. Search seeded denotation relations
        if (!denotedLaw) {
            for (const auto& rel : s_seededRelations) {
                if (!rel || rel->type != "denotes") continue;
                auto* lexeme = dynamic_cast<Singularity::Language::Lexeme*>(rel->a());
                if (lexeme && toLower(lexeme->getSymbol()) == cleaned) {
                    if (auto* law = dynamic_cast<Law*>(rel->b())) {
                        denotedLaw = law;
                        break;
                    }
                }
            }
        }

        // 3. Search world relations via Universe if available
        if (!denotedLaw) {
            for (const auto* rel : Universe::instance().relations()) {
                if (!rel || rel->type != "denotes") continue;
                auto* lexeme = dynamic_cast<Singularity::Language::Lexeme*>(rel->a());
                if (lexeme && toLower(lexeme->getSymbol()) == cleaned) {
                    if (auto* law = dynamic_cast<Law*>(rel->b())) {
                        denotedLaw = law;
                        break;
                    }
                }
            }
        }

        // If law found via denotation, ensure the target has the dynamic slots required and apply
        if (denotedLaw) {
            if (const auto* am = denotedLaw->actionModel()) {
                std::vector<PropertyPath> paths;
                am->collectPaths(paths);
                for (const auto& p : paths) {
                    PropertyValue existing;
                    if (!channelTarget.getDynamicProperty(p.fullId(), existing)) {
                        channelTarget.setDynamicProperty(p.fullId(), PropertyValue(std::string("")));
                    }
                }
            }

            auto res = denotedLaw->applyTo(channelTarget);
            if (res != Law::ApplicationResult::Applied) {
                if (outError) {
                    *outError = "Law " + denotedLaw->getIdentifier() + " failed to apply (res=" + std::to_string(static_cast<int>(res)) + ")";
                }
            }
        }
    }

    // Read the compiled output from the channel target into our ephemeral WebConcept view
    return WebConcept::readFromSingular(channelTarget);
}

WebConcept WebDesignerMetalaws::compilePhrase(const std::string& phrase,
                                             LawManager& laws,
                                             Singular& channelTarget,
                                             std::string* outError,
                                             const std::vector<Relation*>& extraRelations) {
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

    return compileTokens(tokens, laws, channelTarget, outError, extraRelations);
}

std::shared_ptr<Singular> WebDesignerMetalaws::createAuthoredElement(
    const std::string& identifier,
    const WebConcept& concept,
    Singular* author,
    std::vector<std::shared_ptr<Relation>>* outRelations) {

    auto obj = std::make_shared<Object>();
    obj->setObjectID(identifier);

    if (author) {
        // Record authorship property
        obj->setDynamicProperty("authored.by", PropertyValue(author->getIdentifier()));

        // Form first-class Earthcall provenance Relation connecting to the actual author
        auto authRel = std::make_shared<Relation>(WebRelationType::kAuthoredBy, *obj, *author, true, 1.0f);
        if (outRelations) {
            outRelations->push_back(authRel);
        }
        if (Universe::instance().hasRelationRegistrar()) {
            Universe::instance().addRelation(authRel);
        }
    }

    concept.applyToSingular(*obj);
    return obj;
}

} // namespace Web
} // namespace Foreign
} // namespace Singularity
