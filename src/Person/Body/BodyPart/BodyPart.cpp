#include "BodyPart.hpp"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include "Rendering/Renderer.hpp"

BodyPart::BodyPart(const std::string& name, Type type, const Form& form)
    : Object(), Formation(), partName(name), partType(type), geometry(form)
{
    isLiteral = true;
    isSymbolic = false;
    _localTransform = glm::mat4(1.0f);
    
    switch (form.getShape()) {
        case Form::ShapeType::Cube:
            setGeometryType(GeometryType::Cube);
            break;
        case Form::ShapeType::Sphere:
            setGeometryType(GeometryType::Sphere);
            break;
        default:
            setGeometryType(GeometryType::Cube);
            break;
    }
    
    initFaceTextures();
    
    for (size_t f = 0; f < faceTextures.size(); ++f) {
        fillFaceColor(static_cast<int>(f), 0.96f, 0.80f, 0.69f);
    }
    
    setTransform(_localTransform);
    

}

BodyPart::BodyPart(const std::string& name, Type type, const Form& form, const glm::mat4& initialTransform)
    : BodyPart(name, type, form) {
    _localTransform = initialTransform;
    setTransform(initialTransform);
}

void BodyPart::draw() const {
    Renderer& r = currentRenderer();

    // Draw primary shape under body part's world transform
    glm::vec3 dims = geometry.getDimensions();
    r.pushModel(transform);
    r.pushModel(glm::scale(glm::mat4(1.0f), dims));
    drawObject();
    drawHighlightOutline();
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

void BodyPart::update(float deltaTime) {
    
}

void BodyPart::setTransform(const glm::mat4& t) {
    Object::setTransform(t);
    static const glm::mat4 I(1.0f);
    if (_localTransform == I) {
        _localTransform = t;
    }

    glm::mat4 scaled = t * glm::scale(glm::mat4(1.0f), geometry.getDimensions());
    updateCollisionZone(scaled);

    // Propagate world transform to every sub-object so raycasting,
    // collision zones, and tool targeting all use correct positions.
    for (size_t i = 0; i < _subObjects.size(); ++i) {
        if (!_subObjects[i]) continue;
        glm::mat4 worldT = t * _subObjectLocalOffsets[i];
        _subObjects[i]->setTransform(worldT);
    }
}

glm::mat4 BodyPart::getRaycastTransform() const {
    return getTransform() * glm::scale(glm::mat4(1.0f), geometry.getDimensions());
}



// -----------------------------------------------------------------
// Shape management
// -----------------------------------------------------------------
void BodyPart::setPrimaryShape(Object::GeometryType gt) {
    setGeometryType(gt);   // reinitialises faceTextures via Object
    for (size_t f = 0; f < faceTextures.size(); ++f) {
        fillFaceColor(static_cast<int>(f), color[0], color[1], color[2]);
    }
}

bool BodyPart::hasCustomTextures() const {
    // A texture is "custom" if any pixel differs from a flat fill.
    // Quick heuristic: check a few sample pixels against the first pixel.
    for (const auto& ft : faceTextures) {
        if (ft.pixels.size() < 4) continue;
        uint32_t first = *reinterpret_cast<const uint32_t*>(ft.pixels.data());
        size_t totalPixels = ft.pixels.size() / 4;
        for (size_t p = 1; p < totalPixels; p += std::max<size_t>(1, totalPixels / 64)) {
            uint32_t sample = reinterpret_cast<const uint32_t*>(ft.pixels.data())[p];
            if (sample != first) return true;
        }
    }
    return false;
}

// -----------------------------------------------------------------
// Composite sub-object management
// -----------------------------------------------------------------
Object* BodyPart::addSubObject(Object::ShapeKind kind, const glm::mat4& localOffset) {
    auto obj = std::make_unique<Object>();
    obj->setShape(kind);

    obj->setOwnerBodyPart(this);
    glm::mat4 worldT = getTransform() * localOffset;
    obj->setTransform(worldT);
    for (size_t f = 0; f < obj->faceTextures.size(); ++f) {
        obj->fillFaceColor(static_cast<int>(f), color[0], color[1], color[2]);
    }
    Object* raw = obj.get();
    _subObjects.push_back(std::move(obj));
    _subObjectLocalOffsets.push_back(localOffset);
    addMember(raw);
    return raw;
}

Object* BodyPart::addSubObject(std::unique_ptr<Object> obj, const glm::mat4& localOffset) {
    if (!obj) return nullptr;
    obj->setOwnerBodyPart(this);
    glm::mat4 worldT = getTransform() * localOffset;
    obj->setTransform(worldT);
    Object* raw = obj.get();
    _subObjects.push_back(std::move(obj));
    _subObjectLocalOffsets.push_back(localOffset);
    addMember(raw);
    return raw;
}

Object* BodyPart::addSubObject(Object::GeometryType gt, const glm::mat4& localOffset) {
    auto obj = std::make_unique<Object>();
    obj->setGeometryType(gt);

    // Set the back-pointer so tools can find the owning body part
    obj->setOwnerBodyPart(this);

    // Compute and set the world-space transform immediately
    glm::mat4 worldT = getTransform() * localOffset;
    obj->setTransform(worldT);

    for (size_t f = 0; f < obj->faceTextures.size(); ++f) {
        obj->fillFaceColor(static_cast<int>(f), color[0], color[1], color[2]);
    }

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
    result.push_back(this);
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
        glm::mat4 worldT = getTransform() * localOffset;
        _subObjects[index]->setTransform(worldT);
    }
}
