#include "Singularity/Foreign/Web/Designer/WebRealizer.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include <algorithm>
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
    // 1. Neutralize </style> and HTML comments
    std::string out = sanitizeCssBreakout(value);

    // 2. Gate 9: Neutralize semicolons and curly braces in single CSS property values
    // to prevent smuggling additional declarations (e.g. "red; background-image: ...")
    for (char& c : out) {
        if (c == ';' || c == '{' || c == '}') {
            c = ' ';
        }
    }

    // 3. Gate 9: Neutralize network-bearing url(...) unless explicitly allowed
    if (!allowNetworkUrl) {
        std::string lower;
        for (char c : out) lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        size_t uPos = 0;
        while ((uPos = lower.find("url(", uPos)) != std::string::npos) {
            size_t closeParen = lower.find(')', uPos);
            size_t end = (closeParen != std::string::npos) ? closeParen + 1 : lower.size();
            out.replace(uPos, end - uPos, "none");
            lower.replace(uPos, end - uPos, "none");
            uPos += 4;
        }
    }

    return out;
}

void WebDesignerTheme::applyModernDarkTheme(Singular& root, Person* /*author*/) {
    root.setDynamicProperty("theme.backgroundColor", PropertyValue(std::string("#0b0f19")));
    root.setDynamicProperty("theme.surfaceColor", PropertyValue(std::string("#131c2e")));
    root.setDynamicProperty("theme.cardColor", PropertyValue(std::string("#182338")));
    root.setDynamicProperty("theme.subtleColor", PropertyValue(std::string("#0f172a")));
    root.setDynamicProperty("theme.textColor", PropertyValue(std::string("#f8fafc")));
    root.setDynamicProperty("theme.textMutedColor", PropertyValue(std::string("#94a3b8")));
    root.setDynamicProperty("theme.primaryColor", PropertyValue(std::string("#38bdf8")));
    root.setDynamicProperty("theme.accentColor", PropertyValue(std::string("#818cf8")));
    root.setDynamicProperty("theme.radius", PropertyValue(std::string("0.75rem")));
    root.setDynamicProperty("theme.heroBackground", PropertyValue(std::string("radial-gradient(circle at 50% 0%, rgba(56, 189, 248, 0.12) 0%, transparent 70%)")));
    root.setDynamicProperty("theme.heroPadding", PropertyValue(std::string("4rem 1.5rem")));
    root.setDynamicProperty("theme.cardBorder", PropertyValue(std::string("rgba(56, 189, 248, 0.15)")));
    root.setDynamicProperty("theme.cardPadding", PropertyValue(std::string("1.75rem")));
    root.setDynamicProperty("theme.cardHoverBorder", PropertyValue(std::string("#38bdf8")));
    root.setDynamicProperty("theme.quoteBorder", PropertyValue(std::string("#38bdf8")));
    root.setDynamicProperty("theme.quoteBackground", PropertyValue(std::string("#0f172a")));
    root.setDynamicProperty("theme.quotePadding", PropertyValue(std::string("1.5rem 2rem")));
    root.setDynamicProperty("theme.actionPadding", PropertyValue(std::string("0.75rem 1.75rem")));
    root.setDynamicProperty("theme.actionFontSize", PropertyValue(std::string("0.95rem")));
}

void WebDesignerTheme::applyNeutralLightTheme(Singular& root, Person* /*author*/) {
    root.setDynamicProperty("theme.backgroundColor", PropertyValue(std::string("#ffffff")));
    root.setDynamicProperty("theme.surfaceColor", PropertyValue(std::string("#f8fafc")));
    root.setDynamicProperty("theme.cardColor", PropertyValue(std::string("#f1f5f9")));
    root.setDynamicProperty("theme.subtleColor", PropertyValue(std::string("#e2e8f0")));
    root.setDynamicProperty("theme.textColor", PropertyValue(std::string("#0f172a")));
    root.setDynamicProperty("theme.textMutedColor", PropertyValue(std::string("#64748b")));
    root.setDynamicProperty("theme.primaryColor", PropertyValue(std::string("#2563eb")));
    root.setDynamicProperty("theme.accentColor", PropertyValue(std::string("#4f46e5")));
    root.setDynamicProperty("theme.radius", PropertyValue(std::string("0.5rem")));
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
        std::string radius = sanitizeCssValue(getThemeString(root, "theme.radius", "0.5rem"));
        std::string fontSans = sanitizeCssValue(getThemeString(root, "theme.fontFamily", "-apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif"));
        std::string heroBg = sanitizeCssValue(getThemeString(root, "theme.heroBackground", ""));
        std::string heroPadding = sanitizeCssValue(getThemeString(root, "theme.heroPadding", "3rem 1.5rem"));
        std::string cardBorder = sanitizeCssValue(getThemeString(root, "theme.cardBorder", "rgba(128, 128, 128, 0.2)"));
        std::string cardPadding = sanitizeCssValue(getThemeString(root, "theme.cardPadding", "1.5rem"));
        std::string cardHoverBorder = sanitizeCssValue(getThemeString(root, "theme.cardHoverBorder", primary));
        std::string quoteBorder = sanitizeCssValue(getThemeString(root, "theme.quoteBorder", primary));
        std::string quoteBg = sanitizeCssValue(getThemeString(root, "theme.quoteBackground", subtle));
        std::string quotePadding = sanitizeCssValue(getThemeString(root, "theme.quotePadding", "1.5rem 2rem"));
        std::string actionPadding = sanitizeCssValue(getThemeString(root, "theme.actionPadding", "0.75rem 1.5rem"));
        std::string actionFontSize = sanitizeCssValue(getThemeString(root, "theme.actionFontSize", "0.95rem"));

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
        css << "  --web-radius: " << radius << ";\n";
        css << "  --web-radius-pill: 9999px;\n";
        css << "  --web-font-sans: " << fontSans << ";\n";
        if (!heroBg.empty()) css << "  --web-hero-bg: " << heroBg << ";\n";
        css << "  --web-hero-padding: " << heroPadding << ";\n";
        if (!card.empty()) css << "  --web-card-bg: " << card << ";\n";
        css << "  --web-card-border: " << cardBorder << ";\n";
        css << "  --web-card-padding: " << cardPadding << ";\n";
        if (!cardHoverBorder.empty()) css << "  --web-card-hover-border: " << cardHoverBorder << ";\n";
        if (!quoteBorder.empty()) css << "  --web-quote-border: " << quoteBorder << ";\n";
        if (!quoteBg.empty()) css << "  --web-quote-bg: " << quoteBg << ";\n";
        css << "  --web-quote-padding: " << quotePadding << ";\n";
        css << "  --web-action-padding: " << actionPadding << ";\n";
        css << "  --web-action-font-size: " << actionFontSize << ";\n";
    } else {
        // Pure browser mechanics: minimal neutral defaults without opinionated aesthetic design system
        css << "  --web-font-sans: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif;\n"
            << "  --web-radius: 0.5rem;\n"
            << "  --web-radius-pill: 9999px;\n"
            << "  --web-border: rgba(128, 128, 128, 0.2);\n"
            << "  --web-card-padding: 1.5rem;\n"
            << "  --web-action-padding: 0.5rem 1rem;\n";
    }

    css << "  --web-shadow-subtle: 0 1px 3px rgba(0, 0, 0, 0.1);\n"
        << "  --web-shadow-raised: 0 4px 6px rgba(0, 0, 0, 0.15);\n"
        << "  --web-shadow-floating: 0 10px 15px rgba(0, 0, 0, 0.2);\n"
        << "}\n\n"
        << R"CSS(* {
  box-sizing: border-box;
  margin: 0;
  padding: 0;
}

body {
  font-family: var(--web-font-sans);
  background-color: var(--web-bg);
  color: var(--web-text);
  line-height: 1.6;
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
  grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
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
.web-spacing--tight { gap: 0.5rem; }
.web-spacing--normal { gap: 1rem; }
.web-spacing--relaxed { gap: 1.5rem; }
.web-spacing--generous { gap: 2.5rem; }

/* Scales */
.web-scale--small { font-size: 0.875rem; }
.web-scale--normal { font-size: 1rem; }
.web-scale--large { font-size: 1.25rem; }
.web-scale--huge { font-size: 2.5rem; line-height: 1.15; }

/* Typography */
.web-weight--normal { font-weight: 400; }
.web-weight--light { font-weight: 300; }
.web-weight--medium { font-weight: 500; }
.web-weight--bold { font-weight: 700; }

.web-tone--default { color: inherit; }
.web-tone--muted { color: var(--web-text-muted); }
.web-tone--accent { color: var(--web-accent); }
.web-tone--prominent { color: var(--web-primary); }

/* Relational Modifiers */
.web-subordinate {
  opacity: 0.8;
  font-size: 0.95em;
}
.web-prominence--quiet {
  opacity: 0.65;
}
.web-prominence--prominent {
  font-weight: 700;
  color: var(--web-primary);
}
.web-prominence--subordinate {
  opacity: 0.8;
}

/* Headings */
h1.web-heading { font-size: 2.75rem; line-height: 1.1; margin-bottom: 0.5rem; }
h2.web-heading { font-size: 2rem; line-height: 1.2; margin-bottom: 0.5rem; }
h3.web-heading { font-size: 1.5rem; line-height: 1.3; margin-bottom: 0.25rem; }
h4.web-heading { font-size: 1.25rem; }
h5.web-heading { font-size: 1.1rem; }
h6.web-heading { font-size: 1rem; }

/* Surface Variants driven by CSS variables */
.web-surface--card {
  background: var(--web-card-bg);
  border: 1px solid var(--web-card-border);
  padding: var(--web-card-padding);
  transition: border-color 0.2s ease;
}
.web-surface--card:hover {
  border-color: var(--web-card-hover-border);
}

.web-surface--subtle {
  background-color: var(--web-bg-subtle);
  border: 1px solid var(--web-border);
  padding: 2rem;
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
  font-weight: 600;
  cursor: pointer;
  border: 1px solid transparent;
  transition: all 0.2s ease;
  text-decoration: none;
  line-height: 1.2;
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
  background-color: rgba(255, 255, 255, 0.05);
  border: 1px solid var(--web-border);
  color: var(--web-text);
}
.web-surface--outline.web-action:hover,
.web-surface--outline.web-btn:hover {
  border-color: var(--web-primary);
  color: var(--web-primary);
  background-color: rgba(56, 189, 248, 0.1);
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
  border-left: 4px solid var(--web-quote-border);
  background: var(--web-quote-bg);
  padding: var(--web-quote-padding);
  font-style: italic;
  border-radius: 0 var(--web-radius) var(--web-radius) 0;
}

/* Divider */
.web-divider {
  border: 0;
  height: 1px;
  background-color: var(--web-border);
  margin: 2rem 0;
  width: 100%;
}
)CSS";

    if (!options.extraCss.empty()) {
        if (options.allowUnrestrictedCss) {
            // Sun Gate 8: Even unrestricted CSS must preserve the HTML parser boundary.
            // Neutralize literal/end-tag-equivalent </style sequences before embedding.
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
    std::string effectiveRole = concept.roleName.empty() || concept.roleName == "unknown"
                                ? (concept.role != SemanticRole::Unknown ? WebConcept::roleToString(concept.role) : "")
                                : concept.roleName;
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
