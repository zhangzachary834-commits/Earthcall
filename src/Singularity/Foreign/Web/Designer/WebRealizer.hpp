#pragma once

#include "Singularity/Foreign/Web/Designer/WebConcept.hpp"
#include "Relation/Relation.hpp"
#include "Relation/Formation/Formation.hpp"
#include <string>
#include <vector>
#include <memory>

namespace Singularity {
namespace Foreign {
namespace Web {

// ---------------------------------------------------------------------------
// RealizeOptions
// ---------------------------------------------------------------------------

struct RealizeOptions {
    std::string title;
    std::string metaDescription;
    bool includeInteractiveScript;
    bool minify;
    std::string extraCss;

    RealizeOptions()
        : title("Earthcall Web Designer"),
          metaDescription("Authored natively through Earthcall Laws and Lexemes"),
          includeInteractiveScript(true),
          minify(false),
          extraCss(""),
          allowUnrestrictedCss(false) {}

    bool allowUnrestrictedCss; // Requires explicit Person governance to inject raw unrestricted CSS
};

// ---------------------------------------------------------------------------
// WebRealizer: Realization Substrate from Semantic Beings to HTML/CSS/DOM
//
// In accordance with Earthcall ontology:
//   "HTML is an implementation target, not Earthcall's ontology."
//
// The WebRealizer traverses the semantic Singular graph and realizes it into
// clean, accessible, modern HTML5 and responsive CSS.
// ---------------------------------------------------------------------------

class WebRealizer {
public:
    // Realize a full HTML5 document from a root Singular (Page or Site Formation)
    static std::string realizeFullDocument(const Singular& root,
                                           const std::vector<std::shared_ptr<Relation>>& relations,
                                           const RealizeOptions& options = RealizeOptions());

    // Realize a single element (and its children recursively) to an HTML fragment
    static std::string realizeElement(const Singular& element,
                                      const std::vector<std::shared_ptr<Relation>>& relations,
                                      int indentLevel = 0,
                                      std::string* outError = nullptr);

    // Find all children connected to parent via 'contains' and ordered by 'precedes'.
    // Detects cycles in 'precedes' relations and refuses/reports rather than falling back to fiat.
    static std::vector<const Singular*> findChildren(const Singular& parent,
                                                     const std::vector<std::shared_ptr<Relation>>& relations,
                                                     std::string* outError = nullptr);

    // Generate the minimal responsive CSS stylesheet with authored design tokens
    static std::string generateStyleSheet(const Singular* root = nullptr,
                                          const RealizeOptions& options = RealizeOptions());

    // Generate the interactive DOM client script that hooks clicks into Earthcall
    static std::string generateClientScript();

    // Helper: Escapes text for HTML content
    static std::string escapeHtml(const std::string& text);

    // Helper: Escapes attribute value
    static std::string escapeAttr(const std::string& attr);

    // Browser Trust Boundary Sanitizers:
    // C++ may know how a browser works; C++ must not decide what the Person means.
    // Validates HTML tag identifier (letters, digits, dashes; blocks script/style/iframe/etc.; fallback: "div")
    static std::string sanitizeHtmlTag(const std::string& tag);

    // Normalizes CSS class-token fragment to safe characters [a-z0-9_-]
    static std::string normalizeCssToken(const std::string& token);

    // Enforces URL scheme policy on action hrefs (blocks javascript:/data:/vbscript:; permits http, https, mailto, tel, #, /)
    static std::string sanitizeHref(const std::string& href);

    // Enforces URL scheme policy on media src (blocks javascript:/vbscript:/file:; permits http, https, data:image/, relative paths)
    static std::string sanitizeMediaSrc(const std::string& src);

    // Escapes/neutralizes </style> inside CSS values to prevent <style> tag breakout
    static std::string sanitizeCssValue(const std::string& value);
};

} // namespace Web
} // namespace Foreign
} // namespace Singularity
