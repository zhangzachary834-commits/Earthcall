#include "BodyPart.hpp"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include "Singularity/Screen/Renderer.hpp"

BodyPart::BodyPart(const std::string& name, Type type, 
                   ObjectTypes::ShapeKind geometryType, const glm::vec3& dimensions)
    : Formation(), partName(name), partType(type), _dimensions(dimensions)
{
    isLiteral = true;
    isSymbolic = false;
    _localTransform = glm::mat4(1.0f);
    
    _primaryObject = std::make_unique<Object>(name.empty() ? "" : "bodypart." + name);
    _primaryObject->setShape(geometryType);
    addMember(_primaryObject.get());
    
    // initFaceTextures();
    
    // for (size_t f = 0; f < faceTextures.size(); ++f) {
    //     fillFaceColor(static_cast<int>(f), 0.96f, 0.80f, 0.69f);
    // }
    
    setTransform(_localTransform);
    

}

BodyPart::BodyPart(const std::string& name, Type type, 
                   ObjectTypes::ShapeKind geometryType, const glm::vec3& dimensions,
                   const glm::mat4& initialTransform)
    : BodyPart(name, type, geometryType, dimensions) {
    _localTransform = initialTransform;
    setTransform(initialTransform);
}

void BodyPart::draw() const {
    Renderer& r = currentRenderer();

    // Draw primary shape under body part's world transform
    r.pushModel(_transform);
    r.pushModel(glm::scale(glm::mat4(1.0f), _dimensions));
    _primaryObject->drawObject();
    _primaryObject->drawHighlightOutline();
    r.popModel();
    r.popModel();

    // Sub-objects already carry world transforms (set by setTransform),
    // so draw them independently — no nesting under the body part matrix.
    for (const auto& sub : _subObjects) {
        if (!sub) continue;
        r.pushModel(sub->getTransform());
        sub->drawObject();
        sub->drawHighlightOutline();
        r.popModel();
    }
}

void BodyPart::update(float /*deltaTime*/) {
    
}

void BodyPart::setTransform(const glm::mat4& t) {
    _primaryObject->setTransform(t);
    _transform = t;
    static const glm::mat4 I(1.0f);
    if (_localTransform == I) {
        _localTransform = t;
    }

    glm::mat4 scaled = t * glm::scale(glm::mat4(1.0f), _dimensions);
    _primaryObject->updateCollisionZone(scaled);

    // Propagate world transform to every sub-object so raycasting,
    // collision zones, and tool targeting all use correct positions.
    for (size_t i = 0; i < _subObjects.size(); ++i) {
        if (!_subObjects[i]) continue;
        glm::mat4 worldT = t * _subObjectLocalOffsets[i];
        _subObjects[i]->setTransform(worldT);
    }
}

glm::mat4 BodyPart::getRaycastTransform() const {
    return _transform * glm::scale(glm::mat4(1.0f), _dimensions);
}



// -----------------------------------------------------------------
// Shape management
// -----------------------------------------------------------------
void BodyPart::setPrimaryShape(ObjectTypes::ShapeKind gt) {
    _primaryObject->setShape(gt);
    
    // Through setFaceColor, which reinitialises this part's own material's
    // face textures to the new shape's face count and paints them — the
    // reinit that setShapeKind used to do while paint still lived on Object.
    for (int f = 0; f < 6; ++f) setFaceColor(f, color[0], color[1], color[2]);
}

bool BodyPart::hasCustomTextures() const {
    return false;
}

// -----------------------------------------------------------------
// Composite sub-object management
// -----------------------------------------------------------------
Object* BodyPart::addSubObject(ObjectTypes::ShapeKind kind, const glm::mat4& localOffset) {
    std::string subId = getIdentifier() + ".sub-" + std::to_string(_subObjects.size());
    auto obj = std::make_unique<Object>(subId);
    obj->setShape(kind);

    glm::mat4 worldT = _transform * localOffset;
    obj->setTransform(worldT);
    for (int f = 0; f < 6; ++f) obj->setFaceColor(f, color[0], color[1], color[2]);
    Object* raw = obj.get();
    _subObjects.push_back(std::move(obj));
    _subObjectLocalOffsets.push_back(localOffset);
    addMember(raw);
    return raw;
}

Object* BodyPart::addSubObject(std::unique_ptr<Object> obj, const glm::mat4& localOffset) {
    if (!obj) return nullptr;
    glm::mat4 worldT = _transform * localOffset;
    obj->setTransform(worldT);
    Object* raw = obj.get();
    _subObjects.push_back(std::move(obj));
    _subObjectLocalOffsets.push_back(localOffset);
    addMember(raw);
    return raw;
}

void BodyPart::removeSubObject(size_t index) {
    if (index >= _subObjects.size()) return;
    Object* ptr = _subObjects[index].get();
    removeMember(ptr);
    _subObjects.erase(_subObjects.begin() + static_cast<long>(index));
    _subObjectLocalOffsets.erase(_subObjectLocalOffsets.begin() + static_cast<long>(index));
}

Object* BodyPart::getSubObject(size_t index) {
    return index < _subObjects.size() ? _subObjects[index].get() : nullptr;
}

const Object* BodyPart::getSubObject(size_t index) const {
    return index < _subObjects.size() ? _subObjects[index].get() : nullptr;
}

std::vector<Object*> BodyPart::getAllObjects() {
    std::vector<Object*> result;
    if (_primaryObject) result.push_back(_primaryObject.get());
    for (auto& sub : _subObjects) {
        if (sub) result.push_back(sub.get());
    }
    return result;
}

const glm::mat4& BodyPart::getSubObjectLocalOffset(size_t index) const {
    static const glm::mat4 identity(1.0f);
    return index < _subObjectLocalOffsets.size() ? _subObjectLocalOffsets[index] : identity;
}

void BodyPart::setSubObjectLocalOffset(size_t index, const glm::mat4& localOffset) {
    if (index >= _subObjectLocalOffsets.size()) return;
    _subObjectLocalOffsets[index] = localOffset;
    // Recompute world transform for this sub-object
    if (_subObjects[index]) {
        glm::mat4 worldT = _transform * localOffset;
        _subObjects[index]->setTransform(worldT);
    }
}

void BodyPart::setFaceColor(int faceIndex, float r, float g, float b) {
    if (_primaryObject) _primaryObject->setFaceColor(faceIndex, r, g, b);
}

ObjectTypes::ShapeKind BodyPart::getPrimaryShape() const {
    return _primaryObject ? _primaryObject->getShapeKind() : ObjectTypes::ShapeKind::Cube;
}
