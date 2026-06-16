#include "Object.hpp"
#include "Contour.hpp"
#include "AngleTools.hpp"
#include "Automation/AutomationEvents.hpp"
#include <GLFW/glfw3.h>
#include <OpenGL/glu.h>
#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <cstring>
#include <cstdlib> // for rand()
#include <cmath>   // for mathematical functions
#include <limits>  // for numeric_limits
#include <optional>
#include <unordered_set>
#include <unordered_map>
#include <atomic>
#include "Rendering/HighlightSystem.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Event structures for Object hover events
struct ObjectHoverEvent {
    const Object& object;
    glm::vec3 hoverPoint;
    glm::vec2 screenPosition;
    std::time_t timestamp;
    
    ObjectHoverEvent(const Object& obj, const glm::vec3& point, const glm::vec2& screen)
        : object(obj), hoverPoint(point), screenPosition(screen), timestamp(std::time(nullptr)) {}
};

struct ObjectHoverEnterEvent {
    const Object& object;
    glm::vec3 hoverPoint;
    glm::vec2 screenPosition;
    std::time_t timestamp;
    
    ObjectHoverEnterEvent(const Object& obj, const glm::vec3& point, const glm::vec2& screen)
        : object(obj), hoverPoint(point), screenPosition(screen), timestamp(std::time(nullptr)) {}
};

struct ObjectHoverExitEvent {
    const Object& object;
    glm::vec3 lastHoverPoint;
    glm::vec2 lastScreenPosition;
    std::time_t timestamp;
    
    ObjectHoverExitEvent(const Object& obj, const glm::vec3& point, const glm::vec2& screen)
        : object(obj), lastHoverPoint(point), lastScreenPosition(screen), timestamp(std::time(nullptr)) {}
};


// Implementation of PolyhedronData methods

int Object::getDimensions() {
    return dimensions;
}

void Object::setDimensions(int d) {
    dimensions = d;
}

int Object::getCorners() {
    if (geometryType == GeometryType::Polyhedron) {
        return polyhedronData.getVertexCount();
    }
    return corners;
}

void Object::setCorners(int c) {
    corners = c;
    // For polyhedrons, this could trigger a regeneration of the polyhedron
    // For now, we'll just store the value for compatibility
}

int Object::getFaces() {
    if (_hasField)   return 1;
    if (_hasComplex) return complexData.patchCount();
    if (_hasSmooth)  return 1;
    if (geometryType == GeometryType::Polyhedron) {
        return polyhedronData.getFaceCount();
    }
    return faces;
}

void Object::setFaces(int f) {
    faces = f;
    // For polyhedrons, this could trigger a regeneration of the polyhedron
    // For now, we'll just store the value for compatibility
}

int Object::getMassQuantity() {
    return massQuantity;
}

void Object::setMassQuantity(int m) {
    massQuantity = m;
}

int Object::getElements() {
    return elements;
}

void Object::setElements(int e) {
    elements = e;
}

int Object::getRelationships() {
    return relationships;
}

void Object::setRelationships(int r) {
    relationships = r;
}

int Object::getComplexityLevel() {
    return complexityLevel;
}

void Object::setComplexityLevel(int cl) {
    complexityLevel = cl;
}

int Object::getPhysicalObject() {
    return physicalObject ? 1 : 0;
}

void Object::setPhysicalObject(int po) {
    physicalObject = (po != 0);
}

int Object::getSymbolicObject() {
    return physicalObject ? 0 : 1; // Inverse of physical object
}

void Object::setSymbolicObject(int so) {
    physicalObject = (so == 0); // Inverse of symbolic object
}

std::string Object::getObjectID() {
    return objectID;
}

void Object::setObjectID(int oi) {
    objectID = std::to_string(oi);
}

std::string Object::getObjectType() const {
    return objectType;
}

void Object::setObjectType(int ot) {
    objectType = std::to_string(ot);
}

int Object::getX() {
    return x;
}

void Object::setX(int x) {
    this->x = x;
}

std::string Object::screenMode() {

    if (dimensions == 2.0f) {
        return "2D";
    } else if (dimensions == 3.0f) {
        return "3D";
    } else {
        return "Unknown";
    }

}


// Polyhedron-specific methods
void Object::setPolyhedronData(const PolyhedronData& data) {
    polyhedronData = data;
    _hasSmooth = false;   // a polyhedron is flat-faced, not a topology surface
    _hasComplex = false;
    if (geometryType == GeometryType::Polyhedron) {
        initFaceTextures();
    }
}

void Object::createTetrahedron() {
    geometryType = GeometryType::Polyhedron;
    polyhedronData = PolyhedronData::createRegularPolyhedron(4);
    initFaceTextures();
}

void Object::createOctahedron() {
    geometryType = GeometryType::Polyhedron;
    polyhedronData = PolyhedronData::createRegularPolyhedron(8);
    initFaceTextures();
}

void Object::createDodecahedron() {
    geometryType = GeometryType::Polyhedron;
    polyhedronData = PolyhedronData::createRegularPolyhedron(12);
    initFaceTextures();
}

void Object::createIcosahedron() {
    geometryType = GeometryType::Polyhedron;
    polyhedronData = PolyhedronData::createRegularPolyhedron(20);
    initFaceTextures();
}

void Object::createCustomPolyhedron(const std::vector<glm::vec3>& vertices, 
                                   const std::vector<std::vector<int>>& faces) {
    geometryType = GeometryType::Polyhedron;
    polyhedronData = PolyhedronData::createCustomPolyhedron(vertices, faces);
    initFaceTextures();
}

Object::Object() {
    static std::atomic<uint64_t> nextObjectId{1};
    if (objectID.empty()) {
        objectID = "object-" + std::to_string(nextObjectId.fetch_add(1));
    }
    initFaceTextures();
    syncRotationStateFromTransform(transform);
}

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
    
    // Trigger events based on hover state changes
    if (isHovering && !wasHovered) {
        // Mouse entered the object
        ObjectHoverEnterEvent event(*this, _hoverPoint, glm::vec2(0, 0)); // Screen pos would be passed in
        Core::EventBus::instance().publish(event);
    } else if (!isHovering && wasHovered) {
        // Mouse exited the object
        ObjectHoverExitEvent event(*this, _hoverPoint, glm::vec2(0, 0)); // Screen pos would be passed in
        Core::EventBus::instance().publish(event);
    } else if (isHovering) {
        // Mouse is hovering over the object
        ObjectHoverEvent event(*this, _hoverPoint, glm::vec2(0, 0)); // Screen pos would be passed in
        Core::EventBus::instance().publish(event);
    }
}

// --------------------------------------------------------------
// Attributes/Tags implementation
// --------------------------------------------------------------
void Object::setAttribute(const std::string& key, const std::string& value) {
    attributes[key] = value;
}

bool Object::hasAttribute(const std::string& key) const {
    return attributes.find(key) != attributes.end();
}

const std::string& Object::getAttribute(const std::string& key) const {
    static const std::string empty;
    auto it = attributes.find(key);
    return it == attributes.end() ? empty : it->second;
}

void Object::addTag(const std::string& tag) {
    if (!hasTag(tag)) tags.push_back(tag);
}

void Object::removeTag(const std::string& tag) {
    tags.erase(std::remove(tags.begin(), tags.end(), tag), tags.end());
}

bool Object::hasTag(const std::string& tag) const {
    for (const auto& t : tags) if (t == tag) return true;
    return false;
}
