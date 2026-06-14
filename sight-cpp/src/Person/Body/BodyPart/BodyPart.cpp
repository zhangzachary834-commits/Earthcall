#include "BodyPart.hpp"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

BodyPart::BodyPart(const std::string& name, Type type, const Form& form)
    : Object(), Formation(Form::ShapeType::Cube, {1.0f, 1.0f, 1.0f}), partName(name), partType(type), geometry(form)
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
    
    // Set default health based on part type
    switch (type) {
        case Type::Head:
            maxHealth = 50.0f;
            sensitivity = 2.0f;  // Head is very sensitive
            break;
        case Type::Torso:
            maxHealth = 100.0f;
            sensitivity = 1.5f;
            break;
        case Type::Arm:
        case Type::Leg:
            maxHealth = 75.0f;
            sensitivity = 1.2f;
            break;
        case Type::Hand:
        case Type::Foot:
            maxHealth = 25.0f;
            sensitivity = 1.8f;
            break;
        case Type::Organ:
            maxHealth = 30.0f;
            sensitivity = 3.0f;  // Organs are very sensitive
            break;
        default:
            maxHealth = 50.0f;
            sensitivity = 1.0f;
            break;
    }
    
    health = maxHealth;
    // updateColor();  // Commented out - let texture colors take precedence
}

BodyPart::BodyPart(const std::string& name, Type type, const Form& form, const glm::mat4& initialTransform)
    : BodyPart(name, type, form) {
    _localTransform = initialTransform;
    setTransform(initialTransform);
}

void BodyPart::draw() const {
    // Draw primary shape under body part's world transform
    glPushMatrix();
    glMultMatrixf(&transform[0][0]);
    glm::vec3 dims = geometry.getDimensions();
    glPushMatrix();
    glScalef(dims.x, dims.y, dims.z);
    drawObject();
    drawHighlightOutline();
    glPopMatrix();
    glPopMatrix();

    // Sub-objects already carry world transforms (set by setTransform),
    // so draw them independently — no nesting under the body part matrix.
    for (const auto& sub : _subObjects) {
        if (!sub) continue;
        glPushMatrix();
        glMultMatrixf(&sub->getTransform()[0][0]);
        sub->drawObject();
        sub->drawHighlightOutline();
        glPopMatrix();
    }
}

void BodyPart::update(float deltaTime) {
    // Natural regeneration
    if (canRegenerate() && health < maxHealth) {
        heal(regenerationRate * deltaTime);
    }
    
    // Update effects
    // (Could add effect processing here)
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

void BodyPart::setHealth(float h) {
    health = std::clamp(h, 0.0f, maxHealth);
    updateColor();
    
    // Update functionality based on health
    if (health <= 0.0f) {
        isFunctional = false;
    } else if (health >= maxHealth * 0.5f) {
        isFunctional = true;
    }
}

void BodyPart::takeDamage(float damage) {
    if (damage <= 0.0f) return;
    
    // Apply sensitivity multiplier
    float actualDamage = damage * sensitivity;
    health -= actualDamage;
    health = std::max(0.0f, health);
    
    updateColor();
    
    // Update functionality
    if (health <= 0.0f) {
        isFunctional = false;
    } else if (health < maxHealth * 0.5f) {
        isFunctional = false;  // Part becomes non-functional when heavily damaged
    }
}

void BodyPart::heal(float amount) {
    if (amount <= 0.0f) return;
    
    health += amount;
    health = std::min(health, maxHealth);
    
    updateColor();
    
    // Restore functionality if healed enough
    if (health >= maxHealth * 0.5f) {
        isFunctional = true;
    }
}

BodyPart::HealthState BodyPart::getHealthState() const {
    float healthPercent = health / maxHealth;
    
    if (healthPercent <= 0.0f) {
        return HealthState::Missing;
    } else if (healthPercent <= 0.25f) {
        return HealthState::Broken;
    } else if (healthPercent <= 0.5f) {
        return HealthState::Injured;
    } else if (healthPercent <= 0.75f) {
        return HealthState::Bruised;
    } else {
        return HealthState::Healthy;
    }
}

bool BodyPart::isDamaged() const {
    return health < maxHealth;
}

bool BodyPart::isBroken() const {
    return health <= maxHealth * 0.25f;
}

bool BodyPart::isMissing() const {
    return health <= 0.0f;
}

float BodyPart::getFunctionality() const {
    if (!isFunctional) return 0.0f;
    
    float healthPercent = health / maxHealth;
    if (healthPercent >= 0.8f) {
        return 1.0f;  // Full functionality
    } else if (healthPercent >= 0.5f) {
        return 0.5f;  // Reduced functionality
    } else {
        return 0.0f;  // No functionality
    }
}

float BodyPart::getPainLevel() const {
    if (health >= maxHealth) return 0.0f;
    
    float damagePercent = 1.0f - (health / maxHealth);
    return damagePercent * sensitivity;
}

bool BodyPart::canRegenerate() const {
    // Check if any effects prevent regeneration
    if (hasEffect("NoRegeneration") || hasEffect("Cursed")) {
        return false;
    }
    
    // Only regenerate if not missing and has regeneration rate
    return health > 0.0f && regenerationRate > 0.0f;
}

void BodyPart::addEffect(const std::string& effect) {
    if (!hasEffect(effect)) {
        effects.push_back(effect);
    }
}

void BodyPart::removeEffect(const std::string& effect) {
    auto it = std::find(effects.begin(), effects.end(), effect);
    if (it != effects.end()) {
        effects.erase(it);
    }
}

bool BodyPart::hasEffect(const std::string& effect) const {
    return std::find(effects.begin(), effects.end(), effect) != effects.end();
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

void BodyPart::updateColor() {
    float healthPercent = health / maxHealth;
    
    if (healthPercent >= 0.8f) {
        // Healthy - normal color
        color[0] = 1.0f;  // Red
        color[1] = 1.0f;  // Green
        color[2] = 1.0f;  // Blue
    } else if (healthPercent >= 0.5f) {
        // Bruised - slight red tint
        color[0] = 1.0f;
        color[1] = 0.8f;
        color[2] = 0.8f;
    } else if (healthPercent >= 0.25f) {
        // Injured - more red
        color[0] = 1.0f;
        color[1] = 0.5f;
        color[2] = 0.5f;
    } else if (healthPercent > 0.0f) {
        // Broken - very red
        color[0] = 1.0f;
        color[1] = 0.2f;
        color[2] = 0.2f;
    } else {
        // Missing - dark gray
        color[0] = 0.3f;
        color[1] = 0.3f;
        color[2] = 0.3f;
    }
    
    // Apply special effect colors
    if (hasEffect("Burning")) {
        color[0] = 1.0f;
        color[1] = 0.5f;
        color[2] = 0.0f;  // Orange
    } else if (hasEffect("Frozen")) {
        color[0] = 0.5f;
        color[1] = 0.8f;
        color[2] = 1.0f;  // Light blue
    } else if (hasEffect("Poisoned")) {
        color[0] = 0.5f;
        color[1] = 1.0f;
        color[2] = 0.5f;  // Green
    }
}