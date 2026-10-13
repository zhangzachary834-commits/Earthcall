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
          extraCss("") {}
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
                                      int indentLevel = 0);

    // Generate the complete responsive CSS stylesheet with authored design tokens
    static std::string generateStyleSheet(const Singular* root = nullptr);

    // Generate the interactive DOM client script that hooks clicks into Earthcall
    static std::string generateClientScript();

    // Helper: Escapes text for HTML content
    static std::string escapeHtml(const std::string& text);

    // Helper: Escapes attribute value
    static std::string escapeAttr(const std::string& attr);
};

} // namespace Web
} // namespace Foreign
} // namespace Singularity
