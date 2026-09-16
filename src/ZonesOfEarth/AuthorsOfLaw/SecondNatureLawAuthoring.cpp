#include "SecondNatureLawAuthoring.hpp"

#include "ConstructedBeing/Singular/Property/Property.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ConditionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"

#include <algorithm>
#include <ctime>
#include <memory>
#include <string>
#include <vector>

namespace SecondNatureLawAuthoring {
namespace {

std::string readString(Singular& being, const char* property) {
    Property* p = being.findProperty(property);
    if (!p) return {};
    const PropertyValue value = p->value();
    const auto* text = std::get_if<std::string>(&value);
    return text ? *text : std::string{};
}

void writeString(Singular& being, const char* property, const std::string& value) {
    // These are authored/dynamic feedback properties. Do not shadow a registered
    // first-mover property if another being happens to use the same vocabulary.
    if (Property* p = being.findProperty(property)) {
        const PropertyValue current = p->value();
        if (std::holds_alternative<std::string>(current)) {
            p->setValue(PropertyValue(value));
            return;
        }
    }
    being.setDynamicProperty(property, PropertyValue(value));
}

Singular* findBeing(const std::string& id) {
    if (id.empty()) return nullptr;
    for (Singular* being : Universe::instance().beings()) {
        if (being && being->getIdentifier() == id) return being;
    }
    return nullptr;
}

void replaceAll(std::string& text, const std::string& needle,
                const std::string& replacement) {
    if (needle.empty()) return;
    std::size_t at = 0;
    while ((at = text.find(needle, at)) != std::string::npos) {
        text.replace(at, needle.size(), replacement);
        at += replacement.size();
    }
}

// Model trees are already the Law language's serializable syntax. Rebinding a
// concept therefore walks that syntax rather than spelunking through compiled
// closures. Only an explicit token is replaced; ordinary strings are untouched.
void rebindTarget(nlohmann::json& node, const std::string& targetId) {
    if (targetId.empty()) return;
    if (node.is_string()) {
        std::string text = node.get<std::string>();
        replaceAll(text, "$TARGET", targetId);
        node = std::move(text);
        return;
    }
    if (node.is_array()) {
        for (auto& child : node) rebindTarget(child, targetId);
        return;
    }
    if (node.is_object()) {
        for (auto it = node.begin(); it != node.end(); ++it) {
            rebindTarget(it.value(), targetId);
        }
    }
}

std::string nextBirthId(LawManager& laws, const std::string& templateId) {
    const std::string stem = templateId + ".birth-";
    for (unsigned long long n = 1;; ++n) {
        const std::string candidate = stem + std::to_string(n);
        if (!laws.find(candidate)) return candidate;
    }
}

Law* resolveTemplate(ECA::Event const& event, LawManager& laws, Singular*& request) {
    request = event.subject;
    if (!request) return nullptr;

    if (auto* direct = dynamic_cast<Law*>(request)) return direct;

    const std::string templateId = readString(*request, kTemplateProperty);
    if (templateId.empty()) return nullptr;
    return laws.find(templateId);
}

Singular* resolveTarget(ECA::Event const& event, Singular* request) {
    if (event.object) return event.object;
    if (!request) return nullptr;
    return findBeing(readString(*request, kTargetProperty));
}

void report(Singular* request, const std::string& status,
            const std::string& newbornId = {}) {
    if (!request || dynamic_cast<Law*>(request)) return;
    writeString(*request, kStatusProperty, status);
    if (!newbornId.empty()) writeString(*request, kLastCreatedProperty, newbornId);
}

} // namespace

bool instantiate(const ECA::Event& event) {
    if (event.type != kInvokeEvent) return false;

    LawManager* laws = Physics::getLawManager();
    if (!laws) return false;

    Singular* request = nullptr;
    Law* prototype = resolveTemplate(event, *laws, request);
    if (!prototype) {
        report(request, "refused: template Law not found");
        return false;
    }
    if (prototype->isFirstMover()) {
        report(request, "refused: First Mover closures are not capturable Law text");
        return false;
    }
    if (!prototype->hasActionModel()) {
        report(request, "refused: template has no authored ActionModel");
        return false;
    }

    std::vector<Singular*> authors;
    for (Singular* author : prototype->authors().getMembers()) {
        if (author) authors.push_back(author);
    }
    if (authors.empty()) {
        report(request, "refused: template has no present author");
        return false;
    }

    Singular* target = resolveTarget(event, request);
    std::string targetId = target ? target->getIdentifier() : std::string{};

    // `$TARGET` is a declared parameter, not an inference. If a template uses
    // it, an omitted target is a refusal rather than a guess about the Person's
    // intent. Detect it in the serialized syntax before deriving the newborn.
    nlohmann::json conditionJson;
    nlohmann::json actionJson = prototype->actionModel()->toJson();
    if (prototype->hasConditionModel()) conditionJson = prototype->conditionModel()->toJson();
    const std::string combined = conditionJson.dump() + actionJson.dump();
    if (combined.find("$TARGET") != std::string::npos && targetId.empty()) {
        report(request, "refused: template requires a selected target");
        return false;
    }
    rebindTarget(conditionJson, targetId);
    rebindTarget(actionJson, targetId);

    const std::string newbornId = nextBirthId(*laws, prototype->getIdentifier());
    std::string newbornName = request ? readString(*request, kNameProperty) : std::string{};
    if (newbornName.empty()) newbornName = prototype->name() + " · " + newbornId.substr(newbornId.rfind('.') + 1);
    replaceAll(newbornName, "$TARGET", targetId);

    auto newborn = std::make_shared<Law>(newbornName, authors);
    newborn->setLawIdentifier(newbornId);
    newborn->setAuthorityLevel(prototype->authorityLevel());
    newborn->setActivation(prototype->activation());
    newborn->setScope(prototype->scope());
    newborn->setDrives(prototype->drives());
    newborn->setRetrigger(prototype->retrigger());
    newborn->setConditionMode(prototype->conditionMode());
    newborn->setJurisdiction(prototype->jurisdiction());

    if (prototype->hasConditionModel()) {
        newborn->setConditionModel(ConditionNode::fromJson(conditionJson));
    }
    newborn->setActionModel(ActionNode::fromJson(actionJson));

    // A selected target is both a parameter to the text and explicit
    // provenance/reach. When no target was selected, preserve the prototype's
    // target Formation instead of broadening its reach silently.
    if (target) {
        newborn->addTarget(*target);
    } else {
        for (Singular* inheritedTarget : prototype->targets().getMembers()) {
            if (inheritedTarget) newborn->addTarget(*inheritedTarget);
        }
    }
    newborn->recordProvenance("instantiated-from", *newborn, *prototype, true, 1.0f);
    newborn->setEnabled(true);

    laws->add(newborn);

    const auto& triggers = laws->triggersOf(prototype->getIdentifier());
    if (!triggers.empty()) {
        for (const std::string& trigger : triggers) {
            laws->bindTrigger(newbornId, trigger);
        }
    } else if (!prototype->ecaLoop().eventType.empty()) {
        laws->bindTrigger(newbornId, prototype->ecaLoop().eventType);
    }

    report(request, "authored: " + newbornName, newbornId);
    Core::EventBus::instance().publish(
        ECA::Event{kAuthoredEvent, newborn.get(), prototype, std::time(nullptr),
                   authors.front()->getIdentifier()});
    return true;
}

void install() {
    static const bool installed = [] {
        Core::EventBus::instance().subscribe<ECA::Event>(
            [](const ECA::Event& event) {
                if (event.type == kInvokeEvent) (void)instantiate(event);
            },
            // Birth happens before the ordinary law network consumes the same
            // event. The newborn never hears its own creation event unless its
            // authored trigger explicitly says it should.
            50);
        return true;
    }();
    (void)installed;
}

namespace {
// CMake already globs src/*.cpp. This bootstrap does not mint a hidden domain
// object or widget; it installs one First-Mover transition at process startup.
// Its Law outputs remain ordinary, inspectable, serializable Laws.
const bool kInstalledAtBoot = [] {
    SecondNatureLawAuthoring::install();
    return true;
}();
} // namespace

} // namespace SecondNatureLawAuthoring
