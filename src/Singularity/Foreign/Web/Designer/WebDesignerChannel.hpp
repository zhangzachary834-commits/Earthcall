#pragma once

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "Singularity/Foreign/Web/Designer/WebConcept.hpp"
#include "Singularity/Foreign/Web/Designer/WebRealizer.hpp"
#include "Relation/Relation.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>

namespace Singularity {
namespace Foreign {
namespace Web {

// ---------------------------------------------------------------------------
// WebDesignerChannel: The Singularity Modality Channel for Website Authoring
//
// In accordance with Earthcall ontology:
//   "A channel to hardware or foreign software goes inside Singularity/"
//   "No subsystem may define what a thing IS. Subsystems define how the machine
//    senses and acts; Persons author what things are in-world."
//
// WebDesignerChannel is a First Mover Law with a stable identifier
// `@web-designer-channel`. It serves as the compilation and realization
// target for Earthcall Laws, Lexemes, and Metalaws authoring the web.
// ---------------------------------------------------------------------------

class WebDesignerChannel : public Law {
public:
    WebDesignerChannel();
    ~WebDesignerChannel() override = default;

    // First Mover identity
    bool isFirstMover() const override { return true; }
    std::string getIdentifier() const override { return "web-designer-channel"; }

    // Registration and discovery
    static void syncRegister(LawManager& laws, Singular* author = nullptr);
    static WebDesignerChannel* find(LawManager& laws);

    // Compilation and Authoring API
    std::shared_ptr<Singular> authorElementFromPhrase(
        const std::string& identifier,
        const std::string& phrase,
        const std::string& text = "",
        Singular* author = nullptr,
        LawManager* laws = nullptr);

    // Form a parent-child containment relation between two web beings
    std::shared_ptr<Relation> relateContains(
        const std::shared_ptr<Singular>& parent,
        const std::shared_ptr<Singular>& child,
        int order = 0);

    // Access current live relations
    const std::vector<std::shared_ptr<Relation>>& getRelations() const { return _relations; }
    void addRelation(std::shared_ptr<Relation> rel) { if (rel) _relations.push_back(std::move(rel)); }
    void clearRelations() { _relations.clear(); }

    // Find an authored element by its identifier
    std::shared_ptr<Singular> findElement(const std::string& identifier) const;

    // Realize a root element / page to HTML
    std::string realize(const Singular& root, const RealizeOptions& options = RealizeOptions());

    // Export realized HTML to a local file
    bool exportHtml(const Singular& root,
                    const std::string& filePath,
                    const RealizeOptions& options = RealizeOptions());

    // Ingest a web action (click, toggle) and trigger Earthcall Law events
    void handleWebAction(const std::string& beingId,
                         const std::string& eventName,
                         LawManager& laws,
                         Singular* targetBeing = nullptr);

    // Modality Sense/Act frame cycle
    void sense(LawManager& laws);
    void act(LawManager& laws);

protected:
    void buildProperties() override;

private:
    bool _enabled = true;
    std::string _activeSite = "Earthcall Website";
    std::string _activePage = "index";
    std::string _lastRealizedHtml;
    std::string _exportPath;
    std::string _lastActionBeing;
    std::string _lastActionEvent;

    // Track active authored elements and relations for realization
    std::unordered_map<std::string, std::shared_ptr<Singular>> _authoredElements;
    std::vector<std::shared_ptr<Relation>> _relations;

    // Property getters/setters
    bool propEnabled() const { return _enabled; }
    void propSetEnabled(const bool& v) { _enabled = v; }

    std::string propActiveSite() const { return _activeSite; }
    void propSetActiveSite(const std::string& v) { _activeSite = v; }

    std::string propActivePage() const { return _activePage; }
    void propSetActivePage(const std::string& v) { _activePage = v; }

    std::string propLastRealizedHtml() const { return _lastRealizedHtml; }

    std::string propExportPath() const { return _exportPath; }
    void propSetExportPath(const std::string& v) { _exportPath = v; }

    std::string propLastActionBeing() const { return _lastActionBeing; }
    std::string propLastActionEvent() const { return _lastActionEvent; }
};

} // namespace Web
} // namespace Foreign
} // namespace Singularity
