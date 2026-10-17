#include "Singularity/Foreign/Web/Designer/WebDesignerChannel.hpp"
#include "Singularity/Foreign/Web/Designer/WebDesignerMetalaws.hpp"
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"
#include "Singularity/Storage/FileChannel.hpp"
#include "Singularity/Storage/VirtualFileSystem.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "Relation/Formation/Formation.hpp"
#include "Person/Person.hpp"
#include <iostream>

namespace Singularity {
namespace Foreign {
namespace Web {

WebDesignerChannel::WebDesignerChannel()
    : Law("web-designer-channel") {
    setLawIdentifier("web-designer-channel");
    // Legacy numerical authority level. Pending AD-24 (which retires integer authority
    // levels in favor of an authority-over Relation DAG descending from First Movers),
    // grantAuthority(10) serves as compatibility substrate for Kernel/Singularity level authority.
    grantAuthority(10);
}

void WebDesignerChannel::buildProperties() {
    registerProperty(std::make_unique<ComputedProperty<WebDesignerChannel, bool>>(
        "enabled", this, &WebDesignerChannel::propEnabled, &WebDesignerChannel::propSetEnabled));
    registerProperty(std::make_unique<ComputedProperty<WebDesignerChannel, std::string>>(
        "activeSite", this, &WebDesignerChannel::propActiveSite, &WebDesignerChannel::propSetActiveSite));
    registerProperty(std::make_unique<ComputedProperty<WebDesignerChannel, std::string>>(
        "activePage", this, &WebDesignerChannel::propActivePage, &WebDesignerChannel::propSetActivePage));
    registerProperty(std::make_unique<ComputedProperty<WebDesignerChannel, std::string>>(
        "realizedHtml", this, &WebDesignerChannel::propLastRealizedHtml, nullptr));
    registerProperty(std::make_unique<ComputedProperty<WebDesignerChannel, std::string>>(
        "lastActionBeing", this, &WebDesignerChannel::propLastActionBeing, nullptr));
    registerProperty(std::make_unique<ComputedProperty<WebDesignerChannel, std::string>>(
        "lastActionEvent", this, &WebDesignerChannel::propLastActionEvent, nullptr));
}

void WebDesignerChannel::syncRegister(LawManager& laws) {
    if (!laws.find("web-designer-channel")) {
        auto channel = std::make_shared<WebDesignerChannel>();
        laws.add(channel);
    }
    // Also sync-register the underlying Lexemes and Laws
    WebDesignerMetalaws::syncRegister(laws);
}

WebDesignerChannel* WebDesignerChannel::find(LawManager& laws) {
    Law* law = laws.find("web-designer-channel");
    return dynamic_cast<WebDesignerChannel*>(law);
}

Singular& WebDesignerChannel::authorElementFromPhrase(
    Singular& target,
    const std::string& phrase,
    const std::string& text,
    Person* author,
    LawManager* laws) {

    WebConcept concept;
    Object compileSlot;
    compileSlot.setObjectID("web-compiler-" + target.getIdentifier());
    std::vector<Relation*> extraRels;
    for (const auto& r : _draftRelations) {
        if (r) extraRels.push_back(r.get());
    }
    if (laws) {
        concept = WebDesignerMetalaws::compilePhrase(phrase, *laws, compileSlot, nullptr, extraRels);
    } else {
        LawManager tempManager;
        WebDesignerMetalaws::syncRegister(tempManager);
        concept = WebDesignerMetalaws::compilePhrase(phrase, tempManager, compileSlot, nullptr, extraRels);
    }

    if (!text.empty()) {
        concept.text = text;
    }

    concept.applyToSingular(target);

    if (author) {
        auto authRel = std::make_shared<Relation>(WebRelationType::kAuthoredBy, target, *author, true, 1.0f);
        _draftRelations.push_back(authRel);
        if (_activeZone) {
            _activeZone->getFormation().relations().add(authRel);
        }
        if (_activeFormation) {
            _activeFormation->relations().add(authRel);
        }
        if (Universe::instance().hasRelationRegistrar()) {
            Universe::instance().addRelation(authRel);
        }
    }

    return target;
}

std::shared_ptr<Singular> WebDesignerChannel::authorElementFromPhrase(
    const std::string& identifier,
    const std::string& phrase,
    const std::string& text,
    Person* author,
    LawManager* laws) {

    auto elem = std::make_shared<Object>();
    elem->setObjectID(identifier);
    authorElementFromPhrase(*elem, phrase, text, author, laws);

    if (_activeZone) {
        _activeZone->addObject(elem);
    }
    if (_activeFormation) {
        _activeFormation->addMember(elem.get());
    }

    _elementCache[identifier] = elem;
    return elem;
}

std::shared_ptr<Singular> WebDesignerChannel::authorElementInZone(
    Zone& zone,
    const std::string& identifier,
    const std::string& phrase,
    const std::string& text,
    Person* author,
    LawManager* laws) {

    auto elem = std::make_shared<Object>();
    elem->setObjectID(identifier);
    authorElementFromPhrase(*elem, phrase, text, author, laws);
    zone.addObject(elem);
    _elementCache[identifier] = elem;
    return elem;
}

void WebDesignerChannel::setAuthoringContext(Zone* zone, Formation* formation) {
    _activeZone = zone;
    _activeFormation = formation;
}

Singular* WebDesignerChannel::findElement(const std::string& identifier) const {
    // 1. Discover through ordinary Earthcall world ownership first
    for (Singular* b : Universe::instance().beings()) {
        if (b && b->getIdentifier() == identifier) {
            return b;
        }
    }
    // 2. Fallback to weak cache
    auto it = _elementCache.find(identifier);
    if (it != _elementCache.end()) {
        if (auto sp = it->second.lock()) return sp.get();
    }
    return nullptr;
}

std::shared_ptr<Relation> WebDesignerChannel::relateContains(
    const std::shared_ptr<Singular>& parent,
    const std::shared_ptr<Singular>& child) {

    if (!parent || !child) return nullptr;

    auto rel = std::make_shared<Relation>(
        WebRelationType::kContains,
        *parent,
        *child,
        true); // directed = true

    _draftRelations.push_back(rel);
    if (_activeZone) {
        _activeZone->getFormation().relations().add(rel);
    }
    if (_activeFormation) {
        _activeFormation->relations().add(rel);
    }
    if (Universe::instance().hasRelationRegistrar()) {
        Universe::instance().addRelation(rel);
    }
    return rel;
}

std::shared_ptr<Relation> WebDesignerChannel::relatePrecedes(
    const std::shared_ptr<Singular>& first,
    const std::shared_ptr<Singular>& second) {

    if (!first || !second) return nullptr;

    auto rel = std::make_shared<Relation>(
        WebRelationType::kPrecedes,
        *first,
        *second,
        true); // directed = true: first precedes second

    _draftRelations.push_back(rel);
    if (_activeZone) {
        _activeZone->getFormation().relations().add(rel);
    }
    if (_activeFormation) {
        _activeFormation->relations().add(rel);
    }
    if (Universe::instance().hasRelationRegistrar()) {
        Universe::instance().addRelation(rel);
    }
    return rel;
}

std::string WebDesignerChannel::realize(const Singular& root, const RealizeOptions& options) {
    _lastRealizedHtml = WebRealizer::realizeFullDocument(root, _draftRelations, options);
    return _lastRealizedHtml;
}

void WebDesignerChannel::handleWebAction(const std::string& beingId,
                                         const std::string& eventName,
                                         LawManager& laws,
                                         Singular* targetBeing) {
    _lastActionBeing = beingId;
    _lastActionEvent = eventName;

    Singular* subj = targetBeing;
    if (!subj) {
        subj = findElement(beingId);
    }
    if (!subj) subj = this;

    // Publish to Earthcall EventBus
    Core::EventBus::instance().publish(ECA::Event{
        eventName,
        subj,
        nullptr,
        std::time(nullptr)
    });

    Core::EventBus::instance().publish(ECA::Event{
        "web-action",
        subj,
        nullptr,
        std::time(nullptr)
    });

    // Tick the laws to process the event
    laws.tick();
}

void WebDesignerChannel::sense(LawManager& /*laws*/) {
    // Sensed inputs
}

void WebDesignerChannel::act(LawManager& /*laws*/) {
    // Channel outputs
}

} // namespace Web
} // namespace Foreign
} // namespace Singularity
