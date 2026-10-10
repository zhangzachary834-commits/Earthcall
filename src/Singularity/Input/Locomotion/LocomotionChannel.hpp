#pragma once

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"

#include <string>

struct GLFWwindow;
class Person;
class ZoneManager;

namespace Core { class Camera; }

namespace Singularity {
namespace Input {

// First-mover channel: the Input modality actuating the seated Person's vessel.
//
// Not an Avatar and not a kind of being. WASD / jump / sprint are Sense (keys)
// and Act (write camera + Person.position). Those seams stay first movement
// forever (FIRST_MOVER_AUTHORING.md §1). Gravity and jump impulse still live
// here as first-mover C++ — GAME_ELIMINATION_PLAN rung 3 parked them on
// Person; this channel is the extraction, not yet the Law migration.
//
// Walk/idle clips are first-mover authored Automation on BodyParts. Person
// keeps position, velocity, and updatePose() — facts about where someone is,
// not how the window moved them.
class LocomotionChannel : public Law {
public:
    LocomotionChannel();

    bool isFirstMover() const override { return true; }
    std::string getIdentifier() const override { return "locomotion-channel"; }
    const std::string& name() const { return _name; }

    static void syncRegister(LawManager& laws);
    static LocomotionChannel* find(LawManager& laws);

    // Per-frame Sense/Act. Writes person.position() / person.velocity() and the
    // camera; publishes locomotion / jump / landed edges.
    void step(Person& person, ::Core::Camera& camera, GLFWwindow* window,
              ZoneManager& mgr, float dt, bool flying, bool canMove);

    void playIdle(Person& person);
    void playWalk(Person& person, float speed);
    void setLocomotion(Person& person, bool moving, float speed);
    void stopClips(Person& person);
    void tickAutomations(Person& person, float dt);

    // One EventBus router for LocomotionChanged → setLocomotion. Idempotent.
    void installRouting();

    // Law-addressable facts about the local vessel this frame.
    // `enabled` is Law's — write `@locomotion-channel.enabled := false` to
    // set this first mover down. step() refuses when it is off.
    bool  grounded = false;
    bool  moving   = false;
    bool  flying   = false;
    bool  canMove  = false;
    float speed    = 0.0f;

private:
    void buildProperties() override;

    std::string _name{"locomotion-channel"};

    // Kernel / frame-edge state — not a being's meaning. jumpKeyDownLast and
    // wasGrounded exist only so SPACE and landing publish on the transition,
    // not as a per-frame level. walkActive / idleActive keep clip clocks from
    // restarting every frame. routingInstalled is the EventBus subscribe-once
    // latch (the bus has no unsubscribe). _wasActuating is the disable edge:
    // dropping the first mover must clear clips it authored, once.
    bool _jumpKeyDownLast = false;
    bool _wasGrounded     = false;
    bool _wasMoving       = false;
    bool _walkActive      = false;
    bool _idleActive      = false;
    bool _routingInstalled = false;
    bool _wasActuating     = false;
};

} // namespace Input
} // namespace Singularity
