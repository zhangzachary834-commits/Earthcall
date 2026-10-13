#include "Singularity/Foreign/Web/Designer/WebRealizer.hpp"
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

int getBeingOrder(const Singular* s) {
    if (!s) return 0;
    PropertyValue v;
    if (s->getDynamicProperty("web.order", v) || s->getDynamicProperty("order", v)) {
        if (auto d = std::get_if<double>(&v)) return static_cast<int>(*d);
    }
    return 0;
}

// Find all children connected to `parent` via "contains" relations,
// and order them topologically using "precedes" relations.
std::vector<const Singular*> findChildren(const Singular& parent,
                                         const std::vector<std::shared_ptr<Relation>>& relations) {
    std::vector<const Singular*> rawChildren;
    std::unordered_set<const Singular*> childSet;

    // 1. Collect all children connected via "contains"
    for (const auto& rel : relations) {
        if (!rel) continue;
        if (rel->type == WebRelationType::kContains && rel->a() == &parent) {
            const Singular* child = rel->b();
            if (child && childSet.insert(child).second) {
                rawChildren.push_back(child);
            }
        }
    }

    if (rawChildren.empty()) return {};

    const size_t n = rawChildren.size();
    std::unordered_map<const Singular*, size_t> indexMap;
    for (size_t i = 0; i < n; ++i) {
        indexMap[rawChildren[i]] = i;
    }

    // 2. Build DAG from "precedes" relations
    std::vector<int> inDegree(n, 0);
    std::vector<std::vector<size_t>> adj(n);

    for (const auto& rel : relations) {
        if (!rel) continue;
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
    }

    // 3. Topological sort using Kahn's algorithm
    // Comparator for deterministic queue: prefer lower getBeingOrder, then discovery index
    auto cmp = [&](size_t i, size_t j) {
        int ordI = getBeingOrder(rawChildren[i]);
        int ordJ = getBeingOrder(rawChildren[j]);
        if (ordI != ordJ) return ordI > ordJ; // Min-heap
        return i > j;
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

    // Append any unvisited nodes if a cycle or unconstrained node was missed
    if (sortedChildren.size() < n) {
        std::vector<size_t> remaining;
        for (size_t i = 0; i < n; ++i) {
            if (!visited[i]) remaining.push_back(i);
        }
        std::stable_sort(remaining.begin(), remaining.end(), [&](size_t a, size_t b) {
            return getBeingOrder(rawChildren[a]) < getBeingOrder(rawChildren[b]);
        });
        for (size_t idx : remaining) {
            sortedChildren.push_back(rawChildren[idx]);
        }
    }

    return sortedChildren;
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

std::string WebRealizer::generateStyleSheet(const Singular* root) {
    // Read authored theme tokens from root if available, otherwise use clean defaults
    std::string bg = getThemeString(root, "theme.backgroundColor", "#0b0f19");
    std::string surface = getThemeString(root, "theme.surfaceColor", "#131c2e");
    std::string card = getThemeString(root, "theme.cardColor", "#182338");
    std::string subtle = getThemeString(root, "theme.subtleColor", "#0f172a");
    std::string text = getThemeString(root, "theme.textColor", "#f8fafc");
    std::string textMuted = getThemeString(root, "theme.textMutedColor", "#94a3b8");
    std::string primary = getThemeString(root, "theme.primaryColor", "#38bdf8");
    std::string accent = getThemeString(root, "theme.accentColor", "#818cf8");
    std::string radius = getThemeString(root, "theme.radius", "0.75rem");
    std::string fontSans = getThemeString(root, "theme.fontFamily", "-apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif");

    std::ostringstream css;
    css << ":root {\n"
        << "  --web-bg: " << bg << ";\n"
        << "  --web-bg-surface: " << surface << ";\n"
        << "  --web-bg-card: " << card << ";\n"
        << "  --web-bg-subtle: " << subtle << ";\n"
        << "  --web-text: " << text << ";\n"
        << "  --web-text-muted: " << textMuted << ";\n"
        << "  --web-primary: " << primary << ";\n"
        << "  --web-primary-hover: " << primary << ";\n"
        << "  --web-primary-text: " << bg << ";\n"
        << "  --web-secondary: #334155;\n"
        << "  --web-secondary-hover: #475569;\n"
        << "  --web-secondary-text: #f8fafc;\n"
        << "  --web-accent: " << accent << ";\n"
        << "  --web-border: rgba(255, 255, 255, 0.08);\n"
        << "  --web-border-card: rgba(56, 189, 248, 0.15);\n"
        << "  --web-radius: " << radius << ";\n"
        << "  --web-radius-pill: 9999px;\n"
        << "  --web-font-sans: " << fontSans << ";\n"
        << "  --web-shadow-subtle: 0 4px 6px -1px rgba(0, 0, 0, 0.2), 0 2px 4px -2px rgba(0, 0, 0, 0.2);\n"
        << "  --web-shadow-raised: 0 10px 15px -3px rgba(0, 0, 0, 0.4), 0 4px 6px -4px rgba(0, 0, 0, 0.3);\n"
        << "  --web-shadow-floating: 0 20px 25px -5px rgba(0, 0, 0, 0.5), 0 8px 10px -6px rgba(0, 0, 0, 0.4);\n"
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
  max-width: 1200px;
  margin-left: auto;
  margin-right: auto;
  padding-left: 1.5rem;
  padding-right: 1.5rem;
}

.web-navigation {
  width: 100%;
  border-bottom: 1px solid var(--web-border);
  background-color: rgba(11, 15, 25, 0.85);
  backdrop-filter: blur(12px);
  position: sticky;
  top: 0;
  z-index: 100;
  padding: 1rem 1.5rem;
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

/* Surface Variants */
.web-surface--card {
  background-color: var(--web-bg-card);
  border: 1px solid var(--web-border-card);
  padding: 1.75rem;
  transition: border-color 0.2s ease;
}
.web-surface--card:hover {
  border-color: var(--web-primary);
}

.web-surface--subtle {
  background-color: var(--web-bg-subtle);
  border: 1px solid var(--web-border);
  padding: 2rem;
}

.web-surface--hero {
  background: radial-gradient(circle at 50% 0%, rgba(56, 189, 248, 0.12) 0%, transparent 70%);
  padding: 5rem 1.5rem;
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
  padding: 0.75rem 1.75rem;
  font-size: 0.95rem;
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
  border-left: 4px solid var(--web-primary);
  padding: 1.5rem 2rem;
  font-style: italic;
  background-color: var(--web-bg-subtle);
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
                                       int indentLevel) {
    WebConcept concept = WebConcept::readFromSingular(element);
    std::string indent = makeIndent(indentLevel);
    std::ostringstream out;

    // Collect CSS classes
    std::vector<std::string> classes;
    std::string effectiveRole = concept.roleName.empty() || concept.roleName == "unknown"
                                ? WebConcept::roleToString(concept.role)
                                : concept.roleName;
    std::string effectiveFlow = concept.flowName.empty() ? WebConcept::flowToString(concept.flow) : concept.flowName;
    std::string effectiveAlign = concept.alignName.empty() ? WebConcept::alignToString(concept.align) : concept.alignName;
    std::string effectiveJustify = concept.justifyName.empty() ? WebConcept::justifyToString(concept.justify) : concept.justifyName;
    std::string effectiveSpacing = concept.spacingName.empty() ? WebConcept::spacingToString(concept.spacing) : concept.spacingName;
    std::string effectiveScale = concept.scaleName.empty() ? WebConcept::scaleToString(concept.scale) : concept.scaleName;
    std::string effectiveWeight = concept.weightName.empty() ? WebConcept::weightToString(concept.weight) : concept.weightName;
    std::string effectiveTone = concept.toneName.empty() ? WebConcept::toneToString(concept.tone) : concept.toneName;
    std::string effectiveVariant = concept.variantName.empty() || concept.variantName == "default"
                                   ? WebConcept::variantToString(concept.variant)
                                   : concept.variantName;
    std::string effectiveShape = concept.shapeName.empty() ? WebConcept::shapeToString(concept.shape) : concept.shapeName;
    std::string effectiveElevation = concept.elevationName.empty() ? WebConcept::elevationToString(concept.elevation) : concept.elevationName;

    classes.push_back("web-" + effectiveRole);
    classes.push_back("web-flow--" + effectiveFlow);
    classes.push_back("web-align--" + effectiveAlign);
    classes.push_back("web-justify--" + effectiveJustify);
    classes.push_back("web-spacing--" + effectiveSpacing);
    classes.push_back("web-scale--" + effectiveScale);
    classes.push_back("web-weight--" + effectiveWeight);
    classes.push_back("web-tone--" + effectiveTone);
    classes.push_back("web-surface--" + effectiveVariant);
    classes.push_back("web-shape--" + effectiveShape);
    classes.push_back("web-elevation--" + effectiveElevation);

    if (concept.subordinate) {
        classes.push_back("web-subordinate");
    }
    if (!concept.prominence.empty()) {
        classes.push_back("web-prominence--" + concept.prominence);
    }

    std::string classAttr = "class=\"";
    for (size_t i = 0; i < classes.size(); ++i) {
        if (i > 0) classAttr += " ";
        classAttr += classes[i];
    }
    classAttr += "\"";

    // Dynamic style attribute for custom colors
    std::string styleAttr;
    if (!concept.background.empty() || !concept.color.empty()) {
        styleAttr = " style=\"";
        if (!concept.background.empty()) styleAttr += "background-color: " + escapeAttr(concept.background) + "; ";
        if (!concept.color.empty()) styleAttr += "color: " + escapeAttr(concept.color) + "; ";
        styleAttr += "\"";
    }

    std::string beingAttr = "data-being-id=\"" + escapeAttr(element.getIdentifier()) + "\"";
    if (concept.subordinate) {
        beingAttr += " data-subordinate=\"true\"";
    }

    // Children ordered topologically by "precedes" relations
    auto children = findChildren(element, relations);

    // Escape hatch for explicit tag
    if (!concept.elementTag.empty()) {
        std::string tag = escapeAttr(concept.elementTag);
        out << indent << "<" << tag << " id=\"" << escapeAttr(element.getIdentifier()) << "\" " << classAttr << styleAttr << " " << beingAttr << ">\n";
        if (!concept.text.empty()) {
            out << indent << "  " << escapeHtml(concept.text) << "\n";
        }
        for (const auto* child : children) {
            out << realizeElement(*child, relations, indentLevel + 1);
        }
        out << indent << "</" << tag << ">\n";
        return out.str();
    }

    switch (concept.role) {
        case SemanticRole::Page: {
            out << indent << "<main id=\"" << escapeAttr(element.getIdentifier()) << "\" " << classAttr << styleAttr << " " << beingAttr << ">\n";
            for (const auto* child : children) {
                out << realizeElement(*child, relations, indentLevel + 1);
            }
            out << indent << "</main>\n";
            break;
        }

        case SemanticRole::Section: {
            out << indent << "<section id=\"" << escapeAttr(element.getIdentifier()) << "\" " << classAttr << styleAttr << " " << beingAttr << ">\n";
            for (const auto* child : children) {
                out << realizeElement(*child, relations, indentLevel + 1);
            }
            out << indent << "</section>\n";
            break;
        }

        case SemanticRole::Navigation: {
            out << indent << "<nav id=\"" << escapeAttr(element.getIdentifier()) << "\" " << classAttr << styleAttr << " " << beingAttr << ">\n";
            for (const auto* child : children) {
                out << realizeElement(*child, relations, indentLevel + 1);
            }
            out << indent << "</nav>\n";
            break;
        }

        case SemanticRole::Heading: {
            std::string tag = "h" + std::to_string(std::clamp(concept.headingLevel, 1, 6));
            out << indent << "<" << tag << " id=\"" << escapeAttr(element.getIdentifier()) << "\" " << classAttr << styleAttr << " " << beingAttr << ">"
                << escapeHtml(concept.text) << "</" << tag << ">\n";
            break;
        }

        case SemanticRole::Paragraph: {
            out << indent << "<p id=\"" << escapeAttr(element.getIdentifier()) << "\" " << classAttr << styleAttr << " " << beingAttr << ">"
                << escapeHtml(concept.text) << "</p>\n";
            break;
        }

        case SemanticRole::Action: {
            std::string eventAttr = concept.actionEvent.empty() ? "" : " data-action-event=\"" + escapeAttr(concept.actionEvent) + "\"";

            if (!concept.actionHref.empty()) {
                out << indent << "<a href=\"" << escapeAttr(concept.actionHref) << "\" " << classAttr << " " << beingAttr << eventAttr << styleAttr << ">"
                    << escapeHtml(concept.text) << "</a>\n";
            } else {
                out << indent << "<button type=\"button\" " << classAttr << " " << beingAttr << eventAttr << styleAttr << ">"
                    << escapeHtml(concept.text) << "</button>\n";
            }
            break;
        }

        case SemanticRole::Container: {
            out << indent << "<div id=\"" << escapeAttr(element.getIdentifier()) << "\" " << classAttr << styleAttr << " " << beingAttr << ">\n";
            if (!concept.text.empty()) {
                out << indent << "  <p class=\"web-paragraph\">" << escapeHtml(concept.text) << "</p>\n";
            }
            for (const auto* child : children) {
                out << realizeElement(*child, relations, indentLevel + 1);
            }
            out << indent << "</div>\n";
            break;
        }

        case SemanticRole::List: {
            out << indent << "<ul id=\"" << escapeAttr(element.getIdentifier()) << "\" " << classAttr << styleAttr << " " << beingAttr << ">\n";
            for (const auto* child : children) {
                out << realizeElement(*child, relations, indentLevel + 1);
            }
            out << indent << "</ul>\n";
            break;
        }

        case SemanticRole::Item: {
            out << indent << "<li id=\"" << escapeAttr(element.getIdentifier()) << "\" " << classAttr << styleAttr << " " << beingAttr << ">\n";
            if (!concept.text.empty()) {
                out << indent << "  " << escapeHtml(concept.text) << "\n";
            }
            for (const auto* child : children) {
                out << realizeElement(*child, relations, indentLevel + 1);
            }
            out << indent << "</li>\n";
            break;
        }

        case SemanticRole::Media: {
            out << indent << "<img id=\"" << escapeAttr(element.getIdentifier()) << "\" src=\"" << escapeAttr(concept.mediaSrc) << "\" alt=\""
                << escapeAttr(concept.mediaAlt) << "\" " << classAttr << styleAttr << " " << beingAttr << " />\n";
            break;
        }

        case SemanticRole::Quote: {
            out << indent << "<blockquote id=\"" << escapeAttr(element.getIdentifier()) << "\" " << classAttr << styleAttr << " " << beingAttr << ">\n";
            out << indent << "  <p>" << escapeHtml(concept.text) << "</p>\n";
            out << indent << "</blockquote>\n";
            break;
        }

        case SemanticRole::Divider: {
            out << indent << "<hr id=\"" << escapeAttr(element.getIdentifier()) << "\" " << classAttr << " " << beingAttr << " />\n";
            break;
        }

        default: {
            // Open semantic role / generic fallback (e.g. "sanctuary", "article", etc.)
            out << indent << "<div id=\"" << escapeAttr(element.getIdentifier()) << "\" " << classAttr << styleAttr << " " << beingAttr << ">\n";
            if (!concept.text.empty()) {
                out << indent << "  " << escapeHtml(concept.text) << "\n";
            }
            for (const auto* child : children) {
                out << realizeElement(*child, relations, indentLevel + 1);
            }
            out << indent << "</div>\n";
            break;
        }
    }

    return out.str();
}

std::string WebRealizer::realizeFullDocument(const Singular& root,
                                            const std::vector<std::shared_ptr<Relation>>& relations,
                                            const RealizeOptions& options) {
    std::ostringstream doc;
    doc << "<!DOCTYPE html>\n";
    doc << "<html lang=\"en\">\n";
    doc << "<head>\n";
    doc << "  <meta charset=\"UTF-8\" />\n";
    doc << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" />\n";
    doc << "  <title>" << escapeHtml(options.title) << "</title>\n";
    doc << "  <meta name=\"description\" content=\"" << escapeAttr(options.metaDescription) << "\" />\n";
    doc << "  <style>\n" << generateStyleSheet(&root);
    if (!options.extraCss.empty()) {
        doc << "\n" << options.extraCss << "\n";
    }
    doc << "  </style>\n";
    doc << "</head>\n";
    doc << "<body>\n";

    doc << realizeElement(root, relations, 1);

    if (options.includeInteractiveScript) {
        doc << "  <script>\n" << generateClientScript() << "  </script>\n";
    }

    doc << "</body>\n";
    doc << "</html>\n";
    return doc.str();
}

} // namespace Web
} // namespace Foreign
} // namespace Singularity
