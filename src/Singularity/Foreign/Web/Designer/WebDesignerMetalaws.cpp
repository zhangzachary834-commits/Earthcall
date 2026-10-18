#include "Singularity/Foreign/Web/Designer/WebDesignerMetalaws.hpp"
#include "Singularity/Language/LanguageSystem.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ConditionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "Person/Person.hpp"
#include "json.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
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

} // namespace

void WebDesignerMetalaws::syncRegister(LawManager& /*laws*/) {
    // Built-in modality infrastructure initialization.
    // In accordance with Refusal 5, Refusal 7, and Sun's decree:
    // Runtime initialization does NOT manufacture human authorship, nor does it seed
    // unauthored Laws that bypass Law::applyTo().
    // Meaning is authored into the world by Persons via authorVocabulary().
}

void WebDesignerMetalaws::authorWord(Person& author,
                                     LawManager& laws,
                                     const std::string& symbol,
                                     const std::string& stableLexemeId,
                                     const std::string& lawId,
                                     ActionNode action,
                                     Formation* formation,
                                     std::vector<std::shared_ptr<Relation>>* outRelations) {
    // Retain Lexeme canonically in LanguageSystem so relation endpoint never dangles
    auto lexeme = Singularity::Language::LanguageSystem::instance().intern(symbol, stableLexemeId);

    Law* targetLaw = laws.find(lawId);
    if (!targetLaw) {
        auto law = std::make_shared<Law>(lawId, std::vector<Singular*>{&author});
        law->setLawIdentifier(lawId);
        // Sun Gate 2/3: Ordinary authored Laws use clamped authority level (0), never grantAuthority!
        law->setAuthorityLevel(0);
        law->setEnabled(true);
        law->setActionModel(std::move(action));
        laws.add(law);
        targetLaw = law.get();
    } else {
        // Sun Gate 5: Real Semantic Redefinition!
        // Author an updated Law version under Person stewardship, recording provenance
        static int s_redefCounter = 1;
        std::string versionedId = lawId + ".v" + std::to_string(s_redefCounter++);
        auto newLaw = std::make_shared<Law>(versionedId, std::vector<Singular*>{&author});
        newLaw->setLawIdentifier(versionedId);
        newLaw->setAuthorityLevel(0);
        newLaw->setEnabled(true);
        newLaw->setActionModel(std::move(action));
        newLaw->recordProvenance("redefined-from", *targetLaw, author);
        laws.add(newLaw);
        targetLaw = newLaw.get();
    }

    auto rel = std::make_shared<Relation>("denotes", *lexeme, *targetLaw, true, 1.0f);
    // Sun Gate 10: Single sink admission
    if (formation) {
        formation->relations().add(rel);
    } else if (Universe::instance().hasRelationRegistrar()) {
        Universe::instance().addRelation(rel);
    }
    if (outRelations) {
        outRelations->push_back(rel);
    }
}

bool WebDesignerMetalaws::loadAuthoredVocabularyFromJsonString(
    const std::string& jsonString,
    LawManager& laws,
    Formation* formation,
    std::vector<std::shared_ptr<Relation>>* outRelations) {
    try {
        nlohmann::json root = nlohmann::json::parse(jsonString);

        // Sun Gate 1 & 2: Provenance is restored from data; the importer is NOT the author!
        std::string defaultAuthorName;
        if (root.contains("author") && root["author"].is_string()) {
            defaultAuthorName = root["author"].get<std::string>();
        } else if (root.contains("injectedBy") && root["injectedBy"].contains("onBehalfOf")) {
            defaultAuthorName = root["injectedBy"]["onBehalfOf"].get<std::string>();
        }

        auto resolvePerson = [](const std::string& name) -> Person* {
            if (name.empty()) return nullptr;
            for (Singular* b : Universe::instance().beings()) {
                if (auto* p = dynamic_cast<Person*>(b)) {
                    if (p->matchesIdentifier(name) || p->getIdentifier() == name || p->getDisplayName() == name ||
                        (p->called() && p->called()->getSymbol() == name)) {
                        return p;
                    }
                }
            }
            return nullptr;
        };

        // 1. Canonical Earthcall Laws loading (if root contains "laws" array)
        if (root.contains("laws") && root["laws"].is_array()) {
            for (const auto& lj : root["laws"]) {
                auto law = Law::fromJson(lj);
                if (!law) continue;
                // Reattach authors from recorded authors array
                if (lj.contains("authors") && lj["authors"].is_array()) {
                    for (const auto& authVal : lj["authors"]) {
                        std::string authName = authVal.is_string() ? authVal.get<std::string>() : "";
                        if (Person* p = resolvePerson(authName)) {
                            law->addAuthor(*p);
                        }
                    }
                } else if (!defaultAuthorName.empty()) {
                    if (Person* p = resolvePerson(defaultAuthorName)) {
                        law->addAuthor(*p);
                    }
                }
                // Authored authority is clamped to kAuthoredCeiling (0)
                law->setAuthorityLevel(lj.value("authority", 0));
                laws.add(law);
            }
        }

        // 2. Canonical Earthcall Lexemes loading
        if (root.contains("lexemes") && root["lexemes"].is_array()) {
            for (const auto& lx : root["lexemes"]) {
                std::string sym = lx.value("symbol", "");
                std::string lid = lx.value("id", "lexeme.web." + sym);
                if (!sym.empty()) {
                    Singularity::Language::LanguageSystem::instance().intern(sym, lid);
                }
            }
        }

        // 3. Canonical Earthcall Relations loading
        if (root.contains("relations") && root["relations"].is_array()) {
            for (const auto& rj : root["relations"]) {
                std::string type = rj.value("type", "denotes");
                std::string srcId = rj.value("source", rj.value("a", ""));
                std::string tgtId = rj.value("target", rj.value("b", ""));
                Singular* src = nullptr;
                Singular* tgt = nullptr;
                for (Singular* b : Universe::instance().beings()) {
                    if (!b) continue;
                    if (b->getIdentifier() == srcId) src = b;
                    if (b->getIdentifier() == tgtId) tgt = b;
                }
                if (!tgt) {
                    tgt = laws.find(tgtId);
                }
                if (src && tgt) {
                    auto rel = std::make_shared<Relation>(type, *src, *tgt, rj.value("directed", true), rj.value("weight", 1.0f));
                    if (formation) formation->relations().add(rel);
                    else if (Universe::instance().hasRelationRegistrar()) Universe::instance().addRelation(rel);
                    if (outRelations) outRelations->push_back(rel);
                }
            }
        }

        // 4. Vocabulary items format
        if (root.contains("vocabulary") && root["vocabulary"].is_array()) {
            for (const auto& item : root["vocabulary"]) {
                std::string symbol = item.value("symbol", "");
                std::string lexemeId = item.value("lexemeId", "");
                std::string lawId = item.value("lawId", "");
                if (symbol.empty() || lawId.empty()) continue;
                if (lexemeId.empty()) {
                    lexemeId = "lexeme.web." + symbol;
                }

                std::string itemAuthor = item.value("author", defaultAuthorName);
                Person* steward = resolvePerson(itemAuthor);

                std::vector<ActionNode> subActions;
                if (item.contains("actions") && item["actions"].is_array()) {
                    for (const auto& act : item["actions"]) {
                        std::string path = act.value("path", "");
                        if (path.empty()) continue;

                        PropertyValue val;
                        if (act.contains("value")) {
                            const auto& v = act["value"];
                            if (v.is_boolean()) {
                                val = PropertyValue(v.get<bool>());
                            } else if (v.is_number()) {
                                val = PropertyValue(v.get<double>());
                            } else if (v.is_string()) {
                                val = PropertyValue(v.get<std::string>());
                            }
                        }

                        subActions.push_back(ActionNode::set(path, val));
                        subActions.push_back(ActionNode::set("compilation.output.web." + path, val));
                    }
                }

                ActionNode action = subActions.size() == 1 ? subActions[0] : ActionNode::sequence(subActions);

                auto lexeme = Singularity::Language::LanguageSystem::instance().intern(symbol, lexemeId);
                Law* targetLaw = laws.find(lawId);
                if (!targetLaw) {
                    std::vector<Singular*> authorList;
                    if (steward) {
                        authorList.push_back(steward);
                    }
                    // If steward is not found in the world, authorList is empty -> Law remains Unauthored!
                    auto law = std::make_shared<Law>(lawId, authorList);
                    law->setLawIdentifier(lawId);
                    // Authority clamped to kAuthoredCeiling (0)
                    law->setAuthorityLevel(0);
                    law->setEnabled(true);
                    law->setActionModel(std::move(action));
                    laws.add(law);
                    targetLaw = law.get();
                }

                auto rel = std::make_shared<Relation>("denotes", *lexeme, *targetLaw, true, 1.0f);
                if (formation) {
                    formation->relations().add(rel);
                } else if (Universe::instance().hasRelationRegistrar()) {
                    Universe::instance().addRelation(rel);
                }
                if (outRelations) {
                    outRelations->push_back(rel);
                }
            }
        }
        return true;
    } catch (...) {
        return false;
    }
}

bool WebDesignerMetalaws::loadAuthoredVocabulary(
    const std::string& fixturePath,
    LawManager& laws,
    Formation* formation,
    std::vector<std::shared_ptr<Relation>>* outRelations) {
    std::string path = fixturePath.empty() ? "saves/fixtures/web_vocabulary_seed.json" : fixturePath;
    std::ifstream in(path);
    if (!in.is_open()) {
        return false;
    }
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return loadAuthoredVocabularyFromJsonString(content, laws, formation, outRelations);
}

void WebDesignerMetalaws::authorVocabulary(Person& /*author*/,
                                           LawManager& laws,
                                           Formation* formation,
                                           std::vector<std::shared_ptr<Relation>>* outRelations) {
    // In accordance with Sun's decree & Gate 1/Gate 2:
    // C++ does not contain authoritative semantic definitions of web vocabulary.
    // Meaning is loaded from authored Earthcall data fixtures, preserving human stewardship.
    if (!loadAuthoredVocabulary("", laws, formation, outRelations)) {
        // Fallback if running in directory where fixture is relative
        loadAuthoredVocabulary("../saves/fixtures/web_vocabulary_seed.json", laws, formation, outRelations);
    }
}

void WebDesignerMetalaws::loadVocabulary(LawManager& laws,
                                         Formation* formation,
                                         std::vector<std::shared_ptr<Relation>>* outRelations) {
    if (!loadAuthoredVocabulary("", laws, formation, outRelations)) {
        loadAuthoredVocabulary("../saves/fixtures/web_vocabulary_seed.json", laws, formation, outRelations);
    }
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

    // For identical symbols, the most recently added denotes relation takes precedence (repointing)
    std::unordered_map<std::string, Law*> latestMapping;
    for (const auto& m : mappings) {
        latestMapping[m.symbol] = m.law;
    }
    mappings.clear();
    for (const auto& [sym, law] : latestMapping) {
        mappings.push_back({sym, law});
    }

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
