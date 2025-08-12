#include "Physics.hpp"
#include "Form/Object/Object.hpp"
#include "Relation/RelationManager.hpp"
#include "Core/EventBus.hpp"
#include <glm/glm.hpp>
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <cfloat>
#include <atomic>
#include <cmath>

// Static registry of physics relations
static RelationManager g_physicsRegistry;

/* 

Physics is the governing laws of how non-Person Objects interact. It's not strictly physical-world physics, but a conceptual governing system.
Any Person with, the right permission can create, modify, remove, intertwine, or synthesize laws of Physics.
Need to implement 2D physics as well as 3D, and 2D-3D interaction physics

*/

namespace Physics {

    static bool isFlying = false;

    // Map Object* to RigidBody
    static std::unordered_map<Object*, RigidBody> g_objectBodies;
    // Global gravity field parameters
    static float g_gravityConstant = 1.0f;      // Tunable G for gameplay scale
    static float g_softeningEps    = 0.25f;     // Softening to avoid singularities in 1/r^2
    static bool  g_visualizeGravity = false;     // Debug draw of field
    static int   g_vizDensity = 8;               // Samples per axis for field arrows

    // Bond list
    static std::vector<Bond> g_bonds;

    const std::vector<Bond>& getBonds(){ return g_bonds; }

    bool setBondParams(Object* a, Object* b, float restLength, float strength){
        for(auto& bond : g_bonds){
            if((bond.a==a && bond.b==b) || (bond.a==b && bond.b==a)){
                bond.restLength = restLength;
                bond.strength   = strength;
                return true;
            }
        }
        return false;
    }

    // Encode pair of shapes into 32-bit key
    static uint32_t keyFor(Object::GeometryType a, Object::GeometryType b){
        int ai = static_cast<int>(a);
        int bi = static_cast<int>(b);
        if(ai>bi) std::swap(ai,bi);
        return (static_cast<uint32_t>(ai)<<16)|static_cast<uint32_t>(bi);
    }

    static std::unordered_set<uint32_t> g_autoBondRules;

    void setAutoBond(Object::GeometryType a, Object::GeometryType b, bool enabled){
        uint32_t k = keyFor(a,b);
        if(enabled) g_autoBondRules.insert(k); else g_autoBondRules.erase(k);
    }

    bool getAutoBond(Object::GeometryType a, Object::GeometryType b){
        return g_autoBondRules.count(keyFor(a,b))>0;
    }

    void addBond(Object* a, Object* b, float restLength, float strength) {
        if (!a || !b) return;
        // Prevent duplicates
        for (const auto& bond : g_bonds) {
            if ((bond.a == a && bond.b == b) || (bond.a == b && bond.b == a)) return;
        }
        g_bonds.push_back(Bond{a,b,restLength,strength});
    }

    void removeBond(Object* a, Object* b) {
        g_bonds.erase(std::remove_if(g_bonds.begin(), g_bonds.end(), [&](const Bond& bond){
            return (bond.a == a && bond.b == b) || (bond.a == b && bond.b == a);
        }), g_bonds.end());
    }

    // Helper to extract & update object position via its transform
    static glm::vec3 getObjectPos(const Object* obj) {
        glm::mat4 t = obj->getTransform();
        return glm::vec3(t[3]);
    }

    static void setObjectPos(Object* obj, const glm::vec3& pos) {
        glm::mat4 t = obj->getTransform();
        t[3] = glm::vec4(pos, 1.0f);
        obj->setTransform(t);
    }

    void updateBodies(std::vector<std::unique_ptr<Object>>& objects,
                      float deltaTime,
                      float gravityAccel,
                      float airResistance,
                      float groundY) {
        // Apply modular physics laws to all bodies before integration
        // We keep legacy gravity/air as fallback when no laws exist
        const auto& laws = getLaws();

        // 1. Clear forces & apply gravity to each body
        for (const auto& upObj : objects) {
            if (!upObj) continue;
            auto* obj = upObj.get();
            RigidBody& body = getBodyFor(obj);
            clearForces(body);
            bool appliedAny = false;
            for (const auto& law : laws) {
                if (!law.enabled) continue;
                if (!objectMatchesTarget(*obj, law.target)) continue;
                switch (law.type) {
                    case LawType::Gravity: {
                        glm::vec3 dir = glm::normalize(law.direction);
                        if (glm::length(dir) < 1e-6f) dir = glm::vec3(0, -1, 0);
                        applyForce(body, dir * (law.strength * body.mass));
                        appliedAny = true;
                        break;
                    }
                    case LawType::AirResistance: {
                        glm::vec3 drag = -law.strength * body.velocity; // linear drag
                        applyForce(body, drag);
                        appliedAny = true;
                        break;
                    }
                    case LawType::Collision: {
                        // Collision is handled later in broadphase/narrowphase
                        break;
                    }
                    case LawType::CenterGravity: {
                        // Pull toward current world center-of-mass of all eligible objects
                        glm::vec3 com = computeWorldCenterOfMass(objects, &law.target);
                        glm::vec3 pos = getObjectPos(obj);
                        glm::vec3 delta = com - pos;
                        float len = glm::length(delta);
                        if (len > 1e-4f) {
                            glm::vec3 dir = delta / len;
                            // Use strength as acceleration magnitude per unit mass
                            applyForce(body, dir * (law.strength * body.mass));
                            appliedAny = true;
                        }
                        break;
                    }
                    case LawType::CustomForce: {
                        if (law.customApply) law.customApply(*obj, body, deltaTime);
                        else {
                            // Generic directional force with strength
                            glm::vec3 dir = glm::normalize(law.direction);
                            if (glm::length(dir) > 1e-6f)
                                applyForce(body, dir * law.strength);
                        }
                        appliedAny = true;
                        break;
                    }
                    case LawType::GravityField: {
                        // Handled in a separate pairwise loop below for efficiency and symmetry
                        break;
                    }
                }
            }
            // Fallback legacy gravity/air if no law applied a force
            if (!laws.empty() && !appliedAny) {
                // no-op; body has no forces this frame
            } else if (laws.empty()) {
                applyForce(body, glm::vec3(0.0f, -gravityAccel * body.mass, 0.0f));
                glm::vec3 dragForce = -airResistance * body.velocity;
                applyForce(body, dragForce);
            }
        }

        // 1b. Pairwise gravity field accumulation if a GravityField law exists
        bool anyGravityField = false;
        LawTarget gravityFieldTarget{}; // default allObjects
        for (const auto& law : laws) {
            if (law.enabled && law.type == LawType::GravityField) { anyGravityField = true; gravityFieldTarget = law.target; break; }
        }
        if (anyGravityField) {
            const size_t count = objects.size();
            for (size_t i = 0; i < count; ++i) {
                Object* a = objects[i].get(); if (!a) continue; if (!objectMatchesTarget(*a, gravityFieldTarget)) continue;
                glm::vec3 posA = getObjectPos(a);
                RigidBody& bodyA = getBodyFor(a);
                float massA = getObjectMass(a, bodyA.mass);
                for (size_t j = i + 1; j < count; ++j) {
                    Object* b = objects[j].get(); if (!b) continue; if (!objectMatchesTarget(*b, gravityFieldTarget)) continue;
                    glm::vec3 posB = getObjectPos(b);
                    RigidBody& bodyB = getBodyFor(b);
                    float massB = getObjectMass(b, bodyB.mass);
                    glm::vec3 r = posB - posA;
                    float dist2 = glm::dot(r, r) + g_softeningEps * g_softeningEps;
                    if (dist2 <= 1e-12f) continue;
                    float invDist = 1.0f / sqrtf(dist2);
                    glm::vec3 dir = r * invDist;
                    // Force magnitude: G * m1 * m2 / r^2
                    float magnitude = g_gravityConstant * massA * massB / dist2;
                    glm::vec3 force = dir * magnitude;
                    applyForce(bodyA,  force);
                    applyForce(bodyB, -force);
                }
            }
        }

        // 2. Apply bond (spring) forces
        for (const auto& bond : g_bonds) {
            if (!bond.a || !bond.b) continue;
            glm::vec3 posA = getObjectPos(bond.a);
            glm::vec3 posB = getObjectPos(bond.b);
            glm::vec3 delta = posB - posA;
            float dist = glm::length(delta);
            if (dist < 1e-5f) continue;
            glm::vec3 dir = delta / dist;
            float displacement = dist - bond.restLength;
            glm::vec3 force = dir * (bond.strength * displacement);
            applyForce(getBodyFor(bond.a),  force);
            applyForce(getBodyFor(bond.b), -force);
        }

        // Auto-create bonds based on geometry rules (simple n^2 loop for now)
        for(size_t i=0;i<objects.size();++i){
            for(size_t j=i+1;j<objects.size();++j){
                Object* oa = objects[i].get();
                Object* ob = objects[j].get();
                if(!oa||!ob) continue;
                if(!getAutoBond(oa->getGeometryType(), ob->getGeometryType())) continue;
                // check duplicate
                bool exists=false; for(const auto& b : g_bonds){ if((b.a==oa&&b.b==ob)||(b.a==ob&&b.b==oa)){exists=true;break;} }
                if(!exists) addBond(oa,ob,1.0f,10.0f);
            }
        }

        // 3. Integrate each body and update object transforms
        for (const auto& upObj : objects) {
            if (!upObj) continue;
            auto* obj = upObj.get();
            RigidBody& body = getBodyFor(obj);
            glm::vec3 pos = getObjectPos(obj);
            // Use baseline unless an AirResistance law targets this object
            bool airLawForObject = false;
            for (const auto& law : laws) {
                if (!law.enabled) continue;
                if (law.type != LawType::AirResistance) continue;
                if (objectMatchesTarget(*obj, law.target)) { airLawForObject = true; break; }
            }
            integrate(body, pos, deltaTime, airLawForObject ? 0.0f : airResistance, groundY);
            setObjectPos(obj, pos);
        }

        // 4. Detect and resolve object-object collisions (AABB) -----------
        // First update collision zones for all objects using latest transforms
        for(const auto& upObj : objects){
            if(!upObj) continue;
            upObj->updateCollisionZone(upObj->getTransform());
        }

        const size_t objCount = objects.size();
        // If at least one Collision law exists, only resolve collisions for objects matching any Collision law target
        bool anyCollisionLaw = false; for (const auto& law : laws) { if (law.enabled && law.type == LawType::Collision) { anyCollisionLaw = true; break; } }
        for(size_t i = 0; i < objCount; ++i){
            if(!objects[i]) continue;
            // Skip the ground placeholder at index 1 (handled separately by groundY plane)
            if(i == 1) continue;
            Object* a = objects[i].get();
            // Compute AABB for object A
            glm::vec3 minA( FLT_MAX), maxA(-FLT_MAX);
            for(const auto& corner : a->collisionZone.corners){
                minA = glm::min(minA, corner);
                maxA = glm::max(maxA, corner);
            }
            for(size_t j = i + 1; j < objCount; ++j){
                if(!objects[j]) continue;
                if(j == 1) continue; // skip ground
                Object* b = objects[j].get();
                glm::vec3 minB( FLT_MAX), maxB(-FLT_MAX);
                for(const auto& corner : b->collisionZone.corners){
                    minB = glm::min(minB, corner);
                    maxB = glm::max(maxB, corner);
                }

                // Check for overlap along all three axes
                bool overlapX = (minA.x <= maxB.x) && (maxA.x >= minB.x);
                bool overlapY = (minA.y <= maxB.y) && (maxA.y >= minB.y);
                bool overlapZ = (minA.z <= maxB.z) && (maxA.z >= minB.z);
                if(!(overlapX && overlapY && overlapZ)) continue; // no collision
                if (anyCollisionLaw) {
                    bool allowed = false;
                    for (const auto& law : laws) {
                        if (!law.enabled || law.type != LawType::Collision) continue;
                        if (objectMatchesTarget(*a, law.target) || objectMatchesTarget(*b, law.target)) { allowed = true; break; }
                    }
                    if (!allowed) continue; // don't resolve this pair
                }

                // Compute overlap amounts
                float overlapAmtX = std::min(maxA.x, maxB.x) - std::max(minA.x, minB.x);
                float overlapAmtY = std::min(maxA.y, maxB.y) - std::max(minA.y, minB.y);
                float overlapAmtZ = std::min(maxA.z, maxB.z) - std::max(minA.z, minB.z);

                // Find the smallest overlap axis to resolve collision
                float minOverlap = overlapAmtX; int axis = 0;
                if(overlapAmtY < minOverlap){ minOverlap = overlapAmtY; axis = 1; }
                if(overlapAmtZ < minOverlap){ minOverlap = overlapAmtZ; axis = 2; }

                if(minOverlap <= 0.0f) continue; // shouldn't happen but guard

                // Direction: push objects apart along chosen axis away from each other
                glm::vec3 centerA = (minA + maxA) * 0.5f;
                glm::vec3 centerB = (minB + maxB) * 0.5f;
                float sign = 0.0f;
                switch(axis){
                    case 0: sign = (centerA.x < centerB.x) ? -1.0f : 1.0f; break;
                    case 1: sign = (centerA.y < centerB.y) ? -1.0f : 1.0f; break;
                    case 2: sign = (centerA.z < centerB.z) ? -1.0f : 1.0f; break;
                }
                float pushDist = (minOverlap * 0.5f) + 0.001f; // add small epsilon
                glm::vec3 correction(0.0f);
                if(axis == 0) correction.x = pushDist * sign;
                else if(axis == 1) correction.y = pushDist * sign;
                else correction.z = pushDist * sign;

                // Apply corrections to positions
                glm::vec3 posA = getObjectPos(a);
                glm::vec3 posB = getObjectPos(b);
                posA += correction;
                posB -= correction;
                setObjectPos(a, posA);
                setObjectPos(b, posB);

                // Damp velocities along collision axis to prevent tunneling
                RigidBody& bodyA = getBodyFor(a);
                RigidBody& bodyB = getBodyFor(b);
                if(axis == 0){ bodyA.velocity.x = 0.0f; bodyB.velocity.x = 0.0f; }
                else if(axis == 1){ bodyA.velocity.y = 0.0f; bodyB.velocity.y = 0.0f; }
                else { bodyA.velocity.z = 0.0f; bodyB.velocity.z = 0.0f; }

                // Publish collision event for EventBus listeners
                glm::vec3 collisionPoint = (centerA + centerB) * 0.5f;
                glm::vec3 collisionNormal = glm::normalize(centerA - centerB);
                float impactForce = glm::length(bodyA.velocity) + glm::length(bodyB.velocity);
                
                PhysicsCollisionEvent collisionEvent(a, b, collisionPoint, collisionNormal, impactForce);
                Core::EventBus::instance().publish(collisionEvent);

                // Update collision zones after correction for later pairs
                a->updateCollisionZone(a->getTransform());
                b->updateCollisionZone(b->getTransform());
            }
        }
    }

    RigidBody& getBodyFor(Object* obj, float defaultMass) {
        auto& body = g_objectBodies[obj];
        if (body.mass <= 0.0f) body.mass = defaultMass;
        // Keep body mass synchronized with object's declared mass attribute (if present)
        float attributeMass = getObjectMass(obj, body.mass);
        if (attributeMass > 0.0f && std::isfinite(attributeMass)) body.mass = attributeMass;
        return body;
    }

    // Reset registry of rigid bodies (e.g., after loading a scene)
    void resetRigidBodies() {
        g_objectBodies.clear();
    }

    // Clear all bonds
    void clearBonds() {
        g_bonds.clear();
    }

    // ---------------------------------------------------------------------
    // Basic rigid-body helpers
    // ---------------------------------------------------------------------

    void applyForce(RigidBody& body, const glm::vec3& force) {
        body.accumulatedForce += force;
    }

    void clearForces(RigidBody& body) {
        body.accumulatedForce = glm::vec3(0.0f);
    }

    void integrate(RigidBody& body, glm::vec3& position, float deltaTime, float airResistance, float groundY) {
        // Semi-implicit Euler: v += (F/m) * dt, p += v * dt

        // Drag force proportional to velocity (linear air resistance)
        glm::vec3 dragForce = -airResistance * body.velocity;
        applyForce(body, dragForce);

        glm::vec3 acceleration = body.accumulatedForce / std::max(0.0001f, body.mass);
        body.velocity += acceleration * deltaTime;
        position      += body.velocity * deltaTime;

        // Robust ground collision with snapping to avoid small oscillations
        const float GROUND_SNAP_EPS = 1e-3f;
        if (position.y < groundY) {
            position.y = groundY;
            if (body.velocity.y < 0.0f) body.velocity.y = 0.0f;
        } else if (std::abs(position.y - groundY) < GROUND_SNAP_EPS) {
            // Snap to plane if within epsilon and heading downward negligibly
            position.y = groundY;
            body.velocity.y = 0.0f;
        }

        clearForces(body);
    }

    double kineticEnergy(const RigidBody& body) {
        return 0.5 * body.mass * glm::dot(body.velocity, body.velocity);
    }

    double potentialEnergy(const RigidBody& body, float height, float gravityAccel) {
        return body.mass * gravityAccel * height;
    }

    RelationManager& registry() { return g_physicsRegistry; }

    void recordGravity(const Singular& obj, const Singular& env, float strength) {
        g_physicsRegistry.add(Relation{"gravity", obj, env, true, strength});
    }

    void recordCollision(const Singular& a, const Singular& b, float strength) {
        g_physicsRegistry.add(Relation{"collision", a, b, false, strength});
    }

    void applyGravity(glm::vec3& position,
                      bool physicsEnabled,
                      GameMode mode,
                      float deltaTime,
                      float groundY,
                      float gravityAccel,
                      float airResistance) {

        // Preserve a single rigid body to represent the player/camera
        static RigidBody playerBody{ /*mass*/ 70.0f };

        if (!physicsEnabled || mode == GameMode::Spectator || isFlying) {
            // Reset velocity when physics disabled or in non-physical modes
            playerBody.velocity = glm::vec3(0.0f);
            return;
        }

        clearForces(playerBody);

        // First, apply laws that target the camera as if it were an object
        // We re-use the law registry by constructing a dummy LawTarget check
        // Camera is not an Object, so only Gravity/AirResistance general parameters are used
        const auto& laws = getLaws();
        bool anyLawApplied = false;
        for (const auto& law : laws) {
            if (!law.enabled) continue;
            switch (law.type) {
                case LawType::Gravity: {
                    glm::vec3 dir = glm::normalize(law.direction);
                    if (glm::length(dir) < 1e-6f) dir = glm::vec3(0, -1, 0);
                    applyForce(playerBody, dir * (law.strength * playerBody.mass));
                    anyLawApplied = true;
                    break;
                }
                case LawType::AirResistance: {
                    glm::vec3 drag = -law.strength * playerBody.velocity;
                    applyForce(playerBody, drag);
                    anyLawApplied = true;
                    break;
                }
                default: break;
            }
        }

        if (!anyLawApplied) {
            // Legacy fallback
            const float GROUND_EPS = 1e-4f;
            bool grounded = std::abs(position.y - groundY) <= GROUND_EPS && playerBody.velocity.y <= 0.0f;
            if (!grounded) applyForce(playerBody, glm::vec3(0.0f, -gravityAccel * playerBody.mass, 0.0f));
            else playerBody.velocity.y = 0.0f;
        }

        integrate(playerBody, position, deltaTime, airResistance, groundY);

        // Optionally: expose energies for debugging
        // double ke = kineticEnergy(playerBody);
        // double pe = potentialEnergy(playerBody, position.y - groundY, gravityAccel);
    }

    void setFlying(bool enabled) { isFlying = enabled; }
    void toggleFlying() { isFlying = !isFlying; }
    bool getFlying() { return isFlying; }

    // -----------------------------------------------------------------
    // EventBus Integration Helpers
    // -----------------------------------------------------------------
    void setupPhysicsEventListeners() {
        auto& eventBus = Core::EventBus::instance();
        
        // Listen for physics collisions with high priority
        eventBus.subscribe<PhysicsCollisionEvent>([](const PhysicsCollisionEvent& event) {
            // Log collision details
            std::cout << "Physics Collision: Objects " 
                      << (event.objectA ? event.objectA->getIdentifier() : "Unknown") 
                      << " and " 
                      << (event.objectB ? event.objectB->getIdentifier() : "Unknown")
                      << " collided with force " << event.impactForce 
                      << " at point (" << event.collisionPoint.x << ", " << event.collisionPoint.y << ", " << event.collisionPoint.z << ")" << std::endl;
            
            // Record collision in physics registry (existing functionality)
            if (event.objectA && event.objectB) {
                recordCollision(*event.objectA, *event.objectB, event.impactForce);
            }
            
            // You can add more collision response logic here:
            // - Play sound effects
            // - Create particle effects
            // - Update UI elements
            // - Trigger game mechanics
            // - Update formation relations
        }, 10); // High priority for physics events
    }

    void enforceCollisions(glm::vec3& position, const std::vector<std::unique_ptr<Object>>& objects) {
        for (const auto& obj : objects) {
            // Update collision zone based on current transform
            glm::mat4 transform = obj->getTransform();
            obj->updateCollisionZone(transform);

            if (obj->isPointInside(position)) {
                // Determine the axis-aligned bounding box of the object
                glm::vec3 minCorner = obj->collisionZone.corners[0];
                glm::vec3 maxCorner = obj->collisionZone.corners[0];
                for (int i = 1; i < 8; ++i) {
                    minCorner = glm::min(minCorner, obj->collisionZone.corners[i]);
                    maxCorner = glm::max(maxCorner, obj->collisionZone.corners[i]);
                }

                // Push position out to nearest face
                float dx = std::min(std::abs(position.x - minCorner.x), std::abs(position.x - maxCorner.x));
                float dy = std::min(std::abs(position.y - minCorner.y), std::abs(position.y - maxCorner.y));
                float dz = std::min(std::abs(position.z - minCorner.z), std::abs(position.z - maxCorner.z));

                if (dx <= dy && dx <= dz) {
                    position.x = (std::abs(position.x - minCorner.x) < std::abs(position.x - maxCorner.x)) ? minCorner.x : maxCorner.x;
                } else if (dy <= dx && dy <= dz) {
                    position.y = (std::abs(position.y - minCorner.y) < std::abs(position.y - maxCorner.y)) ? minCorner.y : maxCorner.y;
                } else {
                    position.z = (std::abs(position.z - minCorner.z) < std::abs(position.z - maxCorner.z)) ? minCorner.z : maxCorner.z;
                }
            }
        }
    }

    // --------------------------------------------------------------
    // Physics Laws Registry Implementation
    // --------------------------------------------------------------
    static std::vector<PhysicsLaw> g_laws;
    static std::atomic<int> g_nextLawId{1};

    const std::vector<PhysicsLaw>& getLaws() { return g_laws; }

    PhysicsLaw* getLawById(int id) {
        for (auto& law : g_laws) if (law.id == id) return &law; return nullptr;
    }

    int addLaw(const PhysicsLaw& law) {
        PhysicsLaw copy = law;
        copy.id = g_nextLawId++;
        g_laws.push_back(copy);
        return copy.id;
    }

    bool removeLaw(int id) {
        auto it = std::remove_if(g_laws.begin(), g_laws.end(), [&](const PhysicsLaw& l){ return l.id == id; });
        if (it == g_laws.end()) return false;
        g_laws.erase(it, g_laws.end());
        return true;
    }

    bool setLawEnabled(int id, bool on) {
        if (auto* l = getLawById(id)) { l->enabled = on; return true; } return false;
    }

    bool updateLaw(int id, const PhysicsLaw& updated) {
        if (auto* l = getLawById(id)) { *l = updated; l->id = id; return true; } return false;
    }

    bool objectMatchesTarget(const Object& obj, const LawTarget& t) {
        if (t.allObjects) return true;
        // Runtime explicit pointers have highest precedence
        if (!t.explicitObjects.empty()) {
            for (auto* p : t.explicitObjects) if (p == &obj) return true;
            // If explicitObjects is provided, we treat it as the only set unless other filters also match
            // fallthrough to allow other filters as well
        }
        if (t.limitByExplicitList) {
            bool found=false; for(const auto& id : t.objectIdentifiers){ if(obj.getIdentifier()==id){found=true;break;} }
            if(!found) return false;
        }
        if (t.limitByGeometry) {
            bool ok = false;
            for (auto g : t.geometryTypes) if (obj.getGeometryType() == g) { ok = true; break; }
            if (!ok) return false;
        }
        if (t.limitByObjectType) {
            bool ok = false;
            for (const auto& s : t.objectTypes) if (obj.getObjectType() == s) { ok = true; break; }
            if (!ok) return false;
        }
        if (t.limitByAttribute) {
            if (!obj.hasAttribute(t.attributeKey)) return false;
            if (!t.attributeValue.empty() && obj.getAttribute(t.attributeKey) != t.attributeValue) return false;
        }
        if (t.limitByTag) {
            if (!obj.hasTag(t.tag)) return false;
        }
        return true;
    }

    // ---------------------------------------------------------------------
    // Gravity helpers implementation
    // ---------------------------------------------------------------------
    float getObjectMass(Object* obj, float defaultMass) {
        if (!obj) return defaultMass;
        if (obj->hasAttribute("mass")) {
            const std::string& s = obj->getAttribute("mass");
            if (!s.empty()) {
                try {
                    float v = std::stof(s);
                    if (v > 0.0f && std::isfinite(v)) return v;
                } catch (...) {}
            }
        }
        // Fallback to registered rigid body mass
        auto it = g_objectBodies.find(obj);
        if (it != g_objectBodies.end() && it->second.mass > 0.0f) return it->second.mass;
        return defaultMass;
    }

    glm::vec3 computeWorldCenterOfMass(const std::vector<std::unique_ptr<Object>>& objects,
                                       const LawTarget* target) {
        glm::vec3 sumWeighted(0.0f);
        double totalMass = 0.0;
        for (const auto& up : objects) {
            if (!up) continue; Object* obj = up.get();
            if (target && !objectMatchesTarget(*obj, *target)) continue;
            RigidBody& body = getBodyFor(obj);
            float m = getObjectMass(obj, body.mass);
            if (m <= 0.0f) continue;
            glm::vec3 pos = getObjectPos(obj);
            sumWeighted += pos * m;
            totalMass += m;
        }
        if (totalMass <= 1e-8) return glm::vec3(0.0f);
        return sumWeighted / static_cast<float>(totalMass);
    }

    glm::vec3 sampleGravityField(const glm::vec3& position,
                                 const std::vector<std::unique_ptr<Object>>& objects,
                                 float gravitationalConstant,
                                 float softeningEpsilon,
                                 const LawTarget* target) {
        glm::vec3 acc(0.0f);
        for (const auto& up : objects) {
            if (!up) continue; Object* obj = up.get();
            if (target && !objectMatchesTarget(*obj, *target)) continue;
            RigidBody& body = getBodyFor(obj);
            float m = getObjectMass(obj, body.mass);
            if (m <= 0.0f) continue;
            glm::vec3 pos = getObjectPos(obj);
            glm::vec3 r = pos - position;
            float dist2 = glm::dot(r, r) + softeningEpsilon * softeningEpsilon;
            if (dist2 <= 1e-12f) continue;
            float invDist = 1.0f / sqrtf(dist2);
            glm::vec3 dir = r * invDist;
            float aMag = gravitationalConstant * m / dist2; // acceleration due to this mass
            acc += dir * aMag;
        }
        return acc;
    }

    // Tunables
    void setGravityConstants(float G, float epsilon) { g_gravityConstant = G; g_softeningEps = epsilon; }
    void getGravityConstants(float& outG, float& outEpsilon) { outG = g_gravityConstant; outEpsilon = g_softeningEps; }
    void setGravityVisualization(bool enabled) { g_visualizeGravity = enabled; }
    bool getGravityVisualization() { return g_visualizeGravity; }
    void setGravityVisualizationDensity(int samplesPerAxis) { g_vizDensity = std::max(2, samplesPerAxis); }
    int  getGravityVisualizationDensity() { return g_vizDensity; }
}
