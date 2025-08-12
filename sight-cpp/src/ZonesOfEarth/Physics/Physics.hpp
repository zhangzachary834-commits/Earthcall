#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include "Form/Object/Object.hpp"
#include "Relation/RelationManager.hpp"
#include "Singular.hpp"
#include <string>
#include <functional>

namespace Physics {

    // Use Person's GameMode enum instead of storing it in Physics
    enum class GameMode {
        Creative,
        Survival,
        Spectator
    };

    // Applies gravity and basic physics integration (velocity & air resistance)
    // deltaTime       - elapsed time since last update (seconds)
    // groundY         - ground plane height (default 0)
    // gravityAccel    - gravitational acceleration magnitude (m/s^2)
    // airResistance   - simple linear drag coefficient (0-1, per second)
    void applyGravity(glm::vec3& position,
                      bool physicsEnabled,
                      GameMode mode,
                      float deltaTime,
                      float groundY       = 0.0f,
                      float gravityAccel  = 9.81f,
                      float airResistance = 0.1f);

    // Enforces collisions between a point (e.g., camera/player) and all objects' collision zones
    void enforceCollisions(glm::vec3& position, const std::vector<std::unique_ptr<Object>>& objects);

    // --- Flight state helpers ---
    void setFlying(bool enabled);
    void toggleFlying();
    bool getFlying();

    // Basic force representation (direction normalized, magnitude in Newtons)
    struct Force {
        glm::vec3 direction{0.0f};
        float magnitude{0.0f};
    };

    // Simple rigid body used for point-mass entities (e.g., the player)
    struct RigidBody {
        float mass{1.0f};                       // kilograms
        glm::vec3 velocity{0.0f};               // metres per second
        glm::vec3 accumulatedForce{0.0f};       // Newtons, reset each step
    };

    // Accumulate an external force on the body (adds to this frame only)
    void applyForce(RigidBody& body, const glm::vec3& force);

    // Clears the force accumulator
    void clearForces(RigidBody& body);

    // Integrate motion via semi-implicit Euler and handle ground collision
    void integrate(RigidBody& body,
                   glm::vec3& position,
                   float      deltaTime,
                   float      airResistance = 0.1f,
                   float      groundY       = 0.0f);

    // Energy helpers
    double kineticEnergy(const RigidBody& body);
    double potentialEnergy(const RigidBody& body, float height, float gravityAccel = 9.81f);

    // --------------------------------------------------------------
    // RigidBody registry for world Objects
    // --------------------------------------------------------------
    // Create (if absent) and retrieve the RigidBody associated with the Object
    RigidBody& getBodyFor(Object* obj, float defaultMass = 1.0f);

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
    void updateBodies(std::vector<std::unique_ptr<Object>>& objects,
                      float deltaTime,
                      float gravityAccel  = 9.81f,
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
    // Automatic bonding rules by GeometryType pairs
    // -----------------------------------------------------------------
    void setAutoBond(Object::GeometryType a, Object::GeometryType b, bool enabled);
    bool getAutoBond(Object::GeometryType a, Object::GeometryType b);

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
        std::vector<Object::GeometryType> geometryTypes;
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
        std::function<void(Object&, RigidBody&, float /*dt*/)> customApply;

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
    // Resolve the mass to use for an object (reads attribute "mass" if present; falls back to its RigidBody mass or defaultMass)
    float getObjectMass(Object* obj, float defaultMass = 1.0f);

    // Compute world center of mass across objects (optionally filter by LawTarget)
    glm::vec3 computeWorldCenterOfMass(const std::vector<std::unique_ptr<Object>>& objects,
                                       const LawTarget* target = nullptr);

    // Sample the gravity acceleration vector (in world units per second^2) at a point due to all objects
    // Uses G (strength) and softening epsilon to avoid singularities
    glm::vec3 sampleGravityField(const glm::vec3& position,
                                 const std::vector<std::unique_ptr<Object>>& objects,
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
}