// Personal Ourverse Navigator event-boundary regression.
//
// The generic button archetype owns "a button was activated".  The Navigator
// owns only the domain meaning of that activation:
//   object-clicked -> control-activated -> zone-navigation-requested
//
// Keeping those as two authored Laws matters: disabling/replacing navigation
// must not disable the button substrate, and an arbitrary activated control
// must not navigate merely because it happens to carry a destination-like
// property.

#include "ConstructedBeing/CategoryManager.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Relation/RelationManager.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "Singularity/Input/Interaction/ControlPatterns.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "json.hpp"

#include <GLFW/glfw3.h>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

using namespace Singularity::Input;

namespace {
int failures = 0;
int activations = 0;
int navigationRequests = 0;
std::string lastNavigationSubject;

void check(bool ok, const char* message) {
    std::printf("%s: %s\n", ok ? "  ok" : "  FAILED", message);
    if (!ok) ++failures;
}
} // namespace

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "ourverse_navigator_law_test: glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "ourverse_navigator_law_test", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "ourverse_navigator_law_test: no GL context\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    std::filesystem::path lawPath = "saves/laws/law-navigator-button/law.json";
    if (!std::filesystem::exists(lawPath)) {
        lawPath = std::filesystem::path("..") / lawPath;
    }
    check(std::filesystem::exists(lawPath), "authored navigator Law fixture exists");
    if (!std::filesystem::exists(lawPath)) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    nlohmann::json root;
    {
        std::ifstream in(lawPath);
        in >> root;
    }

    check(root.value("identifier", std::string{}) == "law-navigator-button",
          "fixture is the navigator Law");
    check(root.contains("triggers") && root["triggers"].is_array() &&
              root["triggers"].size() == 1 &&
              root["triggers"][0] == "control-activated",
          "navigator hears generic control activation, not raw clicks");
    check(root.contains("law") && root["law"].contains("actionModel") &&
              root["law"]["actionModel"].value("kind", -1) ==
                  static_cast<int>(ActionNode::Kind::Publish) &&
              root["law"]["actionModel"].value("eventType", std::string{}) ==
                  "zone-navigation-requested",
          "navigator publishes only the domain navigation event");

    Object author;
    author.setObjectID("navigator-test-author");

    CategoryManager cats;
    RelationManager graph;
    LawManager laws;

    Universe::instance().setRelationProvider([&](std::vector<Relation*>& out) {
        for (const auto& rel : graph.getAll()) {
            if (rel) out.push_back(rel.get());
        }
    });
    Universe::instance().setRelationRegistrar(
        [&](std::shared_ptr<Relation> rel) { graph.add(std::move(rel)); });

    Object beacon;
    beacon.setObjectID("navigator-test-beacon");
    beacon.setDynamicProperty("destinationZone", PropertyValue(std::string("Sanctum of Beginnings")));

    Object plain;
    plain.setObjectID("navigator-test-rock");
    plain.setDynamicProperty("destinationZone", PropertyValue(std::string("Temple of Echoes")));

    Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
        beings.push_back(&beacon);
        beings.push_back(&plain);
        beings.push_back(&author);
        for (const auto& cat : cats.getAll()) {
            if (cat) beings.push_back(cat.get());
        }
    });

    // The framework's generic button meaning is deliberately independent of
    // the Navigator's domain meaning.
    syncRegisterControlPatterns(laws, cats, author);
    check(makeControl(beacon, Control::kCategoryButton, cats),
          "beacon becomes a button by authored category relation");

    auto navigator = Law::fromJson(root["law"]);
    check(navigator != nullptr, "navigator Law deserializes");
    if (navigator) {
        navigator->addAuthor(author);
        laws.add(navigator);
        laws.bindTrigger(navigator->getIdentifier(), "control-activated");
    }

    laws.connectToEventBus();
    Universe::instance().setClock(0.0, 1.0 / 60.0);

    Core::EventBus::instance().subscribe<ECA::Event>([](const ECA::Event& ev) {
        if (ev.type == "control-activated") {
            ++activations;
        } else if (ev.type == "zone-navigation-requested") {
            ++navigationRequests;
            lastNavigationSubject = ev.subject ? ev.subject->getIdentifier() : std::string{};
        }
    });

    const auto click = [&](Object& subject) {
        Core::EventBus::instance().publish(
            ECA::Event{"object-clicked", &subject, nullptr, std::time(nullptr)});
        laws.tick();
    };

    activations = 0;
    navigationRequests = 0;
    lastNavigationSubject.clear();
    click(beacon);
    check(activations == 1,
          "one click produces exactly one generic control activation");
    check(navigationRequests == 1 &&
              lastNavigationSubject == beacon.getIdentifier(),
          "that activation produces exactly one navigation request for the same subject");

    PropertyValue destination;
    check(lawGetValue(beacon, PropertyPath::parse("destinationZone"), destination) &&
              std::get_if<std::string>(&destination) &&
              *std::get_if<std::string>(&destination) == "Sanctum of Beginnings",
          "navigation subject retains its authored destinationZone");

    // This is the architectural separation the regression primarily guards:
    // the generic control remains a button when the domain Law is laid down.
    if (navigator) navigator->setEnabled(false);
    activations = 0;
    navigationRequests = 0;
    click(beacon);
    check(activations == 1,
          "disabling navigator meaning does not disable generic button activation");
    check(navigationRequests == 0,
          "disabled navigator Law emits no domain navigation request");

    if (navigator) navigator->setEnabled(true);
    activations = 0;
    navigationRequests = 0;
    click(plain);
    check(activations == 0 && navigationRequests == 0,
          "a destination-like property on a non-button does not acquire navigation semantics");

    Universe::instance().setProvider({});
    Universe::instance().setRelationProvider({});
    Universe::instance().setRelationRegistrar({});

    glfwDestroyWindow(window);
    glfwTerminate();

    if (failures) {
        std::printf("ourverse_navigator_law_test: %d failure(s)\n", failures);
        return 1;
    }
    std::printf("ourverse_navigator_law_test: all checks passed\n");
    return 0;
}
