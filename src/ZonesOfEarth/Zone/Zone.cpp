#include "Zone.hpp"
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"
#include "Singularity/Language/JoyHierarchy.hpp"
#include "Singularity/Language/LanguageSystem.hpp"
#include "../../ConstructedBeing/Singular/Lexeme/Lexeme.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include <iostream>
#include <cstdio>
#include <algorithm>
#include <cmath>
#include <ctime>
#include <unordered_set>
#include "Singularity/OntoMath/Field.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"

using Scope = Zone::Scope;

static const char* scopeToString(Scope scope) {
    switch(scope) {
        case Scope::Global:   return "Global";
        case Scope::World:    return "World";
        case Scope::Regional: return "Regional";
        case Scope::Local:    return "Local";
        case Scope::UI:       return "UI";
        default:              return "Unknown";
    }
}

std::string Zone::scopeName() const { return scopeToString(_scope); }

void Zone::buildProperties() {
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<Zone, std::string>>(
        "name", this, &Zone::propName));
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<Zone, std::string>>(
        "scope", this, &Zone::scopeName));
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<Zone, std::string>>(
        "owner", this, &Zone::propOwner));
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<Zone, std::string>>(
        "ownerKind", this, &Zone::propOwnerKind));
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<Zone, std::string>>(
        "kind", this, &Zone::propKind));
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<Zone, bool>>(
        "primary", this, &Zone::propPrimary));
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<Zone, std::string>>(
        "joys", this, &Zone::propJoys, nullptr));

    _propertyRegistry.push_back(std::make_unique<PropertyRef<Zone, std::shared_ptr<OntoMath::ScalarField>>>(
        "spatialField", this, &Zone::_spatialField));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<Zone, std::shared_ptr<OntoMath::VectorField>>>(
        "spatialVectorField", this, &Zone::_spatialVectorField));
}

namespace {
std::string qualityOf(const Zone::Qualities& q, const std::string& key) {
    auto it = q.find(key);
    return it == q.end() ? std::string{} : it->second;
}

bool validOwnerKind(const std::string& kind) {
    return kind.empty()
        || kind == Zone::kOwnerKindPerson
        || kind == Zone::kOwnerKindRelationship
        || kind == Zone::kOwnerKindCommunity;
}
} // namespace

bool Zone::isOurverseGathering() const {
    return qualityOf(_qualities, "kind") == kGatheringKind;
}

void Zone::markOurverseGathering() {
    _qualities["kind"] = kGatheringKind;
    if (!_ownerId.empty()) {
        std::fprintf(stderr,
            "Zone '%s': REFUSED to remain owned after becoming an Ourverse "
            "gathering. No one may own the gathering place (OURVERSE.md).\n",
            _name.c_str());
        _ownerId.clear();
        _qualities.erase("ownerKind");
    }
}

std::string Zone::propKind() const { return qualityOf(_qualities, "kind"); }

bool Zone::propPrimary() const { return isPrimaryHome(); }

std::string Zone::propOwnerKind() const { return qualityOf(_qualities, "ownerKind"); }

bool Zone::isPersonalHome() const {
    return qualityOf(_qualities, "kind") == kHomeKind;
}

bool Zone::isCommunityHome() const {
    return qualityOf(_qualities, "kind") == kCommunityHomeKind;
}

bool Zone::isCommunityZone() const {
    return qualityOf(_qualities, "kind") == kCommunityZoneKind;
}

bool Zone::isHome() const {
    return isPersonalHome() || isCommunityHome();
}

bool Zone::isPrimaryHome() const {
    if (isOurverseGathering() || isCommunityHome() || isCommunityZone()) return false;
    if (qualityOf(_qualities, "primary") == "true" && isPersonalHome()) return true;
    // Pre-primary saves: the slug Home with kind=home is the locked dwelling.
    return _name == "Home" && isPersonalHome();
}

void Zone::markPrimaryHome() {
    if (isOurverseGathering()) {
        std::fprintf(stderr,
            "Zone '%s': REFUSED to become a Home. A gathering place is not a "
            "dwelling (EarthcallOurverse.md).\n",
            _name.c_str());
        return;
    }
    _qualities["kind"] = kHomeKind;
    _qualities["primary"] = "true";
}

void Zone::markCommunityHome() {
    if (isOurverseGathering() || isPrimaryHome()) {
        std::fprintf(stderr,
            "Zone '%s': REFUSED community-home. Gathering and a Person's "
            "primary Home cannot be re-kinded.\n",
            _name.c_str());
        return;
    }
    _qualities["kind"] = kCommunityHomeKind;
    _qualities.erase("primary");
}

void Zone::markCommunityZone() {
    if (isOurverseGathering() || isPrimaryHome()) {
        std::fprintf(stderr,
            "Zone '%s': REFUSED community-zone. Gathering and a Person's "
            "primary Home cannot be re-kinded.\n",
            _name.c_str());
        return;
    }
    _qualities["kind"] = kCommunityZoneKind;
    _qualities.erase("primary");
}

void Zone::setQuality(const std::string& key, const std::string& value) {
    if (isPrimaryHome() && (key == "primary" || key == "kind")) {
        const bool demotePrimary = (key == "primary" && value != "true");
        const bool demoteKind = (key == "kind" && value != kHomeKind);
        if (demotePrimary || demoteKind) {
            std::fprintf(stderr,
                "Zone '%s': REFUSED to change %s. A Person's primary Home is "
                "kernel-locked (EarthcallOurverse.md).\n",
                _name.c_str(), key.c_str());
            return;
        }
    }
    if (isOurverseGathering() && key == "kind" && value != kGatheringKind) {
        std::fprintf(stderr,
            "Zone '%s': REFUSED to change kind. The gathering place stays "
            "unowned and un-homed (OURVERSE.md).\n",
            _name.c_str());
        return;
    }
    _qualities[key] = value;
}

void Zone::setDeletable(const std::string& person, bool flag) {
    if (isPrimaryHome() && flag) {
        std::fprintf(stderr,
            "Zone '%s': REFUSED deletable. A Person's primary Home is not "
            "erased (EarthcallOurverse.md).\n",
            _name.c_str());
        return;
    }
    _deletable[person] = flag;
}

bool Zone::isDeletable(const std::string& person) const {
    if (isPrimaryHome()) return false;
    auto it = _deletable.find(person);
    return it != _deletable.end() ? it->second : false;
}

void Zone::setOwner(const std::string& ownerId) {
    std::string kind = qualityOf(_qualities, "ownerKind");
    if (kind.empty() && !ownerId.empty()) {
        if (isCommunityHome() || isCommunityZone()) kind = kOwnerKindCommunity;
        else kind = kOwnerKindPerson;
    }
    setOwner(ownerId, kind);
}

void Zone::setOwner(const std::string& ownerId, const std::string& ownerKind) {
    if (isOurverseGathering() && !ownerId.empty()) {
        std::fprintf(stderr,
            "Zone '%s': REFUSED owner '%s'. A local Ourverse gathering is "
            "unowned — Person, Relationship, and Community alike "
            "(OURVERSE.md / EarthcallOurverse.md).\n",
            _name.c_str(), ownerId.c_str());
        return;
    }
    if (!validOwnerKind(ownerKind)) {
        std::fprintf(stderr,
            "Zone '%s': REFUSED ownerKind '%s'. Owner is a Person, a "
            "Relationship, or a Community.\n",
            _name.c_str(), ownerKind.c_str());
        return;
    }
    if (isPrimaryHome()) {
        if (ownerKind == kOwnerKindRelationship || ownerKind == kOwnerKindCommunity) {
            std::fprintf(stderr,
                "Zone '%s': REFUSED owner '%s' (%s). A Person's primary Home "
                "is fully owned by that Person, not a Relationship or "
                "Community (EarthcallOurverse.md).\n",
                _name.c_str(), ownerId.c_str(), ownerKind.c_str());
            return;
        }
        if (!_ownerId.empty() && ownerId != _ownerId) {
            std::fprintf(stderr,
                "Zone '%s': REFUSED to transfer primary Home from '%s' to "
                "'%s'. Highest ownership priority is kernel-locked to the "
                "Person who owns this dwelling.\n",
                _name.c_str(), _ownerId.c_str(), ownerId.c_str());
            return;
        }
        if (_ownerId.empty() && ownerId.empty()) return;
    }
    if ((isCommunityHome() || isCommunityZone()) && !ownerId.empty()
        && ownerKind != kOwnerKindCommunity) {
        std::fprintf(stderr,
            "Zone '%s': REFUSED owner '%s' (%s). A Community Home/Zone is "
            "owned by a Community (EarthcallOurverse.md).\n",
            _name.c_str(), ownerId.c_str(), ownerKind.c_str());
        return;
    }

    _ownerId = ownerId;
    if (ownerId.empty()) _qualities.erase("ownerKind");
    else if (!ownerKind.empty()) _qualities["ownerKind"] = ownerKind;

    if (!ownerId.empty() && !isPrimaryHome()) _deletable[ownerId] = true;
}

void Zone::load() {
    std::cout << "🌍 Zone '" << _name << "' loaded with " << _objects.size() << " objects." << std::endl;
}

void Zone::unload() {
    _objects.clear();
    std::cout << "🌍 Zone '" << _name << "' unloaded." << std::endl;
}

void Zone::syncFormationMembers(const std::vector<Singular*>& extraMembers) {
    std::unordered_set<const Singular*> live;
    const auto admit = [&](Singular* member) {
        if (!member) return;
        live.insert(member);
        _formation.addMember(member);
    };

    admit(_spatialRootObject.get());
    for (const auto& up : _objects) admit(up.get());
    for (auto* member : extraMembers) admit(member);

    const std::vector<Singular*> current = _formation.getMembers();
    for (Singular* member : current) {
        if (!member || live.count(member) != 0) continue;
        // Lexemes are language beings admitted into the Zone formation; they
        // are not Zone objects and must survive the object-membership sweep.
        if (dynamic_cast<Singularity::Language::Lexeme*>(member)) continue;
        _formation.removeMember(member);
    }
}

void Zone::applyFormationRelations() {
    _formation.applyAttachmentRelations();
}

Zone::Zone(const std::string& name, const std::string& foundationSymbol, Scope scope)
    : _name(name), _scope(scope), _formation(),
      _spatialRootObject(std::make_shared<geom::FieldNode>(name + "_spatialRoot"))
{
    _spatialField = _spatialRootObject->field;
    _spatialVectorField = _spatialRootObject->vectorField;

    _formation.addMember(_spatialRootObject.get());
    _joys.setIdentifier(name + ".joys");
    Singularity::Language::Lexeme* foundation = nullptr;
    if (!foundationSymbol.empty()) {
        auto& lang = Singularity::Language::LanguageSystem::instance();
        foundation = (foundationSymbol == "default" || foundationSymbol == "strict")
            ? lang.foundation().get()
            : lang.resolve(foundationSymbol).get();
    }
    Singularity::Language::seedJoyHierarchy(_joys, foundation);
    if (_joys.root()) setTelosId(_joys.root()->getIdentifier());
}

Zone::Zone(const Zone& other)
    : _name(other._name), _scope(other._scope), _qualities(other._qualities), _deletable(other._deletable),
      _joys(other._joys), _ownerId(other._ownerId), _formation(),
      _spatialRootObject(std::make_shared<geom::FieldNode>(other._name + "_spatialRoot"))
{
    _spatialField = _spatialRootObject->field;
    _spatialVectorField = _spatialRootObject->vectorField;

    _formation.addMember(_spatialRootObject.get());
    if (_joys.root()) setTelosId(_joys.root()->getIdentifier());
}

Zone& Zone::operator=(const Zone& other)
{
    if(this==&other) return *this;
    Zone tmp(other);
    std::swap(_name, tmp._name);
    std::swap(_scope, tmp._scope);
    std::swap(_qualities, tmp._qualities);
    std::swap(_deletable, tmp._deletable);
    std::swap(_joys, tmp._joys);
    std::swap(_ownerId, tmp._ownerId);
    std::swap(_objects, tmp._objects);
    std::swap(_formation, tmp._formation);
    std::swap(_spatialRootObject, tmp._spatialRootObject);
    std::swap(_spatialField, tmp._spatialField);
    std::swap(_spatialVectorField, tmp._spatialVectorField);
    return *this;
}

void Zone::describe() const {
    std::cout << "🌀 Entering zone: " << _name << " (" << scopeToString(_scope) << ")" << std::endl;

    if(!_qualities.empty()) {
        std::cout << "   Qualities:" << std::endl;
        for (const auto &q : _qualities) {
            std::cout << "     - " << q.first << ": " << q.second << std::endl;
        }
    }

    if(!_deletable.empty()) {
        std::cout << "   Deletable by:" << std::endl;
        for (const auto &d : _deletable) {
            std::cout << "     - " << d.first << ": " << (d.second?"yes":"no") << std::endl;
        }
    }
}

void Zone::addObject(std::shared_ptr<Object> obj) {
    _objects.push_back(std::move(obj));
}

bool Zone::removeObject(Object* obj) {
    if (!obj) return false;
    auto it = std::find_if(_objects.begin(), _objects.end(),
                           [obj](const std::shared_ptr<Object>& p) { return p.get() == obj; });
    if (it == _objects.end()) return false;

    Core::EventBus::instance().publish(
        ECA::Event{"object-destroyed", obj, nullptr, std::time(nullptr)});

    for (const auto& other : _objects) {
        if (other && other.get() != obj) other->removeElement(obj);
    }
    _objects.erase(it);
    return true;
}

bool Zone::removeObjectById(const std::string& identifier) {
    for (const auto& obj : _objects) {
        if (obj && obj->getIdentifier() == identifier) return removeObject(obj.get());
    }
    return false;
}

void Zone::update(float dt) {
    // The floor is the object a First Mover TAGGED as the floor, or the y=0
    // plane. There is no fall-back to "whatever is at index 1".
    float groundY = 0.0f;
    for (const auto& obj : _objects) {
        if (!obj || !obj->hasAttribute("baseline")) continue;
        if (obj->getAttribute("baseline") != std::string("ground")) continue;
        const glm::mat4& gT = obj->getTransform();
        float scaleY = glm::length(glm::vec3(gT[1]));
        groundY = gT[3][1] + 0.5f * scaleY;
        break;
    }

    const float maxStep = 0.02f;
    const float maxFrameTime = 0.1f;
    if (dt > maxFrameTime) dt = maxFrameTime;
    int steps = std::max(1, (int)std::ceil(dt / maxStep));
    float stepDt = dt / steps;

    for (int s = 0; s < steps; ++s) {
        for (const auto& up : _objects) {
            if (!up) continue;
            if (up->hasPendingRotation()) {
                up->updateRotation(stepDt);
            }
            if (up->hasAutomations()) {
                up->updateAutomations(stepDt);
            }
        }
        if (Physics::getLegacyEngineEnabled()) {
            for (const auto& up : _objects) if (up) Physics::getFormFor(up.get());
            Physics::updateBodies(_objects, stepDt, 9.81f, 0.1f, groundY);
        }
    }
}

Zone::~Zone() {}
