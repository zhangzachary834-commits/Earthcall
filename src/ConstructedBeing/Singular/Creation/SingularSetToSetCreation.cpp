#include "ConstructedBeing/Singular/Creation/SingularSetToSetCreation.hpp"

#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Object/Creation/ObjectConcept.hpp"
#include "Time/Moment/Moment.hpp"
#include "ConstructedBeing/Singular/Lexeme/Lexeme.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValueJson.hpp"
#include "Person/Body/Body.hpp"
#include "Person/Body/BodyPart/BodyPart.hpp"
#include "Person/Soul/Soul.hpp"
#include "Person/Perspective/PersonPerspective.hpp"
#include "Person/Relationship/Community/Community.hpp"
#include "Identity/FirstMoverRegister.hpp"
#include "Singularity/TransferPolicy.hpp"
#include "Singularity/Language/LanguageSystem.hpp"
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
#include "ZonesOfEarth/Ourverse/Ourverse.hpp"

#include <memory>
#include <algorithm>
#include <string>
#include <typeinfo>
#include <utility>
#include <vector>
#include <stdexcept>
#include <unordered_set>
#include "Singularity/OntoMath/Field.hpp"

extern MaterialManager materials;

namespace SingularSetToSetCreation {
namespace {
// Borrowed dynamic call context, beneath the Kernel; never persisted as an
// authority grant. Law::applyToImpl and direct First Mover callers arm it.
thread_local const std::vector<Singular*>* actingAuthors = nullptr;
const std::vector<Singular*> noAuthors;

std::vector<Singular*> authorsFor(const Request& request) {
    if (request.author) return {request.author};
    auto authors = actingAuthors ? *actingAuthors : noAuthors;
    authors.erase(std::remove(authors.begin(), authors.end(), nullptr), authors.end());
    return authors;
}

std::string kernelRefusal(const Singular& being) {
    if (dynamic_cast<const Person*>(&being) || dynamic_cast<const Body*>(&being) ||
        dynamic_cast<const BodyPart*>(&being) || dynamic_cast<const Soul*>(&being) ||
        dynamic_cast<const PersonPerspective*>(&being))
        return "a Person and their constitutive body/soul/perspective are real human correspondents, not synthesizable beings";
    if (dynamic_cast<const Identity::FirstMover*>(&being) ||
        dynamic_cast<const Identity::FirstMoverRegister*>(&being))
        return "First Mover recognition and its register cannot be synthesized";
    if (dynamic_cast<const TransferPolicy*>(&being))
        return "the machine's TransferPolicy is Kernel substrate, not a second creatable policy office";
    if (auto* law = dynamic_cast<const Law*>(&being)) {
        if (law->isFirstMover() || typeid(being) != typeid(Law))
            return "First Mover closures and concrete machine channels cannot be synthesized as authored Law text";
    }
    return {};
}

nlohmann::json storedValue(const PropertyValue& value, std::unordered_set<const void*>& containers) {
    return std::visit([&](const auto& item) -> nlohmann::json {
        using T = std::decay_t<decltype(item)>;
        if constexpr (std::is_same_v<T, std::shared_ptr<PropertyList>> || std::is_same_v<T, std::shared_ptr<PropertyDict>>) {
            if (!item) return {{"t", std::is_same_v<T, std::shared_ptr<PropertyList>> ? "list" : "dict"}, {"null", true}};
            if (!containers.insert(item.get()).second)
                throw std::runtime_error("shared or cyclic authored memory needs its topology-preserving copy/storage contract; refusing to flatten it");
            if constexpr (std::is_same_v<T, std::shared_ptr<PropertyList>>) {
                auto values = nlohmann::json::array();
                for (const auto& child : item->elements) values.push_back(storedValue(child, containers));
                return {{"t", "list"}, {"v", std::move(values)}};
            } else {
                auto values = nlohmann::json::object();
                for (const auto& [name, child] : item->elements) values[name] = storedValue(child, containers);
                return {{"t", "dict"}, {"v", std::move(values)}};
            }
        } else if constexpr (std::is_same_v<T, std::shared_ptr<OntoMath::ScalarField>> || std::is_same_v<T, std::shared_ptr<OntoMath::VectorField>>) {
            return {{"t", std::is_same_v<T, std::shared_ptr<OntoMath::ScalarField>> ? "scalar_field" : "vector_field"},
                    {"v", item ? item->toJson() : nlohmann::json(nullptr)}};
        } else if constexpr (std::is_pointer_v<T>) {
            auto out = propertyValueToJson(value);
            out["refKind"] = std::is_same_v<T, Object*> ? "object" : std::is_same_v<T, Relation*> ? "relation" :
                             std::is_same_v<T, Formation*> ? "formation" : "singular";
            return out;
        } else return propertyValueToJson(value);
    }, value);
}

bool referencesAvailable(const nlohmann::json& value, const Resolver& resolve) {
    if (value.is_object() && value.contains("t") && value["t"].is_string() && value["t"] == "ref") {
        const auto id = value.value("id", std::string{});
        return id.empty() || (resolve && resolve(id));
    }
    if (value.is_structured())
        for (const auto& child : value) if (!referencesAvailable(child, resolve)) return false;
    return true;
}

PropertyValue restoredValue(const nlohmann::json& value, const Resolver& resolve) {
    const auto tag = value.is_object() ? value.value("t", std::string{}) : std::string{};
    if (tag == "ref") {
        const auto id = value.value("id", std::string{});
        Singular* target = id.empty() ? nullptr : (resolve ? resolve(id) : nullptr);
        if (!id.empty() && !target) throw std::runtime_error("unresolved authored reference: " + id);
        const auto kind = value.value("refKind", std::string("singular"));
        if (kind == "object") {
            auto* typed = dynamic_cast<Object*>(target);
            if (target && !typed) throw std::runtime_error("Object reference type mismatch");
            return typed;
        }
        if (kind == "relation") {
            auto* typed = dynamic_cast<Relation*>(target);
            if (target && !typed) throw std::runtime_error("Relation reference type mismatch");
            return typed;
        }
        if (kind == "formation") {
            auto* typed = dynamic_cast<Formation*>(target);
            if (target && !typed) throw std::runtime_error("Formation reference type mismatch");
            return typed;
        }
        if (kind != "singular") throw std::runtime_error("unknown authored reference type");
        return target;
    }
    if (tag == "list") {
        if (value.value("null", false)) return std::shared_ptr<PropertyList>{};
        auto out = std::make_shared<PropertyList>();
        for (const auto& child : value.at("v")) out->elements.push_back(restoredValue(child, resolve));
        return out;
    }
    if (tag == "dict") {
        if (value.value("null", false)) return std::shared_ptr<PropertyDict>{};
        auto out = std::make_shared<PropertyDict>();
        for (auto it = value.at("v").begin(); it != value.at("v").end(); ++it)
            out->elements[it.key()] = restoredValue(it.value(), resolve);
        return out;
    }
    if (tag == "scalar_field") return value.at("v").is_null() ? std::shared_ptr<OntoMath::ScalarField>{} : OntoMath::ScalarField::fromJson(value.at("v"));
    if (tag == "vector_field") return value.at("v").is_null() ? std::shared_ptr<OntoMath::VectorField>{} : OntoMath::VectorField::fromJson(value.at("v"));
    return propertyValueFromJson(value);
}

nlohmann::json baseToJson(const Singular& being) {
    std::unordered_set<const void*> containers;
    nlohmann::json out{{"telos", being.telosId()}, {"properties", nlohmann::json::object()},
                       {"dataStructures", nlohmann::json::array()}, {"stakeholders", nlohmann::json::array()}};
    for (const auto& [name, value] : being.dynamicProperties())
        out["properties"][Earthcall::StringInterner::resolve(name)] = storedValue(value, containers);
    for (const auto& [name, structure] : being.dataStructures()) {
        nlohmann::json item{{"name", name}, {"data", storedValue(structure.data, containers)}};
        if (structure.writeBounds) item["writeBounds"] = structure.writeBounds->toJson();
        out["dataStructures"].push_back(std::move(item));
    }
    for (const auto& record : being.stakeholders())
        out["stakeholders"].push_back({{"path", record.propertyPath}, {"author", record.authorId},
                                      {"law", record.lawId}, {"timestamp", record.timestamp}});
    return out;
}

void baseFromJson(Singular& being, const nlohmann::json& state, const Resolver& resolve = {}) {
    being.setTelosId(state.value("telos", std::string{}));
    if (state.contains("properties")) {
        for (auto it = state["properties"].begin(); it != state["properties"].end(); ++it)
            if (!being.setDynamicProperty(it.key(), restoredValue(it.value(), resolve)))
                throw std::runtime_error("authored property restoration refused: " + it.key());
    }
    for (const auto& item : state.value("dataStructures", nlohmann::json::array())) {
        DataStructure structure(item.at("name").get<std::string>(), restoredValue(item.at("data"), resolve));
        if (item.contains("writeBounds"))
            structure.writeBounds = std::make_shared<ConditionNode>(ConditionNode::fromJson(item["writeBounds"]));
        being.addDataStructure(structure);
    }
    for (const auto& item : state.value("stakeholders", nlohmann::json::array()))
        being.addStakeholder(item.at("path").get<std::string>(), item.at("author").get<std::string>(),
                             item.at("law").get<std::string>(), item.at("timestamp").get<std::time_t>());
}

bool idExists(const std::string& id) {
    if (id.empty()) return false;
    if (id.rfind("material.", 0) == 0 && materials.get(id)) return true;
    if (Singularity::Language::LanguageSystem::instance().findById(id)) return true;
    if (LawManager* laws = Physics::getLawManager()) {
        if (laws->find(id)) return true;
    }
    for (Singular* being : Universe::instance().beings()) {
        if (being && being->getIdentifier() == id) return true;
    }
    return false;
}

bool chooseId(const Request& request, std::string& out, std::string& refusal, Zone* resolvedZone = nullptr) {
    const auto exists = [&](const std::string& id) {
        if (id == request.prototype.getIdentifier() || idExists(id)) return true;
        Zone* destination = resolvedZone ? resolvedZone : request.destinationZone;
        if (destination) {
            if (destination->getIdentifier() == id) return true;
            for (auto* being : destination->formation().getMembers())
                if (being && being->getIdentifier() == id) return true;
            for (const auto& being : destination->objects())
                if (being && being->getIdentifier() == id) return true;
        }
        return false;
    };
    if (!request.newbornId.empty()) {
        if (exists(request.newbornId)) {
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
        if (!exists(candidate)) {
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

Resolver requestResolver(const Request& request, Zone* zone) {
    return [&request, zone](const std::string& id) -> Singular* {
        Singular* found = nullptr;
        bool ambiguous = false;
        const auto consider = [&](Singular* candidate) {
            if (!candidate || candidate->getIdentifier() != id) return;
            if (found && found != candidate) ambiguous = true;
            found = candidate;
        };
        consider(&request.prototype);
        consider(request.endpointA);
        consider(request.endpointB);
        for (auto* source : request.sources) consider(source);
        for (auto* being : Universe::instance().beings()) consider(being);
        if (zone) {
            consider(zone);
            for (auto* being : zone->formation().getMembers()) consider(being);
            for (const auto& being : zone->objects()) consider(being.get());
        }
        if (auto lexeme = Singularity::Language::LanguageSystem::instance().findById(id)) consider(lexeme.get());
        return ambiguous ? nullptr : found;
    };
}

Result deriveLaw(const Request& request) {
    auto* prototype = dynamic_cast<Law*>(&request.prototype);
    if (!prototype) return {nullptr, "internal Law codec mismatch"};
    if (prototype->isFirstMover()) {
        return {nullptr, "First Mover closures are substrate and cannot be synthesized as authored Law text"};
    }

    LawManager* laws = Physics::getLawManager();
    if (!laws) return {nullptr, "no LawManager is bound"};

    std::vector<Singular*> authors = authorsFor(request);
    if (authors.empty()) return {nullptr, "a Law birth requires present authorship"};
    if (auto* zones = ZoneManager::live(); zones && request.destinationZone &&
        (zones->zones().empty() || request.destinationZone != &zones->active()))
        return {nullptr, "Law birth into an inactive Zone requires a scoped closure admission; refusing rather than routing it to the active Zone"};

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
    // Authored birth does not mint a reviewed C++ authority grant.
    newborn->setAuthorityLevel(0);
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
    baseFromJson(*newborn, baseToJson(*prototype), requestResolver(request, request.destinationZone));

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
    if (authorsFor(request).empty()) return {nullptr, "an Object birth requires present authorship"};
    // Never slice an Object-derived semantic kind into Object. Law and Zone are
    // checked before this. Any other subclass waits for its persistence codec
    // to join this same universal operation.
    if (typeid(request.prototype) != typeid(Object)) {
        return {nullptr,
                "this Object-derived runtime kind has no set-to-set persistence codec yet; refusing rather than slicing it into Object"};
    }

    auto* prototype = dynamic_cast<Object*>(&request.prototype);
    if (!prototype) return {nullptr, "internal Object codec mismatch"};
    if (prototype->elementCount() != 0)
        return {nullptr, "Object composition birth needs its explicit member-custody contract; refusing to lose its elements"};

    std::string refusal;
    Zone* zone = resolveUniqueDestinationZone(request, refusal);
    if (!zone) return {nullptr, refusal};

    std::string id;
    if (!chooseId(request, id, refusal, zone)) return {nullptr, refusal};

    // The existing semantic codec is the mechanical adapter. Creation logic
    // does not re-list Object's fields; it asks the same persistence boundary
    // that save/load already trusts to reproduce the being's authored state.
    const auto base = baseToJson(*prototype);
    nlohmann::json semantic;
    to_json(semantic, *prototype);
    semantic.erase("authoredProperties");
    semantic.erase("stakeholders");
    bindText(semantic, request.textBindings);

    auto newborn = std::make_shared<Object>();
    from_json(semantic, *newborn);
    baseFromJson(*newborn, base, requestResolver(request, zone));
    newborn->setObjectID(id);
    if (!request.newbornName.empty()) {
        newborn->setDynamicProperty("displayName", PropertyValue(request.newbornName));
    }

    Object* raw = newborn.get();
    zone->addObject(std::move(newborn));

    zone->formation().relations().add(std::make_shared<Relation>(
        "branched-from", *raw, request.prototype, true, 1.0f));
    for (auto* author : authorsFor(request))
        zone->formation().relations().add(std::make_shared<Relation>(
            "authored-by", *raw, *author, true, 1.0f));
    return {raw, {}};
}

Result deriveStored(const Request& request) {
    std::string refusal;
    Zone* zone = resolveUniqueDestinationZone(request, refusal);
    if (!zone) return {nullptr, refusal};
    const auto authors = authorsFor(request);
    if (authors.empty()) return {nullptr, "a Singular birth requires present authorship"};
    auto record = storedToJson(request.prototype);
    if (record.is_null()) return {nullptr, "this concrete Singular has no birth codec; refusing rather than slicing it"};
    std::string id;
    if (!chooseId(request, id, refusal, zone)) return {nullptr, refusal};
    auto& state = record["state"];
    const std::string codec = record["codec"].get<std::string>();
    if (codec == "relation") {
        if (!request.endpointA || !request.endpointB)
            return {nullptr, "Relation birth requires two explicitly authored endpoints"};
        auto* prototype = static_cast<Relation*>(&request.prototype);
        for (const auto& existing : zone->formation().relations().getRelationsBetween(*request.endpointA, *request.endpointB)) {
            if (existing->type != prototype->type || existing->directed != prototype->directed) continue;
            if (!prototype->directed || (existing->a() == request.endpointA && existing->b() == request.endpointB))
                return {nullptr, "the enduring Relation already exists; birth must not duplicate its participants and kind"};
        }
        id = request.endpointA->getIdentifier() + "-" + prototype->type + "-" + request.endpointB->getIdentifier();
        if (!request.newbornId.empty() && request.newbornId != id)
            return {nullptr, "Relation identity is its endpoints and kind, not an arbitrary newborn slug"};
        if (id == prototype->getIdentifier() || idExists(id))
            return {nullptr, "the enduring Relation already exists; birth must not duplicate its identity"};
        state["entityA"] = request.endpointA->getIdentifier();
        state["entityB"] = request.endpointB->getIdentifier();
        // The new participants have not lived the source Relation's events.
        state["events"] = nlohmann::json::array();
    } else if (codec == "material") {
        if (id.rfind("material.", 0) != 0)
            return {nullptr, "Material identity must use the existing material.<name> contract"};
        state["name"] = id.substr(9);
    } else if (codec == "formation" || codec == "community") {
        // Formation's existing copy contract gives owned subformations fresh
        // identities, while actual member beings remain non-owning references.
        Formation snapshot(static_cast<Formation&>(request.prototype));
        snapshot.setIdentifier(id);
        state = snapshot.toJson();
    } else if (codec == "object-concept") {
        state["concept"]["id"] = id;
        if (!request.newbornName.empty()) state["concept"]["name"] = request.newbornName;
    } else {
        state["id"] = id;
    }
    record["id"] = id;
    if (codec == "lexeme" && !request.newbornName.empty()) state["symbol"] = request.newbornName;
    bindText(state, request.textBindings);
    const Resolver resolve = requestResolver(request, zone);
    try {
        std::shared_ptr<Singular> newborn;
        if (codec == "formation" && request.textBindings.empty()) {
            auto formation = std::make_shared<Formation>(static_cast<Formation&>(request.prototype));
            formation->setIdentifier(id);
            newborn = std::move(formation);
        } else if (codec == "community" && request.textBindings.empty()) {
            auto community = std::make_shared<Community>(id);
            static_cast<Formation&>(*community) = static_cast<Formation&>(request.prototype);
            newborn = std::move(community);
        } else newborn = storedFromJson(record, resolve);
        if (!newborn) return {nullptr, "newborn codec cannot resolve its actual participants"};
        if (auto lexeme = std::dynamic_pointer_cast<Singularity::Language::Lexeme>(newborn)) {
            if (!Singularity::Language::LanguageSystem::instance().retainLexeme(lexeme))
                return {nullptr, "language index refused duplicate Lexeme identity"};
        }
        if (auto relation = std::dynamic_pointer_cast<Relation>(newborn)) {
            const auto truth = relation->evaluateConstitutive();
            if (truth == Relation::ConstitutiveStatus::Invalid || truth == Relation::ConstitutiveStatus::Violated)
                return {nullptr, "new Relation violates its constitutive kind"};
        }
        if (auto material = std::dynamic_pointer_cast<Material>(newborn)) materials.add(material);
        if (auto field = std::dynamic_pointer_cast<geom::FieldNode>(newborn)) zone->addSpatialField(field);
        else if (!zone->retainSingular(newborn)) return {nullptr, "destination Zone refused duplicate Singular identity"};
        if (auto relation = std::dynamic_pointer_cast<Relation>(newborn)) zone->formation().relations().add(relation);
        if (auto formation = std::dynamic_pointer_cast<Formation>(newborn))
            for (const auto& relation : formation->relations().getAll())
                if (!zone->formation().relations().retain(relation))
                    throw std::runtime_error("destination graph refused the Formation's actual Relation");
        zone->formation().relations().add(std::make_shared<Relation>("branched-from", *newborn, request.prototype, true, 1.0f));
        for (Singular* author : authors) if (author)
            zone->formation().relations().add(std::make_shared<Relation>("authored-by", *newborn, *author, true, 1.0f));
        return {newborn.get(), {}};
    } catch (const std::exception& error) {
        return {nullptr, std::string("Singular birth refused: ") + error.what()};
    }
}

} // namespace

AuthorScope::AuthorScope(const std::vector<Singular*>& authors) : previous(actingAuthors) { actingAuthors = &authors; }
AuthorScope::~AuthorScope() { actingAuthors = previous; }

nlohmann::json storedToJson(const Singular& being) {
    if (!kernelRefusal(being).empty()) return nullptr;
    const auto base = baseToJson(being); // validate memory before legacy codecs walk it
    nlohmann::json state;
    std::string codec;
    if (typeid(being) == typeid(Singularity::Language::Lexeme)) {
        const auto& lexeme = static_cast<const Singularity::Language::Lexeme&>(being);
        codec = "lexeme";
        std::unordered_set<const void*> containers;
        state = {{"id", lexeme.getIdentifier()}, {"symbol", lexeme.getSymbol()},
                 {"conceptualWeight", storedValue(lexeme.conceptualWeightValue(), containers)}};
    } else if (typeid(being) == typeid(Material)) {
        codec = "material";
        state = static_cast<const Material&>(being).toJson();
    } else if (typeid(being) == typeid(Community)) {
        codec = "community";
        state = static_cast<const Community&>(being).toJson();
    } else if (typeid(being) == typeid(Formation)) {
        codec = "formation";
        state = static_cast<const Formation&>(being).toJson();
    } else if (typeid(being) == typeid(Relation)) {
        codec = "relation";
        state = static_cast<const Relation&>(being).toJson();
    } else if (typeid(being) == typeid(geom::FieldNode)) {
        codec = "field";
        state = static_cast<const geom::FieldNode&>(being).toJson();
    } else if (typeid(being) == typeid(Moment)) {
        codec = "moment";
        state = static_cast<const Moment&>(being).toJson();
    } else if (typeid(being) == typeid(ObjectConcept)) {
        const auto& concept = static_cast<const ObjectConcept&>(being);
        if (concept.elementCount() != 0)
            throw std::runtime_error("ObjectConcept composition birth/storage needs its member-custody contract");
        std::unordered_set<const void*> capturedContainers;
        for (const auto& member : concept.members())
            for (const auto& [name, value] : member.captured)
                if (storedValue(value, capturedContainers) != propertyValueToJson(value))
                    throw std::runtime_error("ObjectConcept captured state needs an exact reference/field codec: " + name);
        codec = "object-concept";
        nlohmann::json object;
        to_json(object, static_cast<const Object&>(being));
        object.erase("authoredProperties");
        object.erase("stakeholders");
        auto conceptState = concept.toJson();
        conceptState.erase("authoredProperties");
        state = {{"concept", std::move(conceptState)}, {"object", std::move(object)}};
    } else return nullptr;
    return {{"codec", codec}, {"id", being.getIdentifier()}, {"state", std::move(state)}, {"base", base}};
}

std::shared_ptr<Singular> storedFromJson(const nlohmann::json& record, const Resolver& resolve) {
    if (!record.is_object()) throw std::runtime_error("Singular codec record is not an object");
    const auto codec = record.at("codec").get<std::string>();
    const auto id = record.at("id").get<std::string>();
    if (id.empty()) throw std::runtime_error("Singular codec identity is empty");
    const auto& state = record.at("state");
    if (!referencesAvailable(record.value("base", nlohmann::json::object()), resolve)) return nullptr;
    std::shared_ptr<Singular> being;
    if (codec == "lexeme") {
        auto lexeme = std::make_shared<Singularity::Language::Lexeme>(state.at("symbol").get<std::string>(), id);
        if (!lexeme->setConceptualWeightValue(restoredValue(state.at("conceptualWeight"), resolve)))
            throw std::runtime_error("Lexeme conceptual weight refused");
        being = std::move(lexeme);
    } else if (codec == "material") {
        being = std::make_shared<Material>(Material::fromJson(state));
    } else if (codec == "field") {
        being = geom::FieldNode::fromJson(state);
    } else if (codec == "moment") {
        being = std::make_shared<Moment>(Moment::fromJson(state));
    } else if (codec == "object-concept") {
        auto concept = ObjectConcept::fromJson(state.at("concept"));
        from_json(state.at("object"), static_cast<Object&>(*concept));
        concept->setConceptId(id);
        being = std::move(concept);
    } else if (codec == "formation" || codec == "community") {
        for (const auto& member : state.value("members", nlohmann::json::array()))
            if (!resolve || !resolve(member.at("id").get<std::string>())) return nullptr;
        if (state.contains("root") && (!resolve || !resolve(state["root"].get<std::string>()))) return nullptr;
        for (const auto& relation : state.value("relations", nlohmann::json::array())) {
            if (!resolve || !resolve(relation.at("entityA").get<std::string>()) ||
                !resolve(relation.at("entityB").get<std::string>())) return nullptr;
            if (relation.contains("typeId") && !resolve(relation["typeId"].get<std::string>())) return nullptr;
        }
        auto membership = state;
        membership["relations"] = nlohmann::json::array();
        auto formation = Formation::fromJson(membership, resolve);
        if (!formation) return nullptr;
        if (state.contains("root") && !state["root"].get<std::string>().empty() && !formation->hasRoot())
            throw std::runtime_error("Formation root is not among its actual members; refusing to erase category grounding");
        formation->setIdentifier(id);
        for (const auto& edge : state.value("relations", nlohmann::json::array())) {
            auto decoded = std::make_shared<Relation>(Relation::fromJson(edge, resolve));
            auto* existing = resolve ? dynamic_cast<Relation*>(resolve(decoded->getIdentifier())) : nullptr;
            if (existing && existing->a() == decoded->a() && existing->b() == decoded->b()) {
                auto retained = RelationManager::retained(existing);
                if (!retained) return nullptr;
                if (!formation->retainRelation(retained)) throw std::runtime_error("Formation refused its actual Relation");
            } else if (!formation->retainRelation(decoded)) throw std::runtime_error("Formation refused its restored Relation");
        }
        if (codec == "community") {
            for (auto* member : formation->getMembers())
                if (!dynamic_cast<Person*>(member)) throw std::runtime_error("Community membership requires actual Persons");
            auto community = std::make_shared<Community>(id);
            static_cast<Formation&>(*community) = *formation;
            being = std::move(community);
        } else being = std::move(formation);
    } else if (codec == "relation") {
        auto relation = std::make_shared<Relation>(Relation::fromJson(state, resolve));
        if (!relation->a() || !relation->b()) return nullptr;
        if (state.contains("typeId") && !relation->hasGroundedType()) return nullptr;
        being = std::move(relation);
    } else throw std::runtime_error("unknown or non-synthesizable Singular codec: " + codec);
    if (!being || being->getIdentifier() != id) throw std::runtime_error("Singular codec identity mismatch");
    baseFromJson(*being, record.value("base", nlohmann::json::object()), resolve);
    if (auto relation = std::dynamic_pointer_cast<Relation>(being)) {
        const auto truth = relation->evaluateConstitutive();
        if (truth == Relation::ConstitutiveStatus::Invalid || truth == Relation::ConstitutiveStatus::Violated)
            throw std::runtime_error("stored Relation violates its constitutive kind");
    }
    return being;
}

void restoreStored(Zone& zone, const nlohmann::json& records, bool replace) {
    if (!records.is_array()) throw std::runtime_error("storedSingulars must be an array");
    std::unordered_set<std::string> recordIds;
    for (const auto& record : records) {
        const auto id = record.at("id").get<std::string>();
        if (id.empty() || id == zone.getIdentifier() || !recordIds.insert(id).second)
            throw std::runtime_error("invalid or duplicate stored Singular identity: " + id);
        for (const auto& object : zone.objects())
            if (object && object->getIdentifier() == id)
                throw std::runtime_error("stored Singular identity conflicts with an Object: " + id);
        if (zone.spatialRoot() && zone.spatialRoot()->getIdentifier() == id)
            throw std::runtime_error("stored Singular identity conflicts with the Zone field root");
        for (const auto& field : zone.additionalSpatialFields())
            if (field && field->getIdentifier() == id)
                throw std::runtime_error("stored Singular identity conflicts with a spatial field");
    }
    // Restore dependencies before their referencing Formations/Relations.
    // Progress, not an arbitrary retry bound, terminates this mechanical walk.
    std::vector<std::shared_ptr<Singular>> staged;
    // A decoded Relation can be referenced by a later Formation before Zone
    // admission. Retain its actual owning handle in the existing mechanical
    // manager registry; retain() emits no authored interaction or event.
    RelationManager stagedRelations;
    using Lexeme = Singularity::Language::Lexeme;
    std::vector<std::pair<std::shared_ptr<Lexeme>, std::shared_ptr<Lexeme>>> lexicalUpdates;
    std::vector<nlohmann::json> pending(records.begin(), records.end());
    const Resolver resolve = [&](const std::string& id) -> Singular* {
        if (id == zone.getIdentifier()) return &zone;
        Singular* found = nullptr;
        const auto consider = [&](Singular* candidate) {
            if (!candidate || candidate->getIdentifier() != id) return true;
            if (found && found != candidate) return false;
            found = candidate;
            return true;
        };
        for (const auto& candidate : staged) if (!consider(candidate.get())) return nullptr;
        for (const auto& candidate : staged) {
            if (auto formation = std::dynamic_pointer_cast<Formation>(candidate))
                for (const auto& edge : formation->relations().getAll()) if (!consider(edge.get())) return nullptr;
        }
        if (found) return found; // this snapshot's staged identity wins over its old instance
        for (const auto& record : pending)
            if (record.at("id").get<std::string>() == id) return nullptr;
        for (auto* candidate : zone.formation().getMembers()) if (!consider(candidate)) return nullptr;
        for (const auto& candidate : zone.objects()) if (!consider(candidate.get())) return nullptr;
        for (const auto& candidate : zone.formation().relations().getAll()) if (!consider(candidate.get())) return nullptr;
        if (found) return found;
        for (auto* candidate : Universe::instance().beings()) if (!consider(candidate)) return nullptr;
        if (!found) {
            if (auto lexeme = Singularity::Language::LanguageSystem::instance().findById(id)) found = lexeme.get();
        }
        return found;
    };
    while (!pending.empty()) {
        bool progress = false;
        for (auto it = pending.begin(); it != pending.end();) {
            const std::string id = it->at("id").get<std::string>();
            if (!replace) {
                bool present = false;
                for (const auto& existing : zone.storedSingulars()) if (existing && existing->getIdentifier() == id) present = true;
                if (present) { it = pending.erase(it); progress = true; continue; }
            }
            bool pendingBond = false;
            if (it->at("codec") == "formation" || it->at("codec") == "community") {
                for (const auto& edge : it->at("state").value("relations", nlohmann::json::array())) {
                    const auto edgeId = edge.at("entityA").get<std::string>() + "-" +
                        edge.value("typeId", edge.at("type").get<std::string>()) + "-" + edge.at("entityB").get<std::string>();
                    for (const auto& dependency : pending)
                        if (dependency.at("id") == edgeId) pendingBond = true;
                }
            }
            if (pendingBond) { ++it; continue; }
            auto being = storedFromJson(*it, resolve);
            if (!being) { ++it; continue; }
            for (const auto& existing : staged)
                if (existing->getIdentifier() == id) throw std::runtime_error("duplicate stored Singular identity: " + id);
            if (auto lexeme = std::dynamic_pointer_cast<Lexeme>(being)) {
                if (auto canonical = Singularity::Language::LanguageSystem::instance().findById(id)) {
                    if (canonical->getSymbol() != lexeme->getSymbol())
                        throw std::runtime_error("Lexeme identity has conflicting symbols");
                    lexicalUpdates.emplace_back(canonical, lexeme);
                    being = std::move(canonical);
                }
            }
            if (auto relation = std::dynamic_pointer_cast<Relation>(being))
                if (!stagedRelations.retain(relation)) throw std::runtime_error("conflicting staged Relation identity");
            if (auto formation = std::dynamic_pointer_cast<Formation>(being))
                for (const auto& relation : formation->relations().getAll())
                    if (!stagedRelations.retain(relation)) throw std::runtime_error("conflicting staged Formation bond");
            staged.push_back(std::move(being));
            it = pending.erase(it);
            progress = true;
        }
        if (!progress) {
            std::string ids;
            for (const auto& record : pending) {
                if (!ids.empty()) ids += ", ";
                ids += record.at("id").get<std::string>();
            }
            throw std::runtime_error("stored Singular graph has unresolved participants; refusing partial restoration: " + ids);
        }
    }
    if (replace) zone.clearStoredSingulars();
    for (const auto& [canonical, snapshot] : lexicalUpdates) {
        static_cast<Singular&>(*canonical) = *snapshot;
        canonical->setConceptualWeightValue(snapshot->conceptualWeightValue());
    }
    for (auto& being : staged) {
        if (auto lexeme = std::dynamic_pointer_cast<Lexeme>(being)) {
            if (!Singularity::Language::LanguageSystem::instance().retainLexeme(lexeme))
                throw std::runtime_error("language index refused restored Lexeme identity");
        }
        if (auto material = std::dynamic_pointer_cast<Material>(being)) materials.add(material);
        if (!zone.retainSingular(being)) throw std::runtime_error("destination refused restored Singular identity");
        if (auto relation = std::dynamic_pointer_cast<Relation>(being))
            if (!zone.formation().relations().retain(relation)) throw std::runtime_error("destination graph refused restored Relation");
        if (auto formation = std::dynamic_pointer_cast<Formation>(being))
            for (const auto& relation : formation->relations().getAll())
                if (!zone.formation().relations().retain(relation)) throw std::runtime_error("destination graph refused Formation Relation");
    }
}

Result derive(const Request& request) {
    try {
        if (auto refusal = kernelRefusal(request.prototype); !refusal.empty()) return {nullptr, refusal};
        if (dynamic_cast<Ourverse*>(&request.prototype))
            return {nullptr, "local Ourverse birth awaits its authored extent representation; a global Ourverse (the continuous machine-wide space) cannot be synthesized"};
        if (!request.newbornName.empty() && typeid(request.prototype) != typeid(Object) &&
            typeid(request.prototype) != typeid(ObjectConcept) && typeid(request.prototype) != typeid(Law) &&
            typeid(request.prototype) != typeid(Singularity::Language::Lexeme))
            return {nullptr, "this Singular kind has no newborn-name surface; author a child property action instead"};

        // Whole-prototype copying has no authored selective-replacement contract
        // yet. A closed source gate therefore prevents this operation from taking
        // its state. Do not silently open a gate or drop the protected field.
        // This is deliberately conservative until codec read footprints are exact.
        for (const auto* property : request.prototype.listProperties()) {
            if (property && property->name() != "enabled" && property->name() != "type" && property->name() != "conditionMode" && property->name() != "drives" && property->name() != "name" && property->name() != "weight" && property->name() != "directed" && !TransferPolicy::instance().canTransfer(PropertyPath::parse(property->name())))
                return {nullptr, "whole-prototype birth refused by TransferPolicy source gate: " + property->name()};
        }
        for (const auto& [name, value] : request.prototype.dynamicProperties()) {
            const auto path = Earthcall::StringInterner::resolve(name);
            if (path != "enabled" && path != "type" && path != "conditionMode" && path != "drives" && path != "name" && path != "weight" && path != "directed" && !TransferPolicy::instance().canTransfer(PropertyPath::parse(path)))
                return {nullptr, "whole-prototype birth refused by TransferPolicy source gate: " + path};
        }

        // Exact concrete storage kinds: a base codec must never slice a subclass.
        if (dynamic_cast<Law*>(&request.prototype)) return deriveLaw(request);
        if (dynamic_cast<Zone*>(&request.prototype)) {
            return {nullptr,
                    "Zone set-to-set codec is not wired yet; Zone birth must preserve owner/jurisdiction rather than degrade to Object"};
        }
        if (typeid(request.prototype) == typeid(ObjectConcept)) return deriveStored(request);
        if (dynamic_cast<Object*>(&request.prototype)) return deriveObject(request);

        return deriveStored(request);
    } catch (const std::exception& error) {
        return {nullptr, std::string("Singular birth refused: ") + error.what()};
    }
}

} // namespace SingularSetToSetCreation
