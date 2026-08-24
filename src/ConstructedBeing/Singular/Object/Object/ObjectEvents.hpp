#pragma once

// ============================================================================
// ObjectEvents.hpp - Event structures for Object hover events
// 
// This header contains event structures used by the Object class for hover
// detection and interaction events.
// ============================================================================

#include "Time/Moment/Moment.hpp"
#include <glm/glm.hpp>

// Forward declaration to avoid circular dependency
class Object;

// ============================================================================
// Hover Event Structures
// ============================================================================

// Base hover event - triggered continuously while mouse is hovering over object
struct ObjectHoverEvent {
    const Object& object;
    glm::vec3 hoverPoint;
    glm::vec2 screenPosition;
    Moment timestamp;
    
    ObjectHoverEvent(const Object& obj, const glm::vec3& point, const glm::vec2& screen)
        : object(obj), hoverPoint(point), screenPosition(screen), timestamp(Moment::now()) {}
};

// Triggered when mouse enters the object's bounds
struct ObjectHoverEnterEvent {
    const Object& object;
    glm::vec3 hoverPoint;
    glm::vec2 screenPosition;
    Moment timestamp;
    
    ObjectHoverEnterEvent(const Object& obj, const glm::vec3& point, const glm::vec2& screen)
        : object(obj), hoverPoint(point), screenPosition(screen), timestamp(Moment::now()) {}
};

// Triggered when mouse exits the object's bounds
struct ObjectHoverExitEvent {
    const Object& object;
    glm::vec3 lastHoverPoint;
    glm::vec2 lastScreenPosition;
    Moment timestamp;
    
    ObjectHoverExitEvent(const Object& obj, const glm::vec3& point, const glm::vec2& screen)
        : object(obj), lastHoverPoint(point), lastScreenPosition(screen), timestamp(Moment::now()) {}
};
