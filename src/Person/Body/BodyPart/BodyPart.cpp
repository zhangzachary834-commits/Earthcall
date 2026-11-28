#include "BodyPart.hpp"
#include <algorithm>
#include <stdexcept>
#include <glm/gtc/matrix_transform.hpp>
#include "Singularity/Screen/Renderer.hpp"
#include "Singularity/OntoMath/LinearAlgebra.hpp"
#include "ConstructedBeing/Singular/Property/PropertyRef.hpp"
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"

namespace {
glm::mat4 authoredBodyPartScale(const glm::vec3& dimensions) {
    const auto value = OntoMath::affineScale(dimensions);
    if (!value) throw std::runtime_error("BodyPart OntoMath scale refusal");
    const auto lowered = value->toGlmMat4();
    if (!lowered) throw std::runtime_error("BodyPart OntoMath scale lowering refusal");
    return *lowered;
}
glm::mat4 authoredBodyPartCompose(const glm::mat4& first, const glm::mat4& second) {
    const auto value = OntoMath::affineCompose(
        OntoMath::MatrixValue::fromGlmMat4(first),
        OntoMath::MatrixValue::fromGlmMat4(second));
    if (!value) throw std::runtime_error("BodyPart OntoMath composition refusal");
    const auto lowered = value->toGlmMat4();
    if (!lowered) throw std::runtime_error("BodyPart OntoMath composition lowering refusal");
    return *lowered;
}
} // namespace

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
    // Born together with the part: custody, not a new interaction between
    // two beings that already stood apart — so no relation-formed echo.
    constitute(_primaryObject.get(), /*newInteraction=*/false);
    
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
    r.pushModel(authoredBodyPartScale(_dimensions));
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

void BodyPart::setTransform(const glm::mat4& t) {
    _primaryObject->setTransform(t);
    _transform = t;
    static const glm::mat4 I(1.0f);
    if (_localTransform == I) {
        _localTransform = t;
    }

    glm::mat4 scaled = authoredBodyPartCompose(t, authoredBodyPartScale(_dimensions));
    _primaryObject->updateCollisionZone(scaled);

    // Propagate world transform to every sub-object so raycasting,
    // collision zones, and tool targeting all use correct positions.
    for (size_t i = 0; i < _subObjects.size(); ++i) {
        if (!_subObjects[i]) continue;
        glm::mat4 worldT = authoredBodyPartCompose(t, _subObjectLocalOffsets[i]);
        _subObjects[i]->setTransform(worldT);
    }
}



glm::mat4 BodyPart::scaledTransform() const {
    return authoredBodyPartCompose(_transform, authoredBodyPartScale(_dimensions));
}

// -----------------------------------------------------------------
// Shape management
// -----------------------------------------------------------------
void BodyPart::setPrimaryShape(ObjectTypes::ShapeKind gt) {
    _primaryObject->setShape(gt);
    
    // Through setFaceColor, which reinitialises this part's own material's
    // face textures to the new shape's face count and paints them — the
    // reinit that setShapeKind used to do while paint still lived on Object.
    for (int f = 0; f < 6; ++f) _primaryObject->setFaceColor(f, color[0], color[1], color[2]);
}

void BodyPart::setColor(float r, float g, float b) {
    color[0] = r; color[1] = g; color[2] = b;
    // Paint, not just remember. Before 2026-10-13 this wrote only `color`
    // (the declared updateColor() was never defined), so the Character
    // Console colour picker changed nothing visible until a shape change.
    if (_primaryObject) {
        for (int f = 0; f < 6; ++f) _primaryObject->setFaceColor(f, r, g, b);
    }
}

void BodyPart::setDimensions(const glm::vec3& dims) {
    _dimensions = dims;
    // Re-measure the collision zone, which is composed with the scale.
    setTransform(_transform);
}

// -----------------------------------------------------------------
// Composite sub-object management
// -----------------------------------------------------------------
Object* BodyPart::addSubObject(ObjectTypes::ShapeKind kind, const glm::mat4& localOffset) {
    std::string subId = getIdentifier() + ".sub-" + std::to_string(_subObjects.size());
    auto obj = std::make_unique<Object>(subId);
    obj->setShape(kind);

    glm::mat4 worldT = authoredBodyPartCompose(_transform, localOffset);
    obj->setTransform(worldT);
    for (int f = 0; f < 6; ++f) obj->setFaceColor(f, color[0], color[1], color[2]);
    Object* raw = obj.get();
    _subObjects.push_back(std::move(obj));
    _subObjectLocalOffsets.push_back(localOffset);
    addMember(raw);
    constitute(raw, /*newInteraction=*/true);
    return raw;
}

Object* BodyPart::addSubObject(std::unique_ptr<Object> obj, const glm::mat4& localOffset) {
    if (!obj) return nullptr;
    glm::mat4 worldT = authoredBodyPartCompose(_transform, localOffset);
    obj->setTransform(worldT);
    Object* raw = obj.get();
    _subObjects.push_back(std::move(obj));
    _subObjectLocalOffsets.push_back(localOffset);
    addMember(raw);
    constitute(raw, /*newInteraction=*/true);
    return raw;
}

void BodyPart::removeSubObject(size_t index) {
    if (index >= _subObjects.size()) return;
    Object* ptr = _subObjects[index].get();
    dissolveConstitution(ptr);
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
        glm::mat4 worldT = authoredBodyPartCompose(_transform, localOffset);
        _subObjects[index]->setTransform(worldT);
    }
}

// -----------------------------------------------------------------
// Constitution — BodyPart <-constituted-by-> Object (Zach, 2026-10-13)
// -----------------------------------------------------------------
// The edges live in this Formation's own RelationManager, entered with
// relations().add/retain directly rather than addRelation(): addRelation
// integrates topology, which would try to admit `this` as its own member
// (refused, loudly — a Formation is not its own ground). The engine exposes
// the Person's constitution edges to Law through Universe's relation
// providers (EngineInit.cpp), so `Related(constituted-by, ...)` can walk
// from a clicked Object to the part it manifests.
//
// The unique_ptrs remain beneath this graph as machine custody: they decide
// lifetime, never meaning. Authority over the part does NOT follow from
// authority over its Object — that gate is TransferPolicy's, still open
// (see the task file).
void BodyPart::constitute(Object* obj, bool newInteraction) {
    if (!obj) return;
    auto edge = std::make_shared<Relation>(kConstitutedByType, *this, *obj, /*directed=*/true,
                                          /*initialWeight=*/1.0f);  // whole, not graded
    if (newInteraction) relations().add(edge);
    else relations().retain(edge);
}

void BodyPart::dissolveConstitution(Object* obj) {
    if (!obj) return;
    // Proved impossible, not stale: the Object is leaving this part's
    // custody right now, so the sub-Relation dissolves
    // (PRIMARY_AND_SUB_RELATIONS.md §2).
    relations().removeBetween(*this, *obj, kConstitutedByType);
}

Relation* BodyPart::constitutionOf(const Object* obj) const {
    if (!obj) return nullptr;
    for (const auto& rel : relations().getAll()) {
        if (rel && rel->type == kConstitutedByType && rel->a() == this && rel->b() == obj) {
            return rel.get();
        }
    }
    return nullptr;
}

std::string BodyPart::propPrimaryObject() const {
    return _primaryObject ? _primaryObject->getIdentifier() : std::string{};
}

std::shared_ptr<PropertyList> BodyPart::propConstitutedBy() const {
    auto list = std::make_shared<PropertyList>();
    for (const auto& rel : relations().getAll()) {
        if (rel && rel->type == kConstitutedByType && rel->a() == this) {
            list->elements.emplace_back(rel->bId());
        }
    }
    return list;
}

std::shared_ptr<PropertyList> BodyPart::propSubObjectLocalOffsets() const {
    auto list = std::make_shared<PropertyList>();
    for (const glm::mat4& offset : _subObjectLocalOffsets) list->elements.emplace_back(offset);
    return list;
}

void BodyPart::buildProperties() {
    Formation::buildProperties();
    // Identity, not predicate: the part's name IS its stable identifier, and
    // its type mirrors the concrete C++ member (Hand, Foot) that cannot
    // change at runtime — a write would make the path lie. Read-only.
    registerProperty(std::make_unique<ComputedProperty<BodyPart, std::string>>(
        "partName", this, &BodyPart::propPartName, nullptr));
    registerProperty(std::make_unique<ComputedProperty<BodyPart, int>>(
        "partType", this, &BodyPart::propPartType, nullptr));
    registerProperty(std::make_unique<PropertyRef<BodyPart, bool>>(
        "isLiteral", this, &BodyPart::isLiteral));
    registerProperty(std::make_unique<PropertyRef<BodyPart, bool>>(
        "isSymbolic", this, &BodyPart::isSymbolic));
    registerProperty(std::make_unique<ComputedProperty<BodyPart, glm::vec3>>(
        "dimensions", this, &BodyPart::propDimensions, &BodyPart::setDimensions));
    registerProperty(std::make_unique<ComputedProperty<BodyPart, glm::vec3>>(
        "color", this, &BodyPart::propColor, &BodyPart::propSetColor));
    registerProperty(std::make_unique<ComputedProperty<BodyPart, glm::mat4>>(
        "localTransform", this, &BodyPart::propLocalTransform, &BodyPart::setLocalTransform));
    // Derived each frame from the Body's pose; a write would be overwritten.
    registerProperty(std::make_unique<ComputedProperty<BodyPart, glm::mat4>>(
        "transform", this, &BodyPart::getTransform, nullptr));
    registerProperty(std::make_unique<ComputedProperty<BodyPart, glm::mat4>>(
        "scaledTransform", this, &BodyPart::scaledTransform, nullptr));
    // Derived from the constitution graph; change it by adding/removing
    // manifestations, not by renaming an edge.
    registerProperty(std::make_unique<ComputedProperty<BodyPart, std::string>>(
        "primaryObject", this, &BodyPart::propPrimaryObject, nullptr));
    registerProperty(std::make_unique<ComputedProperty<BodyPart, std::shared_ptr<PropertyList>>>(
        "constitutedBy", this, &BodyPart::propConstitutedBy, nullptr));
    // Readable now; writes still go through setSubObjectLocalOffset. The
    // offset is a predicate of the constitution edge, not of either being —
    // moving it onto the Relation is the open next rung (task file).
    registerProperty(std::make_unique<ComputedProperty<BodyPart, std::shared_ptr<PropertyList>>>(
        "subObjectLocalOffsets", this, &BodyPart::propSubObjectLocalOffsets, nullptr));
}
