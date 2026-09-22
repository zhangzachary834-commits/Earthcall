#pragma once

#include "Law.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"

// First movers retired INTO the register: each engine physics law (gravity,
// air resistance, gravity field, ...) appears as a legible Law whose
// properties read and write the physics engine itself. The integrator keeps
// doing the work — what changes is GOVERNANCE: "@<bridge-id>.strength := 3"
// or ".enabled := false" is ordinary law-text, so metalaws govern gravity
// like anything else, and the old hard-coded physics stops being a world
// the law system cannot see.
//
// Bridges resolve their physics law BY NAME (ids are reassigned on world
// load); a physics law that no longer exists reads as disabled/zero and
// refuses writes. Bridges are runtime beings: LawManager::toJson skips them
// (the physics laws persist in their own save section) and loadFromJson
// preserves them.
class PhysicsLawBridge : public Law {
public:
    explicit PhysicsLawBridge(const std::string& physicsLawName);

    const std::string& physicsLawName() const { return _physicsName; }
    bool isFirstMover() const override { return true; }

    // Ensure every engine physics law has exactly one bridge in the
    // register (called once per frame — new physics laws get bridged the
    // frame they appear).
    static void syncRegister(LawManager& laws);

private:
    void buildProperties() override;
    Physics::PhysicsLaw* mutableTarget() const;

    bool propEnabled() const;
    void propSetEnabled(const bool& v);
    float propStrength() const;
    void propSetStrength(const float& v);
    float propDamping() const;
    void propSetDamping(const float& v);
    glm::vec3 propDirection() const;
    void propSetDirection(const glm::vec3& v);

    std::string _physicsName;
};
