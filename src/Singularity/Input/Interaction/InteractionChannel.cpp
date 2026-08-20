#include "InteractionChannel.hpp"

#include "ConstructedBeing/Object/Object.hpp"
#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/Tool.hpp"
#include "ConstructedBeing/Singular/Property/PropertyRef.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "Singularity/Screen/Camera.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/World/World.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"

#include <GLFW/glfw3.h>
#include <cmath>
#include <ctime>

namespace Singularity {
namespace Input {

// buildProperties is NOT called from the constructor — Singular builds the
// registry lazily behind _propertiesBuilt, which a constructor call does not
// set, so every property would register a second time on first access. Same
// standing note as CreationChannel.cpp; no_black_box_test holds all channels
// to it.
InteractionChannel::InteractionChannel() = default;

void InteractionChannel::syncRegister(LawManager& laws) {
    if (auto* existing = find(laws)) {
        existing->installWorldReadings();
        return;
    }
    auto channel = std::make_shared<InteractionChannel>();
    channel->installWorldReadings();
    laws.add(channel);
}

InteractionChannel* InteractionChannel::find(LawManager& laws) {
    for (const auto& law : laws.getAll()) {
        if (auto* channel = dynamic_cast<InteractionChannel*>(law.get())) {
            return channel;
        }
    }
    return nullptr;
}

Object* InteractionChannel::findReachable(const std::vector<Object*>& reachable,
                                          const std::string& id) const {
    if (id.empty()) return nullptr;
    for (Object* obj : reachable) {
        if (obj && obj->getIdentifier() == id) return obj;
    }
    return nullptr;
}

void InteractionChannel::publishEdge(const std::string& type, Object* subject) const {
    if (!Universe::instance().anyoneHears(type)) return;
    ::Core::EventBus::instance().publish(
        ECA::Event{type, subject, nullptr, std::time(nullptr)});
}

// ---------------------------------------------------------------------------
// The world readings: facts ABOUT a subject rather than properties ON it.
//
// "@world.pointerOver" is what lets a hover law be authored on a being that
// carries no interaction state — the alternative was a per-object bool nobody
// would remember to add, or hard-coding highlight in C++ (which is what the
// engine did, in HighlightSystem, where no law can reach it).
// ---------------------------------------------------------------------------
void InteractionChannel::installWorldReadings() {
    if (_worldReadingsInstalled) return;
    _worldReadingsInstalled = true;

    registerWorldReading("@world.pointerOver",
                         [this](Singular& subject, PropertyValue& out) {
                             out = PropertyValue(subject.getIdentifier() == hoveredId &&
                                                 !hoveredId.empty());
                             return true;
                         });
    registerWorldReading("@world.pointerPressedOn",
                         [this](Singular& subject, PropertyValue& out) {
                             out = PropertyValue(subject.getIdentifier() == pressedId &&
                                                 !pressedId.empty());
                             return true;
                         });
    registerWorldReading("@world.pointerFocused",
                         [this](Singular& subject, PropertyValue& out) {
                             out = PropertyValue(subject.getIdentifier() == focusedId &&
                                                 !focusedId.empty());
                             return true;
                         });
    // Distance from the pointer's world hit to the subject's position. Only
    // meaningful while something is hit; undefined otherwise, and an undefined
    // reading must FAIL rather than answer 0 — a law never evaluates
    // mathematics on a missing value (MathBinding.hpp).
    registerWorldReading("@world.pointerDistance",
                         [this](Singular& subject, PropertyValue& out) {
                             if (hoveredId.empty()) return false;
                             auto* obj = dynamic_cast<Object*>(&subject);
                             if (!obj) return false;
                             out = PropertyValue(
                                 glm::length(pointerWorld - obj->getPosition()));
                             return true;
                         });
}

// ---------------------------------------------------------------------------
// One frame of pointing.
// ---------------------------------------------------------------------------
void InteractionChannel::observe(const Sense& sense,
                                 const std::vector<Object*>& reachable) {
    // The first mover set down: no picking, no edges, and — importantly — the
    // world is left un-hovered rather than frozen mid-hover. A disabled channel
    // that keeps a being marked "the pointer is on you" is a lie a WhileTrue
    // law would act on forever.
    const bool blind = !isEnabled() || sense.uiCaptured;

    // --- Pick -------------------------------------------------------------
    // pickSurface() is the tree's one per-face pick: nearest object, hit
    // point, and the outward normal derived per shape kind. Reusing it is what
    // keeps this channel and every 3D tool agreeing on what the ray hit — a
    // second copy of the normal derivation would drift, and the Person would
    // find the brush landing somewhere the button did not.
    SurfaceHit surface;
    Object* hit = nullptr;
    glm::vec2 bestUV{0.0f};
    if (!blind && pickSurface(reachable, sense.rayOrigin, sense.rayDirection, surface)) {
        hit = surface.obj;
        // pickSurface does not carry UV; ask the winner alone for it, which is
        // one raycast rather than a duplicated loop.
        float t = 0.0f;
        int face = -1;
        hit->raycastFace(sense.rayOrigin, sense.rayDirection, t, face, bestUV);
    }

    const glm::vec3 hitPoint = hit ? surface.point : glm::vec3(0.0f);

    // --- Levels -----------------------------------------------------------
    pointerX = sense.pointerX;
    pointerY = sense.pointerY;
    hoveredId = hit ? hit->getIdentifier() : std::string();
    hoveredFace = hit ? surface.face : -1;
    hoveredU = hit ? bestUV.x : 0.0f;
    hoveredV = hit ? bestUV.y : 0.0f;
    pointerWorld = hitPoint;
    pointerDistance = hit ? surface.t : 0.0f;
    if (hit) {
        // The surface normal the picked face presents, for laws that place or
        // orient against what the Person is pointing at.
        pointerNormal = surface.normal;
    }
    leftDown = !blind && sense.left;
    rightDown = !blind && sense.right;
    middleDown = !blind && sense.middle;
    shiftDown = sense.shift;
    ctrlDown = sense.ctrl;
    altDown = sense.alt;
    scrollX = blind ? 0.0f : sense.scrollX;
    scrollY = blind ? 0.0f : sense.scrollY;
    scrollTotal += scrollY;

    dragX = _hasPrevPointer ? sense.pointerX - _prevPointerX : 0.0f;
    dragY = _hasPrevPointer ? sense.pointerY - _prevPointerY : 0.0f;
    _prevPointerX = sense.pointerX;
    _prevPointerY = sense.pointerY;
    _hasPrevPointer = true;

    // --- Hover edges ------------------------------------------------------
    // Object::updateHoverState publishes object-hover-entered/exited. Driving
    // it here is what finally gives that vocabulary a caller; before this,
    // OBJECT_HOVER_EVENTS_SYSTEM.md documented a system nothing in the tree
    // invoked.
    for (Object* obj : reachable) {
        if (!obj) continue;
        const bool over = (obj == hit);
        obj->updateHoverState(over, over ? hitPoint : obj->getHoverPoint(),
                              glm::vec2(sense.pointerX, sense.pointerY));
    }

    // --- Button edges -----------------------------------------------------
    const bool leftPressedNow = leftDown && !_prevLeft;
    const bool leftReleasedNow = !leftDown && _prevLeft;

    if (leftPressedNow) {
        _pressX = sense.pointerX;
        _pressY = sense.pointerY;
        pressedId = hoveredId;
        dragTotalX = 0.0f;
        dragTotalY = 0.0f;
        dragging = false;
        if (hit) publishEdge("object-pressed", hit);

        // Focus follows the press, and BOTH sides are edges: a press on
        // nothing unfocuses whoever held it. A focus that can only be gained
        // is a focus that never leaves, and every key event after the first
        // click would go on reaching a being the Person walked away from.
        const std::string nextFocus = hoveredId;
        if (nextFocus != focusedId) {
            if (!focusedId.empty()) {
                publishEdge("object-unfocused", findReachable(reachable, focusedId));
            }
            focusedId = nextFocus;
            if (hit) publishEdge("object-focused", hit);
        }
    }

    if (leftDown && !pressedId.empty()) {
        dragTotalX += dragX;
        dragTotalY += dragY;
        const float travelled = std::sqrt(dragTotalX * dragTotalX +
                                          dragTotalY * dragTotalY);
        if (!dragging && travelled > kClickSlopPixels) {
            dragging = true;
            publishEdge("object-drag-started", findReachable(reachable, pressedId));
        }
    }

    if (leftReleasedNow) {
        Object* pressed = findReachable(reachable, pressedId);
        if (pressed) publishEdge("object-released", pressed);
        if (dragging) {
            publishEdge("object-drag-ended", pressed);
        } else if (pressed && pressed == hit) {
            // A click is press and release on the SAME being, without travel.
            // Releasing somewhere else is a cancelled click — the gesture every
            // pointer surface in the world honours, and the reason
            // object-clicked is not just "the button came up".
            publishEdge("object-clicked", pressed);
        }
        pressedId.clear();
        dragging = false;
        dragTotalX = 0.0f;
        dragTotalY = 0.0f;
    }

    if (!leftDown) {
        dragX = 0.0f;
        dragY = 0.0f;
    }

    // Scroll is discrete by nature — every notch is its own edge — so it
    // publishes per notch rather than per frame, and only when there is one.
    if (!blind && (sense.scrollY != 0.0f || sense.scrollX != 0.0f)) {
        publishEdge("object-scrolled", hit);
    }

    _prevLeft = leftDown;
    _prevRight = rightDown;
    _prevMiddle = middleDown;
}

void InteractionChannel::noteKey(const std::string& keyName, int keyCode, bool down) {
    if (!isEnabled()) return;
    // Level, not edge: a key held down repeats through the GLFW callback, and
    // republishing key-pressed on every repeat would make an event out of a
    // state. Same rule the mouse edges obey above.
    if (down && keyDown && keyCode == lastKeyCode) return;

    lastKey = keyName;
    lastKeyCode = keyCode;
    keyDown = down;

    // The focused being hears the key. Nothing focused = a null subject, which
    // is honest: the key still happened, it just was not addressed to anyone.
    Object* subject = nullptr;
    if (!focusedId.empty()) {
        for (Singular* being : Universe::instance().beings()) {
            if (being && being->getIdentifier() == focusedId) {
                subject = dynamic_cast<Object*>(being);
                break;
            }
        }
    }
    publishEdge(down ? "key-pressed" : "key-released", subject);
}

// ---------------------------------------------------------------------------
// The engine-facing step. Everything here is gathering; every decision is in
// observe().
// ---------------------------------------------------------------------------
void InteractionChannel::step(GLFWwindow* window, ::Core::Camera& camera,
                              ZoneManager& mgr, bool uiCaptured) {
    if (!window) return;

    Sense sense;
    sense.uiCaptured = uiCaptured;

    double cx = 0.0, cy = 0.0;
    glfwGetCursorPos(window, &cx, &cy);
    sense.pointerX = static_cast<float>(cx);
    sense.pointerY = static_cast<float>(cy);

    sense.left = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    sense.right = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    sense.middle = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
    sense.shift = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                  glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
    sense.ctrl = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
                 glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
    sense.alt = glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS ||
                glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS;

    // The wheel is a callback, not a level: whatever accumulated since the
    // last step is this frame's delta, and the accumulator resets here so a
    // frame with no wheel reports none.
    sense.scrollX = _pendingScrollX;
    sense.scrollY = _pendingScrollY;
    _pendingScrollX = 0.0f;
    _pendingScrollY = 0.0f;

    // The pointer ray, from the same view/projection/viewport the renderer
    // used. Shared arithmetic with CursorTools::pickObjectAtCursor3D — kept
    // here rather than called through it because CursorTools is a tool window
    // and this is a channel; a channel that depends on a window is the bug
    // CreationChannel was extracted to fix.
    const int* vp = camera.getViewport();
    const GLdouble* mv = camera.getModelview();
    const GLdouble* pr = camera.getProjection();
    if (vp && mv && pr && vp[2] > 0 && vp[3] > 0) {
        glm::mat4 V(1.0f), P(1.0f);
        for (int c = 0; c < 4; ++c) {
            for (int r = 0; r < 4; ++r) {
                V[c][r] = static_cast<float>(mv[c * 4 + r]);
                P[c][r] = static_cast<float>(pr[c * 4 + r]);
            }
        }
        const glm::mat4 invVP = glm::inverse(P * V);
        const float ndcX =
            ((sense.pointerX - static_cast<float>(vp[0])) / static_cast<float>(vp[2])) * 2.0f - 1.0f;
        const float ndcY =
            1.0f - ((sense.pointerY - static_cast<float>(vp[1])) / static_cast<float>(vp[3])) * 2.0f;
        glm::vec4 nearW = invVP * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
        glm::vec4 farW = invVP * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
        if (nearW.w != 0.0f) nearW /= nearW.w;
        if (farW.w != 0.0f) farW /= farW.w;
        sense.rayOrigin = glm::vec3(nearW);
        const glm::vec3 span = glm::vec3(farW - nearW);
        sense.rayDirection = glm::length(span) > 1e-6f ? glm::normalize(span)
                                                       : camera.getFront();
    } else {
        sense.rayOrigin = camera.getPos();
        sense.rayDirection = camera.getFront();
    }

    std::vector<Object*> reachable;
    const auto& owned = mgr.active().world().objects();
    reachable.reserve(owned.size());
    for (const auto& obj : owned) {
        if (obj) reachable.push_back(obj.get());
    }

    observe(sense, reachable);
}

void InteractionChannel::noteScroll(float dx, float dy) {
    _pendingScrollX += dx;
    _pendingScrollY += dy;
}

// ---------------------------------------------------------------------------
// The vocabulary a control author writes against.
// ---------------------------------------------------------------------------
void InteractionChannel::buildProperties() {
    registerEnabledProperty();

    const auto flt = [this](const char* name, float InteractionChannel::*member) {
        _propertyRegistry.push_back(
            std::make_unique<PropertyRef<InteractionChannel, float>>(name, this, member));
    };
    const auto boolean = [this](const char* name, bool InteractionChannel::*member) {
        _propertyRegistry.push_back(
            std::make_unique<PropertyRef<InteractionChannel, bool>>(name, this, member));
    };
    const auto text = [this](const char* name, std::string InteractionChannel::*member) {
        _propertyRegistry.push_back(
            std::make_unique<PropertyRef<InteractionChannel, std::string>>(name, this, member));
    };
    const auto vector3 = [this](const char* name, glm::vec3 InteractionChannel::*member) {
        _propertyRegistry.push_back(
            std::make_unique<PropertyRef<InteractionChannel, glm::vec3>>(name, this, member));
    };

    flt("pointerX", &InteractionChannel::pointerX);
    flt("pointerY", &InteractionChannel::pointerY);
    vector3("pointerWorld", &InteractionChannel::pointerWorld);
    vector3("pointerNormal", &InteractionChannel::pointerNormal);
    flt("pointerDistance", &InteractionChannel::pointerDistance);

    text("hoveredId", &InteractionChannel::hoveredId);
    _propertyRegistry.push_back(std::make_unique<PropertyRef<InteractionChannel, int>>(
        "hoveredFace", this, &InteractionChannel::hoveredFace));
    flt("hoveredU", &InteractionChannel::hoveredU);
    flt("hoveredV", &InteractionChannel::hoveredV);

    text("pressedId", &InteractionChannel::pressedId);
    text("focusedId", &InteractionChannel::focusedId);

    boolean("leftDown", &InteractionChannel::leftDown);
    boolean("rightDown", &InteractionChannel::rightDown);
    boolean("middleDown", &InteractionChannel::middleDown);

    flt("scrollX", &InteractionChannel::scrollX);
    flt("scrollY", &InteractionChannel::scrollY);
    flt("scrollTotal", &InteractionChannel::scrollTotal);

    flt("dragX", &InteractionChannel::dragX);
    flt("dragY", &InteractionChannel::dragY);
    flt("dragTotalX", &InteractionChannel::dragTotalX);
    flt("dragTotalY", &InteractionChannel::dragTotalY);
    boolean("dragging", &InteractionChannel::dragging);

    text("lastKey", &InteractionChannel::lastKey);
    _propertyRegistry.push_back(std::make_unique<PropertyRef<InteractionChannel, int>>(
        "lastKeyCode", this, &InteractionChannel::lastKeyCode));
    boolean("keyDown", &InteractionChannel::keyDown);
    boolean("shiftDown", &InteractionChannel::shiftDown);
    boolean("ctrlDown", &InteractionChannel::ctrlDown);
    boolean("altDown", &InteractionChannel::altDown);
}

} // namespace Input
} // namespace Singularity
