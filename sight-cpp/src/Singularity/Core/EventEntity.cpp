#include "EventEntity.hpp"
#include "Form/Singular/Property/PropertyRef.hpp"
#include <uuid/uuid.h>

namespace Core {
EventEntity::EventEntity(const std::string& eventType) : _eventType(eventType) {
    uuid_t uuid;
    uuid_generate(uuid);
    char uuid_str[37];
    uuid_unparse(uuid, uuid_str);
    _id = "event_" + std::string(uuid_str);
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
    _propertiesBuilt = true;
}

} // namespace Core
