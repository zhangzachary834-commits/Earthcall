#include "Person.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <functional>
#include <unordered_map>
#include <GLFW/glfw3.h>
#include <OpenGL/glu.h>
#include "Form/Object/Formation/Menu/stb_easy_font.h"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "PersonEvents.hpp"
#include "Core/EventBus.hpp"
#include <glm/gtc/matrix_transform.hpp>

// Forward-declare global ZoneManager defined in main.cpp
extern ZoneManager mgr;

// Event structures for Person events
struct PersonCreatedEvent {
    const Person& person;
    std::time_t timestamp;
    
    PersonCreatedEvent(const Person& p) 
        : person(p), timestamp(std::time(nullptr)) {}
};

struct PersonJoinedEvent {
    const Person& person;
    std::string zoneName;
    std::time_t timestamp;
    
    PersonJoinedEvent(const Person& p, const std::string& zone) 
        : person(p), zoneName(zone), timestamp(std::time(nullptr)) {}
};

struct PersonLoginEvent {
    const Person& person;
    std::string sessionId;
    std::time_t timestamp;
    
    PersonLoginEvent(const Person& p, const std::string& session = "") 
        : person(p), sessionId(session), timestamp(std::time(nullptr)) {}
};

struct PersonLogoutEvent {
    const Person& person;
    std::string sessionId;
    std::time_t timestamp;
    
    PersonLogoutEvent(const Person& p, const std::string& session = "") 
        : person(p), sessionId(session), timestamp(std::time(nullptr)) {}
};

// Need to load and save Persons based on data saved in txt and json files.
// If its "logging in," a Person should be created in the memory via loading.
// If its "singing up", new Person data should first be added to the new txt and json files, and only then should Person created.
Person::Person(Soul& soul, Body& body) : _soul(soul), body(body) {

}




void Person::createDefaultAnimations() {
    // Idle animation
    Animation idle;
    idle.name = "Idle";
    idle.duration = 2.0f;
    idle.isLooping = true;
    
    // Simple breathing motion
    idle.keyframes["Torso"] = {
        glm::vec3(0.0f, 0.3f, 0.0f),
        glm::vec3(0.0f, 0.32f, 0.0f),
        glm::vec3(0.0f, 0.3f, 0.0f)
    };
    
    animations.push_back(idle);
    
    // Walk animation
    Animation walk;
    walk.name = "Walk";
    walk.duration = 1.0f;
    walk.isLooping = true;
    
    // Arm swing
    walk.keyframes["LeftArm"] = {
        glm::vec3(-0.35f, 0.25f, 0.0f),
        glm::vec3(-0.35f, 0.25f, 0.1f),
        glm::vec3(-0.35f, 0.25f, 0.0f),
        glm::vec3(-0.35f, 0.25f, -0.1f)
    };
    
    walk.keyframes["RightArm"] = {
        glm::vec3(0.35f, 0.25f, 0.0f),
        glm::vec3(0.35f, 0.25f, -0.1f),
        glm::vec3(0.35f, 0.25f, 0.0f),
        glm::vec3(0.35f, 0.25f, 0.1f)
    };
    
    animations.push_back(walk);
}

void Person::express() const {
    std::cout << "\n✨ Person: " << soulName << std::endl;
    std::cout << "   Level: " << state.level << " (XP: " << state.experience << ")" << std::endl;
    std::cout << "   Health: " << state.health << "/" << state.maxHealth << std::endl;
    std::cout << "   Energy: " << state.energy << "/" << state.maxEnergy << std::endl;
    std::cout << "   Mood: " << state.mood << std::endl;
    std::cout << "   Friends: " << state.friends << ", Reputation: " << state.reputation << std::endl;
    body.describe();
}

// TODO: Replace this with the Object system.
static void drawUnitCube() {
    glBegin(GL_QUADS);
    // Front
    glVertex3f(-0.5f, -0.5f,  0.5f);
    glVertex3f( 0.5f, -0.5f,  0.5f);
    glVertex3f( 0.5f,  0.5f,  0.5f);
    glVertex3f(-0.5f,  0.5f,  0.5f);
    // Back
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glVertex3f(-0.5f,  0.5f, -0.5f);
    glVertex3f( 0.5f,  0.5f, -0.5f);
    glVertex3f( 0.5f, -0.5f, -0.5f);
    // Left
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glVertex3f(-0.5f, -0.5f,  0.5f);
    glVertex3f(-0.5f,  0.5f,  0.5f);
    glVertex3f(-0.5f,  0.5f, -0.5f);
    // Right
    glVertex3f(0.5f, -0.5f, -0.5f);
    glVertex3f(0.5f,  0.5f, -0.5f);
    glVertex3f(0.5f,  0.5f,  0.5f);
    glVertex3f(0.5f, -0.5f,  0.5f);
    // Top
    glVertex3f(-0.5f, 0.5f, -0.5f);
    glVertex3f(-0.5f, 0.5f,  0.5f);
    glVertex3f( 0.5f, 0.5f,  0.5f);
    glVertex3f( 0.5f, 0.5f, -0.5f);
    // Bottom
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glVertex3f( 0.5f, -0.5f, -0.5f);
    glVertex3f( 0.5f, -0.5f,  0.5f);
    glVertex3f(-0.5f, -0.5f,  0.5f);
    glEnd();
}

void Person::draw() const {
    // Body parts already carry their absolute transforms (updated via updatePose),
    // so an extra translation would double-apply position and make the avatar
    // appear to move faster than the camera. Simply draw the body parts.
    body.draw();
}

// ---------------------------------------------------------------------------------
//  Render a simple nametag above the player's head using stb_easy_font
// ---------------------------------------------------------------------------------
void Person::drawNametag() const {
    // Offset above the head where the nametag should appear (world space)
    const float tagHeight = body.getNametagHeight();

    // Fetch current matrices and viewport
    GLdouble model[16], proj[16];
    GLint viewport[4];
    glGetDoublev(GL_MODELVIEW_MATRIX, model);
    glGetDoublev(GL_PROJECTION_MATRIX, proj);
    glGetIntegerv(GL_VIEWPORT, viewport);

    // Project 3D position (above head) to 2D window coordinates
    GLdouble winX, winY, winZ;
    gluProject(position.x, position.y + tagHeight, position.z,
               model, proj, viewport, &winX, &winY, &winZ);

    // Skip if projected behind camera
    if (winZ < 0.0 || winZ > 1.0) return;

    // Convert Y to top-left origin expected by stb_easy_font
    winY = viewport[3] - winY;

    // Prepare string buffer
    char buf[6000];
    std::string line = soulName;
    int quads = stb_easy_font_print(static_cast<float>(winX), static_cast<float>(winY),
                                    const_cast<char*>(line.c_str()), nullptr,
                                    buf, sizeof(buf));

    // Render in 2D overlay
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, viewport[2], viewport[3], 0, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor3f(1.0f, 1.0f, 1.0f);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 16, buf);
    glDrawArrays(GL_QUADS, 0, quads * 4);
    glDisableClientState(GL_VERTEX_ARRAY);

    // Restore matrices and state
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopAttrib();
}

// -----------------------------------------------------------------------------
void Person::updatePose() {
    glm::mat4 base = glm::translate(glm::mat4(1.0f), position);

    std::unordered_map<std::string, BodyPart*> partsByName;
    partsByName.reserve(body.parts.size());
    for (auto* part : body.parts) {
        if (part) partsByName[part->getName()] = part;
    }

    // Temporary rig resolver: this hard-coded parent chain gives body parts
    // immediate inherited motion, but the real "stickiness" of joints should
    // eventually be expressed through Relation/Bonds. As SpatialKind grows from
    // whole shapes into explicit points, edges, anchors, and surfaces, limbs
    // should connect by relational constraints between those spatial features
    // instead of by a separate bespoke skeleton system.
    auto parentNameFor = [](const std::string& name) -> const char* {
        if (name == "Head") return "Neck";
        if (name == "Neck") return "Torso";
        if (name == "Chest" || name == "Stomach") return "Torso";
        if (name == "LowerTorso") return "Stomach";

        if (name == "LeftShoulder" || name == "RightShoulder") return "Chest";
        if (name == "LeftArm") return "LeftShoulder";
        if (name == "RightArm") return "RightShoulder";
        if (name == "LeftForeArm") return "LeftArm";
        if (name == "RightForeArm") return "RightArm";
        if (name == "LeftHand") return "LeftForeArm";
        if (name == "RightHand") return "RightForeArm";

        if (name == "LeftLeg" || name == "RightLeg") return "LowerTorso";
        if (name == "LeftForeLeg") return "LeftLeg";
        if (name == "RightForeLeg") return "RightLeg";
        if (name == "LeftFoot") return "LeftForeLeg";
        if (name == "RightFoot") return "RightForeLeg";

        return nullptr;
    };

    std::unordered_map<BodyPart*, glm::mat4> resolvedWorld;
    resolvedWorld.reserve(body.parts.size());

    std::function<glm::mat4(BodyPart*)> resolvePart = [&](BodyPart* part) -> glm::mat4 {
        if (!part) return base;
        auto cached = resolvedWorld.find(part);
        if (cached != resolvedWorld.end()) return cached->second;

        const glm::mat4 restLocal = part->localTransform();
        const glm::mat4 animatedLocal = part->hasAutomations()
            ? part->sampleAutomations(restLocal)
            : restLocal;

        glm::mat4 worldT = base * animatedLocal;
        if (const char* parentName = parentNameFor(part->getName())) {
            auto parentIt = partsByName.find(parentName);
            if (parentIt != partsByName.end() && parentIt->second && parentIt->second != part) {
                BodyPart* parent = parentIt->second;
                const glm::mat4 parentWorld = resolvePart(parent);
                const glm::mat4 childFromParentRest = glm::inverse(parent->localTransform()) * animatedLocal;
                worldT = parentWorld * childFromParentRest;
            }
        }

        resolvedWorld[part] = worldT;
        return worldT;
    };

    for (auto* part : body.parts) {
        if (part) part->setTransform(resolvePart(part));
    }
}

void Person::updateBodyAutomations(float deltaTime) {
    for (auto* part : body.parts) {
        if (!part || !part->hasAutomations()) continue;
        // The authored local pose is the rest that animated channels build on.
        part->setAutomationRest(part->localTransform());
        part->advanceAutomations(deltaTime);
    }
}

void Person::stopBodyAutomations() {
    for (auto* part : body.parts) {
        if (part) part->clearAutomations();
    }
    _idleActive = false;
    _walkActive = false;
}

void Person::playIdleAutomation() {
    stopBodyAutomations();
    _idleActive = true;

    for (auto* part : body.parts) {
        if (!part) continue;
        const float sideX = part->localTransform()[3].x;  // <0 left, >0 right
        Automation::Clip clip;
        clip.name = "idle";
        clip.loop = true;

        switch (part->getType()) {
            case BodyPart::Type::Torso:
            case BodyPart::Type::Organ: {
                // Slow breathing: chest rises and swells a touch.
                Automation::Track sclY{Automation::Channel::SclY, Automation::Wave::Sine, 0.025f, 0.3f, 0.0f, 0.0f};
                Automation::Track posY{Automation::Channel::PosY, Automation::Wave::Sine, 0.015f, 0.3f, 0.0f, 0.0f};
                clip.tracks = {sclY, posY};
                break;
            }
            case BodyPart::Type::Head: {
                Automation::Track sway{Automation::Channel::RotY, Automation::Wave::Sine, 5.0f, 0.18f, 0.0f, 0.0f};
                clip.tracks = {sway};
                break;
            }
            case BodyPart::Type::Arm: {
                // Arms drift gently outward/in, opposite on each side.
                Automation::Track sway{Automation::Channel::RotZ, Automation::Wave::Sine, 3.0f, 0.25f, sideX < 0.0f ? 0.0f : 0.5f, 0.0f};
                clip.tracks = {sway};
                break;
            }
            default:
                continue;  // legs/feet stay planted while idle
        }

        part->addAutomation(clip);
        part->setAutomationRest(part->localTransform());
    }
}

void Person::playWalkAutomation(float speed) {
    // Map metres/second of travel to a stride tempo (cycles per second).
    const float tempo = glm::clamp(speed * 0.9f, 0.8f, 3.2f);

    if (!_walkActive) {
        stopBodyAutomations();
        _walkActive = true;

        for (auto* part : body.parts) {
            if (!part) continue;
            const float sideX = part->localTransform()[3].x;  // <0 left, >0 right
            Automation::Clip clip;
            clip.name = "walk";
            clip.loop = true;

            switch (part->getType()) {
                case BodyPart::Type::Leg: {
                    Automation::Track swing{Automation::Channel::RotX, Automation::Wave::Sine, 26.0f, 1.0f, sideX < 0.0f ? 0.0f : 0.5f, 0.0f};
                    clip.tracks = {swing};
                    break;
                }
                case BodyPart::Type::Foot: {
                    Automation::Track swing{Automation::Channel::RotX, Automation::Wave::Sine, 12.0f, 1.0f, sideX < 0.0f ? 0.0f : 0.5f, 0.0f};
                    clip.tracks = {swing};
                    break;
                }
                case BodyPart::Type::Arm: {
                    // Arms swing opposite to the same-side leg.
                    Automation::Track swing{Automation::Channel::RotX, Automation::Wave::Sine, 18.0f, 1.0f, sideX < 0.0f ? 0.5f : 0.0f, 0.0f};
                    clip.tracks = {swing};
                    break;
                }
                case BodyPart::Type::Torso: {
                    // Subtle vertical bob in step with the stride (twice per cycle).
                    Automation::Track bob{Automation::Channel::PosY, Automation::Wave::Sine, 0.02f, 2.0f, 0.0f, 0.0f};
                    clip.tracks = {bob};
                    break;
                }
                default:
                    continue;
            }

            part->addAutomation(clip);
            part->setAutomationRest(part->localTransform());
        }
    }

    // Keep the stride tempo in sync with current travel speed without
    // rebuilding (which would reset the clip clocks and stutter the cycle).
    for (auto* part : body.parts) {
        if (!part) continue;
        for (auto& clip : part->automationState().clips) {
            if (clip.name == "walk") clip.speed = tempo;
        }
    }
}

void Person::setLocomotion(bool moving, float speed) {
    if (moving) {
        playWalkAutomation(speed);
    } else if (_walkActive || !_idleActive) {
        // Only rebuild on the walk->idle transition; otherwise leave the idle
        // clocks running so the breathing cycle doesn't restart every frame.
        playIdleAutomation();
    }
}

void Person::installLocomotionRouting() {
    static bool installed = false;
    if (installed) return;
    installed = true;
    // One subscription for all Persons: the event names its own target, so we
    // avoid per-Person subscriptions (Core::EventBus has no unsubscribe).
    Core::EventBus::instance().subscribe<LocomotionChanged>([](const LocomotionChanged& e) {
        if (e.person) e.person->setLocomotion(e.moving, e.speed);
    });
}

void Person::update(float deltaTime) {
    updateState(deltaTime);
    updateAnimation(deltaTime);
    updateBodyAutomations(deltaTime);
    updatePhysics(deltaTime);
    updatePose();
}

void Person::updateState(float deltaTime) {
    // Natural state changes over time
    if (mode == GameMode::Survival) {
        // Hunger and thirst increase over time
        state.hunger += deltaTime * 0.1f;  // Hunger per second
        state.thirst += deltaTime * 0.15f; // Thirst per second
        
        // Energy decreases with hunger and thirst
        if (state.hunger > 50.0f || state.thirst > 50.0f) {
            state.energy -= deltaTime * 2.0f;
        }
        
        // Health decreases with extreme hunger/thirst
        if (state.hunger > 90.0f || state.thirst > 90.0f) {
            modifyHealth(-deltaTime * 5.0f);
        }
        
        // Natural energy regeneration
        if (state.hunger < 30.0f && state.thirst < 30.0f) {
            state.energy += deltaTime * 5.0f;
        }
    }
    
    // Clamp values
    state.hunger = std::min(state.hunger, 100.0f);
    state.thirst = std::min(state.thirst, 100.0f);
    state.energy = std::clamp(state.energy, 0.0f, state.maxEnergy);
    state.mood = std::clamp(state.mood, -100.0f, 100.0f);
}

void Person::modifyHealth(float amount) {
    state.health += amount;
    state.health = std::clamp(state.health, 0.0f, state.maxHealth);
    
    if (amount < 0) {
        // Damage taken - affect mood
        state.mood -= std::abs(amount) * 0.1f;
    }
}

void Person::modifyEnergy(float amount) {
    state.energy += amount;
    state.energy = std::clamp(state.energy, 0.0f, state.maxEnergy);
}

// Session and Zone Management Methods
void Person::login(const std::string& sessionId) {
    if (!_isLoggedIn) {
        _isLoggedIn = true;
        _currentSession = sessionId.empty() ? "session_" + std::to_string(std::time(nullptr)) : sessionId;
        
        // Trigger PersonLoginEvent
        PersonLoginEvent event(*this, _currentSession);
        Core::EventBus::instance().publish(event);
        
        std::cout << "👤 " << soulName << " logged in (Session: " << _currentSession << ")" << std::endl;
    }
}

void Person::logout(const std::string& sessionId) {
    if (_isLoggedIn) {
        std::string session = sessionId.empty() ? _currentSession : sessionId;
        
        // Trigger PersonLogoutEvent
        PersonLogoutEvent event(*this, session);
        Core::EventBus::instance().publish(event);
        
        _isLoggedIn = false;
        _currentSession.clear();
        
        std::cout << "👤 " << soulName << " logged out (Session: " << session << ")" << std::endl;
    }
}

void Person::joinZone(const std::string& zoneName) {
    // Check if already in this zone
    auto it = std::find(_joinedZones.begin(), _joinedZones.end(), zoneName);
    if (it == _joinedZones.end()) {
        _joinedZones.push_back(zoneName);
        
        // Trigger PersonJoinedEvent
        PersonJoinedEvent event(*this, zoneName);
        Core::EventBus::instance().publish(event);
        
        std::cout << "👤 " << soulName << " joined zone: " << zoneName << std::endl;
    }
}

void Person::leaveZone(const std::string& zoneName) {
    auto it = std::find(_joinedZones.begin(), _joinedZones.end(), zoneName);
    if (it != _joinedZones.end()) {
        _joinedZones.erase(it);
        std::cout << "👤 " << soulName << " left zone: " << zoneName << std::endl;
    }
}

void Person::modifyMood(float amount) {
    state.mood += amount;
    state.mood = std::clamp(state.mood, -100.0f, 100.0f);
}

void Person::addExperience(float amount) {
    state.experience += amount;
    
    // Check for level up
    float xpNeeded = state.level * 100.0f;  // Simple XP formula
    if (state.experience >= xpNeeded) {
        levelUp();
    }
}

void Person::levelUp() {
    state.level++;
    state.experience = 0.0f;
    state.maxHealth += 10.0f;
    state.maxEnergy += 5.0f;
    state.health = state.maxHealth;  // Full heal on level up
    state.energy = state.maxEnergy;
    state.mood += 20.0f;  // Happy about leveling up
    
    std::cout << "🎉 " << soulName << " reached level " << state.level << "!" << std::endl;
}

void Person::addSkill(const std::string& skillName, float value) {
    state.skills[skillName] += value;
    if (state.skills[skillName] > 100.0f) {
        state.skills[skillName] = 100.0f;
    }
}

float Person::getSkill(const std::string& skillName) const {
    auto it = state.skills.find(skillName);
    return (it != state.skills.end()) ? it->second : 0.0f;
}

void Person::addAnimation(const Animation& anim) {
    animations.push_back(anim);
}

void Person::playAnimation(const std::string& name, bool loop) {
    for (auto& anim : animations) {
        if (anim.name == name) {
            currentAnimation = &anim;
            anim.isPlaying = true;
            anim.isLooping = loop;
            anim.currentTime = 0.0f;
            return;
        }
    }
}

void Person::stopAnimation() {
    if (currentAnimation) {
        currentAnimation->isPlaying = false;
        currentAnimation = nullptr;
    }
}

void Person::updateAnimation(float deltaTime) {
    if (!currentAnimation || !currentAnimation->isPlaying) return;
    
    currentAnimation->currentTime += deltaTime;
    
    if (currentAnimation->currentTime >= currentAnimation->duration) {
        if (currentAnimation->isLooping) {
            currentAnimation->currentTime = 0.0f;
        } else {
            stopAnimation();
            return;
        }
    }
    
    // Apply animation to body parts
    float progress = currentAnimation->currentTime / currentAnimation->duration;
    
    for (auto& part : body.parts) {
        if (!part) continue;
        
        auto keyframeIt = currentAnimation->keyframes.find(part->getName());
        if (keyframeIt != currentAnimation->keyframes.end()) {
            const auto& keyframes = keyframeIt->second;
            if (keyframes.size() > 1) {
                // Interpolate between keyframes
                float keyframeIndex = progress * (keyframes.size() - 1);
                int index1 = static_cast<int>(keyframeIndex);
                int index2 = std::min(index1 + 1, static_cast<int>(keyframes.size() - 1));
                float t = keyframeIndex - index1;
                
                glm::vec3 pos = glm::mix(keyframes[index1], keyframes[index2], t);
                glm::mat4 newTransform = glm::translate(glm::mat4(1.0f), pos);
                part->setLocalTransform(newTransform);
            }
        }
    }
}

void Person::interactWith(Person* other) {
    if (!other || !isNearby(other)) return;
    
    // Basic interaction - increase friendship
    state.friends++;
    other->state.friends++;
    
    // Mood boost from social interaction
    modifyMood(10.0f);
    other->modifyMood(10.0f);
    
    // Experience gain
    addExperience(5.0f);
    other->addExperience(5.0f);
    
    std::cout << soulName << " interacted with " << other->soulName << std::endl;
}

void Person::addNearbyAvatar(Person* avatar) {
    if (avatar && avatar != this) {
        auto it = std::find(nearbyAvatars.begin(), nearbyAvatars.end(), avatar);
        if (it == nearbyAvatars.end()) {
            nearbyAvatars.push_back(avatar);
        }
    }
}

void Person::removeNearbyAvatar(Person* avatar) {
    auto it = std::find(nearbyAvatars.begin(), nearbyAvatars.end(), avatar);
    if (it != nearbyAvatars.end()) {
        nearbyAvatars.erase(it);
    }
}

bool Person::isNearby(Person* other) const {
    if (!other) return false;
    float distance = glm::length(position - other->position);
    return distance <= interactionRange;
}

bool Person::addToInventory(const std::string& item) {
    if (inventory.size() >= maxInventorySize) {
        return false;  // Inventory full
    }
    inventory.push_back(item);
    return true;
}

bool Person::removeFromInventory(const std::string& item) {
    auto it = std::find(inventory.begin(), inventory.end(), item);
    if (it != inventory.end()) {
        inventory.erase(it);
        return true;
    }
    return false;
}

bool Person::hasItem(const std::string& item) const {
    return std::find(inventory.begin(), inventory.end(), item) != inventory.end();
}

void Person::setHairStyle(const std::string& style) {
    state.hairStyle = style;
}

void Person::setEyeColor(const std::string& color) {
    state.eyeColor = color;
}

void Person::setSkinTone(const std::string& tone) {
    state.skinTone = tone;
}

void Person::setHeight(float h) {
    state.height = h;
    // Update body scale
    for (auto* part : body.parts) {
        if (part) {
            glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, h, 1.0f));
            part->setLocalTransform(scale * part->localTransform());
        }
    }
}

void Person::setWeight(float w) {
    state.weight = w;
}

void Person::applyForce(const glm::vec3& force) {
    if (physicsEnabled) {
        acceleration += force / state.weight;  // F = ma
    }
}

void Person::setVelocity(const glm::vec3& vel) {
    velocity = vel;
}

void Person::updatePhysics(float deltaTime) {
    if (!physicsEnabled) return;
    
    // Update velocity
    velocity += acceleration * deltaTime;
    
    // Apply damping
    velocity *= 0.95f;
    
    // Update position
    position += velocity * deltaTime;
    
    // Reset acceleration
    acceleration = glm::vec3(0.0f);
    
    // Simple ground collision
    if (position.y < 0.0f) {
        position.y = 0.0f;
        velocity.y = 0.0f;
    }
}
