#include "Singularity/Foreign/Web/Designer/WebDesignerMetalaws.hpp"
#include "Singularity/Language/LanguageSystem.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ConditionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "Person/Person.hpp"
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

void seedAuthoredWord(Person& author,
                      LawManager& laws,
                      Formation* formation,
                      const std::string& symbol,
                      const std::string& stableId,
                      const std::string& lawId,
                      ActionNode action,
                      std::vector<std::shared_ptr<Relation>>* outRelations = nullptr) {
    // Retain Lexeme canonically in LanguageSystem so relation endpoint never dangles
    auto lexeme = Singularity::Language::LanguageSystem::instance().intern(symbol, stableId);

    // Reuse existing Law if already added, allowing multiple Lexemes (synonyms) to denote the same Law
    Law* targetLaw = laws.find(lawId);
    if (!targetLaw) {
        auto law = std::make_shared<Law>(lawId, std::vector<Singular*>{&author});
        law->setLawIdentifier(lawId);
        law->grantAuthority(10);
        law->setEnabled(true);
        law->setActionModel(std::move(action));
        laws.add(law);
        targetLaw = law.get();
    }

    auto rel = std::make_shared<Relation>("denotes", *lexeme, *targetLaw, true, 1.0f);
    if (formation) {
        formation->relations().add(rel);
    }
    if (Universe::instance().hasRelationRegistrar()) {
        Universe::instance().addRelation(rel);
    }
    if (outRelations) {
        outRelations->push_back(rel);
    }
}

} // namespace

void WebDesignerMetalaws::syncRegister(LawManager& /*laws*/) {
    // Built-in modality infrastructure initialization.
    // In accordance with Refusal 5, Refusal 7, and Sun's decree:
    // Runtime initialization does NOT manufacture human authorship, nor does it seed
    // unauthored Laws that bypass Law::applyTo().
    // Meaning is authored into the world by Persons via authorVocabulary().
}

void WebDesignerMetalaws::authorVocabulary(Person& author,
                                           LawManager& laws,
                                           Formation* formation,
                                           std::vector<std::shared_ptr<Relation>>* outRelations) {
    auto seed = [&](const std::string& symbol, const std::string& stableId, const std::string& lawId, ActionNode action) {
        seedAuthoredWord(author, laws, formation, symbol, stableId, lawId, std::move(action), outRelations);
    };
    // 1. Semantic Roles (Atomic mapping — zero procedural aesthetic decree)
    seed("page", "lexeme.web.page", "law.concept.role.page",
                     ActionNode::sequence({
                         ActionNode::set("role", PropertyValue(std::string("page"))),
                         ActionNode::set("compilation.output.web.role", PropertyValue(std::string("page")))
                     }));

    seed("section", "lexeme.web.section", "law.concept.role.section",
                     ActionNode::sequence({
                         ActionNode::set("role", PropertyValue(std::string("section"))),
                         ActionNode::set("compilation.output.web.role", PropertyValue(std::string("section")))
                     }));

    // Navigation: sets role = navigation. Does NOT force layout.flow = horizontal!
    seed("navigation", "lexeme.web.navigation", "law.concept.role.navigation",
                     ActionNode::sequence({
                         ActionNode::set("role", PropertyValue(std::string("navigation"))),
                         ActionNode::set("compilation.output.web.role", PropertyValue(std::string("navigation")))
                     }));

    seed("nav", "lexeme.web.nav", "law.concept.role.navigation",
                     ActionNode::sequence({
                         ActionNode::set("role", PropertyValue(std::string("navigation"))),
                         ActionNode::set("compilation.output.web.role", PropertyValue(std::string("navigation")))
                     }));

    seed("heading", "lexeme.web.heading", "law.concept.role.heading",
                     ActionNode::sequence({
                         ActionNode::set("role", PropertyValue(std::string("heading"))),
                         ActionNode::set("level", PropertyValue(2.0)),
                         ActionNode::set("compilation.output.web.role", PropertyValue(std::string("heading"))),
                         ActionNode::set("compilation.output.web.level", PropertyValue(2.0))
                     }));

    seed("title", "lexeme.web.title", "law.concept.role.heading",
                     ActionNode::sequence({
                         ActionNode::set("role", PropertyValue(std::string("heading"))),
                         ActionNode::set("level", PropertyValue(1.0)),
                         ActionNode::set("compilation.output.web.role", PropertyValue(std::string("heading"))),
                         ActionNode::set("compilation.output.web.level", PropertyValue(1.0))
                     }));

    // Subtitle: sets level = 3. Does NOT force muted tone!
    seed("subtitle", "lexeme.web.subtitle", "law.concept.role.subtitle",
                     ActionNode::sequence({
                         ActionNode::set("role", PropertyValue(std::string("heading"))),
                         ActionNode::set("level", PropertyValue(3.0)),
                         ActionNode::set("compilation.output.web.role", PropertyValue(std::string("heading"))),
                         ActionNode::set("compilation.output.web.level", PropertyValue(3.0))
                     }));

    seed("paragraph", "lexeme.web.paragraph", "law.concept.role.paragraph",
                     ActionNode::sequence({
                         ActionNode::set("role", PropertyValue(std::string("paragraph"))),
                         ActionNode::set("compilation.output.web.role", PropertyValue(std::string("paragraph")))
                     }));

    seed("text", "lexeme.web.text", "law.concept.role.paragraph",
                     ActionNode::sequence({
                         ActionNode::set("role", PropertyValue(std::string("paragraph"))),
                         ActionNode::set("compilation.output.web.role", PropertyValue(std::string("paragraph")))
                     }));

    // Button: sets role = action. Does NOT force surface.shape = rounded!
    seed("button", "lexeme.web.button", "law.concept.role.action",
                     ActionNode::sequence({
                         ActionNode::set("role", PropertyValue(std::string("action"))),
                         ActionNode::set("compilation.output.web.role", PropertyValue(std::string("action")))
                     }));

    seed("action", "lexeme.web.action", "law.concept.role.action",
                     ActionNode::sequence({
                         ActionNode::set("role", PropertyValue(std::string("action"))),
                         ActionNode::set("compilation.output.web.role", PropertyValue(std::string("action")))
                     }));

    seed("link", "lexeme.web.link", "law.concept.role.link",
                     ActionNode::sequence({
                         ActionNode::set("role", PropertyValue(std::string("action"))),
                         ActionNode::set("compilation.output.web.role", PropertyValue(std::string("action")))
                     }));

    seed("container", "lexeme.web.container", "law.concept.role.container",
                     ActionNode::sequence({
                         ActionNode::set("role", PropertyValue(std::string("container"))),
                         ActionNode::set("compilation.output.web.role", PropertyValue(std::string("container")))
                     }));

    // Card: sets role = container, surface.variant = card. Does NOT force rounded or subtle elevation!
    seed("card", "lexeme.web.card", "law.concept.role.card",
                     ActionNode::sequence({
                         ActionNode::set("role", PropertyValue(std::string("container"))),
                         ActionNode::set("surface.variant", PropertyValue(std::string("card"))),
                         ActionNode::set("compilation.output.web.role", PropertyValue(std::string("container"))),
                         ActionNode::set("compilation.output.web.surface.variant", PropertyValue(std::string("card")))
                     }));

    // List: sets role = list. Does NOT force layout.flow = vertical!
    seed("list", "lexeme.web.list", "law.concept.role.list",
                     ActionNode::sequence({
                         ActionNode::set("role", PropertyValue(std::string("list"))),
                         ActionNode::set("compilation.output.web.role", PropertyValue(std::string("list")))
                     }));

    seed("item", "lexeme.web.item", "law.concept.role.item",
                     ActionNode::sequence({
                         ActionNode::set("role", PropertyValue(std::string("item"))),
                         ActionNode::set("compilation.output.web.role", PropertyValue(std::string("item")))
                     }));

    seed("media", "lexeme.web.media", "law.concept.role.media",
                     ActionNode::sequence({
                         ActionNode::set("role", PropertyValue(std::string("media"))),
                         ActionNode::set("compilation.output.web.role", PropertyValue(std::string("media")))
                     }));

    seed("image", "lexeme.web.image", "law.concept.role.media",
                     ActionNode::sequence({
                         ActionNode::set("role", PropertyValue(std::string("media"))),
                         ActionNode::set("compilation.output.web.role", PropertyValue(std::string("media")))
                     }));

    seed("quote", "lexeme.web.quote", "law.concept.role.quote",
                     ActionNode::sequence({
                         ActionNode::set("role", PropertyValue(std::string("quote"))),
                         ActionNode::set("compilation.output.web.role", PropertyValue(std::string("quote")))
                     }));

    seed("divider", "lexeme.web.divider", "law.concept.role.divider",
                     ActionNode::sequence({
                         ActionNode::set("role", PropertyValue(std::string("divider"))),
                         ActionNode::set("compilation.output.web.role", PropertyValue(std::string("divider")))
                     }));

    // 2. Flows & Layout
    seed("vertical", "lexeme.web.vertical", "law.concept.flow.vertical",
                     ActionNode::sequence({
                         ActionNode::set("layout.flow", PropertyValue(std::string("vertical"))),
                         ActionNode::set("compilation.output.web.layout.flow", PropertyValue(std::string("vertical")))
                     }));

    seed("column", "lexeme.web.column", "law.concept.flow.vertical",
                     ActionNode::sequence({
                         ActionNode::set("layout.flow", PropertyValue(std::string("vertical"))),
                         ActionNode::set("compilation.output.web.layout.flow", PropertyValue(std::string("vertical")))
                     }));

    seed("horizontal", "lexeme.web.horizontal", "law.concept.flow.horizontal",
                     ActionNode::sequence({
                         ActionNode::set("layout.flow", PropertyValue(std::string("horizontal"))),
                         ActionNode::set("compilation.output.web.layout.flow", PropertyValue(std::string("horizontal")))
                     }));

    seed("row", "lexeme.web.row", "law.concept.flow.horizontal",
                     ActionNode::sequence({
                         ActionNode::set("layout.flow", PropertyValue(std::string("horizontal"))),
                         ActionNode::set("compilation.output.web.layout.flow", PropertyValue(std::string("horizontal")))
                     }));

    seed("grid", "lexeme.web.grid", "law.concept.flow.grid",
                     ActionNode::sequence({
                         ActionNode::set("layout.flow", PropertyValue(std::string("grid"))),
                         ActionNode::set("compilation.output.web.layout.flow", PropertyValue(std::string("grid")))
                     }));

    // 3. Alignments
    seed("start", "lexeme.web.align.start", "law.concept.align.start",
                     ActionNode::sequence({
                         ActionNode::set("layout.align", PropertyValue(std::string("start"))),
                         ActionNode::set("compilation.output.web.layout.align", PropertyValue(std::string("start")))
                     }));

    seed("center", "lexeme.web.align.center", "law.concept.align.center",
                     ActionNode::sequence({
                         ActionNode::set("layout.align", PropertyValue(std::string("center"))),
                         ActionNode::set("compilation.output.web.layout.align", PropertyValue(std::string("center")))
                     }));

    seed("centered", "lexeme.web.align.centered", "law.concept.align.center",
                     ActionNode::sequence({
                         ActionNode::set("layout.align", PropertyValue(std::string("center"))),
                         ActionNode::set("compilation.output.web.layout.align", PropertyValue(std::string("center")))
                     }));

    seed("end", "lexeme.web.align.end", "law.concept.align.end",
                     ActionNode::sequence({
                         ActionNode::set("layout.align", PropertyValue(std::string("end"))),
                         ActionNode::set("compilation.output.web.layout.align", PropertyValue(std::string("end")))
                     }));

    seed("stretch", "lexeme.web.align.stretch", "law.concept.align.stretch",
                     ActionNode::sequence({
                         ActionNode::set("layout.align", PropertyValue(std::string("stretch"))),
                         ActionNode::set("compilation.output.web.layout.align", PropertyValue(std::string("stretch")))
                     }));

    // 4. Spacing
    seed("tight", "lexeme.web.spacing.tight", "law.concept.spacing.tight",
                     ActionNode::sequence({
                         ActionNode::set("layout.spacing", PropertyValue(std::string("tight"))),
                         ActionNode::set("compilation.output.web.layout.spacing", PropertyValue(std::string("tight")))
                     }));

    seed("normal", "lexeme.web.spacing.normal", "law.concept.spacing.normal",
                     ActionNode::sequence({
                         ActionNode::set("layout.spacing", PropertyValue(std::string("normal"))),
                         ActionNode::set("compilation.output.web.layout.spacing", PropertyValue(std::string("normal")))
                     }));

    seed("relaxed", "lexeme.web.spacing.relaxed", "law.concept.spacing.relaxed",
                     ActionNode::sequence({
                         ActionNode::set("layout.spacing", PropertyValue(std::string("relaxed"))),
                         ActionNode::set("compilation.output.web.layout.spacing", PropertyValue(std::string("relaxed")))
                     }));

    seed("generous", "lexeme.web.spacing.generous", "law.concept.spacing.generous",
                     ActionNode::sequence({
                         ActionNode::set("layout.spacing", PropertyValue(std::string("generous"))),
                         ActionNode::set("compilation.output.web.layout.spacing", PropertyValue(std::string("generous")))
                     }));

    // 5. Scales
    seed("small", "lexeme.web.small", "law.concept.size.small",
                     ActionNode::sequence({
                         ActionNode::set("scale", PropertyValue(std::string("small"))),
                         ActionNode::set("compilation.output.web.scale.size", PropertyValue(std::string("small")))
                     }));

    seed("large", "lexeme.web.large", "law.concept.size.large",
                     ActionNode::sequence({
                         ActionNode::set("scale", PropertyValue(std::string("large"))),
                         ActionNode::set("compilation.output.web.scale.size", PropertyValue(std::string("large")))
                     }));

    seed("huge", "lexeme.web.huge", "law.concept.size.huge",
                     ActionNode::sequence({
                         ActionNode::set("scale", PropertyValue(std::string("huge"))),
                         ActionNode::set("compilation.output.web.scale.size", PropertyValue(std::string("huge")))
                     }));

    // 6. Typography
    seed("bold", "lexeme.web.bold", "law.concept.weight.bold",
                     ActionNode::sequence({
                         ActionNode::set("typography.weight", PropertyValue(std::string("bold"))),
                         ActionNode::set("compilation.output.web.typography.weight", PropertyValue(std::string("bold")))
                     }));

    seed("light", "lexeme.web.light", "law.concept.weight.light",
                     ActionNode::sequence({
                         ActionNode::set("typography.weight", PropertyValue(std::string("light"))),
                         ActionNode::set("compilation.output.web.typography.weight", PropertyValue(std::string("light")))
                     }));

    seed("muted", "lexeme.web.muted", "law.concept.tone.muted",
                     ActionNode::sequence({
                         ActionNode::set("typography.tone", PropertyValue(std::string("muted"))),
                         ActionNode::set("compilation.output.web.typography.tone", PropertyValue(std::string("muted")))
                     }));

    seed("lead", "lexeme.web.lead", "law.concept.tone.prominent",
                     ActionNode::sequence({
                         ActionNode::set("scale", PropertyValue(std::string("large"))),
                         ActionNode::set("typography.tone", PropertyValue(std::string("prominent"))),
                         ActionNode::set("compilation.output.web.scale.size", PropertyValue(std::string("large"))),
                         ActionNode::set("compilation.output.web.typography.tone", PropertyValue(std::string("prominent")))
                     }));

    // 7. Surface & Presentation
    seed("rounded", "lexeme.web.rounded", "law.concept.shape.rounded",
                     ActionNode::sequence({
                         ActionNode::set("surface.shape", PropertyValue(std::string("rounded"))),
                         ActionNode::set("compilation.output.web.surface.shape", PropertyValue(std::string("rounded")))
                     }));

    seed("pill", "lexeme.web.pill", "law.concept.shape.pill",
                     ActionNode::sequence({
                         ActionNode::set("surface.shape", PropertyValue(std::string("pill"))),
                         ActionNode::set("compilation.output.web.surface.shape", PropertyValue(std::string("pill")))
                     }));

    seed("sharp", "lexeme.web.sharp", "law.concept.shape.sharp",
                     ActionNode::sequence({
                         ActionNode::set("surface.shape", PropertyValue(std::string("sharp"))),
                         ActionNode::set("compilation.output.web.surface.shape", PropertyValue(std::string("sharp")))
                     }));

    seed("primary", "lexeme.web.primary", "law.concept.variant.primary",
                     ActionNode::sequence({
                         ActionNode::set("surface.variant", PropertyValue(std::string("primary"))),
                         ActionNode::set("compilation.output.web.surface.variant", PropertyValue(std::string("primary")))
                     }));

    seed("secondary", "lexeme.web.secondary", "law.concept.variant.secondary",
                     ActionNode::sequence({
                         ActionNode::set("surface.variant", PropertyValue(std::string("secondary"))),
                         ActionNode::set("compilation.output.web.surface.variant", PropertyValue(std::string("secondary")))
                     }));

    // Hero: sets surface.variant = hero. Does NOT force layout.align = center!
    seed("hero", "lexeme.web.hero", "law.concept.variant.hero",
                     ActionNode::sequence({
                         ActionNode::set("surface.variant", PropertyValue(std::string("hero"))),
                         ActionNode::set("compilation.output.web.surface.variant", PropertyValue(std::string("hero")))
                     }));

    seed("outline", "lexeme.web.outline", "law.concept.variant.outline",
                     ActionNode::sequence({
                         ActionNode::set("surface.variant", PropertyValue(std::string("outline"))),
                         ActionNode::set("compilation.output.web.surface.variant", PropertyValue(std::string("outline")))
                     }));

    seed("subtle", "lexeme.web.subtle", "law.concept.elevation.subtle",
                     ActionNode::sequence({
                         ActionNode::set("surface.elevation", PropertyValue(std::string("subtle"))),
                         ActionNode::set("compilation.output.web.surface.elevation", PropertyValue(std::string("subtle")))
                     }));

    seed("elevated", "lexeme.web.elevated", "law.concept.elevation.raised",
                     ActionNode::sequence({
                         ActionNode::set("surface.elevation", PropertyValue(std::string("raised"))),
                         ActionNode::set("compilation.output.web.surface.elevation", PropertyValue(std::string("raised")))
                     }));

    // 8. Relational Modifiers
    seed("subordinate", "lexeme.web.subordinate", "law.concept.rel.subordinate",
                     ActionNode::sequence({
                         ActionNode::set("subordinate", PropertyValue(true)),
                         ActionNode::set("compilation.output.web.subordinate", PropertyValue(true))
                     }));

    seed("prominent", "lexeme.web.prominent", "law.concept.rel.prominent",
                     ActionNode::sequence({
                         ActionNode::set("prominence", PropertyValue(std::string("prominent"))),
                         ActionNode::set("compilation.output.web.prominence", PropertyValue(std::string("prominent")))
                     }));

    seed("quiet", "lexeme.web.quiet", "law.concept.rel.quiet",
                     ActionNode::sequence({
                         ActionNode::set("prominence", PropertyValue(std::string("quiet"))),
                         ActionNode::set("compilation.output.web.prominence", PropertyValue(std::string("quiet")))
                     }));
}

WebConcept WebDesignerMetalaws::compilePhrase(const std::string& phrase,
                                             LawManager& laws,
                                             Singular& channelTarget,
                                             std::string* outError,
                                             const std::vector<Relation*>& extraRelations) {
    if (outError) outError->clear();

    // 1. Gather all candidate "denotes" Relations linking in-world Lexemes to Laws
    // No private process-global hidden fallback; gathers strictly from in-world relations.
    struct LexemeMapping {
        std::string symbol;
        Law* law = nullptr;
    };
    std::vector<LexemeMapping> mappings;

    auto collectFrom = [&](const std::vector<Relation*>& rels) {
        for (const auto* rel : rels) {
            if (!rel || rel->type != "denotes") continue;
            auto* lex = dynamic_cast<Singularity::Language::Lexeme*>(rel->a());
            auto* law = dynamic_cast<Law*>(rel->b());
            if (lex && law) {
                mappings.push_back({toLower(trim(lex->getSymbol())), law});
            }
        }
    };

    collectFrom(extraRelations);
    collectFrom(Universe::instance().relations());

    // Sort mappings by symbol length descending (greedy longest-match)
    std::sort(mappings.begin(), mappings.end(), [](const LexemeMapping& a, const LexemeMapping& b) {
        return a.symbol.size() > b.symbol.size();
    });

    std::string lowerPhrase = toLower(phrase);
    size_t pos = 0;
    const size_t len = phrase.size();

    auto isDelimiter = [](char c) {
        return std::isspace(static_cast<unsigned char>(c)) || c == ',' || c == ';' || c == '.';
    };

    while (pos < len) {
        // Skip leading whitespace / delimiters
        while (pos < len && isDelimiter(phrase[pos])) {
            pos++;
        }
        if (pos >= len) break;

        // Try greedy longest match against registered Lexemes
        bool matchedLexeme = false;
        for (const auto& m : mappings) {
            if (m.symbol.empty()) continue;
            size_t symLen = m.symbol.size();
            if (pos + symLen <= len) {
                if (lowerPhrase.compare(pos, symLen, m.symbol) == 0) {
                    // Check word boundary so "nav" does not match prefix of "navigate"
                    bool atWordBoundary = (pos + symLen == len) || isDelimiter(phrase[pos + symLen]);
                    if (atWordBoundary) {
                        Law* denotedLaw = m.law;
                        if (denotedLaw) {
                            if (denotedLaw->isAuthored()) {
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
                            }

                            // Mandate 1: All Laws MUST execute strictly via Law::applyTo().
                            // Unauthored laws refuse (ApplicationResult::Unauthored); NO ActionModel backdoor!
                            auto res = denotedLaw->applyTo(channelTarget);
                            if (res != Law::ApplicationResult::Applied) {
                                if (outError) {
                                    *outError = "Law " + denotedLaw->getIdentifier() + " refused to apply (res=" +
                                                (res == Law::ApplicationResult::Unauthored ? "Unauthored" : std::to_string(static_cast<int>(res))) + ")";
                                }
                            }
                        }
                        pos += symLen;
                        matchedLexeme = true;
                        break;
                    }
                }
            }
        }

        if (matchedLexeme) continue;

        // No Lexeme matched: extract next token
        size_t nextDelim = pos;
        while (nextDelim < len && !isDelimiter(phrase[nextDelim])) {
            nextDelim++;
        }
        std::string token = lowerPhrase.substr(pos, nextDelim - pos);
        pos = nextDelim;

        // Mandate 3: Unmatched text remains open / unresolved; do NOT assign role by fiat!
        if (!token.empty() && token != "a" && token != "an" && token != "the" && token != "with" && token != "and") {
            channelTarget.setDynamicProperty("unresolved.concept." + token, PropertyValue(true));
        }
    }

    return WebConcept::readFromSingular(channelTarget);
}

WebConcept WebDesignerMetalaws::compileTokens(const std::vector<std::string>& tokens,
                                             LawManager& laws,
                                             Singular& channelTarget,
                                             std::string* outError,
                                             const std::vector<Relation*>& extraRelations) {
    std::string phrase;
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (i > 0) phrase += " ";
        phrase += tokens[i];
    }
    return compilePhrase(phrase, laws, channelTarget, outError, extraRelations);
}

Singular& WebDesignerMetalaws::applyAuthoredConcept(
    Singular& bearer,
    const WebConcept& concept,
    Person* author,
    std::vector<std::shared_ptr<Relation>>* outRelations) {

    if (author) {
        // Form first-class Earthcall provenance Relation connecting to the actual author Person.
        // The Relation is the SOLE source of truth — NO shadow string property!
        auto authRel = std::make_shared<Relation>(WebRelationType::kAuthoredBy, bearer, *author, true, 1.0f);
        if (outRelations) {
            outRelations->push_back(authRel);
        }
        if (Universe::instance().hasRelationRegistrar()) {
            Universe::instance().addRelation(authRel);
        }
    }

    concept.applyToSingular(bearer);
    return bearer;
}

std::shared_ptr<Singular> WebDesignerMetalaws::createAuthoredElement(
    const std::string& identifier,
    const WebConcept& concept,
    Person* author,
    std::vector<std::shared_ptr<Relation>>* outRelations) {

    auto obj = std::make_shared<Object>();
    obj->setObjectID(identifier);
    applyAuthoredConcept(*obj, concept, author, outRelations);
    return obj;
}

} // namespace Web
} // namespace Foreign
} // namespace Singularity
