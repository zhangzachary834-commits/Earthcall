// Codex / GPT-6 / 01a1cdbf-bb6d-7861-a9a0-696204c370fe / 2026-11-11T19:15:48-08:00
// Isolated witness for the native interaction audit. Checks characterize the
// audited behavior, including its failures. It never starts the app or saves.
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Person/Person.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "Singularity/Input/Interaction/InteractionChannel.hpp"
#include "Singularity/Screen/Camera.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using Singularity::Input::InteractionChannel;

// GLFW's own transition injector updates its real polled state and invokes
// the callback. Use the Null platform to run step() without an OS window.
struct _GLFWwindow;
extern "C" void _glfwInputMouseClick(_GLFWwindow*, int button, int action, int mods);

namespace {
struct Edge {
    std::string type;
    Singular* subject;
    Singular* agent;
};

void rectangle(Object& object, const std::string& id, float x, float width) {
    Object::ShapeParams shape;
    shape.width2D = width;
    shape.height2D = 60;
    object.setShape(Object::ShapeKind::Shape2D, shape);
    object.setObjectID(id);
    object.setX2D(x);
    object.setY2D(0);
}

InteractionChannel::Sense at(float x, bool down = false) {
    InteractionChannel::Sense sense;
    sense.pointerX = x;
    sense.pointerY = 20;
    sense.rayOrigin = {0, 0, 20};
    sense.left = down;
    return sense;
}
} // namespace

int main() {
    Person person(Soul{}, Body{}, "interaction-audit-person");
    Object a, b;
    rectangle(a, "audit-a", 0, 60);
    rectangle(b, "audit-b", 80, 60);
    InteractionChannel channel;
    channel.setPointingPerson(&person);
    auto& universe = Universe::instance();
    std::vector<Singular*> beings{&person, &channel, &a, &b};
    universe.setProvider([&](std::vector<Singular*>& out) {
        out.insert(out.end(), beings.begin(), beings.end());
    });

    std::vector<Edge> edges;
    const auto subscription = Core::EventBus::instance().subscribe<ECA::Event>([&](const ECA::Event& event) {
        edges.push_back({event.type, event.subject, event.object});
    });
    const auto count = [&](const std::string& type, Singular* target) {
        return std::count_if(edges.begin(), edges.end(), [&](const Edge& edge) {
            return edge.type == type && edge.subject == target;
        });
    };
    int misses = 0;
    const auto check = [&](bool observed, const char* label) {
        std::printf("%s %s\n", observed ? "OBSERVED" : "MISSING ", label);
        misses += !observed;
    };

    channel.observe(at(20), {&a, &b});
    channel.noteMouseButton(true);
    channel.noteMouseButton(false);
    channel.observePending(at(20), {&a, &b});
    check(count("object-clicked", &a) == 1, "stationary same-batch left click (control)");
    const auto namesAgent = [&](const std::string& type, Singular* agent) {
        return std::any_of(edges.begin(), edges.end(), [&](const Edge& edge) {
            return edge.type == type && edge.subject == &a && edge.agent == agent;
        });
    };
    check(namesAgent("object-clicked", &person) && namesAgent("object-hover-entered", nullptr),
          "click names the pointing Person, hover entry omits them");

    edges.clear();
    // The hand presses and releases at A, then moves to B before the drain.
    // noteMouseButton has no parameter with which to tell the channel that.
    channel.noteMouseButton(true);
    channel.noteMouseButton(false);
    channel.observePending(at(100), {&a, &b});
    check(count("object-pressed", &b) == 1 && count("object-clicked", &b) == 1 &&
              count("object-clicked", &a) == 0,
          "same-batch left click is assigned to the final pointer target B");

    edges.clear();
    channel.observe(at(10, true), {&a, &b});
    channel.observe(at(45), {&a, &b});
    check(count("object-clicked", &a) == 1 && count("object-drag-started", &a) == 0,
          "35-point motion first sampled on release still clicks (slop is 12)");

    edges.clear();
    channel.observe(at(10, true), {&a, &b});
    channel.observe(at(45, true), {&a, &b});
    channel.observe(at(45), {&a, &b});
    check(count("object-clicked", &a) == 0 && count("object-drag-ended", &a) == 1,
          "same 35-point motion sampled while held is a drag (control)");

    channel.observe(at(20, true), {&a, &b});
    edges.clear();
    channel.setEnabled(false);
    channel.observe(at(20, true), {&a, &b});
    channel.setEnabled(true);
    channel.observe(at(20, true), {&a, &b});
    channel.observe(at(20), {&a, &b});
    check(count("object-press-cancelled", &a) == 0 && count("object-pressed", &a) == 1 &&
              count("object-clicked", &a) == 1,
          "disable/re-enable while held synthesizes a new press and click");

    Object nextZoneObject;
    rectangle(nextZoneObject, "audit-a", 0, 60);
    beings.push_back(&nextZoneObject);
    edges.clear();
    channel.observe(at(20, true), {&a});
    channel.observe(at(20), {&nextZoneObject});
    check(count("object-pressed", &a) == 1 && count("object-clicked", &nextZoneObject) == 1 &&
              count("object-released", &a) == 0,
          "replacement reachable Object with the same identifier inherits the click");
    check(a.getIsHovered(), "old hovered Object retains its hovered flag outside reach");

    Object near, far, hud;
    near.setObjectID("audit-near");
    far.setObjectID("audit-far");
    far.setPosition({0, 0, -4});
    rectangle(hud, "audit-hud", 0, 60);
    hud.setZOrder2D(2);
    far.setDynamicProperty("pickPriority", 100.0);
    channel.observe(at(20), {&near, &far, &hud});
    check(channel.hoveredId == "audit-hud", "3D priority 100 behind a low-priority 3D candidate is discarded");
    near.setDynamicProperty("pickPriority", -1.0);
    channel.observe(at(20), {&near, &far});
    check(channel.hoveredId == "audit-near", "negative 3D priority still intercepts the ray");

    Object cylinder;
    cylinder.setObjectID("audit-scaled-cylinder");
    cylinder.setShape(Object::ShapeKind::Cylinder, Object::ShapeParams{});
    glm::mat4 stretch(1.0f);
    stretch[2][2] = 4.0f;
    cylinder.setTransform(stretch);
    near.setPosition({0, 0, 10});
    channel.observe(at(20), {&near, &cylinder});
    check(channel.hoveredId == "audit-scaled-cylinder" &&
              std::abs(channel.pointerDistance - 4.5f) < 0.01f &&
              std::abs(channel.pointerWorld.z - 15.5f) < 0.01f,
          "scaled cylinder behind a cube wins the pick and reports an off-surface hit");
    channel.observe(at(20), {&near});
    check(channel.hoveredId == "audit-near" && std::abs(channel.pointerDistance - 9.5f) < 0.01f,
          "foreground cube is 9.5 world units away (control)");

    edges.clear();
    channel.observe(at(20), {&a, &b});
    channel.focusedId = a.getIdentifier();
    channel.noteMouseButton(true);
    channel.noteMouseButton(false);
    channel.noteKey("e", 69, true);
    channel.observePending(at(100), {&a, &b});
    check(count("key-pressed", &a) == 1 && count("key-pressed", &b) == 0 &&
              channel.focusedId == b.getIdentifier(),
          "key after a queued click routes to the previous focused Object");
    channel.noteKey("e", 69, false);

    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_NULL);
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(200, 120, "interaction-audit", nullptr, nullptr);
    check(window && glfwGetWindowAttrib(window, GLFW_FOCUSED), "focused GLFW Null fixture");
    if (window && glfwGetWindowAttrib(window, GLFW_FOCUSED)) {
        ZoneManager zones;
        auto zone = std::make_shared<Zone>("Interaction audit", "interaction-audit");
        zones.addZone(zone);
        auto plate = std::make_shared<Object>();
        rectangle(*plate, "audit-glfw-plate", 0, 60);
        zone->addObject(plate);
        Core::Camera camera;
        InteractionChannel glfwChannel;
        glfwChannel.setPointingPerson(&person);
        glfwSetWindowUserPointer(window, &glfwChannel);
        glfwSetMouseButtonCallback(window, [](GLFWwindow* win, int button, int action, int) {
            if (button == GLFW_MOUSE_BUTTON_LEFT) {
                auto* interaction = static_cast<InteractionChannel*>(glfwGetWindowUserPointer(win));
                interaction->noteMouseButton(action == GLFW_PRESS);
            }
        });
        glfwSetCursorPos(window, 20, 20);
        const auto edge = [&](int button, int action) {
            _glfwInputMouseClick(reinterpret_cast<_GLFWwindow*>(window), button, action, 0);
        };
        glfwChannel.step(window, camera, zones, false);
        edges.clear();
        for (int button : {GLFW_MOUSE_BUTTON_LEFT, GLFW_MOUSE_BUTTON_RIGHT, GLFW_MOUSE_BUTTON_MIDDLE}) {
            edge(button, GLFW_PRESS);
            edge(button, GLFW_RELEASE);
        }
        glfwChannel.step(window, camera, zones, false);
        check(count("object-clicked", plate.get()) == 1 &&
                  count("object-right-clicked", plate.get()) == 0 &&
                  count("object-middle-clicked", plate.get()) == 0,
              "GLFW-to-step loses rapid right/middle clicks while retaining the left click");
        edges.clear();
        edge(GLFW_MOUSE_BUTTON_RIGHT, GLFW_PRESS);
        edge(GLFW_MOUSE_BUTTON_MIDDLE, GLFW_PRESS);
        glfwChannel.step(window, camera, zones, false);
        edge(GLFW_MOUSE_BUTTON_RIGHT, GLFW_RELEASE);
        edge(GLFW_MOUSE_BUTTON_MIDDLE, GLFW_RELEASE);
        glfwChannel.step(window, camera, zones, false);
        check(count("object-right-clicked", plate.get()) == 1 &&
                  count("object-middle-clicked", plate.get()) == 1,
              "GLFW-to-step sees right/middle clicks spanning two frames (control)");
    }
    if (window) glfwDestroyWindow(window);
    glfwTerminate();

    LawManager laws;
    auto keyLaw = std::make_shared<FirstMoverLaw>("audit-e-key");
    keyLaw->addAuthor(person);
    keyLaw->setActivation(Law::Activation::OnEvent);
    keyLaw->setConditionModel(ConditionNode::compare(
        "@interaction-channel.lastKey", ConditionNode::Op::Eq, PropertyValue(std::string("e"))));
    keyLaw->setActionModel(ActionNode::publish("audit-key-activated"));
    laws.add(keyLaw);
    laws.bindTrigger(keyLaw->getIdentifier(), "key-pressed");
    laws.connectToEventBus();
    universe.setClock(0.0, 1.0 / 60.0);
    channel.focusedId = a.getIdentifier();
    edges.clear();
    channel.noteKey("e", 69, true);
    laws.tick();
    check(count("audit-key-activated", &a) == 1, "isolated e key activates the authored Law (control)");

    channel.noteKey("e", 69, false);
    laws.tick();
    edges.clear();
    channel.noteKey("e", 69, true);
    channel.noteKey("q", 81, true);
    laws.tick();
    check(count("key-pressed", &a) == 2 && count("audit-key-activated", &a) == 0,
          "e then q before one Law tick loses the e command");
    channel.noteKey("q", 81, false);
    check(!channel.keyDown, "releasing q clears keyDown while e is held");

    channel.noteKey("e", 69, false);
    laws.tick();
    edges.clear();
    channel.noteKey("q", 81, true);
    channel.noteKey("e", 69, true);
    laws.tick();
    check(count("key-pressed", &a) == 2 && count("audit-key-activated", &a) == 2,
          "q then e before one Law tick activates the e command twice");

    Core::EventBus::instance().unsubscribe(subscription);
    universe.setProvider(nullptr);
    return misses != 0;
}
