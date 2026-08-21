#include "Zone.hpp"
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"
#include "Singularity/Language/JoyHierarchy.hpp"
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
#include "ConstructedBeing/Object/Geometry/FieldNode.hpp"

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
        "joys", this, &Zone::propJoys, nullptr));

    _propertyRegistry.push_back(std::make_unique<PropertyRef<Zone, std::shared_ptr<OntoMath::ScalarField>>>(
        "spatialField", this, &Zone::_spatialField));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<Zone, std::shared_ptr<OntoMath::VectorField>>>(
        "spatialVectorField", this, &Zone::_spatialVectorField));
}

bool Zone::isOurverseGathering() const {
    auto it = _qualities.find("kind");
    return it != _qualities.end() && it->second == kGatheringKind;
}

void Zone::markOurverseGathering() {
    _qualities["kind"] = kGatheringKind;
    if (!_ownerId.empty()) {
        std::fprintf(stderr,
            "Zone '%s': REFUSED to remain owned after becoming an Ourverse "
            "gathering. No one may own the gathering place (OURVERSE.md).\n",
            _name.c_str());
        _ownerId.clear();
    }
}

void Zone::setOwner(const std::string& personId) {
    if (isOurverseGathering() && !personId.empty()) {
        std::fprintf(stderr,
            "Zone '%s': REFUSED owner '%s'. A local Ourverse gathering is "
            "unowned — all may participate equally (OURVERSE.md).\n",
            _name.c_str(), personId.c_str());
        return;
    }
    _ownerId = personId;
    if (!personId.empty()) _deletable[personId] = true;
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
        if (!member || live.count(member) == 0) _formation.removeMember(member);
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
    Singularity::Language::seedJoyHierarchy(_joys, foundationSymbol);
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
