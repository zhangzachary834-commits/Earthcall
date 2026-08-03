// Test: Law replicating the exact functionality of the basic cube 3D Custom Shape generator tool,
// including complex placement capabilities: InFront placement, Cursor Grid Snap, and Manual Distance.
//
// Capabilities tested:
// 1. Triggers on "onMouseClicked" event.
// 2. Evaluates condition: player's activeTool == "ShapeGenerator3D" or "Custom Shape" or active3DMode == "Create".
// 3. Placement Mode: "InFront" - places shape at cursorHitPos.
// 4. Placement Mode: "CursorSnap" - snaps shape placement to gridSnapSize increments.
// 5. Placement Mode: "ManualDistance" - places shape at cameraPos + cameraForward * manualDistance.
// 6. Executes action: Spawns/creates a 3D Shape object of kind ShapeKind::Cube (basic cube) at cursorSpawnTransform.
// 7. Persistence: Round-trips cleanly through JSON serialization preserving all placement law functionality.

#include "Form/Object/Creation/ObjectConcept.hpp"
#include "Form/Object/Object.hpp"
#include "Person/Person.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/World/World.hpp"

#include <GLFW/glfw3.h>
#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

namespace {

int g_checks = 0;
int g_failures = 0;
// Reports through the exit code rather than assert(): a Release build defines
// NDEBUG, which compiles assert() out entirely and would leave this test
// printing FAILED while still exiting 0. The early return also stops a failed
// check from printing "ok" for itself immediately afterwards.
void check(bool condition, const std::string& description) {
    ++g_checks;
    if (!condition) {
        ++g_failures;
        std::cout << "  FAILED: " << description << std::endl;
        return;
    }
    std::cout << "  ok: " << description << std::endl;
}

bool nearVec3(const glm::vec3& a, const glm::vec3& b, float eps = 1e-4f) {
    return std::fabs(a.x - b.x) < eps &&
           std::fabs(a.y - b.y) < eps &&
           std::fabs(a.z - b.z) < eps;
}

} // namespace

int main() {
    std::cout << "============================================================" << std::endl;
    std::cout << "Running Basic Cube 3D Custom Shape Generator Law Test..." << std::endl;
    std::cout << "============================================================" << std::endl;

    // Headless OpenGL initialization required for Object face textures & shaders
    if (!glfwInit()) {
        std::cerr << "basic_cube_law_test: glfwInit failed" << std::endl;
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "basic_cube_law_test", nullptr, nullptr);
    if (!window) {
        std::cerr << "basic_cube_law_test: window creation failed" << std::endl;
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    // 1. Setup World, Player (author), and Universe
    World world;
    Soul soul("Player");
    Body body("humanoid", "default");
    Person player(std::move(soul), std::move(body), "default");
    player.position = glm::vec3(0.0f, 1.0f, 0.0f);

    Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
        beings.push_back(&world);
        beings.push_back(&player);
        for (const auto& obj : world.getOwnedObjects()) {
            if (obj) beings.push_back(obj.get());
        }
    });

    LawManager lawManager;
    lawManager.connectToEventBus();

    // 2. Define the basic cube ObjectConcept ("concept-shape-3d")
    auto cubeConcept = std::make_shared<ObjectConcept>("concept-shape-3d");
    ObjectConcept::MemberTemplate memberTemplate;
    memberTemplate.kind = Object::ShapeKind::Cube;
    memberTemplate.relativeTransform = glm::mat4(1.0f);
    cubeConcept->members().push_back(memberTemplate);
    ConceptRegistry::instance().add(cubeConcept);

    check(ConceptRegistry::instance().find(cubeConcept->getIdentifier()) != nullptr,
          "ConceptRegistry registered basic cube concept");

    // 3. Create the Law replicating the 3D Custom Shape Generator Tool with Placement Path
    // Condition: activeTool == "ShapeGenerator3D" OR activeTool == "Custom Shape" OR active3DMode == "Create"
    ConditionNode conditionShapeGen = ConditionNode::compare("activeTool", ConditionNode::Op::Eq, PropertyValue("ShapeGenerator3D"));
    ConditionNode conditionCustomShape = ConditionNode::compare("activeTool", ConditionNode::Op::Eq, PropertyValue("Custom Shape"));
    ConditionNode conditionCreateMode = ConditionNode::compare("active3DMode", ConditionNode::Op::Eq, PropertyValue("Create"));
    
    ConditionNode toolCondition;
    toolCondition.kind = ConditionNode::Kind::Any;
    toolCondition.children.push_back(conditionShapeGen);
    toolCondition.children.push_back(conditionCustomShape);
    toolCondition.children.push_back(conditionCreateMode);

    // Action: Spawn concept-shape-3d (basic cube) at cursorSpawnTransform
    ActionNode spawnAction = ActionNode::spawn(cubeConcept->getIdentifier());
    spawnAction.spawnPlacementPath = PropertyPath::parse("cursorSpawnTransform");

    auto cubeGeneratorLaw = lawManager.createLaw("Tool: Shape Generator 3D", {&player});
    const std::string lawId = cubeGeneratorLaw->getIdentifier();

    cubeGeneratorLaw->setConditionModel(toolCondition);
    cubeGeneratorLaw->setActionModel(spawnAction);

    // Bind trigger to "onMouseClicked"
    lawManager.bindTrigger(lawId, "onMouseClicked");

    check(cubeGeneratorLaw->isAuthored(), "Law is authored by player");
    check(lawManager.triggersOf(lawId).size() == 1 &&
          lawManager.triggersOf(lawId)[0] == "onMouseClicked",
          "Law trigger bound to onMouseClicked");

    // 4. Test Negative Case: activeTool is "Brush"
    std::cout << "\n[Test 1] Triggering event when activeTool = \"Brush\"..." << std::endl;
    player.activeTool = "Brush";
    player.active3DMode = "Select";
    
    ECA::Event event1{"onMouseClicked", &player, nullptr, 0};
    Core::EventBus::instance().publish(event1);
    lawManager.tick();

    check(world.getOwnedObjects().empty(), "No object created when activeTool is Brush");

    // 5. Test Placement Mode: "InFront"
    std::cout << "\n[Test 2] Triggering event in Placement Mode \"InFront\"..." << std::endl;
    player.activeTool = "ShapeGenerator3D";
    player.placementMode = "InFront";
    player.cursorHitPos = glm::vec3(15.0f, 3.0f, -8.0f);
    player.updatePlacement();

    ECA::Event event2{"onMouseClicked", &player, nullptr, 0};
    Core::EventBus::instance().publish(event2);
    lawManager.tick();

    check(world.getOwnedObjects().size() == 1, "Basic cube object spawned in InFront placement mode");
    if (!world.getOwnedObjects().empty()) {
        Object* obj = world.getOwnedObjects()[0].get();
        check(obj != nullptr, "Spawned object pointer is valid");
        check(obj->getShapeKind() == Object::ShapeKind::Cube, "Object is ShapeKind::Cube");
        check(nearVec3(obj->getPosition(), glm::vec3(15.0f, 3.0f, -8.0f)),
              "InFront mode placed object accurately at cursorHitPos (15, 3, -8)");
    }

    // 6. Test Placement Mode: "CursorSnap" (Grid Snap)
    std::cout << "\n[Test 3] Triggering event in Placement Mode \"CursorSnap\" (Grid Snap)..." << std::endl;
    player.activeTool = "Custom Shape";
    player.placementMode = "CursorSnap";
    player.gridSnapSize = 2.0f; // 2-unit grid snap
    player.cursorHitPos = glm::vec3(10.4f, 3.8f, -7.1f); // Should snap to (10.0, 4.0, -8.0)
    player.updatePlacement();

    ECA::Event event3{"onMouseClicked", &player, nullptr, 0};
    Core::EventBus::instance().publish(event3);
    lawManager.tick();

    check(world.getOwnedObjects().size() == 2, "Second basic cube spawned in CursorSnap placement mode");
    if (world.getOwnedObjects().size() >= 2) {
        Object* obj = world.getOwnedObjects()[1].get();
        check(obj != nullptr, "Second spawned object pointer is valid");
        check(obj->getShapeKind() == Object::ShapeKind::Cube, "Second object is ShapeKind::Cube");
        check(nearVec3(obj->getPosition(), glm::vec3(10.0f, 4.0f, -8.0f)),
              "CursorSnap mode accurately snapped position (10.4, 3.8, -7.1) to grid (10.0, 4.0, -8.0)");
    }

    // 7. Test Placement Mode: "ManualDistance"
    std::cout << "\n[Test 4] Triggering event in Placement Mode \"ManualDistance\"..." << std::endl;
    player.activeTool = "ShapeGenerator3D";
    player.placementMode = "ManualDistance";
    player.manualDistance = 12.0f;
    player.cameraPos = glm::vec3(0.0f, 2.0f, 0.0f);
    player.cameraForward = glm::vec3(0.0f, 0.0f, -1.0f); // Looking along -Z
    player.updatePlacement(); // Expected position: (0.0, 2.0, -12.0)

    ECA::Event event4{"onMouseClicked", &player, nullptr, 0};
    Core::EventBus::instance().publish(event4);
    lawManager.tick();

    check(world.getOwnedObjects().size() == 3, "Third basic cube spawned in ManualDistance placement mode");
    if (world.getOwnedObjects().size() >= 3) {
        Object* obj = world.getOwnedObjects()[2].get();
        check(obj != nullptr, "Third spawned object pointer is valid");
        check(obj->getShapeKind() == Object::ShapeKind::Cube, "Third object is ShapeKind::Cube");
        check(nearVec3(obj->getPosition(), glm::vec3(0.0f, 2.0f, -12.0f)),
              "ManualDistance mode accurately placed object at cameraPos + cameraForward * 12.0 (0, 2, -12)");
    }

    // 8. Test JSON Serialization & Reloading (Persistence of placement law)
    std::cout << "\n[Test 5] Testing JSON Serialization and Persistence of Placement Law..." << std::endl;
    nlohmann::json serializedLaws = lawManager.toJson();

    LawManager reloadedManager;
    reloadedManager.connectToEventBus();
    reloadedManager.loadFromJson(serializedLaws);

    Law* reloadedLaw = reloadedManager.find(lawId);
    check(reloadedLaw != nullptr, "Law restored from JSON using lawId");
    
    // Reattach author after reload
    reloadedLaw->addAuthor(player);
    check(reloadedLaw->isAuthored(), "Author reattached to reloaded law");
    check(reloadedManager.triggersOf(lawId).size() == 1 &&
          reloadedManager.triggersOf(lawId)[0] == "onMouseClicked",
          "Reloaded law retained onMouseClicked trigger binding");

    // Trigger reloaded law in CursorSnap mode with 0.5f grid size
    player.activeTool = "ShapeGenerator3D";
    player.placementMode = "CursorSnap";
    player.gridSnapSize = 0.5f;
    player.cursorHitPos = glm::vec3(4.24f, 1.76f, -3.12f); // Should snap to (4.0, 2.0, -3.0)
    player.updatePlacement();

    ECA::Event event5{"onMouseClicked", &player, nullptr, 0};
    Core::EventBus::instance().publish(event5);
    reloadedManager.tick();

    check(world.getOwnedObjects().size() == 4,
          "Reloaded law executed successfully on event, spawning fourth basic cube object");
    if (world.getOwnedObjects().size() >= 4) {
        Object* fourthObj = world.getOwnedObjects()[3].get();
        check(fourthObj->getShapeKind() == Object::ShapeKind::Cube,
              "Fourth spawned object shape kind is ShapeKind::Cube (Basic Cube)");
        check(nearVec3(fourthObj->getPosition(), glm::vec3(4.0f, 2.0f, -3.0f)),
              "Reloaded law accurately performed CursorSnap placement at (4.0, 2.0, -3.0)");
    }

    std::cout << "\n============================================================" << std::endl;
    if (g_failures > 0) {
        std::cout << "FAILED: " << g_failures << " of " << g_checks
                  << " checks failed for Basic Cube 3D Custom Shape Generator Law!" << std::endl;
    } else {
        std::cout << "SUCCESS! All " << g_checks << " checks passed for Basic Cube 3D Custom Shape Generator Law!" << std::endl;
    }
    std::cout << "============================================================" << std::endl;

    glfwDestroyWindow(window);
    glfwTerminate();
    return g_failures > 0 ? 1 : 0;
}
