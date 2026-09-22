#include "ConstructedBeing/Singular/Creation/SingularSetToSetCreation.hpp"

#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Person/Person.hpp"
#include "Relation/Relation.hpp"
#include "Singularity/Storage/Serialization/ConstructedBeing/ObjectSerialization.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ConditionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"

#include <memory>
#include <string>
#include <typeinfo>
#include <utility>
#include <vector>

namespace SingularSetToSetCreation {
namespace {

bool idExists(const std::string& id) {
    if (id.empty()) return false;
    if (LawManager* laws = Physics::getLawManager()) {
        if (laws->find(id)) return true;
    }
    for (Singular* being : Universe::instance().beings()) {
        if (being && being->getIdentifier() == id) return true;
    }
    return false;
}

bool chooseId(const Request& request, std::string& out, std::string& refusal) {
    if (!request.newbornId.empty()) {
        if (idExists(request.newbornId)) {
            refusal = "requested newborn identity already exists: " + request.newbornId;
            return false;
        }
        out = request.newbornId;
        return true;
    }

    const std::string base = request.prototype.getIdentifier().empty()
                                 ? std::string("singular")
                                 : request.prototype.getIdentifier();
    for (unsigned long long n = 1;; ++n) {
        const std::string candidate = base + ".branch-" + std::to_string(n);
        if (!idExists(candidate)) {
            out = candidate;
            return true;
        }
    }
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

void bindText(nlohmann::json& node,
              const std::unordered_map<std::string, std::string>& bindings) {
    if (bindings.empty()) return;
    if (node.is_string()) {
        std::string text = node.get<std::string>();
        for (const auto& [token, replacement] : bindings) {
            replaceAll(text, token, replacement);
        }
        node = std::move(text);
        return;
    }
    if (node.is_array()) {
        for (auto& child : node) bindText(child, bindings);
        return;
    }
    if (node.is_object()) {
        for (auto it = node.begin(); it != node.end(); ++it) {
            bindText(it.value(), bindings);
        }
    }
}

Zone* resolveUniqueDestinationZone(const Request& request, std::string& refusal) {
    if (request.destinationZone) return request.destinationZone;

    Zone* unique = nullptr;
    for (Singular* being : Universe::instance().beings()) {
        auto* zone = dynamic_cast<Zone*>(being);
        if (!zone) continue;
        if (unique && unique != zone) {
            refusal = "multiple destination Zones are present; creation must name one";
            return nullptr;
        }
        unique = zone;
    }
    if (!unique) refusal = "no destination Zone is present";
    return unique;
}

Result deriveLaw(const Request& request) {
    auto* prototype = dynamic_cast<Law*>(&request.prototype);
    if (!prototype) return {nullptr, "internal Law codec mismatch"};
    if (prototype->isFirstMover()) {
        return {nullptr, "First Mover closures are substrate and cannot be synthesized as authored Law text"};
    }

    LawManager* laws = Physics::getLawManager();
    if (!laws) return {nullptr, "no LawManager is bound"};

    std::vector<Singular*> authors;
    if (request.author) authors.push_back(request.author);
    else {
        for (Singular* author : prototype->authors().getMembers()) {
            if (author) authors.push_back(author);
        }
    }
    if (authors.empty()) return {nullptr, "a Law birth requires present authorship"};

    std::string id;
    std::string refusal;
    if (!chooseId(request, id, refusal)) return {nullptr, refusal};

    std::string displayName = request.newbornName;
    if (displayName.empty()) {
        displayName = prototype->name() + " · " + id.substr(id.rfind('.') + 1);
    }
    for (const auto& [token, replacement] : request.textBindings) {
        replaceAll(displayName, token, replacement);
    }

    auto newborn = std::make_shared<Law>(displayName, authors);
    newborn->setLawIdentifier(id);
    newborn->setAuthorityLevel(prototype->authorityLevel());
    newborn->setActivation(prototype->activation());
    newborn->setScope(prototype->scope());
    newborn->setDrives(prototype->drives());
    newborn->setRetrigger(prototype->retrigger());
    newborn->setConditionMode(prototype->conditionMode());
    newborn->setJurisdiction(prototype->jurisdiction());

    if (prototype->hasConditionModel()) {
        nlohmann::json condition = prototype->conditionModel()->toJson();
        bindText(condition, request.textBindings);
        newborn->setConditionModel(ConditionNode::fromJson(condition));
    }
    if (prototype->hasActionModel()) {
        nlohmann::json action = prototype->actionModel()->toJson();
        bindText(action, request.textBindings);
        newborn->setActionModel(ActionNode::fromJson(action));
    }

    if (request.target) newborn->addTarget(*request.target);
    else {
        for (Singular* target : prototype->targets().getMembers()) {
            if (target) newborn->addTarget(*target);
        }
    }

    newborn->recordProvenance("branched-from", *newborn, request.prototype, true, 1.0f);
    newborn->setEnabled(true);
    laws->add(newborn);

    const auto& triggers = laws->triggersOf(prototype->getIdentifier());
    for (const std::string& trigger : triggers) laws->bindTrigger(id, trigger);
    if (triggers.empty() && !prototype->ecaLoop().eventType.empty()) {
        laws->bindTrigger(id, prototype->ecaLoop().eventType);
    }

    // "Keep the resulting instrument" means Zone membership, not merely a
    // pointer in the process-wide Law register. The live ZoneManager owns the
    // active authored closure loaded from lawRefs. Adopt the newborn into that
    // SAME closure so departure releases it and Save Zone persists its root.
    if (ZoneManager* zones = ZoneManager::live()) {
        if (!zones->adoptLawIntoActiveZone(id)) {
            laws->remove(id);
            return {nullptr, "newborn Law could not enter the active Zone's authored closure"};
        }
    }

    return {newborn.get(), {}};
}

Result deriveObject(const Request& request) {
    // Never slice an Object-derived semantic kind into Object. Law and Zone are
    // checked before this. Any other subclass waits for its persistence codec
    // to join this same universal operation.
    if (typeid(request.prototype) != typeid(Object)) {
        return {nullptr,
                "this Object-derived runtime kind has no set-to-set persistence codec yet; refusing rather than slicing it into Object"};
    }

    auto* prototype = dynamic_cast<Object*>(&request.prototype);
    if (!prototype) return {nullptr, "internal Object codec mismatch"};

    std::string refusal;
    Zone* zone = resolveUniqueDestinationZone(request, refusal);
    if (!zone) return {nullptr, refusal};

    std::string id;
    if (!chooseId(request, id, refusal)) return {nullptr, refusal};

    // The existing semantic codec is the mechanical adapter. Creation logic
    // does not re-list Object's fields; it asks the same persistence boundary
    // that save/load already trusts to reproduce the being's authored state.
    nlohmann::json semantic;
    to_json(semantic, *prototype);
    bindText(semantic, request.textBindings);

    auto newborn = std::make_shared<Object>();
    from_json(semantic, *newborn);
    newborn->setObjectID(id);
    if (!request.newbornName.empty()) {
        newborn->setDynamicProperty("displayName", PropertyValue(request.newbornName));
    }

    Object* raw = newborn.get();
    zone->addObject(std::move(newborn));

    if (Universe::instance().hasRelationRegistrar()) {
        Universe::instance().addRelation(std::make_shared<Relation>(
            "branched-from", *raw, request.prototype, true, 1.0f));
    }
    return {raw, {}};
}

} // namespace

Result derive(const Request& request) {
    // Kernel refusal: a Person is discovered/verified as an actual human, not
    // synthesized from another Person. This constrains the ONE create operator;
    // it does not create a parallel Person-creation subsystem.
    if (dynamic_cast<Person*>(&request.prototype)) {
        return {nullptr,
                "Person is not synthesizable: Person corresponds to an actual human being"};
    }

    // Most-specific first because the present ontology has Law/Zone beneath
    // Object. The operation remains one; these are persistence codecs at the
    // concrete C++ storage boundary, not author-facing creation verbs.
    if (dynamic_cast<Law*>(&request.prototype)) return deriveLaw(request);
    if (dynamic_cast<Zone*>(&request.prototype)) {
        return {nullptr,
                "Zone set-to-set codec is not wired yet; Zone birth must preserve owner/jurisdiction rather than degrade to Object"};
    }
    if (dynamic_cast<Object*>(&request.prototype)) return deriveObject(request);

    return {nullptr,
            "this Singular kind has not yet registered its persistence codec with universal set-to-set creation"};
}

} // namespace SingularSetToSetCreation