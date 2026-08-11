// ============================================================================
// ObjectEvents.cpp - Event-related Object implementations
//
// This file contains event handling implementations for Object,
// including hover detection and event publishing.
// ============================================================================

#include "ConstructedBeing/Object/Object.hpp"
#include "ConstructedBeing/Object/Object/ObjectEvents.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Hover detection method implementations

bool Object::isMouseHovering(const glm::vec2& mousePos, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, int windowWidth, int windowHeight) const {
    // Convert screen coordinates to world coordinates
    glm::vec4 screenPos(mousePos.x, mousePos.y, 0.0f, 1.0f);
    
    // Convert to normalized device coordinates
    screenPos.x = (screenPos.x / windowWidth) * 2.0f - 1.0f;
    screenPos.y = (screenPos.y / windowHeight) * 2.0f - 1.0f;
    screenPos.y = -screenPos.y; // Flip Y coordinate
    
    // Create ray from camera through mouse position
    glm::mat4 invVP = glm::inverse(projectionMatrix * viewMatrix);
    glm::vec4 worldPos = invVP * screenPos;
    worldPos /= worldPos.w;
    
    glm::vec3 rayOrigin = glm::vec3(invVP * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    glm::vec3 rayDirection = glm::normalize(glm::vec3(worldPos) - rayOrigin);
    
    // Check intersection with object's collision zone
    return isMouseHovering(rayOrigin + rayDirection * 10.0f); // Check at reasonable distance
}

bool Object::isMouseHovering(const glm::vec3& worldMousePos) const {
    // Use the existing collision detection system
    return isPointInside(worldMousePos);
}

void Object::updateHoverState(bool isHovering) {
    bool wasHovered = _wasHoveredLastFrame;
    _wasHoveredLastFrame = _isHovered;
    _isHovered = isHovering;
    
    // Trigger events based on hover state changes. Enter/exit also echo as
    // string-typed ECA::Events so Person-authored laws can bind to them; the
    // continuous per-frame hover does not (only discrete edges travel as
    // events — same rule as AutomationEvents.hpp).
    if (isHovering && !wasHovered) {
        // Mouse entered the object
        ObjectHoverEnterEvent event(*this, _hoverPoint, glm::vec2(0, 0)); // Screen pos would be passed in
        Core::EventBus::instance().publish(event);
        Core::EventBus::instance().publish(ECA::Event{"object-hover-entered", this, nullptr, std::time(nullptr)});
    } else if (!isHovering && wasHovered) {
        // Mouse exited the object
        ObjectHoverExitEvent event(*this, _hoverPoint, glm::vec2(0, 0)); // Screen pos would be passed in
        Core::EventBus::instance().publish(event);
        Core::EventBus::instance().publish(ECA::Event{"object-hover-exited", this, nullptr, std::time(nullptr)});
    } else if (isHovering) {
        // Mouse is hovering over the object
        ObjectHoverEvent event(*this, _hoverPoint, glm::vec2(0, 0)); // Screen pos would be passed in
        Core::EventBus::instance().publish(event);
    }
}
