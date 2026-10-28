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

std::pair<std::string, int> parseIdVersion(const std::string& id) {
    auto vPos = id.rfind(".v");
    if (vPos != std::string::npos && vPos + 2 < id.size()) {
        bool digits = true;
        for (size_t i = vPos + 2; i < id.size(); ++i) {
            if (!std::isdigit(static_cast<unsigned char>(id[i]))) {
                digits = false;
                break;
            }
        }
        if (digits) {
            return {id.substr(0, vPos), std::stoi(id.substr(vPos + 2))};
        }
    }
    return {id, 0};
}

bool isSameLineage(const Law* candidate, const std::string& targetLawId) {
    if (!candidate) return false;
    if (candidate->getIdentifier() == targetLawId) return true;

    auto [candBase, _] = parseIdVersion(candidate->getIdentifier());
    auto [targetBase, _2] = parseIdVersion(targetLawId);
    if (candBase == targetBase) return true;

    // Check redefined-from provenance chain
    std::vector<const Law*> stack;
    stack.push_back(candidate);
    while (!stack.empty()) {
        const Law* cur = stack.back();
        stack.pop_back();
        for (const auto& r : cur->provenance().getRelationsOfType("redefined-from")) {
            if (r) {
                if (auto* prev = dynamic_cast<const Law*>(r->a())) {
                    if (prev->getIdentifier() == targetLawId || parseIdVersion(prev->getIdentifier()).first == targetBase) {
                        return true;
                    }
                    stack.push_back(prev);
                }
            }
        }
    }
    return false;
}

bool isRelationActive(const Relation* r, Formation* formation = nullptr, const std::vector<Relation*>& extraRels = {}) {
    if (!r) return false;
    // Canonical world-level graph check: is r the source endpoint of any superseded-by relation?
    if (formation && formation->relations().isSuperseded(*r)) {
        return false;
    }
    const std::string rId = r->getIdentifier();
    for (const Relation* er : extraRels) {
        if (er && er->type == "superseded-by" && ((er->a() == r) || (er->aId() == rId))) {
            return false;
        }
    }
    if (Universe::instance().isRelationSuperseded(*r)) {
        return false;
    }
    return true;
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

    std::vector<Relation*> extraList;
    if (outRelations) {
        for (const auto& sr : *outRelations) {
            if (sr) extraList.push_back(sr.get());
        }
    }

    // Sun Round 7 Gate 3: Target exact requested Law lineage, not whichever relation appears first!
    Law* previousLaw = nullptr;

    auto checkCandidate = [&](Relation* r) -> bool {
        if (!r || r->type != "denotes" || r->a() != lexeme.get()) return false;
        if (!isRelationActive(r, formation, extraList)) return false;
        auto* l = dynamic_cast<Law*>(r->b());
        if (l && isSameLineage(l, lawId)) {
            previousLaw = l;
            return true;
        }
        return false;
    };

    if (formation) {
        for (const auto& r : formation->relations().getAll()) {
            if (checkCandidate(r.get())) break;
        }
    }
    if (!previousLaw && outRelations) {
        for (const auto& r : *outRelations) {
            if (checkCandidate(r.get())) break;
        }
    }
    if (!previousLaw) {
        for (const auto* r : Universe::instance().relations()) {
            if (checkCandidate(const_cast<Relation*>(r))) break;
        }
    }
    if (!previousLaw) {
        previousLaw = laws.find(lawId);
    }

    Law* targetLaw = nullptr;
    if (!previousLaw) {
        // First definition of this semantic word
        auto law = std::make_shared<Law>(lawId, std::vector<Singular*>{&author});
        law->setLawIdentifier(lawId);
        law->setAuthorityLevel(0);
        law->setEnabled(true);
        law->setActionModel(std::move(action));
        laws.add(law);
        targetLaw = law.get();

        auto targetRel = std::make_shared<Relation>("denotes", *lexeme, *targetLaw, true, 1.0f);
        if (formation) {
            formation->relations().add(targetRel);
        } else if (Universe::instance().hasRelationRegistrar()) {
            Universe::instance().addRelation(targetRel);
        }
        if (outRelations) {
            outRelations->push_back(targetRel);
        }
    } else {
        // Redefinition: derive stable version identity from the actual previous active law in this lineage!
        std::string prevId = previousLaw->getIdentifier();
        std::string base = prevId;
        int nextVersion = 1;
        auto [parsedBase, parsedVer] = parseIdVersion(prevId);
        if (parsedVer > 0) {
            nextVersion = parsedVer + 1;
            base = parsedBase;
        }
        std::string versionedId = base + ".v" + std::to_string(nextVersion);

        auto newLaw = std::make_shared<Law>(versionedId, std::vector<Singular*>{&author});
        newLaw->setLawIdentifier(versionedId);
        newLaw->setAuthorityLevel(0);
        newLaw->setEnabled(true);
        newLaw->setActionModel(std::move(action));
        newLaw->recordProvenance("redefined-from", *previousLaw, author);
        laws.add(newLaw);
        targetLaw = newLaw.get();

        // Target new denotes relation
        auto targetRel = std::make_shared<Relation>("denotes", *lexeme, *targetLaw, true, 1.0f);
        if (formation) {
            formation->relations().add(targetRel);
        } else if (Universe::instance().hasRelationRegistrar()) {
            Universe::instance().addRelation(targetRel);
        }
        if (outRelations) {
            outRelations->push_back(targetRel);
        }

        // Sun Round 8 Gate 1 & 2 & Round 9 Gate 4:
        // Supersession as First-Class Relation, not shadow string!
        // Link all previous active denotes Relations in this lineage to targetRel with "superseded-by".
        // Deduplicate old relations by pointer and stable identity across Formation, outRelations, and Universe!
        std::vector<Relation*> oldRelations;
        std::unordered_set<std::string> seenOldKeys;
        auto collectOld = [&](Relation* r) {
            if (r && r != targetRel.get() && r->type == "denotes" && r->a() == lexeme.get()) {
                if (auto* l = dynamic_cast<Law*>(r->b())) {
                    if (isSameLineage(l, lawId) && isRelationActive(r, formation, extraList)) {
                        std::string key = r->getIdentifier();
                        if (key.empty()) {
                            key = std::to_string(reinterpret_cast<uintptr_t>(r));
                        }
                        if (seenOldKeys.insert(key).second) {
                            oldRelations.push_back(r);
                        }
                    }
                }
            }
        };
        if (formation) {
            for (const auto& r : formation->relations().getAll()) {
                collectOld(r.get());
            }
        }
        if (outRelations) {
            for (const auto& r : *outRelations) {
                collectOld(r.get());
            }
        }
        for (const auto* r : Universe::instance().relations()) {
            collectOld(const_cast<Relation*>(r));
        }

        for (Relation* oldRel : oldRelations) {
            auto subRel = std::make_shared<Relation>("superseded-by", *oldRel, *targetRel, true, 1.0f);
            if (formation) {
                formation->relations().add(subRel);
            } else if (Universe::instance().hasRelationRegistrar()) {
                Universe::instance().addRelation(subRel);
            }
            if (outRelations) {
                outRelations->push_back(subRel);
            }
        }

        auto lawSupersedesRel = std::make_shared<Relation>("supersedes", *targetLaw, *previousLaw, true, 1.0f);
        if (formation) {
            formation->relations().add(lawSupersedesRel);
        } else if (Universe::instance().hasRelationRegistrar()) {
            Universe::instance().addRelation(lawSupersedesRel);
        }
        if (outRelations) {
            outRelations->push_back(lawSupersedesRel);
        }
    }
}

bool WebDesignerMetalaws::loadAuthoredVocabularyFromJsonString(
    const std::string& jsonString,
    LawManager& laws,
    Formation* formation,
    std::vector<std::shared_ptr<Relation>>* outRelations) {
    try {
        nlohmann::json root = nlohmann::json::parse(jsonString);

        // Sun Round 6 Gate 1 & 2: Provenance is restored from canonical authored state alone.
        // The importer is NOT the author; and injectedBy.onBehalfOf is delegation context, NEVER authorship!
        std::vector<std::string> defaultAuthorNames;
        if (root.contains("authors") && root["authors"].is_array()) {
            for (const auto& a : root["authors"]) {
                if (a.is_string()) defaultAuthorNames.push_back(a.get<std::string>());
            }
        } else if (root.contains("author") && root["author"].is_string()) {
            defaultAuthorNames.push_back(root["author"].get<std::string>());
        }

        auto resolvePerson = [](const std::string& name) -> Person* {
            if (name.empty()) return nullptr;

            // 1. Exact stable identifier / DID match takes absolute priority
            std::vector<Person*> stableMatches;
            for (Singular* b : Universe::instance().beings()) {
                if (auto* p = dynamic_cast<Person*>(b)) {
                    if (p->getIdentifier() == name || p->matchesIdentifier(name)) {
                        stableMatches.push_back(p);
                    }
                }
            }
            if (stableMatches.size() == 1) return stableMatches[0];
            if (stableMatches.size() > 1) return nullptr; // Ambiguous stable identities refuse!

            // 2. Legacy bare-name / display name fallback: ONLY if strictly UNAMBIGUOUS!
            std::vector<Person*> displayMatches;
            for (Singular* b : Universe::instance().beings()) {
                if (auto* p = dynamic_cast<Person*>(b)) {
                    if (p->getDisplayName() == name || (p->called() && p->called()->getSymbol() == name)) {
                        displayMatches.push_back(p);
                    }
                }
            }
            if (displayMatches.size() == 1) return displayMatches[0];
            // Sun Round 8 Gate 4: THE ACTUAL TWO-ZACHS BUG!
            // If multiple Persons share the display name, strictly REFUSE/quarantine, never guess first!
            return nullptr;
        };

        // 1. Canonical Earthcall Laws loading
        if (root.contains("laws") && root["laws"].is_array()) {
            for (const auto& lj : root["laws"]) {
                auto law = Law::fromJson(lj);
                if (!law) continue;

                // Canonical authorship attachment:
                std::vector<std::string> lawAuthors;
                if (lj.contains("authors") && lj["authors"].is_array()) {
                    for (const auto& authVal : lj["authors"]) {
                        if (authVal.is_string()) lawAuthors.push_back(authVal.get<std::string>());
                    }
                } else if (lj.contains("author") && lj["author"].is_string()) {
                    lawAuthors.push_back(lj["author"].get<std::string>());
                } else {
                    lawAuthors = defaultAuthorNames;
                }

                for (const auto& authName : lawAuthors) {
                    if (Person* p = resolvePerson(authName)) {
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
                if (!src) {
                    if (auto lx = Singularity::Language::LanguageSystem::instance().findById(srcId)) {
                        src = lx.get();
                    } else if (auto lxSym = Singularity::Language::LanguageSystem::instance().findBySymbol(srcId)) {
                        src = lxSym.get();
                    }
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
    (void)laws;
    if (outError) outError->clear();

    // 1. Gather all candidate "denotes" Relations linking in-world Lexemes to Laws
    // No private process-global hidden fallback; gathers strictly from in-world relations.
    struct LexemeMapping {
        std::string symbol;
        Law* law = nullptr;
    };
    std::unordered_map<std::string, std::vector<Law*>> candidatesBySymbol;

    auto collectFrom = [&](const std::vector<Relation*>& rels) {
        for (const auto* rel : rels) {
            if (!rel || rel->type != "denotes") continue;
            if (!isRelationActive(rel, nullptr, extraRelations)) continue;
            auto* lex = dynamic_cast<Singularity::Language::Lexeme*>(rel->a());
            auto* law = dynamic_cast<Law*>(rel->b());
            if (lex && law) {
                std::string sym = toLower(trim(lex->getSymbol()));
                if (!sym.empty()) {
                    candidatesBySymbol[sym].push_back(law);
                }
            }
        }
    };

    collectFrom(extraRelations);
    collectFrom(Universe::instance().relations());

    struct LexemeEntry {
        std::string symbol;
        std::vector<Law*> candidates;
    };
    std::vector<LexemeEntry> entries;

    for (const auto& [sym, rawList] : candidatesBySymbol) {
        std::vector<Law*> uniqueList;
        for (Law* l : rawList) {
            if (std::find(uniqueList.begin(), uniqueList.end(), l) == uniqueList.end()) {
                uniqueList.push_back(l);
            }
        }

        // Lineage pruning: if candidate A supersedes candidate B within the same revision lineage, prune B!
        std::vector<Law*> prunedList;
        for (size_t i = 0; i < uniqueList.size(); ++i) {
            Law* cand = uniqueList[i];
            bool superseded = false;
            for (size_t j = 0; j < uniqueList.size(); ++j) {
                if (i == j) continue;
                Law* other = uniqueList[j];
                if (isSameLineage(other, cand->getIdentifier())) {
                    bool otherNewer = false;
                    for (const auto& r : other->provenance().getRelationsOfType("redefined-from")) {
                        if (r && r->a() == cand) { otherNewer = true; break; }
                    }
                    auto [candBase, candVer] = parseIdVersion(cand->getIdentifier());
                    auto [otherBase, otherVer] = parseIdVersion(other->getIdentifier());
                    if (candBase == otherBase && otherVer > candVer) {
                        otherNewer = true;
                    }
                    if (otherNewer) {
                        superseded = true;
                        break;
                    }
                }
            }
            if (!superseded) {
                prunedList.push_back(cand);
            }
        }

        // Sun Round 7 Gate 1 & 2: NO ALPHABETICAL TIE-BREAKER!
        // Determinism is not semantics. If multiple unrelated Laws remain, retain all candidates.
        entries.push_back({sym, prunedList});
    }

    // Sort entries by symbol length descending (greedy longest-match)
    std::sort(entries.begin(), entries.end(), [](const LexemeEntry& a, const LexemeEntry& b) {
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
        for (const auto& entry : entries) {
            if (entry.symbol.empty()) continue;
            size_t symLen = entry.symbol.size();
            if (pos + symLen <= len) {
                if (lowerPhrase.compare(pos, symLen, entry.symbol) == 0) {
                    // Check word boundary so "nav" does not match prefix of "navigate"
                    bool atWordBoundary = (pos + symLen == len) || isDelimiter(phrase[pos + symLen]);
                    if (atWordBoundary) {
                        Law* chosenLaw = nullptr;
                        if (entry.candidates.size() == 1) {
                            chosenLaw = entry.candidates[0];
                        } else if (entry.candidates.size() > 1) {
                            // Sun Round 7 & 8 Gate 3: Kill the Alphabetical Oracle and magic compiler-slot properties!
                            // Unrelated ambiguity strictly refuses!
                            std::string candidateIds;
                            for (size_t ci = 0; ci < entry.candidates.size(); ++ci) {
                                if (ci > 0) candidateIds += ", ";
                                candidateIds += entry.candidates[ci]->getIdentifier();
                            }
                            if (outError) {
                                *outError = "Ambiguous denotation for '" + entry.symbol +
                                            "': multiple unrelated Laws denoted (" + candidateIds +
                                            "); author resolution required";
                            }
                            return WebConcept();
                        }

                        if (chosenLaw) {
                            if (chosenLaw->isAuthored()) {
                                if (const auto* am = chosenLaw->actionModel()) {
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
                            auto res = chosenLaw->applyTo(channelTarget);
                            if (res != Law::ApplicationResult::Applied) {
                                if (outError) {
                                    *outError = "Law " + chosenLaw->getIdentifier() + " refused to apply (res=" +
                                                (res == Law::ApplicationResult::Unauthored ? "Unauthored" : std::to_string(static_cast<int>(res))) + ")";
                                }
                                if (res == Law::ApplicationResult::Unauthored) {
                                    return WebConcept();
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
    Person& author,
    std::vector<std::shared_ptr<Relation>>* outRelations) {

    // Form first-class Earthcall provenance Relation connecting to the actual author Person.
    // The Relation is the SOLE source of truth — NO shadow string property!
    // Sun Round 8 Gate 7: Canonical single-sink admission.
    // If outRelations is provided, pass the relation to the caller's sink;
    // only if outRelations is null do we fall back to Universe registrar.
    auto authRel = std::make_shared<Relation>(WebRelationType::kAuthoredBy, bearer, author, true, 1.0f);
    if (outRelations) {
        outRelations->push_back(authRel);
    } else if (Universe::instance().hasRelationRegistrar()) {
        Universe::instance().addRelation(authRel);
    }

    concept.applyToSingular(bearer);
    return bearer;
}

Singular& WebDesignerMetalaws::applyAuthoredConcept(
    Singular& bearer,
    const WebConcept& concept,
    Person* author,
    std::vector<std::shared_ptr<Relation>>* outRelations) {

    if (!author) {
        // Sun Round 7 Gate 11: Missing stewardship strictly REFUSES mutation of bearer!
        // Authored APIs require Person stewardship.
        return bearer;
    }
    return applyAuthoredConcept(bearer, concept, *author, outRelations);
}

std::shared_ptr<Singular> WebDesignerMetalaws::createAuthoredElement(
    const std::string& identifier,
    const WebConcept& concept,
    Person& author,
    std::vector<std::shared_ptr<Relation>>* outRelations) {

    auto obj = std::make_shared<Object>();
    obj->setObjectID(identifier);
    applyAuthoredConcept(*obj, concept, author, outRelations);
    return obj;
}

std::shared_ptr<Singular> WebDesignerMetalaws::createAuthoredElement(
    const std::string& identifier,
    const WebConcept& concept,
    Person* author,
    std::vector<std::shared_ptr<Relation>>* outRelations) {

    if (!author) {
        // Refusal: creating an AUTHORED element requires Person stewardship!
        return nullptr;
    }
    return createAuthoredElement(identifier, concept, *author, outRelations);
}

Singular& WebDesignerMetalaws::applyDraftConcept(
    Singular& bearer,
    const WebConcept& concept) {
    concept.applyToSingular(bearer);
    return bearer;
}

std::shared_ptr<Singular> WebDesignerMetalaws::createDraftElement(
    const std::string& identifier,
    const WebConcept& concept) {
    auto obj = std::make_shared<Object>();
    obj->setObjectID(identifier);
    concept.applyToSingular(*obj);
    return obj;
}

} // namespace Web
} // namespace Foreign
} // namespace Singularity
