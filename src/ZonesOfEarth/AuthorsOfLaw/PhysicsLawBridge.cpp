#include "PhysicsLawBridge.hpp"

#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"

PhysicsLawBridge::PhysicsLawBridge(const std::string& physicsLawName)
    : Law("physics: " + physicsLawName), _physicsName(physicsLawName) {}

Physics::PhysicsLaw* PhysicsLawBridge::mutableTarget() const {
    for (const auto& law : Physics::getLaws()) {
        if (law.name == _physicsName) return Physics::getLawById(law.id);
    }
    return nullptr;   // the engine law is gone: reads are inert, writes refuse
}

bool PhysicsLawBridge::propEnabled() const {
    const auto* law = mutableTarget();
    return law && law->enabled;
}
void PhysicsLawBridge::propSetEnabled(const bool& v) {
    if (auto* law = mutableTarget()) law->enabled = v;
}
float PhysicsLawBridge::propStrength() const {
    const auto* law = mutableTarget();
    return law ? law->strength : 0.0f;
}
void PhysicsLawBridge::propSetStrength(const float& v) {
    if (auto* law = mutableTarget()) law->strength = v;
}
float PhysicsLawBridge::propDamping() const {
    const auto* law = mutableTarget();
    return law ? law->damping : 0.0f;
}
void PhysicsLawBridge::propSetDamping(const float& v) {
    if (auto* law = mutableTarget()) law->damping = v;
}
glm::vec3 PhysicsLawBridge::propDirection() const {
    const auto* law = mutableTarget();
    return law ? law->direction : glm::vec3(0.0f);
}
void PhysicsLawBridge::propSetDirection(const glm::vec3& v) {
    if (auto* law = mutableTarget()) law->direction = v;
}

void PhysicsLawBridge::buildProperties() {
    registerProperty(std::make_unique<ComputedProperty<PhysicsLawBridge, bool>>(
        "enabled", this, &PhysicsLawBridge::propEnabled, &PhysicsLawBridge::propSetEnabled));
    registerProperty(std::make_unique<ComputedProperty<PhysicsLawBridge, float>>(
        "strength", this, &PhysicsLawBridge::propStrength,
        &PhysicsLawBridge::propSetStrength));
    registerProperty(std::make_unique<ComputedProperty<PhysicsLawBridge, float>>(
        "damping", this, &PhysicsLawBridge::propDamping,
        &PhysicsLawBridge::propSetDamping));
    registerProperty(std::make_unique<ComputedProperty<PhysicsLawBridge, glm::vec3>>(
        "direction", this, &PhysicsLawBridge::propDirection,
        &PhysicsLawBridge::propSetDirection));
}

void PhysicsLawBridge::syncRegister(LawManager& laws) {
    if (!Physics::getLegacyEngineEnabled()) return;
    for (const auto& physicsLaw : Physics::getLaws()) {
        bool bridged = false;
        for (const auto& law : laws.getAll()) {
            auto* bridge = dynamic_cast<PhysicsLawBridge*>(law.get());
            if (bridge && bridge->physicsLawName() == physicsLaw.name) {
                bridged = true;
                break;
            }
        }
        if (!bridged) {
            laws.add(std::make_shared<PhysicsLawBridge>(physicsLaw.name));
        }
    }
}
