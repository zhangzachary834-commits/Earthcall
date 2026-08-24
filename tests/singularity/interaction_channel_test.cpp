// The Sense half of INTERACTION_AS_LAW.md, held to §11a.
//
// This test exists in the shape it does — driving InteractionChannel::observe()
// with a hand-built Sense struct and a vector of beings — because the thing it
// is testing is a per-frame edge detector, and every previous per-frame edge
// detector in this tree that could only be reached by booting a window shipped
// broken. Object::updateHoverState is the worked example: 199 lines of
// documentation, zero callers, and an enter edge that published TWICE because
// it compared against a field written one frame behind the one it meant.
// Nothing caught it for as long as nothing called it.
//
// So: no window, no GLFW input, no camera. A ray, some beings, and the edges.

#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "Singularity/Input/Interaction/InteractionChannel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"

#include <cmath>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

using Singularity::Input::InteractionChannel;

namespace {

int g_failures = 0;

void check(bool ok, const std::string& what) {
    if (!ok) {
        ++g_failures;
        std::printf("  FAILED: %s\n", what.c_str());
        return;
    }
    std::printf("  ok: %s\n", what.c_str());
}

// Every ECA::Event this test provokes, by type and subject. The bus has no
// unsubscribe, so one subscription for the whole run and a clear() between
// cases.
struct Recorder {
    std::vector<std::pair<std::string, std::string>> events;

    void clear() { events.clear(); }

    int count(const std::string& type) const {
        int n = 0;
        for (const auto& e : events) {
            if (e.first == type) ++n;
        }
        return n;
    }
    int count(const std::string& type, const std::string& subjectId) const {
        int n = 0;
        for (const auto& e : events) {
            if (e.first == type && e.second == subjectId) ++n;
        }
        return n;
    }
    std::string trace() const {
        std::string s;
        for (const auto& e : events) {
            s += e.first + "(" + e.second + ") ";
        }
        return s;
    }
};

Recorder g_recorder;

// A unit cube standing at `at`, addressable by `id`.
void place(Object& obj, const std::string& id, const glm::vec3& at) {
    obj.setObjectID(id);
    obj.setPosition(at);
}

// The pointer looking down -Z from z = +20, aimed at (x, y).
InteractionChannel::Sense look(float x, float y) {
    InteractionChannel::Sense sense;
    sense.pointerX = x * 10.0f + 400.0f;   // an arbitrary but consistent screen
    sense.pointerY = y * 10.0f + 300.0f;   // mapping; only deltas matter here
    sense.rayOrigin = glm::vec3(x, y, 20.0f);
    sense.rayDirection = glm::vec3(0.0f, 0.0f, -1.0f);
    return sense;
}

} // namespace

int main() {
    std::printf("Running interaction channel test...\n");

    Core::EventBus::instance().subscribe<ECA::Event>([](const ECA::Event& e) {
        g_recorder.events.emplace_back(
            e.type, e.subject ? e.subject->getIdentifier() : std::string("null"));
    });

    Object a, b, side;
    place(a, "control-a", glm::vec3(0.0f, 0.0f, 0.0f));
    place(b, "control-b", glm::vec3(0.0f, 0.0f, -6.0f));    // behind a on the ray
    place(side, "control-side", glm::vec3(8.0f, 0.0f, 0.0f));
    std::vector<Object*> reachable{&a, &b, &side};

    InteractionChannel channel;
    Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
        beings.push_back(&a);
        beings.push_back(&b);
        beings.push_back(&side);
        beings.push_back(&channel);
    });

    // ------------------------------------------------------------------
    // 1. Every advertised path resolves, and to the advertised type.
    //    channel_paths_test walks the PICKER's list; this walks the
    //    channel's own registry, which is the other half of the same
    //    promise — a registered property the picker forgot is unreachable,
    //    and a picker entry nobody registered reads as silence.
    // ------------------------------------------------------------------
    {
        const std::map<std::string, const char*> advertised{
            {"enabled", "bool"},        {"pointerX", "float"},
            {"pointerY", "float"},      {"pointerWorld", "vec3"},
            {"pointerNormal", "vec3"},  {"pointerDistance", "float"},
            {"hoveredId", "string"},    {"hoveredFace", "int"},
            {"hoveredU", "float"},      {"hoveredV", "float"},
            {"pressedId", "string"},    {"focusedId", "string"},
            {"leftDown", "bool"},       {"rightDown", "bool"},
            {"middleDown", "bool"},     {"scrollX", "float"},
            {"scrollY", "float"},       {"scrollTotal", "float"},
            {"dragX", "float"},         {"dragY", "float"},
            {"dragTotalX", "float"},    {"dragTotalY", "float"},
            {"dragging", "bool"},       {"lastKey", "string"},
            {"lastKeyCode", "int"},     {"keyDown", "bool"},
            {"shiftDown", "bool"},      {"ctrlDown", "bool"},
            {"altDown", "bool"},
        };
        bool allResolved = true;
        bool allTyped = true;
        for (const auto& entry : advertised) {
            PropertyValue v;
            const auto result = PropertyPath::parse(entry.first).getValue(channel, v);
            if (result != PropertyPath::PathResult::Ok) {
                std::printf("    unresolved: %s\n", entry.first.c_str());
                allResolved = false;
                continue;
            }
            const std::string want = entry.second;
            const bool typed = (want == "bool" && std::holds_alternative<bool>(v)) ||
                               (want == "float" && std::holds_alternative<float>(v)) ||
                               (want == "int" && std::holds_alternative<int>(v)) ||
                               (want == "string" && std::holds_alternative<std::string>(v)) ||
                               (want == "vec3" && std::holds_alternative<glm::vec3>(v));
            if (!typed) {
                std::printf("    wrong type: %s (wanted %s)\n", entry.first.c_str(), entry.second);
                allTyped = false;
            }
        }
        check(allResolved, "every sensed fact is a registered, resolvable path");
        check(allTyped, "every sensed fact carries the type its author expects");
    }

    // ------------------------------------------------------------------
    // 2. The pick takes the NEAREST being, and hover enters exactly once.
    // ------------------------------------------------------------------
    {
        g_recorder.clear();
        channel.observe(look(0.0f, 0.0f), reachable);
        check(channel.hoveredId == "control-a",
              "the pointer picks the nearest being, not the first in the list");
        check(g_recorder.count("object-hover-entered", "control-a") == 1,
              "object-hover-entered fires once on entry");

        // Four more frames on the same being. An edge that publishes per frame
        // is the bug CLAUDE.md names outright, and the two-frame-lag version
        // of this code published a SECOND enter on frame two.
        for (int i = 0; i < 4; ++i) channel.observe(look(0.0f, 0.0f), reachable);
        check(g_recorder.count("object-hover-entered") == 1,
              "holding still republishes nothing — the level is the property");
        check(g_recorder.count("object-hover-exited") == 0,
              "nor does it publish an exit it did not experience");
    }

    // ------------------------------------------------------------------
    // 3. Exit fires when the pointer moves to another being, and when it
    //    moves to nothing at all.
    // ------------------------------------------------------------------
    {
        g_recorder.clear();
        channel.observe(look(8.0f, 0.0f), reachable);
        check(channel.hoveredId == "control-side", "the pointer follows to another being");
        check(g_recorder.count("object-hover-exited", "control-a") == 1,
              "leaving a being publishes its exit");
        check(g_recorder.count("object-hover-entered", "control-side") == 1,
              "arriving at another publishes its entry");

        g_recorder.clear();
        channel.observe(look(40.0f, 0.0f), reachable);   // empty space
        check(channel.hoveredId.empty(), "pointing at nothing hovers nothing");
        check(g_recorder.count("object-hover-exited", "control-side") == 1,
              "leaving for empty space still publishes the exit");
    }

    // ------------------------------------------------------------------
    // 4. press -> release -> click, on the same being.
    // ------------------------------------------------------------------
    {
        g_recorder.clear();
        auto down = look(0.0f, 0.0f);
        down.left = true;
        channel.observe(look(0.0f, 0.0f), reachable);    // arrive, button up
        channel.observe(down, reachable);                // press
        check(g_recorder.count("object-pressed", "control-a") == 1, "press publishes on the being");
        check(channel.pressedId == "control-a", "the press is remembered");
        check(g_recorder.count("object-focused", "control-a") == 1, "the press takes focus");
        check(channel.focusedId == "control-a", "focus is remembered");

        channel.observe(look(0.0f, 0.0f), reachable);    // release
        check(g_recorder.count("object-released", "control-a") == 1, "release publishes");
        check(g_recorder.count("object-clicked", "control-a") == 1,
              "press and release on the same being is a click");
        check(channel.pressedId.empty(), "the press is forgotten on release");
    }

    // ------------------------------------------------------------------
    // 5. Release on a DIFFERENT being: released, but NOT clicked. This is
    //    the cancelled click — the gesture a Person reaches for the first
    //    time they change their mind mid-press.
    // ------------------------------------------------------------------
    {
        g_recorder.clear();
        auto down = look(0.0f, 0.0f);
        down.left = true;
        channel.observe(look(0.0f, 0.0f), reachable);
        channel.observe(down, reachable);                // press on a

        auto heldElsewhere = look(8.0f, 0.0f);
        heldElsewhere.left = true;
        channel.observe(heldElsewhere, reachable);       // still held, over side
        channel.observe(look(8.0f, 0.0f), reachable);    // release over side

        check(g_recorder.count("object-released", "control-a") == 1,
              "release is reported against the being that was PRESSED");
        check(g_recorder.count("object-clicked") == 0,
              "releasing elsewhere cancels the click: " + g_recorder.trace());
    }

    // ------------------------------------------------------------------
    // 6. Travel past the slop is a drag, not a click.
    // ------------------------------------------------------------------
    {
        g_recorder.clear();
        auto down = look(0.0f, 0.0f);
        down.left = true;
        channel.observe(look(0.0f, 0.0f), reachable);
        channel.observe(down, reachable);                // press on a

        // Same being, but the pointer has travelled far past kClickSlopPixels
        // in screen space (look() maps one world unit to ten pixels).
        auto dragged = look(0.0f, 0.0f);
        dragged.left = true;
        dragged.pointerX += 60.0f;
        channel.observe(dragged, reachable);
        check(g_recorder.count("object-drag-started", "control-a") == 1,
              "travel past the slop starts a drag");
        check(channel.dragging, "and the level says so");

        auto up = look(0.0f, 0.0f);
        up.pointerX += 60.0f;
        channel.observe(up, reachable);
        check(g_recorder.count("object-drag-ended", "control-a") == 1, "release ends the drag");
        check(g_recorder.count("object-clicked") == 0,
              "a drag is not a click: " + g_recorder.trace());
    }

    // ------------------------------------------------------------------
    // 7. Tremor: a press that moves less than the slop is still a click.
    // ------------------------------------------------------------------
    {
        g_recorder.clear();
        auto down = look(0.0f, 0.0f);
        down.left = true;
        channel.observe(look(0.0f, 0.0f), reachable);
        channel.observe(down, reachable);
        auto wobble = down;
        wobble.pointerX += 2.0f;
        channel.observe(wobble, reachable);
        auto up = look(0.0f, 0.0f);
        up.pointerX += 2.0f;
        channel.observe(up, reachable);
        check(g_recorder.count("object-drag-started") == 0, "a tremor does not start a drag");
        check(g_recorder.count("object-clicked", "control-a") == 1,
              "a tremor still clicks: " + g_recorder.trace());
    }

    // ------------------------------------------------------------------
    // 8. Focus is LOST, including to nothing. A focus that can only be
    //    gained is a focus that never leaves.
    // ------------------------------------------------------------------
    {
        g_recorder.clear();
        auto downOnA = look(0.0f, 0.0f);
        downOnA.left = true;
        channel.observe(look(0.0f, 0.0f), reachable);
        channel.observe(downOnA, reachable);
        channel.observe(look(0.0f, 0.0f), reachable);
        check(channel.focusedId == "control-a", "a is focused");

        g_recorder.clear();
        auto downOnNothing = look(40.0f, 0.0f);
        downOnNothing.left = true;
        channel.observe(look(40.0f, 0.0f), reachable);
        channel.observe(downOnNothing, reachable);
        check(g_recorder.count("object-unfocused", "control-a") == 1,
              "clicking empty space unfocuses whoever held it");
        check(channel.focusedId.empty(), "and focus is genuinely empty, not stale");
        channel.observe(look(40.0f, 0.0f), reachable);   // settle the button
    }

    // ------------------------------------------------------------------
    // 9. Keys reach the focused being; repeats do not republish.
    // ------------------------------------------------------------------
    {
        auto downOnA = look(0.0f, 0.0f);
        downOnA.left = true;
        channel.observe(look(0.0f, 0.0f), reachable);
        channel.observe(downOnA, reachable);
        channel.observe(look(0.0f, 0.0f), reachable);

        g_recorder.clear();
        channel.noteKey("e", 69, true);
        check(g_recorder.count("key-pressed", "control-a") == 1,
              "the key reaches the focused being: " + g_recorder.trace());
        channel.noteKey("e", 69, true);      // GLFW repeat
        check(g_recorder.count("key-pressed") == 1,
              "a held key does not republish — the level is keyDown");
        check(channel.keyDown && channel.lastKey == "e", "and the level is legible");
        channel.noteKey("e", 69, false);
        check(g_recorder.count("key-released", "control-a") == 1, "release publishes");
        check(!channel.keyDown, "and clears the level");
    }

    // ------------------------------------------------------------------
    // 10. Scroll publishes per notch, against the being under the pointer.
    // ------------------------------------------------------------------
    {
        g_recorder.clear();
        channel.observe(look(0.0f, 0.0f), reachable);   // no wheel
        check(g_recorder.count("object-scrolled") == 0, "a still wheel publishes nothing");

        const float before = channel.scrollTotal;
        auto notch = look(0.0f, 0.0f);
        notch.scrollY = 1.0f;
        channel.observe(notch, reachable);
        check(g_recorder.count("object-scrolled", "control-a") == 1,
              "a notch publishes against the being under the pointer");
        check(std::fabs(channel.scrollTotal - (before + 1.0f)) < 1e-5f,
              "and accumulates, so a tuner can read it directly");

        channel.observe(look(0.0f, 0.0f), reachable);
        check(std::fabs(channel.scrollY) < 1e-6f,
              "the per-frame delta clears — a frame with no wheel reports none");
    }

    // ------------------------------------------------------------------
    // 11. The world readings answer ABOUT a subject.
    // ------------------------------------------------------------------
    {
        channel.installWorldReadings();
        channel.observe(look(0.0f, 0.0f), reachable);

        PropertyValue v;
        check(lawGetValue(a, PropertyPath::parse("@world.pointerOver"), v) &&
                  std::get<bool>(v),
              "@world.pointerOver is true of the being under the pointer");
        check(lawGetValue(side, PropertyPath::parse("@world.pointerOver"), v) &&
                  !std::get<bool>(v),
              "and false of everyone else");
        check(lawGetValue(a, PropertyPath::parse("@world.pointerDistance"), v),
              "@world.pointerDistance answers while something is hit");

        channel.observe(look(40.0f, 0.0f), reachable);   // pointing at nothing
        check(!lawGetValue(a, PropertyPath::parse("@world.pointerDistance"), v),
              "and FAILS rather than answering 0 when nothing is hit");
    }

    // ------------------------------------------------------------------
    // 12. hovered / hoverPoint are readable off the being itself — the
    //     refusal #6 repair. Read-only, because they are derived.
    // ------------------------------------------------------------------
    {
        channel.observe(look(0.0f, 0.0f), reachable);
        PropertyValue v;
        check(PropertyPath::parse("hovered").getValue(a, v) == PropertyPath::PathResult::Ok &&
                  std::get<bool>(v),
              "a being's own `hovered` is legible to law");
        check(PropertyPath::parse("hoverPoint").getValue(a, v) == PropertyPath::PathResult::Ok,
              "so is where the pointer met it");
        check(PropertyPath::parse("hovered").setValue(a, PropertyValue(false)) !=
                  PropertyPath::PathResult::Ok,
              "and neither can be written — a law that could would be lying");
    }

    // ------------------------------------------------------------------
    // 13. Setting the first mover down blinds it, and RELEASES the world.
    //     A disabled channel that leaves a being marked "the pointer is on
    //     you" is a lie a WhileTrue law would act on forever.
    // ------------------------------------------------------------------
    {
        channel.observe(look(0.0f, 0.0f), reachable);
        check(a.getIsHovered(), "a is hovered before the channel is set down");

        g_recorder.clear();
        channel.setEnabled(false);
        auto down = look(0.0f, 0.0f);
        down.left = true;
        channel.observe(down, reachable);
        check(channel.hoveredId.empty(), "a disabled channel picks nothing");
        check(!a.getIsHovered(), "and releases whoever it was holding");
        check(g_recorder.count("object-hover-exited", "control-a") == 1,
              "announcing the release rather than dropping it silently");
        check(g_recorder.count("object-pressed") == 0, "no press reaches the world");
        check(!channel.leftDown, "and the button level reads up");
        channel.setEnabled(true);
    }

    // ------------------------------------------------------------------
    // 14. A foreign UI surface owning the pointer is the same veto, for
    //     one frame, without touching the law-visible `enabled` bit.
    // ------------------------------------------------------------------
    {
        channel.observe(look(0.0f, 0.0f), reachable);
        g_recorder.clear();
        auto captured = look(0.0f, 0.0f);
        captured.left = true;
        captured.uiCaptured = true;
        channel.observe(captured, reachable);
        check(channel.hoveredId.empty(), "an ImGui panel over the pointer blinds the world");
        check(g_recorder.count("object-pressed") == 0, "and swallows the press");
        check(channel.isEnabled(), "without disabling the channel — that is a Person's choice");
    }

    Universe::instance().setProvider(nullptr);

    if (g_failures) {
        std::printf("interaction_channel_test: %d FAILURES\n", g_failures);
        return 1;
    }
    std::printf("interaction_channel_test: all checks passed\n");
    return 0;
}
