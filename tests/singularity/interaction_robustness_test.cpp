// Robustness of the 2D interaction surface — INTERACTION_AS_LAW.md §4, held
// to invariants rather than to scripted cases.
//
// interaction_channel_test walks the edges one scripted gesture at a time.
// That style missed three bugs at once (docs/plans/
// 2D_Interface_Robustness_Pass_2026-09-22.md, Tier 0): a wheel notch counted
// once per replayed button edge, a press that lost its release when the
// window lost focus or the Zone changed, and a pick that handed equal-z
// overlaps to the being drawn UNDERNEATH. None of them is a strange gesture;
// each is an ordinary one nobody scripted.
//
// So, after four targeted regressions, this drives observePending() — the
// same entry step() uses — with thousands of seeded random frames (pointer
// moves, press/release bursts inside one frame, wheel, a foreign UI taking
// the pointer, the window losing focus, beings leaving and re-entering the
// reachable set) and checks the event STREAM against a small state machine:
//
//   * every object-pressed is closed by exactly one object-released on the
//     same being, before any other press;
//   * object-clicked only right after the release of an untravelled press;
//     object-drag-ended only after a travelled one; press-cancelled never
//     followed by a click;
//   * hover-entered / hover-exited alternate per being;
//   * scrollTotal equals the wheel the world could hear, exactly.
//
// Headless: no window, no GL. Shape2D beings, because their pick is a
// deterministic rectangle test.
//
// Claude Opus 5.5, session b0dcb70f-a02a-4081-8589-0aae3ab30551, 2026-09-22,
// at Zach's request to make the Singular/Law-driven 2D interfaces more robust.

#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "Singularity/Input/Interaction/InteractionChannel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"

#include <cmath>
#include <cstdio>
#include <map>
#include <random>
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

std::vector<std::pair<std::string, std::string>> g_events;

int count(const std::string& type, const std::string& subject = "") {
    int n = 0;
    for (const auto& e : g_events) {
        if (e.first == type && (subject.empty() || e.second == subject)) ++n;
    }
    return n;
}

void rect(Object& obj, const std::string& id, float x, float y, float w, float h,
          int z) {
    obj.setObjectID(id);
    Object::ShapeParams p;
    p.width2D = w;
    p.height2D = h;
    obj.setShape(Object::ShapeKind::Shape2D, p);
    obj.setX2D(x);
    obj.setY2D(y);
    obj.setZOrder2D(z);
}

InteractionChannel::Sense at(float x, float y, bool left = false) {
    InteractionChannel::Sense s;
    s.pointerX = x;
    s.pointerY = y;
    // A ray that meets nothing: every being in this test is screen-space.
    s.rayOrigin = glm::vec3(0.0f, 0.0f, 1e6f);
    s.rayDirection = glm::vec3(0.0f, 0.0f, 1.0f);
    s.left = left;
    return s;
}

// The left-button gesture grammar, checked event by event.
struct GestureModel {
    std::string held;            // being holding the press, "" = none
    bool travelled = false;
    std::string closed;          // being whose press was JUST released
    bool closedTravelled = false;
    std::map<std::string, bool> hovered;
    std::vector<std::string> violations;
    std::vector<std::string> recent;   // the stream leading up to a violation

    void bad(const std::string& why) {
        if (violations.size() >= 12) return;
        std::string context = why + "\n        after: ";
        for (const auto& r : recent) context += r + " ";
        violations.push_back(context);
    }

    void feed(const std::string& type, const std::string& who) {
        recent.push_back(type.substr(type.find('-') + 1) + "(" + who + ")");
        if (recent.size() > 10) recent.erase(recent.begin());
        if (type == "object-pressed") {
            if (!held.empty()) bad("pressed(" + who + ") while " + held + " still held");
            held = who;
            travelled = false;
            closed.clear();
        } else if (type == "object-drag-started") {
            if (held != who || travelled) bad("drag-started(" + who + ") out of order");
            travelled = true;
        } else if (type == "object-dragged") {
            if (held != who || !travelled) bad("dragged(" + who + ") without a started drag");
        } else if (type == "object-released") {
            if (held != who) bad("released(" + who + ") but held='" + held + "'");
            closed = who;
            closedTravelled = travelled;
            held.clear();
            travelled = false;
        } else if (type == "object-drag-ended") {
            if (closed != who || !closedTravelled) bad("drag-ended(" + who + ") out of order");
        } else if (type == "object-clicked") {
            if (closed != who || closedTravelled) bad("clicked(" + who + ") without an untravelled release");
            closed.clear();
        } else if (type == "object-press-cancelled") {
            if (closed != who) bad("press-cancelled(" + who + ") without its release");
            closed.clear();   // a click after this would now be caught above
        } else if (type == "object-hover-entered") {
            if (hovered[who]) bad("hover-entered(" + who + ") twice");
            hovered[who] = true;
        } else if (type == "object-hover-exited") {
            if (!hovered[who]) bad("hover-exited(" + who + ") without entering");
            hovered[who] = false;
        }
    }
};

} // namespace

int main() {
    std::printf("Running interaction robustness test...\n");

    Core::EventBus::instance().subscribe<ECA::Event>([](const ECA::Event& e) {
        g_events.emplace_back(e.type,
                              e.subject ? e.subject->getIdentifier() : std::string("null"));
    });

    // Two equal-z plates overlapping at (150..200, 0..100); a higher-z HUD
    // over part of both; a lone plate elsewhere.
    Object under, over, hud, lone;
    rect(under, "plate-under", 0.0f, 0.0f, 200.0f, 100.0f, 0);
    rect(over, "plate-over", 150.0f, 0.0f, 200.0f, 100.0f, 0);
    rect(hud, "plate-hud", 180.0f, 40.0f, 60.0f, 20.0f, 5);
    rect(lone, "plate-lone", 0.0f, 300.0f, 100.0f, 100.0f, 0);
    std::vector<Object*> all{&under, &over, &hud, &lone};

    InteractionChannel channel;
    Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
        for (Object* o : all) beings.push_back(o);
        beings.push_back(&channel);
    });

    // ------------------------------------------------------------------
    // 1. Equal z: the pick goes to the being DRAWN on top — the last in the
    //    list, which is what EngineRender's stable_sort draws last.
    // ------------------------------------------------------------------
    {
        channel.observe(at(170.0f, 20.0f), all);
        check(channel.hoveredId == "plate-over",
              "an equal-z overlap picks the plate drawn on top, not the one beneath");
        channel.observe(at(200.0f, 50.0f), all);
        check(channel.hoveredId == "plate-hud", "higher z still wins over both");
        channel.observe(at(50.0f, 20.0f), all);
        check(channel.hoveredId == "plate-under", "and the lower plate is reachable where it shows");
    }

    // ------------------------------------------------------------------
    // 2. A burst of button edges inside one frame counts the wheel ONCE.
    // ------------------------------------------------------------------
    {
        g_events.clear();
        const float before = channel.scrollTotal;
        channel.noteMouseButton(true);
        channel.noteMouseButton(false);
        channel.noteMouseButton(true);
        channel.noteMouseButton(false);
        channel.noteScroll(0.0f, 1.0f);
        channel.observePending(at(50.0f, 350.0f, channel.liveLeftDown()), all);
        check(std::fabs(channel.scrollTotal - before - 1.0f) < 1e-6f,
              "one notch adds exactly 1 to scrollTotal through a 4-edge replay");
        check(count("object-scrolled") == 1, "and publishes object-scrolled once");
        check(count("object-clicked", "plate-lone") == 2,
              "while both clicks in the burst still land");
    }

    // ------------------------------------------------------------------
    // 3. The window losing focus mid-drag ENDS the gesture, as edges.
    // ------------------------------------------------------------------
    {
        g_events.clear();
        channel.observe(at(20.0f, 320.0f, true), all);
        channel.observe(at(60.0f, 330.0f, true), all);   // past the slop
        check(channel.dragging, "the press is a drag");
        channel.onWindowFocus(false);
        check(count("object-released", "plate-lone") == 1, "focus loss publishes the release");
        check(count("object-drag-ended", "plate-lone") == 1, "and ends the drag");
        check(count("object-press-cancelled", "plate-lone") == 1, "and says it was cancelled");
        check(count("object-clicked") == 0, "and is never a click");
        check(channel.pressedId.empty() && !channel.dragging, "and holds nothing afterwards");
        channel.onWindowFocus(true);
        channel.observe(at(20.0f, 320.0f), all);
    }

    // ------------------------------------------------------------------
    // 4. The pressed and focused being leaving the reachable set (a Zone
    //    switch) ends the press and the focus, instead of orphaning them.
    // ------------------------------------------------------------------
    {
        channel.observe(at(20.0f, 320.0f, true), all);   // press + focus lone
        check(channel.focusedId == "plate-lone", "the press focused the plate");
        g_events.clear();
        const std::vector<Object*> otherZone{&under, &over, &hud};
        channel.observe(at(20.0f, 320.0f, true), otherZone);
        check(count("object-released", "plate-lone") == 1, "a Zone switch releases the held press");
        check(count("object-press-cancelled", "plate-lone") == 1, "as a cancellation");
        check(count("object-unfocused", "plate-lone") == 1, "and unfocuses the being left behind");
        check(channel.focusedId.empty(), "so keys no longer reach a being in another Zone");
        channel.observe(at(20.0f, 320.0f, false), otherZone);
        check(count("object-clicked") == 0, "the later physical release clicks nothing");
        channel.observe(at(20.0f, 320.0f, false), all);
    }

    // ------------------------------------------------------------------
    // 5. The model: seeded random frames, the stream checked as a grammar.
    // ------------------------------------------------------------------
    {
        const unsigned seeds[] = {1u, 7u, 42u, 2026u, 90210u};
        int totalFrames = 0;
        bool grammarHeld = true;
        bool scrollExact = true;
        bool burstsBounded = true;
        for (unsigned seed : seeds) {
            InteractionChannel ch;
            std::vector<Object*> bag = all;
            Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
                for (Object* o : all) beings.push_back(o);
                beings.push_back(&ch);
            });
            // Hover state lives on the beings; start every seed from clean.
            for (Object* o : all) o->updateHoverState(false, glm::vec3(0.0f), glm::vec2(0.0f));
            g_events.clear();

            std::mt19937 rng(seed);
            std::uniform_real_distribution<float> px(-20.0f, 420.0f);
            std::uniform_int_distribution<int> die(0, 99);
            GestureModel model;
            float x = 100.0f, y = 50.0f;
            bool physical = false;      // the Person's finger
            bool windowFocused = true;
            float heardScroll = 0.0f;

            for (int frame = 0; frame < 2000; ++frame, ++totalFrames) {
                // Marked BEFORE anything this frame can publish: onWindowFocus
                // publishes its cancellations outside observePending.
                const std::size_t before = g_events.size();
                const int roll = die(rng);
                if (roll < 45) {                        // small move (drag-ish)
                    x += px(rng) * 0.05f - 10.0f;
                    y += px(rng) * 0.05f - 10.0f;
                } else if (roll < 55) {                 // jump anywhere
                    x = px(rng);
                    y = px(rng);
                }
                int toggles = 0;
                if (die(rng) < 20) toggles = 1 + die(rng) % 3;   // up to 3 edges in one frame
                for (int t = 0; t < toggles && windowFocused; ++t) {
                    physical = !physical;
                    ch.noteMouseButton(physical);
                }
                float wheel = 0.0f;
                if (die(rng) < 10) {
                    wheel = static_cast<float>(die(rng) % 5) - 2.0f;
                    ch.noteScroll(0.0f, wheel);
                }
                if (die(rng) < 3) {                     // a being leaves / returns
                    if (bag.size() == all.size()) {
                        bag.erase(bag.begin() + die(rng) % static_cast<int>(bag.size()));
                    } else {
                        bag = all;
                    }
                }
                if (windowFocused && die(rng) < 2) {    // window loses focus
                    windowFocused = false;
                    physical = false;
                    ch.onWindowFocus(false);
                } else if (!windowFocused && die(rng) < 30) {
                    windowFocused = true;
                    ch.onWindowFocus(true);
                }
                const bool foreignUI = die(rng) < 5;

                InteractionChannel::Sense s = at(x, y, ch.liveLeftDown());
                s.uiCaptured = foreignUI || !windowFocused;
                if (!s.uiCaptured) heardScroll += wheel;

                ch.observePending(s, bag);
                int scrolled = 0;
                for (std::size_t i = before; i < g_events.size(); ++i) {
                    model.feed(g_events[i].first, g_events[i].second);
                    if (g_events[i].first == "object-scrolled") ++scrolled;
                }
                if (scrolled > 1) burstsBounded = false;
            }

            // Let go of everything, in reach of everything: no gesture may
            // be left open.
            physical = false;
            ch.noteMouseButton(false);
            const std::size_t before = g_events.size();
            ch.observePending(at(x, y, false), all);
            for (std::size_t i = before; i < g_events.size(); ++i) {
                model.feed(g_events[i].first, g_events[i].second);
            }
            if (!model.held.empty()) model.bad("press on " + model.held + " never closed");

            if (!model.violations.empty()) {
                grammarHeld = false;
                std::printf("    seed %u:\n", seed);
                for (const auto& v : model.violations) std::printf("      %s\n", v.c_str());
            }
            if (std::fabs(ch.scrollTotal - heardScroll) > 1e-3f) {
                scrollExact = false;
                std::printf("    seed %u: scrollTotal %.3f, world heard %.3f\n", seed,
                            ch.scrollTotal, heardScroll);
            }
        }
        std::printf("    (%d random frames across %zu seeds)\n", totalFrames,
                    sizeof(seeds) / sizeof(seeds[0]));
        check(grammarHeld, "every press closes once; clicks, drags and cancels keep their order; hover alternates");
        check(scrollExact, "scrollTotal is exactly the wheel the world could hear");
        check(burstsBounded, "never more than one object-scrolled per frame");
    }

    Universe::instance().setProvider(nullptr);

    if (g_failures) {
        std::printf("interaction_robustness_test: %d FAILURES\n", g_failures);
        return 1;
    }
    std::printf("interaction_robustness_test: all checks passed\n");
    return 0;
}
