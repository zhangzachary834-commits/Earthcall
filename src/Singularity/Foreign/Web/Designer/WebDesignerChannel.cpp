#include "Singularity/Foreign/Web/Designer/WebDesignerChannel.hpp"
#include "Singularity/Foreign/Web/Designer/WebDesignerMetalaws.hpp"
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"
#include "Singularity/Core/EventBus.hpp"
#include <fstream>
#include <filesystem>
#include <iostream>

namespace Singularity {
namespace Foreign {
namespace Web {

WebDesignerChannel::WebDesignerChannel()
    : Law("web-designer-channel") {
    setLawIdentifier("web-designer-channel");
    grantAuthority(10); // Kernel/Singularity level authority for modality channel
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
        "exportPath", this, &WebDesignerChannel::propExportPath, &WebDesignerChannel::propSetExportPath));
    registerProperty(std::make_unique<ComputedProperty<WebDesignerChannel, std::string>>(
        "lastActionBeing", this, &WebDesignerChannel::propLastActionBeing, nullptr));
    registerProperty(std::make_unique<ComputedProperty<WebDesignerChannel, std::string>>(
        "lastActionEvent", this, &WebDesignerChannel::propLastActionEvent, nullptr));
}

void WebDesignerChannel::syncRegister(LawManager& laws, Singular* author) {
    if (!laws.find("web-designer-channel")) {
        auto channel = std::make_shared<WebDesignerChannel>();
        laws.add(channel);
    }
    // Also sync-register the underlying Metalaws
    WebDesignerMetalaws::syncRegister(laws, author);
}

WebDesignerChannel* WebDesignerChannel::find(LawManager& laws) {
    Law* law = laws.find("web-designer-channel");
    return dynamic_cast<WebDesignerChannel*>(law);
}

std::shared_ptr<Singular> WebDesignerChannel::authorElementFromPhrase(
    const std::string& identifier,
    const std::string& phrase,
    const std::string& text,
    Singular* author,
    LawManager* laws) {

    WebConcept concept;
    Object compileSlot;
    compileSlot.setObjectID("web-compiler-" + identifier);
    if (laws) {
        concept = WebDesignerMetalaws::compilePhrase(phrase, *laws, compileSlot);
    } else {
        // Fallback to phrase compilation with dummy target
        LawManager tempManager;
        WebDesignerMetalaws::syncRegister(tempManager, author);
        concept = WebDesignerMetalaws::compilePhrase(phrase, tempManager, compileSlot);
    }

    if (!text.empty()) {
        concept.text = text;
    }

    auto elem = WebDesignerMetalaws::createAuthoredElement(identifier, concept, author);
    _authoredElements[identifier] = elem;
    return elem;
}

std::shared_ptr<Singular> WebDesignerChannel::findElement(const std::string& identifier) const {
    auto it = _authoredElements.find(identifier);
    if (it != _authoredElements.end()) return it->second;
    return nullptr;
}

std::shared_ptr<Relation> WebDesignerChannel::relateContains(
    const std::shared_ptr<Singular>& parent,
    const std::shared_ptr<Singular>& child,
    int order) {

    if (!parent || !child) return nullptr;

    auto rel = std::make_shared<Relation>(
        WebRelationType::kContains,
        *parent,
        *child,
        true); // directed = true

    // Save ordering hint on the child being
    child->setDynamicProperty("web.order", PropertyValue(static_cast<double>(order)));

    _relations.push_back(rel);
    return rel;
}

std::string WebDesignerChannel::realize(const Singular& root, const RealizeOptions& options) {
    _lastRealizedHtml = WebRealizer::realizeFullDocument(root, _relations, options);
    return _lastRealizedHtml;
}

bool WebDesignerChannel::exportHtml(const Singular& root,
                                   const std::string& filePath,
                                   const RealizeOptions& options) {
    std::string html = realize(root, options);
    try {
        std::filesystem::path path(filePath);
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path());
        }
        std::ofstream out(path);
        if (!out.is_open()) return false;
        out << html;
        out.close();
        _exportPath = filePath;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "WebDesignerChannel::exportHtml error: " << e.what() << std::endl;
        return false;
    }
}

void WebDesignerChannel::handleWebAction(const std::string& beingId,
                                        const std::string& eventName,
                                        LawManager& laws,
                                        Singular* targetBeing) {
    _lastActionBeing = beingId;
    _lastActionEvent = eventName;

    Singular* subj = targetBeing;
    if (!subj) {
        auto found = findElement(beingId);
        if (found) subj = found.get();
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
