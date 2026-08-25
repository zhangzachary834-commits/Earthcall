// The substrate's advertised vocabulary, held to its word.
//
// The law-authoring window does not quiz a Person for a property path — it
// OFFERS the ones the substrate has. Most of that list is probed live from the
// registries and cannot drift. The rest is hand-written, and a hand-written
// entry naming a property nobody registered is worse than a missing one: the
// picker presents it, the author picks it, the law reads it, and the read
// fails silently. The law then does something subtly other than what it says.
//
// That is exactly what happened to `activeShapeKind`. The field existed on
// CreationChannel, the picker offered it under "Channel — Creation", and
// buildProperties never registered it — so every law spawning a shape from the
// author's live selection quietly fell back to the concept's template instead.
// It was found by a test that used the path, not by anything checking the list.
//
// So: walk every entry, resolve it against a live instance of the being its
// own `group` label names, and check the value is the KIND the picker claims.
// A path that cannot answer is a promise the substrate is not keeping.

#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Relation/Formation/Formation.hpp"
#include "Singularity/Core/CreationChannel.hpp"
#include "Singularity/Input/Locomotion/LocomotionChannel.hpp"
#include "Singularity/Input/Interaction/InteractionChannel.hpp"
#include "Singularity/Screen/LawGraphWindow.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include "ZonesOfEarth/Ourverse/Ourverse.hpp"

#include <GLFW/glfw3.h>
#include <cstdio>
#include <string>

namespace {

int g_checked = 0;
int g_failures = 0;

// Reported rather than asserted: one unregistered path should not hide the
// rest of the list behind it. The whole point is to see every gap at once.
void fail(const std::string& path, const std::string& group, const std::string& why) {
    ++g_failures;
    std::printf("  FAILED  %-28s [%s] %s\n", path.c_str(), group.c_str(), why.c_str());
}

bool groupIs(const char* group, const char* prefix) {
    return std::string(group).rfind(prefix, 0) == 0;
}

// Does the value the path produced match what the picker promised it holds?
// The picker's `type` is what the editor renders — a text box for "text", a
// toggle for "toggle", a drag-number for "number". Offering a number editor
// for a string property is a control that cannot write its own property.
bool typeMatches(const char* declared, const PropertyValue& v) {
    const std::string t = declared;
    if (t == "vector")    return std::holds_alternative<glm::vec3>(v);
    if (t == "transform") return std::holds_alternative<glm::mat4>(v);
    if (t == "text")      return std::holds_alternative<std::string>(v);
    if (t == "toggle")    return std::holds_alternative<bool>(v);
    if (t == "number") {
        double n = 0.0;
        return propertyValueToNumber(v, n);
    }
    return true;   // unrecognised type label: nothing to check
}

// Resolve one advertised path against the being that should own it.
void check(const Rendering::PathOption& option, Singular& owner) {
    ++g_checked;
    PropertyValue v;
    const PropertyPath path = PropertyPath::parse(option.path);
    const auto result = path.getValue(owner, v);
    if (result != PropertyPath::PathResult::Ok) {
        fail(option.path, option.group,
             "does not resolve — the picker offers a property nobody registered");
        return;
    }
    if (!typeMatches(option.type, v)) {
        fail(option.path, option.group,
             std::string("resolves, but does not hold a ") + option.type +
             " — the editor would render the wrong control");
    }
}

} // namespace

int main() {
    // Object construction reaches the renderer boundary; give it a context.
    if (!glfwInit()) { std::fprintf(stderr, "channel_paths_test: glfwInit failed\n"); return 1; }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "channel_paths_test", nullptr, nullptr);
    if (!window) { std::fprintf(stderr, "channel_paths_test: no GL context\n"); glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);

    Object object;
    Law law("channel-paths-probe");
    Soul soul("Prober");
    Body body("humanoid", "default");
    Person person(std::move(soul), std::move(body), "default");
    Singularity::Core::CreationChannel channel;
    Singularity::Input::LocomotionChannel locomotion;
    Singularity::Input::InteractionChannel interaction;
    // The readings must be installed before the picker's "@world.*" entries can
    // be answered for — that is the whole point of checking them.
    interaction.installWorldReadings();
    Formation formation;
    Soul soulProbe;
    Ourverse ourverse;

    int skipped = 0;
    for (const Rendering::PathOption& option : Rendering::knownPathOptions()) {
        if (groupIs(option.group, "Object"))              check(option, object);
        else if (groupIs(option.group, "Law"))            check(option, law);
        else if (groupIs(option.group, "Person"))         check(option, person);
        else if (groupIs(option.group, "Channel — Creation")) check(option, channel);
        else if (groupIs(option.group, "Channel — Locomotion")) check(option, locomotion);
        else if (groupIs(option.group, "Channel — Interaction")) check(option, interaction);
        else if (groupIs(option.group, "Reading —")) {
            // A world reading has no owning being to resolve against: it is a
            // closure a modality channel registered, answering ABOUT whatever
            // subject it is handed. So the promise to check is REGISTRATION —
            // a picker entry naming a reading nobody registered is exactly the
            // silent failure this test exists for. Whether it answers for a
            // particular subject depends on the frame (@world.pointerDistance
            // is deliberately undefined while nothing is under the pointer),
            // and interaction_channel_test case 11 is where that is held.
            ++g_checked;
            if (worldReadings().count(option.path) == 0) {
                fail(option.path, option.group,
                     "names a world reading no channel registered — the picker "
                     "offers it, the law binds it, and the read fails silently");
            }
        }
        else if (groupIs(option.group, "Formation"))      check(option, formation);
        else if (groupIs(option.group, "Soul"))           check(option, soulProbe);
        else if (groupIs(option.group, "Ourverse"))       check(option, ourverse);
        else if (groupIs(option.group, "Time")) {
            // The world clock is not a property of any being — Singularity owns
            // time, and `time`, `time.delta`, `time.sinceApplied` are resolved
            // by the math-binding machinery when a law is applied, against a
            // clock that has no meaning outside a tick. Nothing to probe here;
            // continuous_law_test and time_flow_test exercise them where they
            // do have meaning.
            ++skipped;
        } else {
            fail(option.path, option.group,
                 "is offered under a group this test does not know how to own — "
                 "add the being here, or the path is homeless");
        }
    }

    std::printf("\nchannel_paths_test: %d advertised paths resolved, %d skipped (clock), %d failed\n",
                g_checked - g_failures, skipped, g_failures);

    glfwDestroyWindow(window);
    glfwTerminate();
    if (g_failures > 0) {
        std::printf("channel_paths_test: FAILED — the picker is offering paths the substrate "
                    "does not answer\n");
        return 1;
    }
    std::printf("channel_paths_test: ALL OK\n");
    return 0;
}
