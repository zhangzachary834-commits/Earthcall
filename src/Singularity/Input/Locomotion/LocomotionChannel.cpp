#include "LocomotionChannel.hpp"

#include "ConstructedBeing/Object/Automation/Automation.hpp"
#include "ConstructedBeing/Singular/Property/PropertyRef.hpp"
#include "Person/Body/BodyPart/BodyPart.hpp"
#include "Person/Person.hpp"
#include "Person/PersonEvents.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "Singularity/Screen/Camera.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"

#include <GLFW/glfw3.h>
#include <memory>
#include <algorithm>
#include <cmath>
#include <ctime>
#include <glm/glm.hpp>

namespace Singularity {
namespace Input {

LocomotionChannel::LocomotionChannel() = default;

void LocomotionChannel::syncRegister(LawManager& laws) {
    if (find(laws)) return;
    laws.add(std::make_shared<LocomotionChannel>());
}

LocomotionChannel* LocomotionChannel::find(LawManager& laws) {
    for (const auto& law : laws.getAll()) {
        if (auto* channel = dynamic_cast<LocomotionChannel*>(law.get())) {
            return channel;
        }
    }
    return nullptr;
}

void LocomotionChannel::buildProperties() {
    registerEnabledProperty();
    _propertyRegistry.push_back(std::make_unique<PropertyRef<LocomotionChannel, bool>>(
        "grounded", this, &LocomotionChannel::grounded));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<LocomotionChannel, bool>>(
        "moving", this, &LocomotionChannel::moving));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<LocomotionChannel, bool>>(
        "flying", this, &LocomotionChannel::flying));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<LocomotionChannel, bool>>(
        "canMove", this, &LocomotionChannel::canMove));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<LocomotionChannel, float>>(
        "speed", this, &LocomotionChannel::speed));
}

void LocomotionChannel::tickAutomations(Person& person, float dt) {
    for (auto* part : person.getBody().parts) {
        if (!part || !part->hasAutomations()) continue;
        part->setAutomationRest(part->localTransform());
        part->advanceAutomations(dt);
    }
}

void LocomotionChannel::stopClips(Person& person) {
    for (auto* part : person.getBody().parts) {
        if (part) part->clearAutomations();
    }
    _idleActive = false;
    _walkActive = false;
}

void LocomotionChannel::playIdle(Person& person) {
    stopClips(person);
    _idleActive = true;

    for (auto* part : person.getBody().parts) {
        if (!part) continue;
        const float sideX = part->localTransform()[3].x;
        Automation::Clip clip;
        clip.name = "idle";
        clip.loop = true;

        switch (part->getType()) {
            case BodyPart::Type::Torso: {
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
                Automation::Track sway{Automation::Channel::RotZ, Automation::Wave::Sine, 3.0f, 0.25f, sideX < 0.0f ? 0.0f : 0.5f, 0.0f};
                clip.tracks = {sway};
                break;
            }
            default:
                continue;
        }

        part->addAutomation(clip);
        part->setAutomationRest(part->localTransform());
    }
}

void LocomotionChannel::playWalk(Person& person, float travelSpeed) {
    const float tempo = glm::clamp(travelSpeed * 0.9f, 0.8f, 3.2f);

    if (!_walkActive) {
        stopClips(person);
        _walkActive = true;

        for (auto* part : person.getBody().parts) {
            if (!part) continue;
            const float sideX = part->localTransform()[3].x;
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
                    Automation::Track swing{Automation::Channel::RotX, Automation::Wave::Sine, 18.0f, 1.0f, sideX < 0.0f ? 0.5f : 0.0f, 0.0f};
                    clip.tracks = {swing};
                    break;
                }
                case BodyPart::Type::Torso: {
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

    for (auto* part : person.getBody().parts) {
        if (!part) continue;
        for (auto& clip : part->automationState().clips) {
            if (clip.name == "walk") clip.speed = tempo;
        }
    }
}

void LocomotionChannel::setLocomotion(Person& person, bool isMoving, float travelSpeed) {
    if (!isEnabled()) return;
    if (isMoving) {
        playWalk(person, travelSpeed);
    } else if (_walkActive || !_idleActive) {
        playIdle(person);
    }
}

void LocomotionChannel::installRouting() {
    if (_routingInstalled) return;
    _routingInstalled = true;
    Core::EventBus::instance().subscribe<LocomotionChanged>([this](const LocomotionChanged& e) {
        if (e.person) setLocomotion(*e.person, e.moving, e.speed);
    });
}

// Order each frame:
//   1. horizontal intent (WASD, pitch-flattened)  -> move on XZ
//   2. resolve object collisions HORIZONTALLY ONLY -> can't walk through walls
//   3. find the support height under the feet      -> floor or object top
//   4. vertical: fly input, or gravity + jump, clamped so feet never sink
//   5. write camera.pos once, then pose the body parts from it
//
// Vertical contact is owned solely here; collisions never push the player up.
void LocomotionChannel::step(Person& person, ::Core::Camera& camera, GLFWwindow* window,
                             ZoneManager& mgr, float dt, bool flyingNow, bool canMoveNow) {
    // First movers are the bootstrap, not a lock. A Person (or a metalaw)
    // writes enabled := false to set this one down; authored locomotion
    // can then own the vessel. Refuse out loud by doing nothing — do not
    // keep integrating WASD under the table.
    if (!isEnabled()) {
        if (_wasActuating) {
            stopClips(person);
            grounded = false;
            moving   = false;
            speed    = 0.0f;
            _wasActuating = false;
        }
        return;
    }
    _wasActuating = true;
    flying  = flyingNow;
    canMove = canMoveNow;

    const auto& objects = mgr.active().world().getOwnedObjects();
    const float eyeH = person.getBody().getEyeHeight();

    // Latch: person.position vs camera.pos - eyeH. A mismatch is a teleport:
    // this step adopts it by snapping the camera onto the Person. Writers that
    // move the camera without writing person.position (loadState,
    // loadTestObservation — see settlePersonToCamera in ZoneManager.cpp)
    // look like a no-op: the next frame undoes them. EngineInit writes both.
    glm::vec3 expectedPersonPos = camera.pos - glm::vec3(0.0f, eyeH, 0.0f);
    if (glm::distance(person.position, expectedPersonPos) > 1e-4f) {
        camera.pos = person.position + glm::vec3(0.0f, eyeH, 0.0f);
        person.velocity.y = 0.0f;
    }
    const glm::vec3 posBefore = camera.pos;
    const bool groundedLast = _wasGrounded;

    float actualSpeed = camera.speed;
    if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) actualSpeed *= 2.5f;
    if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) actualSpeed *= 0.3f;

    if (canMove) {
        glm::vec3 forwardXZ = glm::normalize(glm::vec3(camera.front.x, 0.0f, camera.front.z));
        if (glm::length(forwardXZ) < 1e-3f) forwardXZ = glm::vec3(0.0f, 0.0f, -1.0f);
        glm::vec3 rightXZ = glm::normalize(glm::cross(forwardXZ, camera.up));

        glm::vec3 move(0.0f);
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) move += forwardXZ;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) move -= forwardXZ;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) move += rightXZ;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) move -= rightXZ;
        if (glm::length(move) > 1e-4f) camera.pos += glm::normalize(move) * actualSpeed;
    }

    {
        constexpr float RADIUS = 0.3f;
        glm::vec3 rightVec  = glm::normalize(glm::cross(camera.front, camera.up));
        glm::vec3 forwardXZ = glm::normalize(glm::vec3(camera.front.x, 0.0f, camera.front.z));
        if (glm::length(forwardXZ) < 1e-3f) forwardXZ = glm::vec3(0.0f, 0.0f, 1.0f);
        glm::vec3 offsets[5] = { glm::vec3(0), rightVec*RADIUS, -rightVec*RADIUS,
                                 forwardXZ*RADIUS, -forwardXZ*RADIUS };
        for (const auto& off : offsets) {
            for (float h : { 0.0f, eyeH }) {
                glm::vec3 sample = camera.pos + off - glm::vec3(0.0f, h, 0.0f);
                glm::vec3 before = sample;
                Physics::enforceCollisions(sample, objects);
                glm::vec3 d = sample - before;
                d.y = 0.0f;
                camera.pos += d;
            }
        }
    }

    float supportY = 0.0f;
    {
        const float feetY = camera.pos.y - eyeH;
        const float standTol = 0.05f;
        for (const auto& up : objects) {
            if (!up) continue;
            up->updateCollisionZone(up->getTransform());
            glm::vec3 mn = up->collisionZone.corners[0], mx = mn;
            for (int i = 1; i < 8; ++i) {
                mn = glm::min(mn, up->collisionZone.corners[i]);
                mx = glm::max(mx, up->collisionZone.corners[i]);
            }
            if (camera.pos.x < mn.x || camera.pos.x > mx.x) continue;
            if (camera.pos.z < mn.z || camera.pos.z > mx.z) continue;
            if (mx.y <= feetY + standTol) supportY = std::max(supportY, mx.y);
        }
    }
    const float minEyeY = supportY + eyeH;

    const bool jumpKeyDown = canMove && glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    if (flying) {
        float vy = 0.0f;
        if (jumpKeyDown) vy += actualSpeed;
        if (canMove && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) vy -= actualSpeed;
        camera.pos.y += vy;
        person.velocity.y = 0.0f;
        grounded = false;
    } else {
        constexpr float GRAVITY = 9.81f;
        constexpr float JUMP_SPEED = 5.0f;
        if (jumpKeyDown && !_jumpKeyDownLast && grounded) {
            person.velocity.y = JUMP_SPEED;
            grounded = false;
            Core::EventBus::instance().publish(ECA::Event{"jump-started", &person, nullptr, std::time(nullptr)});
        }
        person.velocity.y -= GRAVITY * dt;
        camera.pos.y += person.velocity.y * dt;
    }
    _jumpKeyDownLast = jumpKeyDown;

    if (camera.pos.y <= minEyeY) {
        camera.pos.y = minEyeY;
        if (person.velocity.y < 0.0f) person.velocity.y = 0.0f;
        grounded = true;
    } else {
        grounded = !flying && (camera.pos.y - minEyeY) <= 1e-3f;
    }
    if (grounded && !groundedLast) {
        Core::EventBus::instance().publish(ECA::Event{"landed", &person, nullptr, std::time(nullptr)});
    }
    _wasGrounded = grounded;

    glm::vec3 horizDelta = camera.pos - posBefore;
    horizDelta.y = 0.0f;
    const float distance = glm::length(horizDelta);
    moving = distance > 1e-5f;
    speed  = (moving && dt > 1e-5f) ? distance / dt : 0.0f;
    Core::EventBus::instance().publish(LocomotionChanged{&person, moving, speed});
    if (moving && !_wasMoving) {
        Core::EventBus::instance().publish(ECA::Event{"locomotion-started", &person, nullptr, std::time(nullptr)});
    } else if (!moving && _wasMoving) {
        Core::EventBus::instance().publish(ECA::Event{"locomotion-stopped", &person, nullptr, std::time(nullptr)});
    }
    _wasMoving = moving;

    person.position = camera.pos - glm::vec3(0.0f, eyeH, 0.0f);
    tickAutomations(person, dt);
    person.updatePose();
}

} // namespace Input
} // namespace Singularity
