#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Relation/RelationManager.hpp"
#include "ConstructedBeing/Singular/Singular.hpp"
#include <string>
#include <functional>

class LawManager;

// Zach: This should be moved to First Movers instead of being here because Physics is NOT a first-order property of Zones
// Its a first mover developer tool to test Laws in because default Physics provides a familiar and stable interface to work in
// and real physics should ultimately become purely runtime-created Law Formations.
namespace Physics {
    // Legacy engine toggle
    bool getLegacyEngineEnabled();
    void setLegacyEngineEnabled(bool enabled);


    // Applies gravity and basic physics integration (velocity & air resistance)
    // deltaTime       - elapsed time since last update (seconds)
    // groundY         - ground plane height (default 0)
    // gravityAccel    - gravitational acceleration magnitude (m/s^2)
    // airResistance   - simple linear drag coefficient (0-1, per second)
    void applyGravity(glm::vec3& position,
                      float deltaTime,
                      float groundY       = 0.0f,
                      float gravityAccel  = 9.81f,
                      float airResistance = 0.1f);

    // Enforces collisions between a point (e.g., camera/player) and all objects' collision zones
    void enforceCollisions(glm::vec3& position, const std::vector<std::shared_ptr<Object>>& objects);

    // --- Flight state helpers ---
    void setFlying(bool enabled);
    void toggleFlying();
    bool getFlying();

    // Basic force representation (direction normalized, magnitude in Newtons)
    struct Force {
        glm::vec3 direction{0.0f};
        float magnitude{0.0f};
    };

    // Simple rigid form used for spatial entities
    struct RigidForm {
        float mass{1.0f};                       // kilograms
        glm::vec3 velocity{0.0f};               // metres per second
        glm::vec3 accumulatedForce{0.0f};       // Newtons, reset each step

        // Rotational kinematics & dynamics
        glm::vec3 angularVelocity{0.0f};        // degrees per second
        glm::vec3 accumulatedTorque{0.0f};      // Newton-metres, reset each step
        glm::vec3 centerOfMassOffset{0.0f};     // local offset from object origin
        float momentOfInertia{0.1f};            // kg * m^2 rotational inertia
    };

    // Accumulate an external torque on the form (adds to this frame only)
    void applyTorque(RigidForm& form, const glm::vec3& torque);
    void clearTorque(RigidForm& form);

    // Accumulate an external force on the form (adds to this frame only)
    void applyForce(RigidForm& form, const glm::vec3& force);

    // Clears the force accumulator
    void clearForces(RigidForm& form);

    // Integrate motion via semi-implicit Euler and handle ground collision
    void integrate(RigidForm& form,
                   glm::vec3& position,
                   float      deltaTime,
                   float      airResistance = 0.1f,
                   float      groundY       = 0.0f);

    // Energy helpers
    double kineticEnergy(const RigidForm& form);
    double potentialEnergy(const RigidForm& form, float height, float gravityAccel = 9.81f);

    // --------------------------------------------------------------
    // RigidForm registry for world Objects
    // --------------------------------------------------------------
    // Create (if absent) and retrieve the RigidForm associated with the Object
    RigidForm& getFormFor(Object* obj, float defaultMass = 1.0f);

    // -----------------------------------------------------------------
    // Global registries maintenance (used during scene load/reset)
    // -----------------------------------------------------------------
    // Clear all registered rigid bodies (positions remain on Objects; velocities reset)
    void resetRigidBodies();
    // Remove all bonds
    void clearBonds();

    // --------------------------------------------------------------
    // Bond system – simple spring constraints between pairs of objects
    // --------------------------------------------------------------
    struct Bond {
        Object* a{nullptr};
        Object* b{nullptr};
        float restLength{1.0f};   // desired separation
        float strength{10.0f};    // spring constant (N/m)
    };

    void addBond(Object* a, Object* b, float restLength = 1.0f, float strength = 10.0f);
    void removeBond(Object* a, Object* b);

    // Access existing bonds (read-only)
    const std::vector<Bond>& getBonds();

    // Modify parameters of an existing bond; returns true if found
    bool setBondParams(Object* a, Object* b, float restLength, float strength);

    // Apply bond forces and integrate all registered object bodies
    void updateBodies(std::vector<std::shared_ptr<Object>>& objects,
                      float deltaTime,
                      float gravityAccel  = 0.0f,
                      float airResistance = 0.1f,
                      float groundY       = 0.0f);

    // -----------------------------------------------------------------
    // Relational physics registry
    // -----------------------------------------------------------------
    // Global collection of physics relations (gravity, collisions, etc.)
    RelationManager& registry();

    // Record that gravity is acting between an object and the environment
    void recordGravity(const Singular& obj, const Singular& env, float strength = 1.0f);

    /* TODO: Integrate this into our Event Bus-Handler system. */
    // Record an object-object collision relation
    void recordCollision(const Singular& a, const Singular& b, float strength = 1.0f);

    // -----------------------------------------------------------------
    // Automatic bonding rules by ShapeKind pairs
    // -----------------------------------------------------------------
    void setAutoBond(Object::ShapeKind a, Object::ShapeKind b, bool enabled);
    bool getAutoBond(Object::ShapeKind a, Object::ShapeKind b);

    // -----------------------------------------------------------------
    // EventBus Integration
    // -----------------------------------------------------------------
    // Set up default physics event listeners (call this during initialization)
    void setupPhysicsEventListeners();

    // -----------------------------------------------------------------
    // Physics Events for EventBus integration
    // -----------------------------------------------------------------
    struct PhysicsCollisionEvent {
        Object* objectA{nullptr};
        Object* objectB{nullptr};
        glm::vec3 collisionPoint{0.0f};
        glm::vec3 collisionNormal{0.0f};
        float impactForce{0.0f};
        std::time_t timestamp{0};
        
        PhysicsCollisionEvent() = default;
        PhysicsCollisionEvent(Object* a, Object* b, const glm::vec3& point, const glm::vec3& normal, float force)
            : objectA(a), objectB(b), collisionPoint(point), collisionNormal(normal), impactForce(force), timestamp(std::time(nullptr)) {}
    };

    // -----------------------------------------------------------------
    // Modular Physics Laws
    // -----------------------------------------------------------------
    enum class LawType {
        Gravity,
        AirResistance,
        Collision,
        CustomForce,
        // Gradient gravity where every object attracts every other based on mass and distance
        GravityField,
        // Pull toward world center-of-mass (soft global attraction)
        CenterGravity
    };

    struct LawTarget {
        // Apply to all objects in zone if true
        bool allObjects = true;
        // Filter by geometry type flags
        bool limitByGeometry = false;
        std::vector<Object::ShapeKind> geometryTypes;
        // Preferred topology-level filter. When enabled, this is checked before
        // legacy geometryTypes so laws can target the real spatial category.
        bool limitBySpatialKind = false;
        std::vector<Object::SpatialKind> spatialKinds;
        // Filter by object type string equality
        bool limitByObjectType = false;
        std::vector<std::string> objectTypes;
        // Filter by attribute key/value (value empty => any value)
        bool limitByAttribute = false;
        std::string attributeKey;
        std::string attributeValue;
        // Filter by tag membership
        bool limitByTag = false;
        std::string tag;
        // Explicit object selection by identifier
        bool limitByExplicitList = false;
        std::vector<std::string> objectIdentifiers;
        // Runtime-only explicit object pointers (not persisted)
        std::vector<Object*> explicitObjects;
    };

    struct PhysicsLaw {
        int         id = 0;            // unique id
        std::string name;              // display name
        LawType     type = LawType::Gravity;
        bool        enabled = true;

        // Parameters (simple scalar config; custom can use strength as generic)
        float strength = 9.81f;        // Gravity accel, drag intensity, etc.
        float damping = 0.1f;          // For air, springs, etc.
        glm::vec3 direction{0.0f, -1.0f, 0.0f}; // For gravity/custom directional force

        // Optional custom applicator
        std::function<void(Object&, RigidForm&, float /*dt*/)> customApply;

        LawTarget   target;
    };

    // Registry
    const std::vector<PhysicsLaw>& getLaws();
    PhysicsLaw* getLawById(int id);
    int addLaw(const PhysicsLaw& law);
    bool removeLaw(int id);
    bool setLawEnabled(int id, bool on);
    bool updateLaw(int id, const PhysicsLaw& updated);

    // Helpers
    bool objectMatchesTarget(const Object& obj, const LawTarget& target);

    // -----------------------------------------------------------------
    // Gravity field helpers (for gameplay and debug visualization)
    // -----------------------------------------------------------------
    // Resolve the mass to use for an object (reads attribute "mass" if present; falls back to its RigidForm mass or defaultMass)
    float getObjectMass(Object* obj, float defaultMass = 1.0f);

    // Compute world center of mass across objects (optionally filter by LawTarget)
    glm::vec3 computeWorldCenterOfMass(const std::vector<std::shared_ptr<Object>>& objects,
                                       const LawTarget* target = nullptr);

    // Sample the gravity acceleration vector (in world units per second^2) at a point due to all objects
    // Uses G (strength) and softening epsilon to avoid singularities
    glm::vec3 sampleGravityField(const glm::vec3& position,
                                 const std::vector<std::shared_ptr<Object>>& objects,
                                 float gravitationalConstant,
                                 float softeningEpsilon,
                                 const LawTarget* target = nullptr);

    // Global tunables and visualization toggles
    void setGravityConstants(float G, float epsilon);
    void getGravityConstants(float& outG, float& outEpsilon);
    void setGravityVisualization(bool enabled);
    bool getGravityVisualization();
    void setGravityVisualizationDensity(int samplesPerAxis);
    int  getGravityVisualizationDensity();

    // Query helper for First Mover law states ("physics-gravity", "physics-collision")
    void setLawManager(LawManager* lm);
    LawManager* getLawManager();
    bool isGravityEnabled(const LawManager* lm = nullptr);
    bool isCollisionEnabled(const LawManager* lm = nullptr);
}
