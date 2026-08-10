#include "Person.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <functional>
#include <unordered_map>
#include <GLFW/glfw3.h>
#include "Rendering/GL/GluCompat.hpp"
#include "Rendering/Renderer.hpp"
#include "Form/Object/Formation/Menu/stb_easy_font.h"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "PersonEvents.hpp"
#include "Form/Singular/Property/ComputedProperty.hpp"
#include "Form/Singular/Property/PropertyRef.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include "Singularity/Network/WebSocketClient.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Forward-declare global ZoneManager defined in main.cpp
extern ZoneManager mgr;

// PersonJoinedEvent/PersonLeftZoneEvent/PersonLoginEvent/PersonLogoutEvent
// are defined in PersonEvents.hpp (not here), so external code can
// subscribe<>() to them.

// Need to load and save Persons based on data saved in txt and json files.
// If its "logging in," a Person should be created in the memory via loading.
// If its "singing up", new Person data should first be added to the new txt and json files, and only then should Person created.
void Person::buildProperties() {
    _propertyRegistry.push_back(std::make_unique<PropertyRef<Person, glm::vec3>>(
        "position", this, &Person::position));
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<Person, std::string>>(
        "name", this, &Person::propName));   // read-only: identity is not a slot
        
    // --- Law System Perception Properties ---
    _propertyRegistry.push_back(std::make_unique<PropertyRef<Person, std::string>>(
        "activeTool", this, &Person::activeTool));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<Person, std::string>>(
        "active3DMode", this, &Person::active3DMode));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<Person, glm::vec3>>(
        "cursorHitPos", this, &Person::cursorHitPos));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<Person, glm::vec3>>(
        "cursorHitNormal", this, &Person::cursorHitNormal));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<Person, glm::vec3>>(
        "cursorSpawnPos", this, &Person::cursorSpawnPos));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<Person, glm::vec3>>(
        "cursorSpawnRot", this, &Person::cursorSpawnRot));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<Person, glm::vec3>>(
        "cursorSpawnScale", this, &Person::cursorSpawnScale));
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<Person, glm::mat4>>(
        "cursorSpawnTransform", this, &Person::getCursorSpawnTransform, nullptr));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<Person, glm::vec3>>(
        "cameraPos", this, &Person::cameraPos));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<Person, glm::vec3>>(
        "cameraForward", this, &Person::cameraForward));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<Person, int>>(
        "activeShapeKind", this, &Person::activeShapeKind));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<Person, glm::vec3>>(
        "activeColor", this, &Person::activeColor));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<Person, std::string>>(
        "placementMode", this, &Person::placementMode));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<Person, float>>(
        "gridSnapSize", this, &Person::gridSnapSize));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<Person, bool>>(
        "gridSnap", this, &Person::gridSnap));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<Person, float>>(
        "inFrontDistance", this, &Person::inFrontDistance));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<Person, glm::vec3>>(
        "manualOffset", this, &Person::manualOffset));
}

float Person::spawnSurfaceOffset(const glm::vec3& normal) const {
    // Mirrors Game::getBrushCreateSurfaceOffset: project the spawn box's half
    // extents onto the surface normal, so the shape is pushed out by exactly
    // its own reach and sits flush instead of sinking in.
    const glm::vec3 n = glm::length(normal) > 1e-6f ? glm::normalize(normal)
                                                    : glm::vec3(0.0f, 1.0f, 0.0f);
    glm::mat4 rotation(1.0f);
    rotation = glm::rotate(rotation, glm::radians(cursorSpawnRot.x), glm::vec3(1.0f, 0.0f, 0.0f));
    rotation = glm::rotate(rotation, glm::radians(cursorSpawnRot.y), glm::vec3(0.0f, 1.0f, 0.0f));
    rotation = glm::rotate(rotation, glm::radians(cursorSpawnRot.z), glm::vec3(0.0f, 0.0f, 1.0f));

    const glm::vec3 half = cursorSpawnScale * 0.5f;
    const glm::vec3 axisX = glm::normalize(glm::vec3(rotation * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f)));
    const glm::vec3 axisY = glm::normalize(glm::vec3(rotation * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f)));
    const glm::vec3 axisZ = glm::normalize(glm::vec3(rotation * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f)));

    return std::abs(glm::dot(n, axisX)) * half.x +
           std::abs(glm::dot(n, axisY)) * half.y +
           std::abs(glm::dot(n, axisZ)) * half.z;
}

glm::vec3 Person::computeSpawnPosition() const {
    glm::vec3 spawnPos;

    if (placementMode == "ManualDistance") {
        // Relative to the frozen anchor, not the live camera: the whole point
        // of the mode is that the shape stays where it was put while you look
        // around. GameUpdate captures the anchor on entering the mode.
        spawnPos = manualAnchorPos +
                   manualAnchorRight * manualOffset.x +
                   manualAnchorUp * manualOffset.y +
                   manualAnchorForward * manualOffset.z;
    } else if (placementMode == "CursorSnap") {
        // Against the surface under the cursor, offset along its normal so
        // the shape rests on it. Falls back to the InFront placement when
        // nothing was hit, which is what the hard-coded tool did.
        spawnPos = cursorHitPos + cursorHitNormal * spawnSurfaceOffset(cursorHitNormal);
    } else {   // "InFront"
        spawnPos = cameraPos + cameraForward * inFrontDistance;
    }

    // Snapping rounds whatever the mode produced -- it is a toggle, not a mode.
    if (gridSnap && gridSnapSize > 1e-6f) {
        spawnPos.x = std::round(spawnPos.x / gridSnapSize) * gridSnapSize;
        spawnPos.y = std::round(spawnPos.y / gridSnapSize) * gridSnapSize;
        spawnPos.z = std::round(spawnPos.z / gridSnapSize) * gridSnapSize;
    }
    return spawnPos;
}

void Person::updatePlacement() {
    cursorSpawnPos = computeSpawnPosition();
}

glm::mat4 Person::getCursorSpawnTransform() const {
    // cursorSpawnPos is the answer, not a hint: updatePlacement() derives it
    // from placementMode once per frame, and a law is free to overwrite it
    // afterwards (which is the documented contract -- see GameUpdate). This
    // used to re-derive the mode math here instead, which meant a law's
    // cursorSpawnPos was ignored in two of the three modes, and the third
    // treated an exact vec3(0) as "unset" -- so the world origin was not a
    // placeable position.
    glm::mat4 t = glm::translate(glm::mat4(1.0f), cursorSpawnPos);
    t = glm::rotate(t, glm::radians(cursorSpawnRot.x), glm::vec3(1.0f, 0.0f, 0.0f));
    t = glm::rotate(t, glm::radians(cursorSpawnRot.y), glm::vec3(0.0f, 1.0f, 0.0f));
    t = glm::rotate(t, glm::radians(cursorSpawnRot.z), glm::vec3(0.0f, 0.0f, 1.0f));
    t = glm::scale(t, cursorSpawnScale);
    return t;
}

Person::Person(Soul soul, Body body, const std::string& joyOrdering) : _soul(std::move(soul)), body(std::move(body)) {
    // The soul's identity seeds the Person's NAME, not their identity. Left
    // unset it was the empty string — invisible in law authorship records and
    // unmatchable when a saved world reattaches authors by identifier.
    // The cryptographic personId is assigned separately (setPersonId), because
    // minting one here would give every temporary copy its own identity.
    displayName = _soul.getIdentifier();
    if (displayName.empty()) displayName = "Person";
    
    // Validate that a joy ordering is provided, fulfilling the Singularity
    // level requirement that Persons have a worship-ordering.
    if (joyOrdering.empty()) {
        std::cerr << "[WARNING] Person instantiated without a Joy-Ordering." << std::endl;
    }
}

nlohmann::json Person::serialize() const {
    nlohmann::json j;
    j["displayName"] = displayName;
    // "soulName" is still written so a save from here still opens in a build
    // from before the split. It is a label in both directions and never the
    // identity, so emitting it grants nothing.
    j["soulName"] = displayName;
    if (_personId.canAuthenticate()) j["personId"] = _personId.toString();
    j["position"] = {position.x, position.y, position.z};
    j["velocity"] = {velocity.x, velocity.y, velocity.z};
    j["grounded"] = grounded;
    j["wasGrounded"] = wasGrounded;
    j["wasMoving"] = wasMoving;
    j["jumpKeyDownLast"] = jumpKeyDownLast;
    return j;
}

void Person::deserialize(const nlohmann::json& j) {
    // Type-checked: this reads save data, which is untrusted by construction.
    // Prefer the new key, fall back to the pre-split one.
    if (j.contains("displayName") && j["displayName"].is_string()) {
        displayName = j["displayName"].get<std::string>();
    } else if (j.contains("soulName") && j["soulName"].is_string()) {
        displayName = j["soulName"].get<std::string>();
    }

    // A personId read from a file is only a CLAIM to that identity. Parsing it
    // records who this Person says they are; it proves nothing on its own,
    // because the public key is public — anyone can copy one into a save. What
    // makes it binding is a signed Claim verified against it, which is the
    // authority layer's job, not this one's. Legacy saves simply have none,
    // and migration mints a key for them on first load.
    if (j.contains("personId") && j["personId"].is_string()) {
        Identity::SingularId claimed =
            Identity::SingularId::parse(j["personId"].get<std::string>());
        if (claimed.canAuthenticate()) _personId = claimed;
    }
    if (j.contains("position") && j["position"].is_array() && j["position"].size() >= 3) {
        position = glm::vec3(j["position"][0], j["position"][1], j["position"][2]);
    }
    if (j.contains("velocity") && j["velocity"].is_array() && j["velocity"].size() >= 3) {
        velocity = glm::vec3(j["velocity"][0], j["velocity"][1], j["velocity"][2]);
    }
    if (j.contains("grounded") && j["grounded"].is_boolean()) {
        grounded = j["grounded"].get<bool>();
    }
    if (j.contains("wasGrounded") && j["wasGrounded"].is_boolean()) {
        wasGrounded = j["wasGrounded"].get<bool>();
    }
    if (j.contains("wasMoving") && j["wasMoving"].is_boolean()) {
        wasMoving = j["wasMoving"].get<bool>();
    }
    if (j.contains("jumpKeyDownLast") && j["jumpKeyDownLast"].is_boolean()) {
        jumpKeyDownLast = j["jumpKeyDownLast"].get<bool>();
    }
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
    std::cout << "\n✨ Person: " << displayName << std::endl;
    body.describe();
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

    // Read the camera off the renderer rather than out of the GL matrix stack —
    // portable, and independent of what the stack happens to hold right now.
    const Renderer& r = currentRenderer();
    const glm::ivec4& vp = r.viewport();
    double model[16], proj[16];
    int viewport[4] = {vp.x, vp.y, vp.z, vp.w};
    for (int i = 0; i < 16; ++i) {
        model[i] = static_cast<double>(glm::value_ptr(r.view())[i]);
        proj[i]  = static_cast<double>(glm::value_ptr(r.proj())[i]);
    }

    // Project 3D position (above head) to 2D window coordinates
    double winX, winY, winZ;
    ecgl::project(position.x, position.y + tagHeight, position.z,
                  model, proj, viewport, &winX, &winY, &winZ);

    // Skip if projected behind camera
    if (winZ < 0.0 || winZ > 1.0) return;

    // Convert Y to top-left origin expected by stb_easy_font
    winY = viewport[3] - winY;

    // Prepare string buffer
    char buf[6000];
    std::string line = displayName;
    int quads = stb_easy_font_print(static_cast<float>(winX), static_cast<float>(winY),
                                    const_cast<char*>(line.c_str()), nullptr,
                                    buf, sizeof(buf));

    // Render in 2D overlay
    Renderer& rw = currentRenderer();
    rw.begin2D(static_cast<uint32_t>(viewport[2]), static_cast<uint32_t>(viewport[3]));
    rw.drawTris2D(draw::easyFontToTris(buf, quads), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    rw.end2D();
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
            case BodyPart::Type::Torso: {
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
    updateAnimation(deltaTime);
    updateBodyAutomations(deltaTime);
    updatePhysics(deltaTime);
    updatePose();
}

// Session and Zone Management Methods
void Person::login(const std::string& sessionId) {
    if (!_isLoggedIn) {
        _isLoggedIn = true;
        _currentSession = sessionId.empty() ? "session_" + std::to_string(std::time(nullptr)) : sessionId;
        
        // Trigger PersonLoginEvent
        PersonLoginEvent event(*this, _currentSession);
        Core::EventBus::instance().publish(event);
        Core::EventBus::instance().publish(ECA::Event{"person-logged-in", this, nullptr, std::time(nullptr)});

        std::cout << "👤 " << displayName << " logged in (Session: " << _currentSession << ")" << std::endl;
    }
}

void Person::logout(const std::string& sessionId) {
    if (_isLoggedIn) {
        std::string session = sessionId.empty() ? _currentSession : sessionId;
        
        // Trigger PersonLogoutEvent
        PersonLogoutEvent event(*this, session);
        Core::EventBus::instance().publish(event);
        Core::EventBus::instance().publish(ECA::Event{"person-logged-out", this, nullptr, std::time(nullptr)});

        _isLoggedIn = false;
        _currentSession.clear();
        
        std::cout << "👤 " << displayName << " logged out (Session: " << session << ")" << std::endl;
    }
}

// The zone is a being now: resolve the name so join/leave events can carry
// it as the event OBJECT — "@event.object.owner" etc. testify in laws.
static Zone* zoneByName(const std::string& zoneName) {
    for (auto& z : mgr.zones()) {
        if (z.name() == zoneName) return &z;
    }
    return nullptr;
}

void Person::joinZone(const std::string& zoneName) {
    // Check if already in this zone
    auto it = std::find(_joinedZones.begin(), _joinedZones.end(), zoneName);
    if (it == _joinedZones.end()) {
        _joinedZones.push_back(zoneName);

        // Trigger PersonJoinedEvent
        PersonJoinedEvent event(*this, zoneName);
        Core::EventBus::instance().publish(event);
        Core::EventBus::instance().publish(ECA::Event{"person-joined-zone", this, zoneByName(zoneName), std::time(nullptr)});

        std::cout << "👤 " << displayName << " joined zone: " << zoneName << std::endl;
    }
}

void Person::leaveZone(const std::string& zoneName) {
    auto it = std::find(_joinedZones.begin(), _joinedZones.end(), zoneName);
    if (it != _joinedZones.end()) {
        _joinedZones.erase(it);

        // Trigger PersonLeftZoneEvent
        PersonLeftZoneEvent event(*this, zoneName);
        Core::EventBus::instance().publish(event);
        Core::EventBus::instance().publish(ECA::Event{"person-left-zone", this, zoneByName(zoneName), std::time(nullptr)});

        std::cout << "👤 " << displayName << " left zone: " << zoneName << std::endl;
    }
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


void Person::applyForce(const glm::vec3& force) {
    acceleration += force / 70.0f;  // F = ma (assuming 70kg)
}

void Person::setVelocity(const glm::vec3& vel) {
    velocity = vel;
}

void Person::updatePhysics(float deltaTime) {
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

void Person::requestAIAction(const std::string& context, const std::string& targetObjectId) {
    nlohmann::json payload = {
        {"type", "request_ai_action"},
        {"context", context},
        {"target_singular_id", targetObjectId}
    };
    Singularity::Network::WebSocketClient::instance().send(payload.dump());
}
