#include "SecondNatureLawAuthoring.hpp"

#include "ConstructedBeing/Singular/Creation/SingularSetToSetCreation.hpp"
#include "ConstructedBeing/Singular/Property/Property.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"

#include <ctime>
#include <string>

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

Law* resolvePrototype(const ECA::Event& event, LawManager& laws, Singular*& request) {
    request = event.subject;
    if (!request) return nullptr;
    if (auto* direct = dynamic_cast<Law*>(request)) return direct;

    const std::string prototypeId = readString(*request, kPrototypeProperty);
    if (prototypeId.empty()) return nullptr;
    return laws.find(prototypeId);
}

Singular* resolveTarget(const ECA::Event& event, Singular* request) {
    if (event.object) return event.object;
    if (!request) return nullptr;
    return findBeing(readString(*request, kTargetProperty));
}

Singular* resolveAuthor(const ECA::Event& event, Law& prototype) {
    if (!event.author.empty()) {
        if (Singular* author = findBeing(event.author)) return author;
    }
    for (Singular* author : prototype.authors().getMembers()) {
        if (author) return author;
    }
    return nullptr;
}

void report(Singular* request, const std::string& status,
            const std::string& newbornId = {}) {
    if (!request || dynamic_cast<Law*>(request)) return;
    writeString(*request, kStatusProperty, status);
    if (!newbornId.empty()) writeString(*request, kLastCreatedProperty, newbornId);
}

bool needsTarget(const Law& prototype) {
    std::string text;
    if (prototype.hasConditionModel()) text += prototype.conditionModel()->toJson().dump();
    if (prototype.hasActionModel()) text += prototype.actionModel()->toJson().dump();
    return text.find("$TARGET") != std::string::npos;
}

} // namespace

bool instantiate(const ECA::Event& event) {
    if (event.type != kInvokeEvent) return false;

    LawManager* laws = Physics::getLawManager();
    if (!laws) return false;

    Singular* request = nullptr;
    Law* prototype = resolvePrototype(event, *laws, request);
    if (!prototype) {
        report(request, "refused: prototype Law not found");
        return false;
    }

    Singular* target = resolveTarget(event, request);
    if (needsTarget(*prototype) && !target) {
        report(request, "refused: prototype requires a selected target");
        return false;
    }

    SingularSetToSetCreation::Request derivation{
        *prototype,
        {},
        resolveAuthor(event, *prototype),
        target,
        nullptr,
        {},
        request ? readString(*request, kNameProperty) : std::string{},
        {}}
    ;
    if (request) derivation.sources.push_back(request);
    if (target) {
        derivation.sources.push_back(target);
        derivation.textBindings["$TARGET"] = target->getIdentifier();
    }

    SingularSetToSetCreation::Result result =
        SingularSetToSetCreation::derive(derivation);
    if (!result) {
        report(request, "refused: " + result.refusal);
        return false;
    }

    auto* newborn = dynamic_cast<Law*>(result.newborn);
    if (!newborn) {
        report(request, "refused: universal creation did not return a Law");
        return false;
    }

    report(request, "authored: " + newborn->name(), newborn->getIdentifier());
    Core::EventBus::instance().publish(
        ECA::Event{kAuthoredEvent, newborn, prototype, std::time(nullptr),
                   derivation.author ? derivation.author->getIdentifier() : std::string{}});
    return true;
}

void install() {
    static const bool installed = [] {
        Core::EventBus::instance().subscribe<ECA::Event>(
            [](const ECA::Event& event) {
                if (event.type == kInvokeEvent) (void)instantiate(event);
            },
            50);
        return true;
    }();
    (void)installed;
}

namespace {
const bool kInstalledAtBoot = [] {
    SecondNatureLawAuthoring::install();
    return true;
}();
} // namespace

} // namespace SecondNatureLawAuthoring
