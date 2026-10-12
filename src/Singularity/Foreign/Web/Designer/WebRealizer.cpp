#include "Singularity/Foreign/Web/Designer/WebRealizer.hpp"
#include <algorithm>
#include <sstream>

namespace Singularity {
namespace Foreign {
namespace Web {

namespace {

std::string makeIndent(int level) {
    return std::string(level * 2, ' ');
}

// Find all children connected to `parent` via "contains" relations
std::vector<const Singular*> findChildren(const Singular& parent,
                                         const std::vector<std::shared_ptr<Relation>>& relations) {
    std::vector<std::pair<int, const Singular*>> orderedChildren;

    for (const auto& rel : relations) {
        if (!rel) continue;
        if (rel->type == WebRelationType::kContains && rel->a() == &parent) {
            const Singular* child = rel->b();
            if (child) {
                // Determine order
                int order = 0;
                PropertyValue v;
                if (child->getDynamicProperty("web.order", v)) {
                    if (auto d = std::get_if<double>(&v)) order = static_cast<int>(*d);
                }
                orderedChildren.push_back({order, child});
            }
        }
    }

    // Stable sort by order
    std::stable_sort(orderedChildren.begin(), orderedChildren.end(),
                     [](const auto& a, const auto& b) { return a.first < b.first; });

    std::vector<const Singular*> result;
    result.reserve(orderedChildren.size());
    for (const auto& pair : orderedChildren) {
        result.push_back(pair.second);
    }
    return result;
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

std::string WebRealizer::generateStyleSheet() {
    return R"CSS(
:root {
  --web-bg: #0b0f19;
  --web-bg-surface: #131c2e;
  --web-bg-card: #182338;
  --web-bg-subtle: #0f172a;
  --web-text: #f8fafc;
  --web-text-muted: #94a3b8;
  --web-primary: #38bdf8;
  --web-primary-hover: #0284c7;
  --web-primary-text: #0b0f19;
  --web-secondary: #334155;
  --web-secondary-hover: #475569;
  --web-secondary-text: #f8fafc;
  --web-accent: #818cf8;
  --web-border: rgba(255, 255, 255, 0.08);
  --web-border-card: rgba(56, 189, 248, 0.15);
  --web-radius: 0.75rem;
  --web-radius-pill: 9999px;
  --web-font-sans: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif;
  --web-shadow-subtle: 0 4px 6px -1px rgba(0, 0, 0, 0.2), 0 2px 4px -2px rgba(0, 0, 0, 0.2);
  --web-shadow-raised: 0 10px 15px -3px rgba(0, 0, 0, 0.4), 0 4px 6px -4px rgba(0, 0, 0, 0.3);
  --web-shadow-floating: 0 20px 25px -5px rgba(0, 0, 0, 0.5), 0 8px 10px -6px rgba(0, 0, 0, 0.4);
}

* {
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
  margin: 0 auto;
  padding: 3rem 1.5rem;
}

.web-nav {
  width: 100%;
  border-bottom: 1px solid var(--web-border);
  background-color: rgba(11, 15, 25, 0.85);
  backdrop-filter: blur(12px);
  position: sticky;
  top: 0;
  z-index: 50;
  padding: 1rem 2rem;
}

/* Layout Flow */
.web-flow--vertical {
  display: flex;
  flex-direction: column;
}

.web-flow--horizontal {
  display: flex;
  flex-direction: row;
  flex-wrap: wrap;
}

.web-flow--horizontal.web-align--center {
  justify-content: center;
}

.web-flow--grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
  width: 100%;
}

.web-navigation {
  width: 100%;
  display: flex;
  align-items: center;
  padding: 1.25rem 0;
  border-bottom: 1px solid var(--web-border);
  margin-bottom: 1.5rem;
}

.web-navigation.web-flow--horizontal {
  flex-wrap: nowrap;
}

.web-navigation .web-container {
  width: auto;
}

/* Alignment & Justification */
.web-align--center {
  align-items: center;
  text-align: center;
}
.web-align--start {
  align-items: flex-start;
  text-align: left;
}
.web-align--end {
  align-items: flex-end;
  text-align: right;
}
.web-align--stretch {
  align-items: stretch;
}

.web-justify--start { justify-content: flex-start; }
.web-justify--center { justify-content: center; }
.web-justify--end { justify-content: flex-end; }
.web-justify--between { justify-content: space-between; }
.web-justify--around { justify-content: space-around; }

/* Spacing Scales */
.web-spacing--none { gap: 0; }
.web-spacing--tight { gap: 0.5rem; }
.web-spacing--normal { gap: 1rem; }
.web-spacing--relaxed { gap: 1.5rem; }
.web-spacing--generous { gap: 2.5rem; }

/* Typography & Scale */
.web-heading {
  font-weight: 700;
  line-height: 1.2;
  letter-spacing: -0.025em;
  color: var(--web-text);
}
.web-scale--small { font-size: 0.875rem; }
.web-scale--normal { font-size: 1rem; }
.web-scale--large { font-size: 2.25rem; }
.web-scale--huge { font-size: 3.5rem; font-weight: 800; letter-spacing: -0.04em; }

.web-scale--large.web-action,
.web-scale--large.web-btn {
  padding: 0.9rem 2rem;
  font-size: 1.05rem;
}

.web-weight--bold { font-weight: 700; }
.web-weight--medium { font-weight: 500; }
.web-weight--light { font-weight: 300; }

.web-tone--muted { color: var(--web-text-muted); }
.web-tone--prominent { color: var(--web-text); font-size: 1.25rem; line-height: 1.7; }
.web-tone--accent { color: var(--web-primary); }

.web-paragraph {
  max-width: 65ch;
  color: var(--web-text-muted);
}

/* Containers & Cards */
.web-container {
  width: 100%;
}
.web-flow--horizontal > .web-container {
  width: auto;
}

.web-surface--card {
  background-color: var(--web-bg-card);
  border: 1px solid var(--web-border-card);
  padding: 2rem;
  transition: transform 0.2s ease, border-color 0.2s ease, box-shadow 0.2s ease;
}
.web-surface--card:hover {
  border-color: var(--web-primary);
  transform: translateY(-2px);
}

.web-surface--subtle {
  background-color: var(--web-bg-subtle);
  border: 1px solid var(--web-border);
  padding: 2rem;
}

.web-surface--hero {
  background: radial-gradient(circle at 50% 0%, rgba(56, 189, 248, 0.15) 0%, transparent 70%);
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
  transform: translateY(-1px);
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

/* Interactive Feedback Bar */
#earthcall-feedback {
  position: fixed;
  bottom: 1.5rem;
  right: 1.5rem;
  background-color: #1e293b;
  border: 1px solid var(--web-primary);
  color: var(--web-primary);
  padding: 0.75rem 1.25rem;
  border-radius: var(--web-radius);
  font-size: 0.875rem;
  font-weight: 500;
  box-shadow: var(--web-shadow-raised);
  opacity: 0;
  transform: translateY(10px);
  transition: opacity 0.3s ease, transform 0.3s ease;
  pointer-events: none;
  z-index: 1000;
}
#earthcall-feedback.active {
  opacity: 1;
  transform: translateY(0);
}
)CSS";
}

std::string WebRealizer::generateClientScript() {
    return R"JS(
(function() {
  function showFeedback(msg) {
    let bar = document.getElementById('earthcall-feedback');
    if (!bar) {
      bar = document.createElement('div');
      bar.id = 'earthcall-feedback';
      document.body.appendChild(bar);
    }
    bar.textContent = msg;
    bar.classList.add('active');
    setTimeout(() => bar.classList.remove('active'), 2500);
  }

  document.addEventListener('DOMContentLoaded', () => {
    document.querySelectorAll('[data-action-event]').forEach(el => {
      el.addEventListener('click', (e) => {
        const eventName = el.getAttribute('data-action-event');
        const beingId = el.getAttribute('data-being-id') || 'unknown';
        console.log('[Earthcall Web Designer] Action triggered:', eventName, 'from being:', beingId);
        showFeedback('Law Event: ' + eventName + ' (' + beingId + ')');

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
    classes.push_back("web-" + WebConcept::roleToString(concept.role));
    classes.push_back("web-flow--" + WebConcept::flowToString(concept.flow));
    classes.push_back("web-align--" + WebConcept::alignToString(concept.align));
    classes.push_back("web-justify--" + WebConcept::justifyToString(concept.justify));
    classes.push_back("web-spacing--" + WebConcept::spacingToString(concept.spacing));
    classes.push_back("web-scale--" + WebConcept::scaleToString(concept.scale));
    classes.push_back("web-weight--" + WebConcept::weightToString(concept.weight));
    classes.push_back("web-tone--" + WebConcept::toneToString(concept.tone));
    classes.push_back("web-surface--" + WebConcept::variantToString(concept.variant));
    classes.push_back("web-shape--" + WebConcept::shapeToString(concept.shape));
    classes.push_back("web-elevation--" + WebConcept::elevationToString(concept.elevation));

    std::string classAttr = "class=\"";
    for (size_t i = 0; i < classes.size(); ++i) {
        if (i > 0) classAttr += " ";
        classAttr += classes[i];
    }
    classAttr += "\"";

    // Style overrides
    std::string styleAttr;
    if (!concept.background.empty() || !concept.color.empty()) {
        styleAttr = " style=\"";
        if (!concept.background.empty()) styleAttr += "background-color: " + escapeAttr(concept.background) + "; ";
        if (!concept.color.empty()) styleAttr += "color: " + escapeAttr(concept.color) + "; ";
        styleAttr += "\"";
    }

    // Children
    auto children = findChildren(element, relations);

    switch (concept.role) {
        case SemanticRole::Page: {
            out << indent << "<div id=\"" << escapeAttr(element.getIdentifier()) << "\" " << classAttr << styleAttr << ">\n";
            for (const auto* child : children) {
                out << realizeElement(*child, relations, indentLevel + 1);
            }
            out << indent << "</div>\n";
            break;
        }

        case SemanticRole::Section: {
            out << indent << "<section id=\"" << escapeAttr(element.getIdentifier()) << "\" " << classAttr << styleAttr << ">\n";
            for (const auto* child : children) {
                out << realizeElement(*child, relations, indentLevel + 1);
            }
            out << indent << "</section>\n";
            break;
        }

        case SemanticRole::Navigation: {
            out << indent << "<nav id=\"" << escapeAttr(element.getIdentifier()) << "\" " << classAttr << " role=\"navigation\"" << styleAttr << ">\n";
            for (const auto* child : children) {
                out << realizeElement(*child, relations, indentLevel + 1);
            }
            out << indent << "</nav>\n";
            break;
        }

        case SemanticRole::Heading: {
            std::string tag = "h" + std::to_string(std::clamp(concept.headingLevel, 1, 6));
            out << indent << "<" << tag << " id=\"" << escapeAttr(element.getIdentifier()) << "\" " << classAttr << styleAttr << ">"
                << escapeHtml(concept.text) << "</" << tag << ">\n";
            break;
        }

        case SemanticRole::Paragraph: {
            out << indent << "<p id=\"" << escapeAttr(element.getIdentifier()) << "\" " << classAttr << styleAttr << ">"
                << escapeHtml(concept.text) << "</p>\n";
            break;
        }

        case SemanticRole::Action: {
            std::string beingAttr = "data-being-id=\"" + escapeAttr(element.getIdentifier()) + "\"";
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
            out << indent << "<div id=\"" << escapeAttr(element.getIdentifier()) << "\" " << classAttr << styleAttr << ">\n";
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
            out << indent << "<ul id=\"" << escapeAttr(element.getIdentifier()) << "\" " << classAttr << styleAttr << ">\n";
            for (const auto* child : children) {
                out << realizeElement(*child, relations, indentLevel + 1);
            }
            out << indent << "</ul>\n";
            break;
        }

        case SemanticRole::Item: {
            out << indent << "<li id=\"" << escapeAttr(element.getIdentifier()) << "\" " << classAttr << styleAttr << ">\n";
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
                << escapeAttr(concept.mediaAlt) << "\" " << classAttr << styleAttr << " />\n";
            break;
        }

        case SemanticRole::Quote: {
            out << indent << "<blockquote id=\"" << escapeAttr(element.getIdentifier()) << "\" " << classAttr << styleAttr << ">\n";
            out << indent << "  <p>" << escapeHtml(concept.text) << "</p>\n";
            out << indent << "</blockquote>\n";
            break;
        }

        case SemanticRole::Divider: {
            out << indent << "<hr id=\"" << escapeAttr(element.getIdentifier()) << "\" " << classAttr << " />\n";
            break;
        }

        default: {
            // Generic container fallback
            out << indent << "<div id=\"" << escapeAttr(element.getIdentifier()) << "\" " << classAttr << styleAttr << ">\n";
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
    doc << "  <style>\n" << generateStyleSheet();
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
