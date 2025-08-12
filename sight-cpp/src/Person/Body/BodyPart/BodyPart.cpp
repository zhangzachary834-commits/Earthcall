#include "BodyPart.hpp"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

BodyPart::BodyPart(const std::string& name, Type type, const Form& form)
    : Object(), Formations(Form::ShapeType::Cube, {1.0f, 1.0f, 1.0f}), partName(name), partType(type), geometry(form)
{
    // By default, literal parts are physical
    isLiteral = true;
    isSymbolic = false;
    _localTransform = glm::mat4(1.0f);
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
    updateColor();
}

BodyPart::BodyPart(const std::string& name, Type type, const Form& form, const glm::mat4& initialTransform)
    : BodyPart(name, type, form) {
    _localTransform = initialTransform;
    setTransform(initialTransform);
}

void BodyPart::draw() const {
    // Apply this part's transform then draw its geometry
    glPushMatrix();
    glMultMatrixf(&transform[0][0]);
    glColor3f(color[0],color[1],color[2]);
    geometry.draw();
    glPopMatrix();
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
    // Store base transform
    Object::setTransform(t);
    static const glm::mat4 I(1.0f);
    if (_localTransform == I) {
        _localTransform = t;
    }

    // Build a matrix that includes the geometry scale (since updateCollisionZone assumes unit cube)
    glm::mat4 scaled = t * glm::scale(glm::mat4(1.0f), geometry.getDimensions());
    updateCollisionZone(scaled);
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