#include "EventEntity.hpp"
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"
#include "ConstructedBeing/Singular/Property/PropertyRef.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include <uuid/uuid.h>

namespace Core {
EventEntity::EventEntity(const std::string& eventType) : _eventType(eventType) {
    uuid_t uuid;
    uuid_generate(uuid);
    char uuid_str[37];
    uuid_unparse(uuid, uuid_str);
    _id = "event_" + std::string(uuid_str);
    Universe::instance().addActiveEvent(this);
}

EventEntity::~EventEntity() {
    Universe::instance().removeActiveEvent(this);
    Core::EventBus::instance().publish(
        ECA::Event{"object-destroyed", this, nullptr, std::time(nullptr)});
}

std::string EventEntity::getIdentifier() const {
    return _id;
}

void EventEntity::buildProperties() {
    _propertyRegistry.push_back(
        std::make_unique<PropertyRef<EventEntity, std::string>>("eventType", this, &EventEntity::_eventType));
    _propertyRegistry.push_back(
        std::make_unique<PropertyRef<EventEntity, std::string>>("sourceId", this, &EventEntity::_sourceId));
    _propertyRegistry.push_back(
        std::make_unique<PropertyRef<EventEntity, std::string>>("targetId", this, &EventEntity::_targetId));
    _propertyRegistry.push_back(
        std::make_unique<ComputedProperty<EventEntity, double>>(
            "moment", this, &EventEntity::propMomentStart, &EventEntity::setMomentStart));
    _propertyRegistry.push_back(
        std::make_unique<ComputedProperty<EventEntity, double>>(
            "momentEnd", this, &EventEntity::propMomentEnd, &EventEntity::setMomentEnd));
    _propertiesBuilt = true;
}

} // namespace Core
