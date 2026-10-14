#include "Singularity/Foreign/Web/Designer/WebDesignerChannel.hpp"
#include "Singularity/Foreign/Web/Designer/WebDesignerMetalaws.hpp"
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"
#include "Singularity/Storage/FileChannel.hpp"
#include "Singularity/Storage/VirtualFileSystem.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
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
        "exportPath", this, &WebDesignerChannel::propExportPath, &WebDesignerChannel::propSetExportPath));
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

std::shared_ptr<Singular> WebDesignerChannel::authorElementFromPhrase(
    const std::string& identifier,
    const std::string& phrase,
    const std::string& text,
    Person* author,
    LawManager* laws) {

    WebConcept concept;
    Object compileSlot;
    compileSlot.setObjectID("web-compiler-" + identifier);
    if (laws) {
        concept = WebDesignerMetalaws::compilePhrase(phrase, *laws, compileSlot);
    } else {
        // Fallback to phrase compilation with temporary manager
        LawManager tempManager;
        WebDesignerMetalaws::syncRegister(tempManager);
        concept = WebDesignerMetalaws::compilePhrase(phrase, tempManager, compileSlot);
    }

    if (!text.empty()) {
        concept.text = text;
    }

    // Create authored element with real Earthcall Person authorship relation
    auto elem = WebDesignerMetalaws::createAuthoredElement(identifier, concept, author, &_relations);
    _elementCache[identifier] = elem;
    return elem;
}

std::shared_ptr<Singular> WebDesignerChannel::findElement(const std::string& identifier) const {
    // 1. Discover through ordinary Earthcall world ownership first
    for (Singular* b : Universe::instance().beings()) {
        if (b && b->getIdentifier() == identifier) {
            auto it = _elementCache.find(identifier);
            if (it != _elementCache.end()) {
                if (auto sp = it->second.lock()) return sp;
            }
            return std::shared_ptr<Singular>(b, [](Singular*){});
        }
    }
    // 2. Fallback to weak cache
    auto it = _elementCache.find(identifier);
    if (it != _elementCache.end()) {
        if (auto sp = it->second.lock()) return sp;
    }
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

    if (order != 0) {
        child->setDynamicProperty("web.order", PropertyValue(static_cast<double>(order)));
    }

    _relations.push_back(rel);
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

    _relations.push_back(rel);
    if (Universe::instance().hasRelationRegistrar()) {
        Universe::instance().addRelation(rel);
    }
    return rel;
}

std::string WebDesignerChannel::realize(const Singular& root, const RealizeOptions& options) {
    _lastRealizedHtml = WebRealizer::realizeFullDocument(root, _relations, options);
    return _lastRealizedHtml;
}

bool WebDesignerChannel::saveToVfs(const Singular& root,
                                   const std::string& vfsUri,
                                   LawManager* laws,
                                   const RealizeOptions& options) {
    std::string html = realize(root, options);
    if (vfsUri.empty()) return false;

    // 1. If in-memory VFS URI (e.g. "memory://site.html"), write directly to RAM VFS
    if (Storage::VirtualFileSystem::isMemory(vfsUri)) {
        bool ok = Storage::VirtualFileSystem::instance().writeMemoryFile(vfsUri, html);
        if (ok) _exportPath = vfsUri;
        return ok;
    }

    // 2. Resolve VFS path
    std::string resolvedPath = Storage::VirtualFileSystem::resolve(vfsUri);

    // 3. Perform atomic governed file write through Singularity::Storage::FileChannel
    // Refusal 6 & Mandate 9 compliant: No raw std::ofstream or ungoverned direct disk I/O.
    Storage::FileChannel localFc;
    Storage::FileChannel* fc = &localFc;
    if (laws) {
        if (auto* foundFc = Storage::FileChannel::find(*laws)) {
            fc = foundFc;
        }
    }

    PropertyPath::parse("file.path").setValue(*fc, PropertyValue(resolvedPath));
    PropertyPath::parse("file.content").setValue(*fc, PropertyValue(html));
    PropertyPath::parse("file.atomicWrite").setValue(*fc, PropertyValue(true));
    bool success = fc->executeWrite();
    if (success) {
        _exportPath = resolvedPath;
    } else {
        PropertyValue errVal;
        PropertyPath::parse("file.lastError").getValue(*fc, errVal);
        std::string errMsg;
        if (auto s = std::get_if<std::string>(&errVal)) errMsg = *s;
        std::cerr << "WebDesignerChannel::saveToVfs FileChannel write failed: "
                  << errMsg << " for path: " << resolvedPath << std::endl;
    }
    return success;
}

bool WebDesignerChannel::exportHtml(const Singular& root,
                                   const std::string& filePath,
                                   const RealizeOptions& options) {
    // Route file export through saveToVfs to preserve FileChannel governance
    return saveToVfs(root, filePath, nullptr, options);
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
