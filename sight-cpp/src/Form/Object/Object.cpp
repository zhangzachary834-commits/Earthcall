#include "Object.hpp"
#include "Contour.hpp"
#include "AngleTools.hpp"
#include "Automation/AutomationEvents.hpp"
#include "Form/Singular/Property/ComputedProperty.hpp"
#include "Form/Singular/Property/PropertyRef.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
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

namespace {
std::atomic<uint64_t> g_nextObjectId{1};
}

// A restored identity must ADVANCE the fresh-id counter past itself, or the
// next newborn collides with a loaded being ("object-3" twice) and every
// by-identifier reference — law targets, @-paths, relations — blurs.
void Object::claimIdentifierAtLeast(const std::string& id) {
    const std::string prefix = "object-";
    if (id.rfind(prefix, 0) != 0) return;
    const uint64_t n = std::strtoull(id.c_str() + prefix.size(), nullptr, 10);
    uint64_t current = g_nextObjectId.load();
    while (n + 1 > current &&
           !g_nextObjectId.compare_exchange_weak(current, n + 1)) {
    }
}

Object::Object() {
    if (objectID.empty()) {
        objectID = "object-" + std::to_string(g_nextObjectId.fetch_add(1));
    }
    initFaceTextures();
    syncRotationStateFromTransform(transform);
}

void Object::setPosition(const glm::vec3& p) {
    glm::mat4 t = transform;
    t[3] = glm::vec4(p, 1.0f);
    setTransform(t);   // virtual: rotation-state sync + collision-zone update
}

// The first-mover property registry: what a Person (through PropertyPath, and
// later through Laws) can address on an Object without touching C++. Names may
// be dotted ("shape.r") — PropertyPath matches registered names longest-first.
// ---------------------------------------------------------------------------
// Shape parameters that REGENERATE geometry when written: a law that sets
// shape.r on a sphere reshapes the sphere, not just a number. Kinds whose
// visible form does not come from the params (cube/polyhedron: vertex data;
// field/patch: sculpted payloads) take the raw write — regenerating would
// wipe paint or sculpt for nothing.
// ---------------------------------------------------------------------------
namespace {

bool paramsShapeGeometry(const Object& o) {
    switch (o.getShapeKind()) {
        case Object::ShapeKind::Sphere:
        case Object::ShapeKind::Cylinder:
        case Object::ShapeKind::Cone:
        case Object::ShapeKind::Ellipsoid:
        case Object::ShapeKind::Ovoid:
        case Object::ShapeKind::Paraboloid:
        case Object::ShapeKind::Torus:
        case Object::ShapeKind::RoundedBox:
            break;
        default:
            return false;
    }
    const auto spatial = o.getSpatialKind();
    return spatial != Object::SpatialKind::Field &&
           spatial != Object::SpatialKind::Patch;
}

class ShapeParamBridge : public Property {
public:
    ShapeParamBridge(std::string name, Object* owner,
                     float Object::ShapeParams::*member)
        : _name(std::move(name)), _owner(owner), _member(member) {}

    std::string name() const override { return _name; }
    std::string typeName() const override { return "float"; }

    PropertyValue value() const override {
        return PropertyValue(_owner->getShapeParams().*_member);
    }
    bool setValue(const PropertyValue& v) override {
        double n = 0.0;
        if (!propertyValueToNumber(v, n)) return false;
        Object::ShapeParams params = _owner->getShapeParams();
        params.*_member = static_cast<float>(n);
        if (paramsShapeGeometry(*_owner)) {
            _owner->setShape(_owner->getShapeKind(), params);   // regenerate
        } else {
            _owner->assignShapeParams(params);
        }
        return true;
    }

private:
    std::string _name;
    Object* _owner;
    float Object::ShapeParams::*_member;
};

// The shape's KIND itself is governable: a law can transmute a cube into a
// sphere. Field (10) and Patch (11) are refused as targets — those forms are
// sculpted payloads, not an integer's worth of information.
class ShapeKindBridge : public Property {
public:
    explicit ShapeKindBridge(Object* owner) : _owner(owner) {}
    std::string name() const override { return "shape.kind"; }
    std::string typeName() const override { return "int"; }
    PropertyValue value() const override {
        return PropertyValue(static_cast<int>(_owner->getShapeKind()));
    }
    bool setValue(const PropertyValue& v) override {
        double n = 0.0;
        if (!propertyValueToNumber(v, n)) return false;
        const int kind = static_cast<int>(n);
        if (kind < 0 || kind > static_cast<int>(Object::ShapeKind::RoundedBox)) {
            return false;
        }
        _owner->setShape(static_cast<Object::ShapeKind>(kind),
                         _owner->getShapeParams());
        return true;
    }

private:
    Object* _owner;
};

// Motion state made legible: velocity and mass live in the physics engine's
// rigid-body registry; these bridges are what let collision RESPONSE migrate
// into authored laws ("on collision, reflect @event.object's velocity").
class RigidBodyBridge : public Property {
public:
    enum class Field { Velocity, Mass };
    RigidBodyBridge(std::string name, Object* owner, Field field)
        : _name(std::move(name)), _owner(owner), _field(field) {}

    std::string name() const override { return _name; }
    std::string typeName() const override {
        return _field == Field::Velocity ? "vec3" : "float";
    }
    PropertyValue value() const override {
        Physics::RigidBody& body = Physics::getBodyFor(_owner);
        return _field == Field::Velocity ? PropertyValue(body.velocity)
                                         : PropertyValue(body.mass);
    }
    bool setValue(const PropertyValue& v) override {
        Physics::RigidBody& body = Physics::getBodyFor(_owner);
        if (_field == Field::Velocity) {
            const auto* vec = std::get_if<glm::vec3>(&v);
            if (!vec) return false;
            body.velocity = *vec;
            return true;
        }
        double n = 0.0;
        if (!propertyValueToNumber(v, n) || n <= 0.0) return false;   // massless
        body.mass = static_cast<float>(n);                            // is a lie
        return true;
    }

private:
    std::string _name;
    Object* _owner;
    Field _field;
};

} // namespace

// ---------------------------------------------------------------------------
// The paintable skin made legible, face by face: color, layer structure,
// opacity, blend mode. Pixel buffers and stroke history stay source-code-only
// — buffers are not slots. One bridge class covers every face field.
// ---------------------------------------------------------------------------
namespace {

class FacePropertyBridge : public Property {
public:
    enum class Field { Color, LayerCount, ActiveLayer, UseLayers, LayerOpacity, BlendMode, TextureSize };

    FacePropertyBridge(std::string name, Object* owner, int face, Field field)
        : _name(std::move(name)), _owner(owner), _face(face), _field(field) {}

    std::string name() const override { return _name; }
    std::string typeName() const override {
        switch (_field) {
            case Field::Color: return "vec3";
            case Field::UseLayers: return "bool";
            case Field::LayerOpacity: return "float";
            default: return "int";
        }
    }

    PropertyValue value() const override {
        const FaceTexture* tex = texture();
        switch (_field) {
            case Field::Color:
                if (_face < 6) {
                    return PropertyValue(glm::vec3(_owner->faceColors[_face][0],
                                                   _owner->faceColors[_face][1],
                                                   _owner->faceColors[_face][2]));
                }
                return PropertyValue(glm::vec3(1.0f));
            case Field::LayerCount:
                return PropertyValue(tex ? static_cast<int>(tex->layers.size()) : 0);
            case Field::ActiveLayer:
                return PropertyValue(tex ? tex->activeLayer : 0);
            case Field::UseLayers:
                return PropertyValue(tex ? tex->useLayers : false);
            case Field::LayerOpacity: {
                if (!tex || tex->layers.empty()) return PropertyValue(1.0f);
                const int layer = clampLayer(*tex);
                return PropertyValue(tex->layerOpacities[layer]);
            }
            case Field::BlendMode: {
                if (!tex || tex->layers.empty()) return PropertyValue(0);
                const int layer = clampLayer(*tex);
                return PropertyValue(tex->blendModes[layer]);
            }
            case Field::TextureSize:
                return PropertyValue(tex ? tex->size : 0);
        }
        return PropertyValue{};
    }

    bool setValue(const PropertyValue& v) override {
        FaceTexture* tex = texture();
        switch (_field) {
            case Field::Color: {
                const auto* c = std::get_if<glm::vec3>(&v);
                if (!c) return false;
                _owner->setFaceColor(_face, c->x, c->y, c->z);
                return true;
            }
            case Field::ActiveLayer: {
                double n = 0.0;
                if (!tex || tex->layers.empty() || !propertyValueToNumber(v, n)) return false;
                tex->activeLayer = std::max(
                    0, std::min(static_cast<int>(tex->layers.size()) - 1,
                                static_cast<int>(n)));
                return true;
            }
            case Field::UseLayers: {
                if (!tex) return false;
                if (const auto* b = std::get_if<bool>(&v)) {
                    tex->useLayers = *b;
                } else {
                    double n = 0.0;
                    if (!propertyValueToNumber(v, n)) return false;
                    tex->useLayers = n != 0.0;
                }
                recomposite(*tex);
                return true;
            }
            case Field::LayerOpacity: {
                double n = 0.0;
                if (!tex || tex->layers.empty() || !propertyValueToNumber(v, n)) return false;
                tex->setLayerOpacity(clampLayer(*tex), static_cast<float>(n));
                recomposite(*tex);
                return true;
            }
            case Field::BlendMode: {
                double n = 0.0;
                if (!tex || tex->layers.empty() || !propertyValueToNumber(v, n)) return false;
                tex->setBlendMode(clampLayer(*tex), static_cast<int>(n));
                recomposite(*tex);
                return true;
            }
            case Field::LayerCount:
            case Field::TextureSize:
                return false;   // structure is made with tools, not assigned
        }
        return false;
    }

private:
    FaceTexture* texture() const {
        if (_face < 0 || _face >= static_cast<int>(_owner->faceTextures.size())) {
            return nullptr;
        }
        return &_owner->faceTextures[static_cast<std::size_t>(_face)];
    }
    static int clampLayer(const FaceTexture& tex) {
        return std::max(0, std::min(static_cast<int>(tex.layers.size()) - 1,
                                    tex.activeLayer));
    }
    static void recomposite(const FaceTexture& tex) {
        if (tex.useLayers) tex.compositeLayers();
        tex.updateWholeGPU();
    }

    std::string _name;
    Object* _owner;
    int _face;
    Field _field;
};

} // namespace

void Object::buildProperties() {
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<Object, glm::vec3>>(
        "position", this, &Object::getPosition, &Object::setPosition));
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<Object, glm::vec3>>(
        "rotation", this, &Object::getRotationEulerDegrees, &Object::setRotationEulerDegrees));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<Object, glm::vec3>>(
        "center", this, &Object::center));

    // Shape parameters, flat under dotted names. v1 is raw read/write;
    // geometry regeneration on change follows through the setShape path
    // (LAW_AND_CREATION_SYSTEM.md, Stage 2 follow-up).
    auto addShapeParam = [this](const char* name, float ShapeParams::*member) {
        _propertyRegistry.push_back(
            std::make_unique<ShapeParamBridge>(name, this, member));
    };
    _propertyRegistry.push_back(std::make_unique<ShapeKindBridge>(this));
    addShapeParam("shape.r", &ShapeParams::r);
    addShapeParam("shape.ry", &ShapeParams::ry);
    addShapeParam("shape.rz", &ShapeParams::rz);
    addShapeParam("shape.halfH", &ShapeParams::halfH);
    addShapeParam("shape.majorR", &ShapeParams::majorR);
    addShapeParam("shape.minorR", &ShapeParams::minorR);
    addShapeParam("shape.paraboloidA", &ShapeParams::paraboloidA);
    addShapeParam("shape.ovoidAsym", &ShapeParams::ovoidAsym);
    addShapeParam("shape.fillet", &ShapeParams::fillet);

    // Whether the world's physics touches this being — governable state
    // ("make this object immaterial while the ritual runs").
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<Object, bool>>(
        "physical", this, &Object::propPhysical, &Object::propSetPhysical));
    // Motion state: the rigid body's truth, addressable — collision RESPONSE
    // becomes authorable law-text.
    _propertyRegistry.push_back(std::make_unique<RigidBodyBridge>(
        "velocity", this, RigidBodyBridge::Field::Velocity));
    _propertyRegistry.push_back(std::make_unique<RigidBodyBridge>(
        "mass", this, RigidBodyBridge::Field::Mass));
    // The object's tint (uniform across faces when written; face 0 when read).
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<Object, glm::vec3>>(
        "color", this, &Object::propColor, &Object::propSetColor));

    // The whole face-texture surface, face by face (color, layers, opacity,
    // blend) — laws can fade a face's layer or recolor one side; set-to-set
    // carries surface structure. Pixels/strokes stay source-code-only.
    const int faceCount =
        std::min<int>(static_cast<int>(faceTextures.size()), 32);
    for (int f = 0; f < faceCount; ++f) {
        const std::string base = "face." + std::to_string(f) + ".";
        auto addFace = [&](const char* leaf, FacePropertyBridge::Field field) {
            _propertyRegistry.push_back(std::make_unique<FacePropertyBridge>(
                base + leaf, this, f, field));
        };
        addFace("color", FacePropertyBridge::Field::Color);
        addFace("layerCount", FacePropertyBridge::Field::LayerCount);
        addFace("activeLayer", FacePropertyBridge::Field::ActiveLayer);
        addFace("useLayers", FacePropertyBridge::Field::UseLayers);
        addFace("layerOpacity", FacePropertyBridge::Field::LayerOpacity);
        addFace("blendMode", FacePropertyBridge::Field::BlendMode);
        addFace("textureSize", FacePropertyBridge::Field::TextureSize);
    }
}

void Object::propSetColor(const glm::vec3& c) {
    const int faces = getFaces() > 0 ? getFaces() : 6;
    for (int f = 0; f < faces; ++f) setFaceColor(f, c.x, c.y, c.z);
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
    
    // Trigger events based on hover state changes. Enter/exit also echo as
    // string-typed ECA::Events so Person-authored laws can bind to them; the
    // continuous per-frame hover does not (only discrete edges travel as
    // events — same rule as AutomationEvents.hpp).
    if (isHovering && !wasHovered) {
        // Mouse entered the object
        ObjectHoverEnterEvent event(*this, _hoverPoint, glm::vec2(0, 0)); // Screen pos would be passed in
        Core::EventBus::instance().publish(event);
        Core::EventBus::instance().publish(ECA::Event{"object-hover-enter", this, nullptr, std::time(nullptr)});
    } else if (!isHovering && wasHovered) {
        // Mouse exited the object
        ObjectHoverExitEvent event(*this, _hoverPoint, glm::vec2(0, 0)); // Screen pos would be passed in
        Core::EventBus::instance().publish(event);
        Core::EventBus::instance().publish(ECA::Event{"object-hover-exit", this, nullptr, std::time(nullptr)});
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
