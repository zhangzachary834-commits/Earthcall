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

class Person;
class Zone;
class Formation;

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
// `@web-designer-channel`. It serves as the compilation, sense/act, and
// realization target for Earthcall Laws, Lexemes, and Metalaws authoring the web.
// ---------------------------------------------------------------------------

class WebDesignerChannel : public Law {
public:
    WebDesignerChannel();
    ~WebDesignerChannel() override = default;

    // First Mover identity
    bool isFirstMover() const override { return true; }
    std::string getIdentifier() const override { return "web-designer-channel"; }

    // Registration and discovery (built-in channel has no human author)
    static void syncRegister(LawManager& laws);
    static WebDesignerChannel* find(LawManager& laws);

    // Compilation and Authoring API (author must be a Person)
    // Authors an element onto an existing Singular bearer (Object, Formation, etc.)
    Singular& authorElementFromPhrase(
        Singular& target,
        const std::string& phrase,
        const std::string& text = "",
        Person* author = nullptr,
        LawManager* laws = nullptr);

    // Convenience authoring: creates a new Singular bearer (Object by default)
    std::shared_ptr<Singular> authorElementFromPhrase(
        const std::string& identifier,
        const std::string& phrase,
        const std::string& text = "",
        Person* author = nullptr,
        LawManager* laws = nullptr);

    // Form a parent-child containment relation between two web beings
    // Relational ordering is governed by relatePrecedes; no numeric order parameter.
    std::shared_ptr<Relation> relateContains(
        const std::shared_ptr<Singular>& parent,
        const std::shared_ptr<Singular>& child);

    // Form a precedence relation between two sibling web beings (first precedes second in DOM sequence)
    std::shared_ptr<Relation> relatePrecedes(
        const std::shared_ptr<Singular>& first,
        const std::shared_ptr<Singular>& second);

    // Canonical production authoring context:
    // When set, newly authored elements and relations are admitted directly into the world/Zone/Formation,
    // ensuring beings are world-owned rather than detached or privately held.
    void setAuthoringContext(Zone* zone, Formation* formation = nullptr);
    Zone* getAuthoringZone() const { return _activeZone; }
    Formation* getAuthoringFormation() const { return _activeFormation; }

    // Canonical production authoring directly into a Zone
    std::shared_ptr<Singular> authorElementInZone(
        Zone& zone,
        const std::string& identifier,
        const std::string& phrase,
        const std::string& text = "",
        Person* author = nullptr,
        LawManager* laws = nullptr);

    // Access uncommitted draft relations (ephemeral authoring scratchpad).
    // Canonical world topology is stored in the Zone/Formation/Universe graph.
    const std::vector<std::shared_ptr<Relation>>& getDraftRelations() const { return _draftRelations; }
    const std::vector<std::shared_ptr<Relation>>& getRelations() const { return _draftRelations; }
    void addDraftRelation(std::shared_ptr<Relation> rel) { if (rel) _draftRelations.push_back(std::move(rel)); }
    void addRelation(std::shared_ptr<Relation> rel) { addDraftRelation(std::move(rel)); }
    void clearDraftRelations() { _draftRelations.clear(); }
    void clearRelations() { clearDraftRelations(); }

    // Find an authored element by its identifier (discovers from Universe/world state first).
    // Returns a borrowed Singular* pointer; never constructs fake shared ownership.
    Singular* findElement(const std::string& identifier) const;

    // Realize a root element / page to HTML (Pure Realization: the core modality interface).
    // Web realization computes and returns HTML; persistent storage is orthogonal.
    std::string realize(const Singular& root, const RealizeOptions& options = RealizeOptions());

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
    std::string _lastActionBeing;
    std::string _lastActionEvent;

    Zone* _activeZone = nullptr;
    Formation* _activeFormation = nullptr;

    // Ephemeral non-authoritative cache of authored elements. The world (Universe/Zone/Formation)
    // owns the beings; the channel holds weak references to prevent private lifetime capture.
    std::unordered_map<std::string, std::weak_ptr<Singular>> _elementCache;

    // Ephemeral draft cache of relations created during authoring.
    // Canonical relational topology is admitted to the active Zone's Formation and/or Universe.
    std::vector<std::shared_ptr<Relation>> _draftRelations;

    // Property getters/setters
    bool propEnabled() const { return _enabled; }
    void propSetEnabled(const bool& v) { _enabled = v; }

    std::string propActiveSite() const { return _activeSite; }
    void propSetActiveSite(const std::string& v) { _activeSite = v; }

    std::string propActivePage() const { return _activePage; }
    void propSetActivePage(const std::string& v) { _activePage = v; }

    std::string propLastRealizedHtml() const { return _lastRealizedHtml; }

    std::string propLastActionBeing() const { return _lastActionBeing; }
    std::string propLastActionEvent() const { return _lastActionEvent; }
};

} // namespace Web
} // namespace Foreign
} // namespace Singularity
