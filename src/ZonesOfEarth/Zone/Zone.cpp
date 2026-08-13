#include "Zone.hpp"
#include "../World/World.hpp"
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"
#include <iostream>
#include <algorithm>
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

    _propertyRegistry.push_back(std::make_unique<PropertyRef<Zone, std::shared_ptr<OntoMath::ScalarField>>>(
        "spatialField", this, &Zone::_spatialField));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<Zone, std::shared_ptr<OntoMath::VectorField>>>(
        "spatialVectorField", this, &Zone::_spatialVectorField));
}

void Zone::load() {
    _world->load();
    std::cout << "🌍 Zone '" << _name << "' loaded with " << _world->objects().size() << " objects." << std::endl;
}

void Zone::unload() {
    _world->unload();
    std::cout << "🌍 Zone '" << _name << "' unloaded." << std::endl;
}

void Zone::syncFormationMembers(const std::vector<Singular*>& extraMembers) {
    std::unordered_set<const Singular*> live;
    const auto admit = [&](Singular* member) {
        if (!member) return;
        live.insert(member);
        _formation.addMember(member);
    };

    admit(_world.get());
    admit(_spatialRootObject.get());
    for (const auto& up : _world->getOwnedObjects()) admit(up.get());
    for (auto* member : extraMembers) admit(member);

    const std::vector<Singular*> current = _formation.getMembers();
    for (Singular* member : current) {
        if (!member || live.count(member) == 0) _formation.removeMember(member);
    }
}

void Zone::applyFormationRelations() {
    _formation.applyAttachmentRelations();
}

Zone::Zone(const std::string& name, const std::string& joyOrdering, Scope scope)
    : _name(name), _scope(scope), _joyOrdering(joyOrdering), _world(std::make_unique<World>()), _formation(),
      _spatialRootObject(std::make_shared<geom::FieldNode>(name + "_spatialRoot"))
{
    _spatialField = _spatialRootObject->field;
    _spatialVectorField = _spatialRootObject->vectorField;

    _formation.addMember(_world.get());
    _formation.addMember(_spatialRootObject.get());
}

Zone::Zone(const Zone& other)
    : _name(other._name), _scope(other._scope), _qualities(other._qualities), _deletable(other._deletable), _joyOrdering(other._joyOrdering), _ownerId(other._ownerId), _world(std::make_unique<World>()), _formation(),
      _spatialRootObject(std::make_shared<geom::FieldNode>(other._name + "_spatialRoot"))
{
    _spatialField = _spatialRootObject->field;
    _spatialVectorField = _spatialRootObject->vectorField;

    _formation.addMember(_world.get());
    _formation.addMember(_spatialRootObject.get());
}

Zone& Zone::operator=(const Zone& other)
{
    if(this==&other) return *this;
    Zone tmp(other);
    std::swap(_name, tmp._name);
    std::swap(_scope, tmp._scope);
    std::swap(_qualities, tmp._qualities);
    std::swap(_deletable, tmp._deletable);
    std::swap(_joyOrdering, tmp._joyOrdering);
    std::swap(_ownerId, tmp._ownerId);
    std::swap(_world, tmp._world);
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

Zone::~Zone() {}
