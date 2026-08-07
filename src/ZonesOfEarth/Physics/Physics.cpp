#include "Physics.hpp"
#include "ZonesOfEarth/Physics/CollisionDispatcher.hpp"
#include "Form/Object/Object.hpp"
#include "Relation/RelationManager.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include <glm/glm.hpp>
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <cfloat>
#include <atomic>
#include <cmath>
#include <limits>

// Static registry of physics relations
static RelationManager g_physicsRegistry;

/* 

Physics is the governing laws of how non-Person Objects interact. It's not strictly physical-world physics, but a conceptual governing system.
Any Person with, the right permission can create, modify, remove, intertwine, or synthesize laws of Physics.
Need to implement 2D physics as well as 3D, and 2D-3D interaction physics

*/

namespace Physics {

    static bool isFlying = false;
    static bool g_legacyEngineEnabled = true;

    bool getLegacyEngineEnabled() { return g_legacyEngineEnabled; }
    void setLegacyEngineEnabled(bool enabled) { g_legacyEngineEnabled = enabled; }

    // Map Object* to RigidForm
    static std::unordered_map<Object*, RigidForm> g_objectForms;
    // Pairs touching as of the last resolved frame. There's no symmetric
    // physics callback for "stopped touching" the way there is for a fresh
    // collision, so we diff this set frame-to-frame to synthesize the edge
    // (see the "contact-ended" echo below).
    static std::set<std::pair<Object*, Object*>> g_touchingPairs;
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

    void updateBodies(std::vector<std::shared_ptr<Object>>& objects,
                      float deltaTime,
                      float gravityAccel,
                      float airResistance,
                      float groundY) {
        // Apply modular physics laws to all bodies before integration
        // We keep legacy gravity/air as fallback when no laws exist
        const auto& laws = getLaws();

        // 1. Clear forces & apply gravity to each form
        for (const auto& upObj : objects) {
            if (!upObj) continue;
            auto* obj = upObj.get();
            RigidForm& form = getFormFor(obj);
            clearForces(form);
            bool appliedAny = false;
            for (const auto& law : laws) {
                if (!law.enabled) continue;
                if (!objectMatchesTarget(*obj, law.target)) continue;
                switch (law.type) {
                    case LawType::Gravity: {
                        glm::vec3 dir = glm::normalize(law.direction);
                        if (glm::length(dir) < 1e-6f) dir = glm::vec3(0, -1, 0);
                        applyForce(form, dir * (law.strength * form.mass));
                        appliedAny = true;
                        break;
                    }
                    case LawType::AirResistance: {
                        glm::vec3 drag = -law.strength * form.velocity; // linear drag
                        applyForce(form, drag);
                        appliedAny = true;
                        break;
                    }
                    case LawType::Collision: {
                        // Collision is handled later in broadphase/narrowphase
                        // Clean this code up so its not good to have collision part here and another one separately
                        break;
                    }
                    case LawType::CenterGravity: {
                        // Pull toward current world center-of-mass of all eligible objects
                        glm::vec3 com = computeWorldCenterOfMass(objects, &law.target);
                        glm::vec3 pos = obj->getWorldCenter();
                        glm::vec3 delta = com - pos;
                        float len = glm::length(delta);
                        if (len > 1e-4f) {
                            glm::vec3 dir = delta / len;
                            // Use strength as acceleration magnitude per unit mass
                            applyForce(form, dir * (law.strength * form.mass));
                            appliedAny = true;
                        }
                        break;
                    }
                    case LawType::CustomForce: {
                        if (law.customApply) law.customApply(*obj, form, deltaTime);
                        else {
                            // Generic directional force with strength
                            glm::vec3 dir = glm::normalize(law.direction);
                            if (glm::length(dir) > 1e-6f)
                                applyForce(form, dir * law.strength);
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
                // no-op; form has no forces this frame
            } else if (laws.empty()) {
                applyForce(form, glm::vec3(0.0f, -gravityAccel * form.mass, 0.0f));
                glm::vec3 dragForce = -airResistance * form.velocity;
                applyForce(form, dragForce);
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
                glm::vec3 posA = a->getWorldCenter();
                RigidForm& formA = getFormFor(a);
                float massA = getObjectMass(a, formA.mass);
                for (size_t j = i + 1; j < count; ++j) {
                    Object* b = objects[j].get(); if (!b) continue; if (!objectMatchesTarget(*b, gravityFieldTarget)) continue;
                    glm::vec3 posB = b->getWorldCenter();
                    RigidForm& formB = getFormFor(b);
                    float massB = getObjectMass(b, formB.mass);
                    glm::vec3 r = posB - posA;
                    float dist2 = glm::dot(r, r) + g_softeningEps * g_softeningEps;
                    if (dist2 <= 1e-12f) continue;
                    float invDist = 1.0f / sqrtf(dist2);
                    glm::vec3 dir = r * invDist;
                    // Force magnitude: G * m1 * m2 / r^2
                    float magnitude = g_gravityConstant * massA * massB / dist2;
                    glm::vec3 force = dir * magnitude;
                    applyForce(formA,  force);
                    applyForce(formB, -force);
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
            applyForce(getFormFor(bond.a),  force);
            applyForce(getFormFor(bond.b), -force);
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

        // 3. Integrate each form and update object transforms
        for (const auto& upObj : objects) {
            if (!upObj) continue;
            auto* obj = upObj.get();
            RigidForm& form = getFormFor(obj);
            glm::vec3 pos = getObjectPos(obj);
            // Use baseline unless an AirResistance law targets this object
            bool airLawForObject = false;
            for (const auto& law : laws) {
                if (!law.enabled) continue;
                if (law.type != LawType::AirResistance) continue;
                if (objectMatchesTarget(*obj, law.target)) { airLawForObject = true; break; }
            }
            integrate(form, pos, deltaTime, airLawForObject ? 0.0f : airResistance, groundY);
            setObjectPos(obj, pos);
        }

        // 4. Detect and resolve object-object collisions -------------------
        // First update collision zones for all objects using latest transforms
        for(const auto& upObj : objects){
            if(!upObj) continue;
            upObj->updateCollisionZone(upObj->getTransform());
        }

        const size_t objCount = objects.size();
        // If necesary its better to keep the AABB thing as an algorithm to check if another collision algorithm is necesary to be used
        // If at least one Collision law exists, only resolve collisions for objects matching any Collision law target
        bool anyCollisionLaw = false; for (const auto& law : laws) { if (law.enabled && law.type == LawType::Collision) { anyCollisionLaw = true; break; } }
        std::set<std::pair<Object*, Object*>> currentTouching;
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

                // Cheap AABB pre-filter (used as a perf gate, not the authoritative test).
                bool overlapX = (minA.x <= maxB.x) && (maxA.x >= minB.x);
                bool overlapY = (minA.y <= maxB.y) && (maxA.y >= minB.y);
                bool overlapZ = (minA.z <= maxB.z) && (maxA.z >= minB.z);
                if(!(overlapX && overlapY && overlapZ)) continue;

                if (anyCollisionLaw) {
                    bool allowed = false;
                    for (const auto& law : laws) {
                        if (!law.enabled || law.type != LawType::Collision) continue;
                        if (objectMatchesTarget(*a, law.target) || objectMatchesTarget(*b, law.target)) { allowed = true; break; }
                    }
                    if (!allowed) continue; // don't resolve this pair
                }

                CollisionResult collision = dispatchCollision(*a, *b);
                if (!collision.hit) continue;

                glm::vec3 collisionNormal = collision.normal;
                float penetrationDepth = collision.depth;
                if (glm::dot(collisionNormal, collisionNormal) <= 1e-12f || penetrationDepth <= 0.0f) {
                    continue;
                }

                collisionNormal = glm::normalize(collisionNormal);
                float pushDist = (penetrationDepth * 0.5f) + 0.001f;
                glm::vec3 correction = collisionNormal * pushDist;

                // Apply corrections to positions
                glm::vec3 posA = getObjectPos(a);
                glm::vec3 posB = getObjectPos(b);
                posA += correction;
                posB -= correction;
                setObjectPos(a, posA);
                setObjectPos(b, posB);

                RigidForm& formA = getFormFor(a);
                RigidForm& formB = getFormFor(b);
                float impactForce = glm::length(formA.velocity) + glm::length(formB.velocity);
                formA.velocity -= collisionNormal * glm::dot(formA.velocity, collisionNormal);
                formB.velocity -= collisionNormal * glm::dot(formB.velocity, collisionNormal);
                
                glm::vec3 collisionPoint = collision.point;
                PhysicsCollisionEvent collisionEvent(a, b, collisionPoint, collisionNormal, impactForce);
                Core::EventBus::instance().publish(collisionEvent);
                // String-typed echo for Person-authored laws ("collision",
                // subject = a, object = b).
                Core::EventBus::instance().publish(ECA::Event{"collision", a, b, std::time(nullptr)});
                currentTouching.insert(a < b ? std::make_pair(a, b) : std::make_pair(b, a));

                // Update collision zones after correction for later pairs
                a->updateCollisionZone(a->getTransform());
                b->updateCollisionZone(b->getTransform());
            }
        }

        // Any pair touching last frame but absent from this frame's set has
        // separated. Echo it so laws can express "while touching" (paired
        // with the Overlaps predicate) instead of polling every tick.
        // BOTH participants must still be alive this frame: an object
        // deleted mid-contact (the Combine tool consumes its operand) must
        // not be resurrected as a dangling event subject.
        {
            std::unordered_set<Object*> alive;
            for (size_t i = 0; i < objCount; ++i) {
                if (objects[i]) alive.insert(objects[i].get());
            }
            for (const auto& pair : g_touchingPairs) {
                if (currentTouching.count(pair)) continue;
                if (!alive.count(pair.first) || !alive.count(pair.second)) continue;
                Core::EventBus::instance().publish(ECA::Event{
                    "contact-ended", pair.first, pair.second, std::time(nullptr)});
            }
        }
        g_touchingPairs = std::move(currentTouching);
    }

    RigidForm& getFormFor(Object* obj, float defaultMass) {
        auto& form = g_objectForms[obj];
        if (form.mass <= 0.0f) form.mass = defaultMass;
        // Keep form mass synchronized with object's declared mass attribute (if present)
        float attributeMass = getObjectMass(obj, form.mass);
        if (attributeMass > 0.0f && std::isfinite(attributeMass)) form.mass = attributeMass;
        return form;
    }

    // Reset registry of rigid bodies (e.g., after loading a scene)
    void resetRigidBodies() {
        g_objectForms.clear();
        g_touchingPairs.clear();
    }

    // Clear all bonds
    void clearBonds() {
        g_bonds.clear();
    }

    // ---------------------------------------------------------------------
    // Basic rigid-form helpers
    // ---------------------------------------------------------------------

    void applyForce(RigidForm& form, const glm::vec3& force) {
        form.accumulatedForce += force;
    }

    void clearForces(RigidForm& form) {
        form.accumulatedForce = glm::vec3(0.0f);
    }

    void integrate(RigidForm& form, glm::vec3& position, float deltaTime, float airResistance, float groundY) {
        // Semi-implicit Euler: v += (F/m) * dt, p += v * dt

        // Drag force proportional to velocity (linear air resistance)
        glm::vec3 dragForce = -airResistance * form.velocity;
        applyForce(form, dragForce);

        // If the object is resting on (or very near) the ground, cancel any
        // net downward force so gravity doesn't keep pulling it through the
        // floor every frame (which caused the ground-level jitter).
        const float GROUND_REST_EPS = 0.01f;
        bool onGround = (position.y - groundY) < GROUND_REST_EPS;
        if (onGround) {
            // Cancel downward component of accumulated force (ground reaction)
            if (form.accumulatedForce.y < 0.0f) {
                form.accumulatedForce.y = 0.0f;
            }
            // Kill any residual downward velocity
            if (form.velocity.y < 0.0f) {
                form.velocity.y = 0.0f;
            }
            // Snap position exactly to ground if slightly below
            if (position.y < groundY) {
                position.y = groundY;
            }
        }

        glm::vec3 acceleration = form.accumulatedForce / std::max(0.0001f, form.mass);
        form.velocity += acceleration * deltaTime;
        position      += form.velocity * deltaTime;

        // Final safety clamp: never allow below ground
        if (position.y < groundY) {
            position.y = groundY;
            if (form.velocity.y < 0.0f) form.velocity.y = 0.0f;
        }

        clearForces(form);
    }

    double kineticEnergy(const RigidForm& form) {
        return 0.5 * form.mass * glm::dot(form.velocity, form.velocity);
    }

    double potentialEnergy(const RigidForm& form, float height, float gravityAccel) {
        return form.mass * gravityAccel * height;
    }

    RelationManager& registry() { return g_physicsRegistry; }

    void recordGravity(const Singular& obj, const Singular& env, float strength) {
        // g_physicsRegistry.add(Relation{"gravity", obj, env, true, strength});
        auto rel = std::make_shared<Relation>("gravity", obj, env, true, strength);
        g_physicsRegistry.add(rel);
    }

    void recordCollision(const Singular& a, const Singular& b, float strength) {
        // g_physicsRegistry.add(Relation{"collision", a, b, false, strength});
        auto rel = std::make_shared<Relation>("collision", a, b, false, strength);
        g_physicsRegistry.add(rel);
    }

    void applyGravity(glm::vec3& position,
                      float deltaTime,
                      float groundY,
                      float gravityAccel,
                      float airResistance) {

        // Preserve a single rigid form to represent the player/camera
        static RigidForm playerForm{ /*mass*/ 70.0f };

        if (isFlying) {
            // Reset velocity when physics disabled or in non-physical modes
            playerForm.velocity = glm::vec3(0.0f);
            return;
        }

        clearForces(playerForm);

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
                    applyForce(playerForm, dir * (law.strength * playerForm.mass));
                    anyLawApplied = true;
                    break;
                }
                case LawType::AirResistance: {
                    glm::vec3 drag = -law.strength * playerForm.velocity;
                    applyForce(playerForm, drag);
                    anyLawApplied = true;
                    break;
                }
                default: break;
            }
        }

        if (!anyLawApplied) {
            // Legacy fallback
            const float GROUND_EPS = 1e-4f;
            bool grounded = std::abs(position.y - groundY) <= GROUND_EPS && playerForm.velocity.y <= 0.0f;
            if (!grounded) applyForce(playerForm, glm::vec3(0.0f, -gravityAccel * playerForm.mass, 0.0f));
            else playerForm.velocity.y = 0.0f;
        }

        // integrate() now handles ground-rest cancellation internally
        integrate(playerForm, position, deltaTime, airResistance, groundY);

        // Optionally: expose energies for debugging
        // double ke = kineticEnergy(playerForm);
        // double pe = potentialEnergy(playerForm, position.y - groundY, gravityAccel);
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

    void enforceCollisions(glm::vec3& position, const std::vector<std::shared_ptr<Object>>& objects) {
        for (const auto& obj : objects) {
            if (!obj) continue;
            // Skip the baseline ground placeholder: it is a solid AABB cube whose
            // top face sits exactly at groundY, so resting on it registers as a
            // perpetual penetration and fights gravity (the ground-level jitter).
            // Ground contact is handled separately by the groundY plane clamp in
            // integrate(), exactly as the object-object resolver already does.
            if (obj->getAttribute("baseline") == std::string("ground")) continue;
            // Update collision zone based on current transform
            glm::mat4 transform = obj->getTransform();
            obj->updateCollisionZone(transform);

            glm::vec3 correction(0.0f);
            if (obj->computePointPenetration(position, correction)) {
                position += correction;
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
        if (t.limitBySpatialKind) {
            bool ok = false;
            for (auto kind : t.spatialKinds) if (obj.getSpatialKind() == kind) { ok = true; break; }
            if (!ok) return false;
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
        // Fallback to registered rigid form mass
        auto it = g_objectForms.find(obj);
        if (it != g_objectForms.end() && it->second.mass > 0.0f) return it->second.mass;
        return defaultMass;
    }

    glm::vec3 computeWorldCenterOfMass(const std::vector<std::shared_ptr<Object>>& objects,
                                       const LawTarget* target) {
        glm::vec3 sumWeighted(0.0f);
        double totalMass = 0.0;
        for (const auto& up : objects) {
            if (!up) continue; Object* obj = up.get();
            if (target && !objectMatchesTarget(*obj, *target)) continue;
            RigidForm& form = getFormFor(obj);
            float m = getObjectMass(obj, form.mass);
            if (m <= 0.0f) continue;
            glm::vec3 pos = obj->getWorldCenter();
            sumWeighted += pos * m;
            totalMass += m;
        }
        if (totalMass <= 1e-8) return glm::vec3(0.0f);
        return sumWeighted / static_cast<float>(totalMass);
    }

    glm::vec3 sampleGravityField(const glm::vec3& position,
                                 const std::vector<std::shared_ptr<Object>>& objects,
                                 float gravitationalConstant,
                                 float softeningEpsilon,
                                 const LawTarget* target) {
        glm::vec3 acc(0.0f);
        for (const auto& up : objects) {
            if (!up) continue; Object* obj = up.get();
            if (target && !objectMatchesTarget(*obj, *target)) continue;
            RigidForm& form = getFormFor(obj);
            float m = getObjectMass(obj, form.mass);
            if (m <= 0.0f) continue;
            glm::vec3 pos = obj->getWorldCenter();
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
