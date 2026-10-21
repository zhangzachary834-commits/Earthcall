#include "Singularity/Foreign/Web/Designer/WebRealizer.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "Person/Person.hpp"
#include "Singularity/TransferPolicy.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <queue>
#include <unordered_map>
#include <unordered_set>

namespace Singularity {
namespace Foreign {
namespace Web {

namespace {

std::string makeIndent(int level) {
    return std::string(level * 2, ' ');
}

std::string getThemeString(const Singular* root, const std::string& key, const std::string& fallback) {
    if (!root) return fallback;
    PropertyValue v;
    if (root->getDynamicProperty(key, v)) {
        if (auto s = std::get_if<std::string>(&v)) {
            if (!s->empty()) return *s;
        }
    }
    return fallback;
}

} // namespace

// Find all children connected to `parent` via "contains" relations,
// querying both caller-supplied relations and Universe relations,
// and order them topologically using "precedes" relations.
// Detects cycles in "precedes" relations and refuses rather than falling back to numeric fiat.
std::vector<const Singular*> WebRealizer::findChildren(const Singular& parent,
                                                      const std::vector<std::shared_ptr<Relation>>& relations,
                                                      std::string* outError) {
    std::vector<const Singular*> rawChildren;
    std::unordered_set<const Singular*> childSet;

    auto checkContains = [&](const Relation* rel) {
        if (!rel) return;
        if (rel->type == WebRelationType::kContains && rel->a() == &parent) {
            const Singular* child = rel->b();
            if (child && childSet.insert(child).second) {
                rawChildren.push_back(child);
            }
        }
    };

    // 1. Collect all children connected via "contains"
    for (const auto& rel : relations) checkContains(rel.get());
    for (const auto* rel : Universe::instance().relations()) checkContains(rel);

    if (rawChildren.empty()) return {};

    const size_t n = rawChildren.size();
    std::unordered_map<const Singular*, size_t> indexMap;
    for (size_t i = 0; i < n; ++i) {
        indexMap[rawChildren[i]] = i;
    }

    // 2. Build DAG from "precedes" relations
    std::vector<int> inDegree(n, 0);
    std::vector<std::vector<size_t>> adj(n);

    auto checkPrecedes = [&](const Relation* rel) {
        if (!rel) return;
        if (rel->type == WebRelationType::kPrecedes) {
            const Singular* a = rel->a();
            const Singular* b = rel->b();
            auto itA = indexMap.find(a);
            auto itB = indexMap.find(b);
            if (itA != indexMap.end() && itB != indexMap.end()) {
                size_t u = itA->second;
                size_t v = itB->second;
                adj[u].push_back(v);
                inDegree[v]++;
            }
        }
    };

    for (const auto& rel : relations) checkPrecedes(rel.get());
    for (const auto* rel : Universe::instance().relations()) checkPrecedes(rel);

    // 3. Topological sort using Kahn's algorithm
    // Deterministic tie-breaking using stable identifiers for nodes with equal in-degree
    auto cmp = [&](size_t i, size_t j) {
        return rawChildren[i]->getIdentifier() > rawChildren[j]->getIdentifier();
    };
    std::priority_queue<size_t, std::vector<size_t>, decltype(cmp)> pq(cmp);

    for (size_t i = 0; i < n; ++i) {
        if (inDegree[i] == 0) {
            pq.push(i);
        }
    }

    std::vector<const Singular*> sortedChildren;
    sortedChildren.reserve(n);
    std::vector<bool> visited(n, false);

    while (!pq.empty()) {
        size_t u = pq.top();
        pq.pop();
        if (visited[u]) continue;
        visited[u] = true;
        sortedChildren.push_back(rawChildren[u]);

        for (size_t v : adj[u]) {
            inDegree[v]--;
            if (inDegree[v] == 0 && !visited[v]) {
                pq.push(v);
            }
        }
    }

    // Cycle detection: If fewer nodes were visited than exist, a precedence cycle exists!
    // Refusal: Do NOT silently fall back to numeric fiat. Report cycle and refuse.
    if (sortedChildren.size() < n) {
        if (outError) {
            *outError = "Precedence cycle detected among children of '" + parent.getIdentifier() + "'. Relational ordering refused.";
        }
        return {};
    }

    return sortedChildren;
}

std::string WebRealizer::escapeHtml(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (char c : text) {
        switch (c) {
            case '&':  out += "&amp;"; break;
            case '<':  out += "&lt;"; break;
            case '>':  out += "&gt;"; break;
            case '"':  out += "&quot;"; break;
            case '\'': out += "&#39;"; break;
            default:   out += c; break;
        }
    }
    return out;
}

std::string WebRealizer::escapeAttr(const std::string& attr) {
    return escapeHtml(attr);
}

std::string WebRealizer::sanitizeHtmlTag(const std::string& tag) {
    if (tag.empty()) return "div";
    std::string lower;
    lower.reserve(tag.size());
    for (char c : tag) {
        lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    // Block executable or document-structure tags
    if (lower == "script" || lower == "style" || lower == "iframe" ||
        lower == "object" || lower == "embed" || lower == "frame" ||
        lower == "frameset" || lower == "html" || lower == "head" ||
        lower == "body" || lower == "applet" || lower == "base" ||
        lower == "link" || lower == "meta") {
        return "div";
    }
    // Must start with [a-z]
    if (!std::isalpha(static_cast<unsigned char>(lower[0]))) return "div";
    // Allowed characters: [a-z0-9-]
    for (char c : lower) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-') {
            return "div";
        }
    }
    return lower;
}

std::string WebRealizer::normalizeCssToken(const std::string& token) {
    if (token.empty()) return "";
    std::string out;
    out.reserve(token.size());
    for (char c : token) {
        char low = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (std::isalnum(static_cast<unsigned char>(low)) || low == '_') {
            out += low;
        } else {
            out += '-';
        }
    }
    // Collapse consecutive dashes
    std::string collapsed;
    collapsed.reserve(out.size());
    bool lastDash = false;
    for (char c : out) {
        if (c == '-') {
            if (!lastDash) collapsed += c;
            lastDash = true;
        } else {
            collapsed += c;
            lastDash = false;
        }
    }
    // Trim leading and trailing dashes and underscores
    size_t first = collapsed.find_first_not_of("-_");
    if (first == std::string::npos) return "";
    size_t last = collapsed.find_last_not_of("-_");
    return collapsed.substr(first, last - first + 1);
}

std::string WebRealizer::sanitizeHref(const std::string& href) {
    auto isIgnored = [](char c) {
        return std::isspace(static_cast<unsigned char>(c)) || std::iscntrl(static_cast<unsigned char>(c));
    };

    size_t first = 0;
    while (first < href.size() && isIgnored(href[first])) {
        first++;
    }
    if (first == href.size()) return "#";

    size_t last = href.size() - 1;
    while (last > first && isIgnored(href[last])) {
        last--;
    }
    std::string trimmed = href.substr(first, last - first + 1);

    // If a colon exists, extract the scheme and validate against an explicit allowlist
    size_t colonPos = trimmed.find(':');
    if (colonPos != std::string::npos) {
        std::string scheme;
        for (size_t i = 0; i < colonPos; ++i) {
            char c = trimmed[i];
            if (!isIgnored(c)) {
                scheme += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
        }

        // Strict scheme allowlist: only safe hyperlinks permitted
        if (scheme == "http" || scheme == "https" || scheme == "mailto" || scheme == "tel") {
            return trimmed;
        }

        // Explicitly block executable, script, or arbitrary data schemes
        // (javascript:, data:, vbscript:, file:, blob:, etc.)
        return "#";
    }

    // Relative URLs, anchor fragments, root-relative paths, and relative file links
    if (trimmed[0] == '#' || trimmed[0] == '/' || trimmed[0] == '.') {
        return trimmed;
    }

    // Safe relative document link (e.g. "index.html", "about")
    return trimmed;
}

std::string WebRealizer::sanitizeMediaSrc(const std::string& src) {
    auto isIgnored = [](char c) {
        return std::isspace(static_cast<unsigned char>(c)) || std::iscntrl(static_cast<unsigned char>(c));
    };

    size_t first = 0;
    while (first < src.size() && isIgnored(src[first])) {
        first++;
    }
    if (first == src.size()) return "";

    size_t last = src.size() - 1;
    while (last > first && isIgnored(src[last])) {
        last--;
    }
    std::string trimmed = src.substr(first, last - first + 1);

    // If colon exists, extract scheme
    size_t colonPos = trimmed.find(':');
    if (colonPos != std::string::npos) {
        std::string scheme;
        for (size_t i = 0; i < colonPos; ++i) {
            char c = trimmed[i];
            if (!isIgnored(c)) {
                scheme += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
        }

        // Safe web schemes
        if (scheme == "http" || scheme == "https") {
            return trimmed;
        }

        // Safe image data URL scheme (strictly images, never text/html or scripts)
        if (scheme == "data") {
            std::string lowerTrimmed;
            for (char c : trimmed) {
                if (!isIgnored(c)) lowerTrimmed += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            if (lowerTrimmed.rfind("data:image/png", 0) == 0 ||
                lowerTrimmed.rfind("data:image/jpeg", 0) == 0 ||
                lowerTrimmed.rfind("data:image/jpg", 0) == 0 ||
                lowerTrimmed.rfind("data:image/webp", 0) == 0 ||
                lowerTrimmed.rfind("data:image/gif", 0) == 0 ||
                lowerTrimmed.rfind("data:image/svg+xml", 0) == 0) {
                return trimmed;
            }
        }

        // Explicitly block all other schemes (javascript:, vbscript:, file:, blob:, data:text/html, etc.)
        return "";
    }

    // Relative paths and images (e.g. "images/hero.png", "./photo.jpg", "/assets/logo.svg")
    if (trimmed[0] == '/' || trimmed[0] == '.' || std::isalnum(static_cast<unsigned char>(trimmed[0]))) {
        return trimmed;
    }

    return "";
}

std::string WebRealizer::sanitizeCssBreakout(const std::string& css) {
    std::string out = css;
    std::string lower;
    lower.reserve(out.size());
    for (char c : out) {
        lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    // Neutralize any end tag or breakout sequence for </style>
    // Handles </style, </ style, < / style, </STYLE, <\style, etc.
    size_t pos = 0;
    while (pos < lower.size()) {
        if (lower[pos] == '<') {
            size_t nextPos = pos + 1;
            while (nextPos < lower.size() && std::isspace(static_cast<unsigned char>(lower[nextPos]))) {
                nextPos++;
            }
            if (nextPos < lower.size() && (lower[nextPos] == '/' || lower[nextPos] == '\\')) {
                nextPos++;
                while (nextPos < lower.size() && std::isspace(static_cast<unsigned char>(lower[nextPos]))) {
                    nextPos++;
                }
                if (nextPos + 5 <= lower.size() && lower.compare(nextPos, 5, "style") == 0) {
                    size_t matchLen = (nextPos + 5) - pos;
                    out.replace(pos, matchLen, "<\\/style");
                    lower.replace(pos, matchLen, "<\\/style");
                    pos += 8;
                    continue;
                }
            }
        }
        pos++;
    }

    // Neutralize HTML comment delimiters (<!-- and -->) into safe CSS comment delimiters
    pos = 0;
    while ((pos = out.find("<!--", pos)) != std::string::npos) {
        out.replace(pos, 4, "/* --");
        pos += 5;
    }
    pos = 0;
    while ((pos = out.find("-->", pos)) != std::string::npos) {
        out.replace(pos, 3, "-- */");
        pos += 5;
    }

    return out;
}

std::string WebRealizer::sanitizeCssValue(const std::string& value, bool allowNetworkUrl) {
    // 1. Gate 8: Ordinary CSS values use narrow, structured grammars
    // Network url(...), @import, expression(...), and backslash escape sequences are rejected
    if (!allowNetworkUrl) {
        if (value.find('\\') != std::string::npos) {
            return "none";
        }
        std::string lower;
        for (char c : value) lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

        if (lower.find("url(") != std::string::npos ||
            lower.find("@import") != std::string::npos ||
            lower.find("expression(") != std::string::npos) {
            return "none";
        }
    }

    // 2. Neutralize </style> and HTML comments
    std::string out = sanitizeCssBreakout(value);

    // 3. Neutralize semicolons and curly braces in single CSS property values
    // to prevent smuggling additional declarations
    for (char& c : out) {
        if (c == ';' || c == '{' || c == '}') {
            c = ' ';
        }
    }

    return out;
}

namespace {

void applyThemeWithProvenance(
    const std::string& themeName,
    Singular& root,
    Person* applicantAuthor,
    Formation* formation,
    std::vector<std::shared_ptr<Relation>>* outRelations,
    Zone* zone = nullptr) {

    std::unordered_map<std::string, std::string> tokens;
    std::vector<std::string> themeAuthors;
    WebDesignerTheme::loadThemeTokens(themeName, tokens, &themeAuthors);

    // 1. Author aesthetic tokens onto root
    for (const auto& [k, v] : tokens) {
        root.setDynamicProperty(k, PropertyValue(v));
    }
    root.setDynamicProperty("theme.name", PropertyValue(themeName));

    if (!applicantAuthor) {
        // Ephemeral / unauthored draft application: sets tokens without establishing provenance relations
        return;
    }

    // 2. Sun Round 8 Gate 5 & Round 9 Gate 1: Theme Definition is ordinary world-owned authored state,
    // NOT a private static plush and NOT a dangling Theme Ghost!
    // It must be held in genuine Zone custody.
    std::string themeDefId = "theme.definition." + themeName;
    Singular* existingDef = nullptr;
    if (zone) {
        for (const auto& obj : zone->objects()) {
            if (obj && obj->getIdentifier() == themeDefId) {
                existingDef = obj.get();
                break;
            }
        }
    } else if (ZoneManager::live()) {
        try {
            for (const auto& obj : ZoneManager::live()->active().objects()) {
                if (obj && obj->getIdentifier() == themeDefId) {
                    existingDef = obj.get();
                    break;
                }
            }
        } catch (...) {}
    }
    if (!existingDef && formation) {
        for (Singular* b : formation->getMembers()) {
            if (b && b->getIdentifier() == themeDefId) {
                existingDef = b;
                break;
            }
        }
    }
    if (!existingDef) {
        for (Singular* b : Universe::instance().beings()) {
            if (b && b->getIdentifier() == themeDefId) {
                existingDef = b;
                break;
            }
        }
    }

    // Resolve theme definition author from recorded fixture metadata (Gates 4 & 6)
    // No "missing => Zach" fallback!
    Person* defPerson = nullptr;
    if (!themeAuthors.empty()) {
        const std::string& recordedDefAuthorName = themeAuthors[0];
        // 1. Exact stable identifier / DID match takes absolute priority
        std::vector<Person*> stableMatches;
        for (Singular* b : Universe::instance().beings()) {
            if (auto* p = dynamic_cast<Person*>(b)) {
                if (p->getIdentifier() == recordedDefAuthorName || p->matchesIdentifier(recordedDefAuthorName)) {
                    stableMatches.push_back(p);
                }
            }
        }
        if (stableMatches.size() == 1) {
            defPerson = stableMatches[0];
        } else if (stableMatches.empty()) {
            // 2. Legacy bare-name / display name fallback: ONLY if strictly UNAMBIGUOUS!
            std::vector<Person*> displayMatches;
            for (Singular* b : Universe::instance().beings()) {
                if (auto* p = dynamic_cast<Person*>(b)) {
                    if (p->getDisplayName() == recordedDefAuthorName || (p->called() && p->called()->getSymbol() == recordedDefAuthorName)) {
                        displayMatches.push_back(p);
                    }
                }
            }
            if (displayMatches.size() == 1) {
                defPerson = displayMatches[0];
            }
            // If displayMatches.size() > 1 (THE TWO-ZACHS BUG), defPerson remains nullptr!
        }
    }

    std::shared_ptr<Object> newlyCreatedDef;
    if (!existingDef) {
        // Sun Round 9 & 10 (Closing the Ghost Door):
        // An owning overload does not make the non-owning overload owning!
        // We only create and admit a persistent world theme definition if an actual owner exists
        // (explicit Zone or live ZoneManager).
        // When no owner exists, do NOT create or admit a persistent theme definition into a Formation,
        // because Formation is non-owning and newlyCreatedDef would dangle upon function return.
        if (zone || ZoneManager::live()) {
            newlyCreatedDef = WebDesignerTheme::authorThemeDefinition(themeName, defPerson, formation, outRelations, zone);
            existingDef = newlyCreatedDef.get();
        }
    }

    if (existingDef) {
        auto usesThemeRel = std::make_shared<Relation>("uses-theme", root, *existingDef, true, 1.0f);
        if (formation) {
            formation->relations().add(usesThemeRel);
        } else if (Universe::instance().hasRelationRegistrar()) {
            Universe::instance().addRelation(usesThemeRel);
        }
        if (outRelations) {
            outRelations->push_back(usesThemeRel);
        }
    }

    // 3. Application stewardship: Person who selected/applied this theme to root
    if (applicantAuthor) {
        auto appRel = std::make_shared<Relation>(WebRelationType::kAuthoredBy, root, *applicantAuthor, true, 1.0f);
        if (formation) {
            formation->relations().add(appRel);
        } else if (Universe::instance().hasRelationRegistrar()) {
            Universe::instance().addRelation(appRel);
        }
        if (outRelations) {
            outRelations->push_back(appRel);
        }
    }
}

} // namespace

std::shared_ptr<Object> WebDesignerTheme::authorThemeDefinition(
    const std::string& themeName,
    Person* author,
    Formation* formation,
    std::vector<std::shared_ptr<Relation>>* outRelations,
    Zone* zone) {

    std::unordered_map<std::string, std::string> tokens;
    std::vector<std::string> themeAuthors;
    loadThemeTokens(themeName, tokens, &themeAuthors);

    std::string themeDefId = "theme.definition." + themeName;
    auto themeDef = std::make_shared<Object>();
    themeDef->setObjectID(themeDefId);
    themeDef->setDynamicProperty("theme.name", PropertyValue(themeName));
    for (const auto& [k, v] : tokens) {
        themeDef->setDynamicProperty(k, PropertyValue(v));
    }

    // Sun Round 9 Gate 1: Genuine Zone custody prevents Theme Ghost!
    if (zone) {
        zone->addObject(themeDef);
    } else if (ZoneManager::live()) {
        try {
            ZoneManager::live()->active().addObject(themeDef);
        } catch (...) {}
    }

    if (formation) {
        formation->addMember(themeDef.get());
    }

    if (author) {
        auto defAuthRel = std::make_shared<Relation>(WebRelationType::kAuthoredBy, *themeDef, *author, true, 1.0f);
        if (formation) {
            formation->relations().add(defAuthRel);
        } else if (Universe::instance().hasRelationRegistrar()) {
            Universe::instance().addRelation(defAuthRel);
        }
        if (outRelations) {
            outRelations->push_back(defAuthRel);
        }
    }

    return themeDef;
}

bool WebDesignerTheme::loadThemeTokens(
    const std::string& themeName,
    std::unordered_map<std::string, std::string>& outTokens,
    std::vector<std::string>* outAuthors,
    const std::string& fixturePath) {

    std::string path = fixturePath.empty() ? "saves/fixtures/web_themes.json" : fixturePath;
    std::ifstream in(path);
    if (!in.is_open()) {
        in.open("../saves/fixtures/web_themes.json");
    }
    if (!in.is_open()) {
        return false;
    }

    try {
        nlohmann::json root = nlohmann::json::parse(in);
        if (outAuthors && root.contains("authors") && root["authors"].is_array()) {
            for (const auto& a : root["authors"]) {
                if (a.is_string()) outAuthors->push_back(a.get<std::string>());
            }
        }
        if (root.contains("themes") && root["themes"].contains(themeName)) {
            const auto& themeJson = root["themes"][themeName];
            for (auto it = themeJson.begin(); it != themeJson.end(); ++it) {
                if (it.value().is_string()) {
                    outTokens[it.key()] = it.value().get<std::string>();
                }
            }
            return true;
        }
    } catch (...) {}
    return false;
}

void WebDesignerTheme::authorThemeTokens(
    Singular& root,
    Person& author,
    const std::unordered_map<std::string, std::string>& tokens,
    Formation* formation,
    std::vector<std::shared_ptr<Relation>>* outRelations) {

    for (const auto& [k, v] : tokens) {
        root.setDynamicProperty(k, PropertyValue(v));
    }

    auto authRel = std::make_shared<Relation>(WebRelationType::kAuthoredBy, root, author, true, 1.0f);
    if (formation) {
        formation->relations().add(authRel);
    } else if (Universe::instance().hasRelationRegistrar()) {
        Universe::instance().addRelation(authRel);
    }
    if (outRelations) {
        outRelations->push_back(authRel);
    }
}

void WebDesignerTheme::authorThemeTokens(
    Singular& root,
    Person* author,
    const std::unordered_map<std::string, std::string>& tokens,
    Formation* formation,
    std::vector<std::shared_ptr<Relation>>* outRelations) {

    if (author) {
        authorThemeTokens(root, *author, tokens, formation, outRelations);
    } else {
        // Ephemeral application without authorship; does NOT self-award FirstMover!
        for (const auto& [k, v] : tokens) {
            root.setDynamicProperty(k, PropertyValue(v));
        }
    }
}

void WebDesignerTheme::applyModernDarkTheme(
    Singular& root,
    Person& author,
    Formation* formation,
    std::vector<std::shared_ptr<Relation>>* outRelations) {
    applyThemeWithProvenance("modern-dark", root, &author, formation, outRelations, nullptr);
}

void WebDesignerTheme::applyModernDarkTheme(
    Singular& root,
    Person* author,
    Formation* formation,
    std::vector<std::shared_ptr<Relation>>* outRelations) {
    applyThemeWithProvenance("modern-dark", root, author, formation, outRelations, nullptr);
}

void WebDesignerTheme::applyModernDarkTheme(
    Singular& root,
    Person& author,
    Zone& zone,
    Formation* formation,
    std::vector<std::shared_ptr<Relation>>* outRelations) {
    applyThemeWithProvenance("modern-dark", root, &author, formation, outRelations, &zone);
}

void WebDesignerTheme::applyModernDarkTheme(
    Singular& root,
    Person* author,
    Zone& zone,
    Formation* formation,
    std::vector<std::shared_ptr<Relation>>* outRelations) {
    applyThemeWithProvenance("modern-dark", root, author, formation, outRelations, &zone);
}

void WebDesignerTheme::applyNeutralLightTheme(
    Singular& root,
    Person& author,
    Formation* formation,
    std::vector<std::shared_ptr<Relation>>* outRelations) {
    applyThemeWithProvenance("neutral-light", root, &author, formation, outRelations, nullptr);
}

void WebDesignerTheme::applyNeutralLightTheme(
    Singular& root,
    Person* author,
    Formation* formation,
    std::vector<std::shared_ptr<Relation>>* outRelations) {
    applyThemeWithProvenance("neutral-light", root, author, formation, outRelations, nullptr);
}

void WebDesignerTheme::applyNeutralLightTheme(
    Singular& root,
    Person& author,
    Zone& zone,
    Formation* formation,
    std::vector<std::shared_ptr<Relation>>* outRelations) {
    applyThemeWithProvenance("neutral-light", root, &author, formation, outRelations, &zone);
}

void WebDesignerTheme::applyNeutralLightTheme(
    Singular& root,
    Person* author,
    Zone& zone,
    Formation* formation,
    std::vector<std::shared_ptr<Relation>>* outRelations) {
    applyThemeWithProvenance("neutral-light", root, author, formation, outRelations, &zone);
}

std::string WebRealizer::generateStyleSheet(const Singular* root, const RealizeOptions& options) {
    // Read authored theme tokens from root if authored, otherwise fallback is completely neutral
    PropertyValue pv;
    bool hasAuthoredTheme = false;
    if (root) {
        hasAuthoredTheme = root->getDynamicProperty("theme.backgroundColor", pv) ||
                           root->getDynamicProperty("theme.primaryColor", pv) ||
                           root->getDynamicProperty("theme.textColor", pv) ||
                           root->getDynamicProperty("theme.radius", pv);
    }

    std::ostringstream css;
    css << ":root {\n";

    if (hasAuthoredTheme) {
        std::string bg = sanitizeCssValue(getThemeString(root, "theme.backgroundColor", ""));
        std::string surface = sanitizeCssValue(getThemeString(root, "theme.surfaceColor", ""));
        std::string card = sanitizeCssValue(getThemeString(root, "theme.cardColor", ""));
        std::string subtle = sanitizeCssValue(getThemeString(root, "theme.subtleColor", ""));
        std::string text = sanitizeCssValue(getThemeString(root, "theme.textColor", ""));
        std::string textMuted = sanitizeCssValue(getThemeString(root, "theme.textMutedColor", ""));
        std::string primary = sanitizeCssValue(getThemeString(root, "theme.primaryColor", ""));
        std::string accent = sanitizeCssValue(getThemeString(root, "theme.accentColor", ""));
        std::string radius = sanitizeCssValue(getThemeString(root, "theme.radius", ""));
        std::string fontSans = sanitizeCssValue(getThemeString(root, "theme.fontFamily", ""));
        std::string heroBg = sanitizeCssValue(getThemeString(root, "theme.heroBackground", ""));
        std::string heroPadding = sanitizeCssValue(getThemeString(root, "theme.heroPadding", ""));
        std::string cardBorder = sanitizeCssValue(getThemeString(root, "theme.cardBorder", ""));
        std::string cardPadding = sanitizeCssValue(getThemeString(root, "theme.cardPadding", ""));
        std::string cardHoverBorder = sanitizeCssValue(getThemeString(root, "theme.cardHoverBorder", ""));
        std::string quoteBorder = sanitizeCssValue(getThemeString(root, "theme.quoteBorder", ""));
        std::string quoteBg = sanitizeCssValue(getThemeString(root, "theme.quoteBackground", ""));
        std::string quotePadding = sanitizeCssValue(getThemeString(root, "theme.quotePadding", ""));
        std::string actionPadding = sanitizeCssValue(getThemeString(root, "theme.actionPadding", ""));
        std::string actionFontSize = sanitizeCssValue(getThemeString(root, "theme.actionFontSize", ""));

        std::string spTight = sanitizeCssValue(getThemeString(root, "theme.spacingTight", ""));
        std::string spNormal = sanitizeCssValue(getThemeString(root, "theme.spacingNormal", ""));
        std::string spRelaxed = sanitizeCssValue(getThemeString(root, "theme.spacingRelaxed", ""));
        std::string spGenerous = sanitizeCssValue(getThemeString(root, "theme.spacingGenerous", ""));
        std::string scSmall = sanitizeCssValue(getThemeString(root, "theme.scaleSmall", ""));
        std::string scNormal = sanitizeCssValue(getThemeString(root, "theme.scaleNormal", ""));
        std::string scLarge = sanitizeCssValue(getThemeString(root, "theme.scaleLarge", ""));
        std::string scHuge = sanitizeCssValue(getThemeString(root, "theme.scaleHuge", ""));
        std::string wtNormal = sanitizeCssValue(getThemeString(root, "theme.weightNormal", ""));
        std::string wtLight = sanitizeCssValue(getThemeString(root, "theme.weightLight", ""));
        std::string wtMedium = sanitizeCssValue(getThemeString(root, "theme.weightMedium", ""));
        std::string wtBold = sanitizeCssValue(getThemeString(root, "theme.weightBold", ""));
        std::string lhTight = sanitizeCssValue(getThemeString(root, "theme.lineHeightTight", ""));
        std::string gridMin = sanitizeCssValue(getThemeString(root, "theme.gridMinWidth", ""));
        std::string shSubtle = sanitizeCssValue(getThemeString(root, "theme.shadowSubtle", ""));
        std::string shRaised = sanitizeCssValue(getThemeString(root, "theme.shadowRaised", ""));
        std::string shFloating = sanitizeCssValue(getThemeString(root, "theme.shadowFloating", ""));
        std::string h1Sz = sanitizeCssValue(getThemeString(root, "theme.h1Size", ""));
        std::string h1Lh = sanitizeCssValue(getThemeString(root, "theme.h1LineHeight", ""));
        std::string hMb = sanitizeCssValue(getThemeString(root, "theme.hHeadingMarginBottom", ""));
        std::string h2Sz = sanitizeCssValue(getThemeString(root, "theme.h2Size", ""));
        std::string h2Lh = sanitizeCssValue(getThemeString(root, "theme.h2LineHeight", ""));
        std::string h3Sz = sanitizeCssValue(getThemeString(root, "theme.h3Size", ""));
        std::string h3Lh = sanitizeCssValue(getThemeString(root, "theme.h3LineHeight", ""));
        std::string h3Mb = sanitizeCssValue(getThemeString(root, "theme.h3MarginBottom", ""));
        std::string h4Sz = sanitizeCssValue(getThemeString(root, "theme.h4Size", ""));
        std::string h5Sz = sanitizeCssValue(getThemeString(root, "theme.h5Size", ""));
        std::string h6Sz = sanitizeCssValue(getThemeString(root, "theme.h6Size", ""));
        std::string subtlePad = sanitizeCssValue(getThemeString(root, "theme.subtlePadding", ""));
        std::string subOpacity = sanitizeCssValue(getThemeString(root, "theme.subordinateOpacity", ""));
        std::string subSize = sanitizeCssValue(getThemeString(root, "theme.subordinateSize", ""));
        std::string quietOp = sanitizeCssValue(getThemeString(root, "theme.quietOpacity", ""));
        std::string promWt = sanitizeCssValue(getThemeString(root, "theme.prominentWeight", ""));
        std::string transNorm = sanitizeCssValue(getThemeString(root, "theme.transitionNormal", ""));
        std::string actWt = sanitizeCssValue(getThemeString(root, "theme.actionWeight", ""));
        std::string actTrans = sanitizeCssValue(getThemeString(root, "theme.actionTransition", ""));

        if (!bg.empty()) css << "  --web-bg: " << bg << ";\n";
        if (!surface.empty()) css << "  --web-bg-surface: " << surface << ";\n";
        if (!card.empty()) css << "  --web-bg-card: " << card << ";\n";
        if (!subtle.empty()) css << "  --web-bg-subtle: " << subtle << ";\n";
        if (!text.empty()) css << "  --web-text: " << text << ";\n";
        if (!textMuted.empty()) css << "  --web-text-muted: " << textMuted << ";\n";
        if (!primary.empty()) {
            css << "  --web-primary: " << primary << ";\n";
            css << "  --web-primary-hover: " << primary << ";\n";
            css << "  --web-primary-text: " << (bg.empty() ? "#ffffff" : bg) << ";\n";
        }
        if (!accent.empty()) css << "  --web-accent: " << accent << ";\n";
        if (!radius.empty()) css << "  --web-radius: " << radius << ";\n";
        css << "  --web-radius-pill: 9999px;\n";
        if (!fontSans.empty()) css << "  --web-font-sans: " << fontSans << ";\n";
        if (!heroBg.empty()) css << "  --web-hero-bg: " << heroBg << ";\n";
        if (!heroPadding.empty()) css << "  --web-hero-padding: " << heroPadding << ";\n";
        if (!card.empty()) css << "  --web-card-bg: " << card << ";\n";
        if (!cardBorder.empty()) css << "  --web-card-border: " << cardBorder << ";\n";
        if (!cardPadding.empty()) css << "  --web-card-padding: " << cardPadding << ";\n";
        if (!cardHoverBorder.empty()) css << "  --web-card-hover-border: " << cardHoverBorder << ";\n";
        if (!quoteBorder.empty()) css << "  --web-quote-border: " << quoteBorder << ";\n";
        if (!quoteBg.empty()) css << "  --web-quote-bg: " << quoteBg << ";\n";
        if (!quotePadding.empty()) css << "  --web-quote-padding: " << quotePadding << ";\n";
        if (!actionPadding.empty()) css << "  --web-action-padding: " << actionPadding << ";\n";
        if (!actionFontSize.empty()) css << "  --web-action-font-size: " << actionFontSize << ";\n";

        if (!spTight.empty()) css << "  --web-spacing-tight: " << spTight << ";\n";
        if (!spNormal.empty()) css << "  --web-spacing-normal: " << spNormal << ";\n";
        if (!spRelaxed.empty()) css << "  --web-spacing-relaxed: " << spRelaxed << ";\n";
        if (!spGenerous.empty()) css << "  --web-spacing-generous: " << spGenerous << ";\n";
        if (!scSmall.empty()) css << "  --web-scale-small: " << scSmall << ";\n";
        if (!scNormal.empty()) css << "  --web-scale-normal: " << scNormal << ";\n";
        if (!scLarge.empty()) css << "  --web-scale-large: " << scLarge << ";\n";
        if (!scHuge.empty()) css << "  --web-scale-huge: " << scHuge << ";\n";
        if (!wtNormal.empty()) css << "  --web-weight-normal: " << wtNormal << ";\n";
        if (!wtLight.empty()) css << "  --web-weight-light: " << wtLight << ";\n";
        if (!wtMedium.empty()) css << "  --web-weight-medium: " << wtMedium << ";\n";
        if (!wtBold.empty()) css << "  --web-weight-bold: " << wtBold << ";\n";
        if (!lhTight.empty()) css << "  --web-lh-tight: " << lhTight << ";\n";
        if (!gridMin.empty()) css << "  --web-grid-min-width: " << gridMin << ";\n";
        if (!shSubtle.empty()) css << "  --web-shadow-subtle: " << shSubtle << ";\n";
        if (!shRaised.empty()) css << "  --web-shadow-raised: " << shRaised << ";\n";
        if (!shFloating.empty()) css << "  --web-shadow-floating: " << shFloating << ";\n";
        if (!h1Sz.empty()) css << "  --web-h1-size: " << h1Sz << ";\n";
        if (!h1Lh.empty()) css << "  --web-h1-lh: " << h1Lh << ";\n";
        if (!hMb.empty()) css << "  --web-h-mb: " << hMb << ";\n";
        if (!h2Sz.empty()) css << "  --web-h2-size: " << h2Sz << ";\n";
        if (!h2Lh.empty()) css << "  --web-h2-lh: " << h2Lh << ";\n";
        if (!h3Sz.empty()) css << "  --web-h3-size: " << h3Sz << ";\n";
        if (!h3Lh.empty()) css << "  --web-h3-lh: " << h3Lh << ";\n";
        if (!h3Mb.empty()) css << "  --web-h3-mb: " << h3Mb << ";\n";
        if (!h4Sz.empty()) css << "  --web-h4-size: " << h4Sz << ";\n";
        if (!h5Sz.empty()) css << "  --web-h5-size: " << h5Sz << ";\n";
        if (!h6Sz.empty()) css << "  --web-h6-size: " << h6Sz << ";\n";
        if (!subtlePad.empty()) css << "  --web-subtle-padding: " << subtlePad << ";\n";
        if (!subOpacity.empty()) css << "  --web-subordinate-opacity: " << subOpacity << ";\n";
        if (!subSize.empty()) css << "  --web-subordinate-size: " << subSize << ";\n";
        if (!quietOp.empty()) css << "  --web-quiet-opacity: " << quietOp << ";\n";
        if (!promWt.empty()) css << "  --web-prominent-weight: " << promWt << ";\n";
        if (!transNorm.empty()) css << "  --web-transition-normal: " << transNorm << ";\n";
        if (!actWt.empty()) css << "  --web-action-weight: " << actWt << ";\n";
        if (!actTrans.empty()) css << "  --web-action-transition: " << actTrans << ";\n";

        if (root) {
            for (const auto& [kId, val] : root->dynamicProperties()) {
                std::string k = Earthcall::StringInterner::resolve(kId);
                if (k.rfind("--", 0) == 0 && std::holds_alternative<std::string>(val)) {
                    std::string v = sanitizeCssValue(std::get<std::string>(val));
                    css << "  " << k << ": " << v << ";\n";
                }
            }
        }
    } else {
        // Pure browser mechanics: minimal neutral defaults without opinionated aesthetic design system
        css << "  --web-font-sans: system-ui, -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;\n"
            << "  --web-bg: transparent;\n"
            << "  --web-text: inherit;\n"
            << "  --web-border: currentColor;\n";
    }

    css << "}\n\n"
        << R"CSS(* {
  box-sizing: border-box;
  margin: 0;
  padding: 0;
}

body {
  font-family: var(--web-font-sans);
  background-color: var(--web-bg);
  color: var(--web-text);
  line-height: inherit;
  -webkit-font-smoothing: antialiased;
  min-height: 100vh;
}

.web-page {
  display: flex;
  flex-direction: column;
  min-height: 100vh;
  width: 100%;
}

.web-section {
  width: 100%;
}

.web-navigation {
  width: 100%;
}

/* Flows */
.web-flow--vertical {
  display: flex;
  flex-direction: column;
}
.web-flow--horizontal {
  display: flex;
  flex-direction: row;
}
.web-flow--grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(var(--web-grid-min-width), 1fr));
}

/* Alignments */
.web-align--start { align-items: flex-start; }
.web-align--center { align-items: center; }
.web-align--end { align-items: flex-end; }
.web-align--stretch { align-items: stretch; }

/* Justifications */
.web-justify--start { justify-content: flex-start; }
.web-justify--center { justify-content: center; }
.web-justify--end { justify-content: flex-end; }
.web-justify--between { justify-content: space-between; }
.web-justify--around { justify-content: space-around; }

/* Spacing */
.web-spacing--none { gap: 0; }
.web-spacing--tight { gap: var(--web-spacing-tight); }
.web-spacing--normal { gap: var(--web-spacing-normal); }
.web-spacing--relaxed { gap: var(--web-spacing-relaxed); }
.web-spacing--generous { gap: var(--web-spacing-generous); }

/* Scales */
.web-scale--small { font-size: var(--web-scale-small); }
.web-scale--normal { font-size: var(--web-scale-normal); }
.web-scale--large { font-size: var(--web-scale-large); }
.web-scale--huge { font-size: var(--web-scale-huge); line-height: var(--web-lh-tight); }

/* Typography */
.web-weight--normal { font-weight: var(--web-weight-normal); }
.web-weight--light { font-weight: var(--web-weight-light); }
.web-weight--medium { font-weight: var(--web-weight-medium); }
.web-weight--bold { font-weight: var(--web-weight-bold); }

.web-tone--default { color: inherit; }
.web-tone--muted { color: var(--web-text-muted); }
.web-tone--accent { color: var(--web-accent); }
.web-tone--prominent { color: var(--web-primary); }

/* Relational Modifiers */
.web-subordinate {
  opacity: var(--web-subordinate-opacity);
  font-size: var(--web-subordinate-size);
}
.web-prominence--quiet {
  opacity: var(--web-quiet-opacity);
}
.web-prominence--prominent {
  font-weight: var(--web-prominent-weight);
  color: var(--web-primary);
}
.web-prominence--subordinate {
  opacity: var(--web-subordinate-opacity);
}

/* Headings */
h1.web-heading { font-size: var(--web-h1-size); line-height: var(--web-h1-lh); margin-bottom: var(--web-h-mb); }
h2.web-heading { font-size: var(--web-h2-size); line-height: var(--web-h2-lh); margin-bottom: var(--web-h-mb); }
h3.web-heading { font-size: var(--web-h3-size); line-height: var(--web-h3-lh); margin-bottom: var(--web-h3-mb); }
h4.web-heading { font-size: var(--web-h4-size); }
h5.web-heading { font-size: var(--web-h5-size); }
h6.web-heading { font-size: var(--web-h6-size); }

/* Surface Variants driven by CSS variables */
.web-surface--card {
  background: var(--web-card-bg);
  border: 1px solid var(--web-card-border);
  padding: var(--web-card-padding);
  transition: border-color var(--web-transition-normal);
}
.web-surface--card:hover {
  border-color: var(--web-card-hover-border);
}

.web-surface--subtle {
  background-color: var(--web-bg-subtle);
  border: 1px solid var(--web-border);
  padding: var(--web-subtle-padding);
}

.web-surface--hero {
  background: var(--web-hero-bg);
  padding: var(--web-hero-padding);
}

/* Shapes */
.web-shape--sharp { border-radius: 0; }
.web-shape--rounded { border-radius: var(--web-radius); }
.web-shape--pill { border-radius: var(--web-radius-pill); }
.web-shape--circle { border-radius: 50%; }

/* Elevations */
.web-elevation--subtle { box-shadow: var(--web-shadow-subtle); }
.web-elevation--raised { box-shadow: var(--web-shadow-raised); }
.web-elevation--floating { box-shadow: var(--web-shadow-floating); }

/* Buttons & Interactive Actions */
.web-action,
.web-btn {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  padding: var(--web-action-padding);
  font-size: var(--web-action-font-size);
  font-weight: var(--web-action-weight);
  cursor: pointer;
  border: 1px solid transparent;
  transition: var(--web-action-transition);
  text-decoration: none;
  line-height: inherit;
  box-sizing: border-box;
  font-family: inherit;
}

.web-action:focus-visible,
.web-btn:focus-visible {
  outline: 2px solid var(--web-primary);
  outline-offset: 2px;
}

.web-surface--primary.web-action,
.web-surface--primary.web-btn {
  background-color: var(--web-primary);
  color: var(--web-primary-text);
  border-color: transparent;
}
.web-surface--primary.web-action:hover,
.web-surface--primary.web-btn:hover {
  background-color: var(--web-primary-hover);
}

.web-surface--secondary.web-action,
.web-surface--secondary.web-btn {
  background-color: var(--web-secondary);
  color: var(--web-secondary-text);
  border-color: transparent;
}
.web-surface--secondary.web-action:hover,
.web-surface--secondary.web-btn:hover {
  background-color: var(--web-secondary-hover);
}

.web-surface--outline.web-action,
.web-surface--outline.web-btn {
  background-color: transparent;
  border: 1px solid var(--web-border);
  color: var(--web-text);
}
.web-surface--outline.web-action:hover,
.web-surface--outline.web-btn:hover {
  border-color: var(--web-primary);
  color: var(--web-primary);
  background-color: transparent;
}

.web-surface--default.web-action,
.web-surface--default.web-btn {
  background-color: transparent;
  color: var(--web-text);
}

/* Lists */
.web-list {
  list-style: none;
  width: 100%;
}
.web-item {
  width: 100%;
}

/* Quote */
.web-quote {
  border-left-width: 4px;
  border-left-style: solid;
  border-left-color: var(--web-quote-border);
  background: var(--web-quote-bg);
  padding: var(--web-quote-padding);
  font-style: italic;
  border-radius: 0 var(--web-radius) var(--web-radius) 0;
}

/* Divider */
.web-divider {
  border: 0;
  border-top: 1px solid var(--web-border);
  margin: 1em 0;
  width: 100%;
}
)CSS";

    if (!options.extraCss.empty() && options.allowUnrestrictedCss) {
        // Sun Round 8 Gate 8: Raw CSS capability is governed by a first-class Relation in the world graph:
        // Relation("grants-capability", *options.governingAuthor, *root) with capability == "web.rawCss"
        // Neither a Person pointer alone nor a global set-to-set TransferPolicy bool can authorize raw CSS.
        bool hasCapabilityGrant = false;
        if (options.governingAuthor && root) {
            auto checkGrant = [&](const Relation* r) {
                if (!r) return false;
                if ((r->type == "grants-capability" || r->type == "capability-grant") &&
                    r->a() == options.governingAuthor && r->b() == root) {
                    PropertyValue capVal;
                    if (r->getDynamicProperty("capability", capVal)) {
                        if (auto s = std::get_if<std::string>(&capVal)) {
                            if (*s == "web.rawCss") return true;
                        }
                    }
                    return false;
                }
                return false;
            };
            for (const auto* r : Universe::instance().relations()) {
                if (checkGrant(r)) {
                    hasCapabilityGrant = true;
                    break;
                }
            }
        }
        if (hasCapabilityGrant) {
            // Even authorized raw CSS must preserve the HTML parser boundary.
            std::string safeRaw = sanitizeCssBreakout(options.extraCss);
            css << "\n/* Author-governed raw CSS */\n" << safeRaw << "\n";
        }
    }

    return css.str();
}

std::string WebRealizer::generateClientScript() {
    return R"JS(
(function() {
  document.addEventListener('DOMContentLoaded', () => {
    document.querySelectorAll('[data-action-event]').forEach(el => {
      el.addEventListener('click', (e) => {
        const eventName = el.getAttribute('data-action-event');
        const beingId = el.getAttribute('data-being-id') || 'unknown';
        console.log('[Earthcall Web Designer] Action triggered:', eventName, 'from being:', beingId);

        // Bridge to WebKit if inside RealWebView
        if (window.webkit && window.webkit.messageHandlers && window.webkit.messageHandlers.earthcall) {
          window.webkit.messageHandlers.earthcall.postMessage({
            type: 'web-action',
            event: eventName,
            beingId: beingId,
            timestamp: Date.now()
          });
        }
      });
    });
  });
})();
)JS";
}

std::string WebRealizer::realizeElement(const Singular& element,
                                       const std::vector<std::shared_ptr<Relation>>& relations,
                                       int indentLevel,
                                       std::string* outError) {
    WebConcept concept = WebConcept::readFromSingular(element);
    std::string indent = makeIndent(indentLevel);
    std::ostringstream out;

    // Collect CSS classes: ONLY for authored concepts!
    std::vector<std::string> classes;
    std::string effectiveRole = concept.role.empty() ? concept.roleName : concept.role;
    if (!effectiveRole.empty() && effectiveRole != "unknown") {
        classes.push_back("web-" + normalizeCssToken(effectiveRole));
    }

    if (!concept.flowName.empty()) {
        classes.push_back("web-flow--" + normalizeCssToken(concept.flowName));
    }
    if (!concept.alignName.empty()) {
        classes.push_back("web-align--" + normalizeCssToken(concept.alignName));
    }
    if (!concept.justifyName.empty()) {
        classes.push_back("web-justify--" + normalizeCssToken(concept.justifyName));
    }
    if (!concept.spacingName.empty()) {
        classes.push_back("web-spacing--" + normalizeCssToken(concept.spacingName));
    }
    if (!concept.scaleName.empty()) {
        classes.push_back("web-scale--" + normalizeCssToken(concept.scaleName));
    }
    if (!concept.weightName.empty()) {
        classes.push_back("web-weight--" + normalizeCssToken(concept.weightName));
    }
    if (!concept.toneName.empty()) {
        classes.push_back("web-tone--" + normalizeCssToken(concept.toneName));
    }
    if (!concept.variantName.empty() && concept.variantName != "default") {
        classes.push_back("web-surface--" + normalizeCssToken(concept.variantName));
    }
    if (!concept.shapeName.empty()) {
        classes.push_back("web-shape--" + normalizeCssToken(concept.shapeName));
    }
    if (!concept.elevationName.empty() && concept.elevationName != "flat") {
        classes.push_back("web-elevation--" + normalizeCssToken(concept.elevationName));
    }

    if (concept.subordinate) {
        classes.push_back("web-subordinate");
    }
    if (!concept.prominence.empty()) {
        classes.push_back("web-prominence--" + normalizeCssToken(concept.prominence));
    }

    std::string classAttr = "class=\"";
    for (size_t i = 0; i < classes.size(); ++i) {
        if (i > 0) classAttr += " ";
        classAttr += classes[i];
    }
    classAttr += "\"";

    // Dynamic style attribute for authored presentation styles
    std::string styleAttr;
    std::string inlineStyles;
    if (!concept.background.empty()) inlineStyles += "background-color: " + escapeAttr(sanitizeCssValue(concept.background)) + "; ";
    if (!concept.color.empty()) inlineStyles += "color: " + escapeAttr(sanitizeCssValue(concept.color)) + "; ";

    // Read any authored element-level presentation overrides
    PropertyValue sp;
    if (element.getDynamicProperty("surface.padding", sp) || element.getDynamicProperty("padding", sp)) {
        if (auto s = std::get_if<std::string>(&sp)) inlineStyles += "padding: " + escapeAttr(sanitizeCssValue(*s)) + "; ";
    }
    if (element.getDynamicProperty("surface.border", sp) || element.getDynamicProperty("border", sp)) {
        if (auto s = std::get_if<std::string>(&sp)) inlineStyles += "border: " + escapeAttr(sanitizeCssValue(*s)) + "; ";
    }
    if (!inlineStyles.empty()) {
        styleAttr = " style=\"" + inlineStyles + "\"";
    }

    std::string beingAttr = "data-being-id=\"" + escapeAttr(element.getIdentifier()) + "\"";
    if (concept.subordinate) {
        beingAttr += " data-subordinate=\"true\"";
    }

    // Children ordered topologically by "precedes" relations
    auto children = findChildren(element, relations, outError);

    // Escape hatch for explicit tag override
    if (!concept.elementTag.empty()) {
        std::string tag = sanitizeHtmlTag(concept.elementTag);
        out << indent << "<" << tag << " id=\"" << escapeAttr(element.getIdentifier()) << "\" " << classAttr << styleAttr << " " << beingAttr << ">\n";
        if (!concept.text.empty()) {
            out << indent << "  " << escapeHtml(concept.text) << "\n";
        }
        for (const auto* child : children) {
            out << realizeElement(*child, relations, indentLevel + 1, outError);
        }
        out << indent << "</" << tag << ">\n";
        return out.str();
    }

    // Open Semantic Role Mapping to HTML Landmarks
    if (effectiveRole == "page") {
        out << indent << "<main id=\"" << escapeAttr(element.getIdentifier()) << "\" " << classAttr << styleAttr << " " << beingAttr << ">\n";
        for (const auto* child : children) {
            out << realizeElement(*child, relations, indentLevel + 1, outError);
        }
        out << indent << "</main>\n";
    } else if (effectiveRole == "section") {
        out << indent << "<section id=\"" << escapeAttr(element.getIdentifier()) << "\" " << classAttr << styleAttr << " " << beingAttr << ">\n";
        for (const auto* child : children) {
            out << realizeElement(*child, relations, indentLevel + 1, outError);
        }
        out << indent << "</section>\n";
    } else if (effectiveRole == "navigation" || effectiveRole == "nav") {
        out << indent << "<nav id=\"" << escapeAttr(element.getIdentifier()) << "\" " << classAttr << styleAttr << " " << beingAttr << ">\n";
        for (const auto* child : children) {
            out << realizeElement(*child, relations, indentLevel + 1, outError);
        }
        out << indent << "</nav>\n";
    } else if (effectiveRole == "heading" || effectiveRole == "title" || effectiveRole == "subtitle") {
        std::string tag = "h" + std::to_string(std::clamp(concept.headingLevel.value_or(2), 1, 6));
        out << indent << "<" << tag << " id=\"" << escapeAttr(element.getIdentifier()) << "\" " << classAttr << styleAttr << " " << beingAttr << ">"
            << escapeHtml(concept.text) << "</" << tag << ">\n";
    } else if (effectiveRole == "paragraph" || effectiveRole == "text") {
        out << indent << "<p id=\"" << escapeAttr(element.getIdentifier()) << "\" " << classAttr << styleAttr << " " << beingAttr << ">"
            << escapeHtml(concept.text) << "</p>\n";
    } else if (effectiveRole == "action" || effectiveRole == "button" || effectiveRole == "link") {
        std::string eventAttr = concept.actionEvent.empty() ? "" : " data-action-event=\"" + escapeAttr(concept.actionEvent) + "\"";
        if (!concept.actionHref.empty()) {
            std::string safeHref = sanitizeHref(concept.actionHref);
            out << indent << "<a href=\"" << escapeAttr(safeHref) << "\" " << classAttr << " " << beingAttr << eventAttr << styleAttr << ">"
                << escapeHtml(concept.text) << "</a>\n";
        } else {
            out << indent << "<button type=\"button\" " << classAttr << " " << beingAttr << eventAttr << styleAttr << ">"
                << escapeHtml(concept.text) << "</button>\n";
        }
    } else if (effectiveRole == "list") {
        out << indent << "<ul id=\"" << escapeAttr(element.getIdentifier()) << "\" " << classAttr << styleAttr << " " << beingAttr << ">\n";
        for (const auto* child : children) {
            out << realizeElement(*child, relations, indentLevel + 1, outError);
        }
        out << indent << "</ul>\n";
    } else if (effectiveRole == "item") {
        out << indent << "<li id=\"" << escapeAttr(element.getIdentifier()) << "\" " << classAttr << styleAttr << " " << beingAttr << ">\n";
        if (!concept.text.empty()) {
            out << indent << "  " << escapeHtml(concept.text) << "\n";
        }
        for (const auto* child : children) {
            out << realizeElement(*child, relations, indentLevel + 1, outError);
        }
        out << indent << "</li>\n";
    } else if (effectiveRole == "media" || effectiveRole == "image") {
        std::string safeMediaSrc = sanitizeMediaSrc(concept.mediaSrc);
        out << indent << "<img id=\"" << escapeAttr(element.getIdentifier()) << "\" src=\"" << escapeAttr(safeMediaSrc) << "\" alt=\""
            << escapeAttr(concept.mediaAlt) << "\" " << classAttr << styleAttr << " " << beingAttr << " />\n";
    } else if (effectiveRole == "quote") {
        out << indent << "<blockquote id=\"" << escapeAttr(element.getIdentifier()) << "\" " << classAttr << styleAttr << " " << beingAttr << ">\n";
        if (!concept.text.empty()) {
            out << indent << "  " << escapeHtml(concept.text) << "\n";
        }
        for (const auto* child : children) {
            out << realizeElement(*child, relations, indentLevel + 1, outError);
        }
        out << indent << "</blockquote>\n";
    } else if (effectiveRole == "divider") {
        out << indent << "<hr id=\"" << escapeAttr(element.getIdentifier()) << "\" " << classAttr << styleAttr << " " << beingAttr << " />\n";
    } else {
        // Any custom authored role (e.g. "sanctuary", "hero", "card", "container", etc.) defaults to container <div>
        out << indent << "<div id=\"" << escapeAttr(element.getIdentifier()) << "\" " << classAttr << styleAttr << " " << beingAttr << ">\n";
        if (!concept.text.empty()) {
            out << indent << "  <p class=\"web-paragraph\">" << escapeHtml(concept.text) << "</p>\n";
        }
        for (const auto* child : children) {
            out << realizeElement(*child, relations, indentLevel + 1, outError);
        }
        out << indent << "</div>\n";
    }

    return out.str();
}

std::string WebRealizer::realizeFullDocument(const Singular& root,
                                             const std::vector<std::shared_ptr<Relation>>& relations,
                                             const RealizeOptions& options) {
    std::ostringstream doc;
    doc << "<!DOCTYPE html>\n"
        << "<html lang=\"en\">\n"
        << "<head>\n"
        << "  <meta charset=\"UTF-8\" />\n"
        << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" />\n"
        << "  <title>" << escapeHtml(options.title) << "</title>\n";

    if (!options.metaDescription.empty()) {
        doc << "  <meta name=\"description\" content=\"" << escapeAttr(options.metaDescription) << "\" />\n";
    }

    doc << "  <style>\n"
        << generateStyleSheet(&root, options)
        << "  </style>\n";

    doc << "</head>\n"
        << "<body>\n";

    doc << realizeElement(root, relations, 1);

    if (options.includeInteractiveScript) {
        doc << "  <script>\n"
            << generateClientScript()
            << "  </script>\n";
    }

    doc << "</body>\n"
        << "</html>\n";

    return doc.str();
}

} // namespace Web
} // namespace Foreign
} // namespace Singularity
