#pragma once

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"

#include <glm/glm.hpp>
#include <string>
#include <vector>

struct GLFWwindow;
class Object;
class ZoneManager;

namespace Core { class Camera; }

namespace Singularity {
namespace Input {

// First-mover channel: the pointer, the wheel, and the keys, as law-readable
// facts and past-tense events.
//
// This is the Sense half of INTERACTION_AS_LAW.md. It exists because a GUI in
// Earthcall is not a subsystem — a button is an ordinary spatial Object plus a
// Law that says when(object-clicked on me) act(math on my properties). For
// that sentence to be authorable, two things must be true and neither was:
//
//   1. "the pointer is over THIS being" has to be a fact law can read.
//      Object::_isHovered existed and was registered nowhere, and nothing in
//      the tree ever called updateHoverState() — the hover event system had a
//      199-line document and zero callers.
//   2. "the Person clicked THAT being" has to be an event law can hear. The
//      only click in the vocabulary was "onMouseClicked", published from the
//      GLFW callback with the CreationChannel as its subject: it says a click
//      happened, never what was clicked. No control can be authored on it.
//
// So this channel picks the being under the pointer once per frame and
// publishes the edges — object-pressed / object-released / object-clicked /
// object-scrolled / object-drag-started / object-drag-ended / object-focused /
// object-unfocused, plus the hover pair Object already published but nobody
// drove. Levels (is a button down, where is the pointer, how far has it
// dragged) are PROPERTIES, read by WhileTrue laws. Nothing here decides what a
// control means; that is authored.
//
// Refusal #1 holds: there is no Button, no Widget, no Control class anywhere
// in this framework. A control is a being that carries the properties and
// stands in the categories the authored control laws read.
class InteractionChannel : public Law {
public:
    InteractionChannel();

    bool isFirstMover() const override { return true; }
    std::string getIdentifier() const override { return "interaction-channel"; }
    const std::string& name() const { return _name; }

    static void syncRegister(LawManager& laws);
    static InteractionChannel* find(LawManager& laws);

    // ------------------------------------------------------------------
    // The raw frame, as sensed. Split out from step() so the whole channel
    // is exercisable without a window: every edge rule below is decided
    // here, and a rule that can only be reached by booting GLFW is a rule
    // nothing tests (the standing lesson of createShapeGenerator3DLaw).
    //
    // `uiCaptured` is the foreign-surface veto: while an ImGui window owns
    // the pointer, the world sees no pointer at all. It is a parameter and
    // not a property because the engine answers it, not the world.
    // ------------------------------------------------------------------
    struct Sense {
        float pointerX = 0.0f;          // screen pixels
        float pointerY = 0.0f;
        glm::vec3 rayOrigin{0.0f};
        glm::vec3 rayDirection{0.0f, 0.0f, -1.0f};
        bool left = false;              // button LEVELS this frame
        bool right = false;
        bool middle = false;
        float scrollX = 0.0f;           // this frame's wheel delta
        float scrollY = 0.0f;
        bool shift = false;
        bool ctrl = false;
        bool alt = false;
        bool uiCaptured = false;
    };

    // Sense the frame against `reachable`, write the properties, publish the
    // edges. Objects not in `reachable` are not picked and are not un-hovered:
    // the caller decides what the pointer can touch (the active Zone's world,
    // today), and a being outside that set is simply not in this frame's
    // question.
    void observe(const Sense& sense, const std::vector<Object*>& reachable);

    // Engine-facing wrapper: builds the pointer ray from the camera, gathers
    // the active Zone's objects, calls observe(). Stepped from Engine::update,
    // never from a render function (CreationChannel is the worked example of
    // why: collapsing the console froze the channel).
    void step(GLFWwindow* window, ::Core::Camera& camera, ZoneManager& mgr,
              bool uiCaptured);

    // Keys arrive as callbacks, not levels, so they enter here rather than
    // through Sense — an edge by construction. Publishes key-pressed /
    // key-released with the focused being as subject (null when nothing holds
    // focus), and updates lastKey / lastKeyCode / keyDown.
    void noteKey(const std::string& keyName, int keyCode, bool down);

    // The wheel is a callback too, and unlike a button it has no level to
    // poll: what the GLFW scroll callback reports is all there is. It
    // accumulates here and step() drains it, so a frame with no wheel reports
    // none instead of repeating the last notch forever.
    void noteScroll(float dx, float dy);

    // The left button's level used to come from polling glfwGetMouseButton()
    // once per frame in step(). A press-and-release that both land inside one
    // glfwPollEvents() batch — routine at 60 fps for a human click, and the
    // norm once the frame budget is tight — is invisible to a poll: by the
    // time step() asks, the button is already back up, so leftDown never
    // reads true and the whole gesture is dropped. Called from the GLFW
    // mouse-button callback, this keeps the level edge-accurate and latches
    // a same-frame press+release pair so step() can replay both edges rather
    // than lose them. See CHESS_GESTURE_HANDOFF_2026-08-26.md §3.
    void noteMouseButton(bool pressed);

    // Window focus gained or lost. Losing focus drops any held mouse press,
    // clears dragging, and cancels in-flight gestures so an unfocused window
    // cannot leak a stuck button or phantom drag across tab switches.
    void onWindowFocus(bool focused);

    // Registered as "@world.pointerOver" / "@world.pointerDistance": readings
    // ABOUT the subject rather than properties ON it, so "if the pointer is
    // over me" is authorable on a being that carries no interaction state of
    // its own. Idempotent; called by syncRegister.
    void installWorldReadings();

    // ------------------------------------------------------------------
    // The sensed facts. All registered (refusal #6): a control author reads
    // these by path, and `enabled` is Law's — write
    // `@interaction-channel.enabled := false` to set the pointer down.
    // ------------------------------------------------------------------
    float pointerX = 0.0f;
    float pointerY = 0.0f;
    glm::vec3 pointerWorld{0.0f};      // ray hit on the picked being
    glm::vec3 pointerNormal{0.0f, 1.0f, 0.0f};
    float pointerDistance = 0.0f;      // along the ray to the hit; 0 = no hit

    std::string hoveredId;             // "" when the pointer is over nothing
    int hoveredFace = -1;
    float hoveredU = 0.0f;
    float hoveredV = 0.0f;

    std::string pressedId;             // who received the press still held
    std::string rightPressedId;
    std::string middlePressedId;
    std::string focusedId;             // last being clicked, until another is

    bool leftDown = false;
    bool rightDown = false;
    bool middleDown = false;

    float scrollX = 0.0f;              // this frame only
    float scrollY = 0.0f;
    float scrollTotal = 0.0f;          // accumulated wheel, for tuner laws

    float dragX = 0.0f;                // pointer delta this frame while pressed
    float dragY = 0.0f;
    float dragTotalX = 0.0f;           // since the press began; 0 when released
    float dragTotalY = 0.0f;
    bool dragging = false;

    float rightDragTotalX = 0.0f;
    float rightDragTotalY = 0.0f;
    bool rightDragging = false;

    float middleDragTotalX = 0.0f;
    float middleDragTotalY = 0.0f;
    bool middleDragging = false;

    std::string lastKey;
    int lastKeyCode = 0;
    bool keyDown = false;
    bool shiftDown = false;
    bool ctrlDown = false;
    bool altDown = false;

    // How far the pointer may travel between press and release and still be a
    // click rather than a drag, in WINDOW points (the same space `pointerX`/
    // `pointerY` and `dragTotalX`/`dragTotalY` are measured in). Used to be a
    // `static constexpr` nothing could see or change — Refusal 6 on its face,
    // since deciding a gesture was a drag rather than a click is a meaning
    // decision, not a sensing one. Registered so a law (or a Person, through
    // the Law Graph) can read or tune it; 12.0 is a reasoned default (roughly
    // half a chess pawn's on-screen radius at chess_app's saved camera — see
    // CHESS_APP_EVERY_GESTURE_IS_A_DRAG_2026-08-26.md §3), not a measured
    // human constant, and Zach may want a different number.
    float clickSlopPixels = 12.0f;

    // Whether the OS cursor is confined/hidden for first-person look
    // (GLFW_CURSOR_DISABLED). While true, the pick ray leaves the viewport
    // centre rather than the pointer (step()) — a fact that used to be
    // invisible from inside the world. Written by step(); read-only to law.
    bool pointerLocked = false;

private:
    void buildProperties() override;

    // Publish `type` with `subject` (looked up in `reachable` by id) when
    // anyone is listening. One place, so every edge in this channel obeys the
    // same interest check and the same past-tense naming.
    void publishEdge(const std::string& type, Object* subject) const;

    Object* findReachable(const std::vector<Object*>& reachable,
                          const std::string& id) const;

    std::string _name{"interaction-channel"};

    // Kernel / frame-edge state — not a being's meaning, and deliberately
    // unregistered: these exist only so the edges above are edges. Naming
    // them here is the NO_BLACK_BOX exemption being taken out loud rather
    // than by omission.
    bool _prevLeft = false;
    bool _prevRight = false;
    bool _prevMiddle = false;
    float _pressX = 0.0f;              // where the press landed
    float _pressY = 0.0f;
    float _rightPressX = 0.0f;
    float _rightPressY = 0.0f;
    float _middlePressX = 0.0f;
    float _middlePressY = 0.0f;
    float _prevPointerX = 0.0f;
    float _prevPointerY = 0.0f;
    bool _hasPrevPointer = false;
    bool _worldReadingsInstalled = false;
    float _pendingScrollX = 0.0f;      // callback accumulator, drained by step()
    float _pendingScrollY = 0.0f;

    // Edge-accurate left-button state, kept by noteMouseButton() from the
    // GLFW callback instead of a per-frame glfwGetMouseButton() poll (see
    // noteMouseButton's doc comment for why polling drops fast clicks).
    bool _liveLeftDown = false;
    std::vector<bool> _pendingLeftEdges;

public:
    bool liveLeftDown() const { return _liveLeftDown; }
    // Test helpers to emulate step() in headless tests
    const std::vector<bool>& pendingLeftEdges() const { return _pendingLeftEdges; }
    void clearPendingLeftEdges() { _pendingLeftEdges.clear(); }
};

} // namespace Input
} // namespace Singularity
