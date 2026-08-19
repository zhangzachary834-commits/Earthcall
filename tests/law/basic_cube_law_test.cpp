// Test: Law replicating the exact functionality of the basic cube 3D Custom Shape generator tool,
// including complex placement capabilities: InFront placement, Cursor Grid Snap, and Manual Distance.
//
// Capabilities tested:
// 1. Triggers on "onMouseClicked" event.
// 2. Evaluates condition: the creation channel's active3DMode == "Create".
// 3. Placement Mode: "InFront" - places shape at cursorHitPos.
// 4. Placement Mode: "CursorSnap" - snaps shape placement to gridSnapSize increments.
// 5. Placement Mode: "ManualDistance" - places shape at cameraPos + cameraForward * manualDistance.
// 6. Executes action: Spawns/creates a 3D Shape object of kind ShapeKind::Cube (basic cube) at cursorSpawnTransform.
// 7. Persistence: Round-trips cleanly through JSON serialization preserving all placement law functionality.

#include "ConstructedBeing/Object/Creation/ObjectConcept.hpp"
#include "ConstructedBeing/Object/Object.hpp"
#include "Person/Person.hpp"
#include "Singularity/Core/CreationChannel.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/World/World.hpp"

#include <cassert>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "test_save_helper.hpp"

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

    // 1. Setup World, Player (author), creation channel, and Universe.
    //
    // The tool selection and the placement arithmetic are the state of the
    // CREATION CHANNEL, not of the Person. They lived on Person until the
    // refusals audit took them off (a Person is not a Person's tool); they are
    // now `Singularity::Core::CreationChannel`, a First Mover under
    // Singularity/ whose fields are registered properties, so the law text
    // below reads exactly the same paths it always read — only the being those
    // paths are addressed against has changed. Person keeps cameraPos and
    // cameraForward, which are facts about where the Person is looking from.
    World world;
    Soul soul("Player");
    Body body("humanoid", "default");
    Person player(std::move(soul), std::move(body), "default");
    player.position = glm::vec3(0.0f, 1.0f, 0.0f);

    Singularity::Core::CreationChannel channel;

    Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
        beings.push_back(&world);
        beings.push_back(&player);
        beings.push_back(&channel);
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

    // 3. Create the Law replicating the 3D Custom Shape Generator Tool.
    //
    // Condition: active3DMode == "Create", and nothing else. The tool only
    // ever ran inside Mode3D::BrushCreate. Two conditions that were here are
    // deliberately gone:
    //   - activeTool == "ShapeGenerator3D" is unsatisfiable. Tool::Type has no
    //     such member since the hard-coded tool was deleted, so getTypeName()
    //     can never return it.
    //   - activeTool == "Custom Shape" is Tool::Type::CustomShape, a 2D DESIGN
    //     tool. OR-ed in, it fired the law on any click with that tool
    //     selected -- including while drawing in 2D, or in Selection mode.
    ConditionNode toolCondition =
        ConditionNode::compare("active3DMode", ConditionNode::Op::Eq, PropertyValue("Create"));

    // Action: spawn the concept at cursorSpawnTransform, taking the shape kind
    // and colour from the author's live selection the way the tool read
    // getCurrentShapeKind()/getCurrentColor() on every click.
    ActionNode spawnAction = ActionNode::spawn(cubeConcept->getIdentifier());
    spawnAction.spawnPlacementPath = PropertyPath::parse("cursorSpawnTransform");
    spawnAction.spawnShapeKindPath = PropertyPath::parse("activeShapeKind");
    spawnAction.spawnColorPath = PropertyPath::parse("activeColor");

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

    // Generate test save
    dump_test_save("basic_cube_law_test", world, lawManager, player);

    // 4. Test Negative Case: not in Create mode
    std::cout << "\n[Test 1] Triggering event when activeTool = \"Brush\"..." << std::endl;
    channel.activeTool = "Brush";
    channel.active3DMode = "Select";
    channel.activeShapeKind = static_cast<int>(Object::ShapeKind::Cube);
    channel.activeColor = glm::vec3(1.0f, 1.0f, 1.0f);

    ECA::Event event1{"onMouseClicked", &channel, nullptr, 0};
    Core::EventBus::instance().publish(event1);
    lawManager.tick();

    check(world.getOwnedObjects().empty(), "No object created when activeTool is Brush");

    // The 2D design tool must NOT create 3D geometry. This is the misfire the
    // old `activeTool == "Custom Shape"` condition caused: that string is
    // Tool::Type::CustomShape, so every click while it was selected spawned a
    // cube regardless of mode -- even while drawing in 2D.
    std::cout << "\n[Test 1b] Triggering with the 2D \"Custom Shape\" tool outside Create mode..." << std::endl;
    channel.activeTool = "Custom Shape";
    channel.active3DMode = "Select";

    ECA::Event event1b{"onMouseClicked", &channel, nullptr, 0};
    Core::EventBus::instance().publish(event1b);
    lawManager.tick();

    check(world.getOwnedObjects().empty(),
          "2D \"Custom Shape\" tool creates nothing outside Create mode");

    // 5. Test Placement Mode: "InFront" -- camera position plus forward, the
    //    distance the tool hard-coded as 2.0. NOT the cursor hit point.
    std::cout << "\n[Test 2] Triggering event in Placement Mode \"InFront\"..." << std::endl;
    channel.activeTool = "Brush";          // irrelevant now: only the mode gates
    channel.active3DMode = "Create";
    channel.placementMode = "InFront";
    player.cameraPos = glm::vec3(0.0f, 2.0f, 0.0f);
    player.cameraForward = glm::vec3(0.0f, 0.0f, -1.0f);
    channel.inFrontDistance = 2.0f;
    channel.cursorHitPos = glm::vec3(15.0f, 3.0f, -8.0f);   // must be ignored
    channel.updatePlacement(player.cameraPos, player.cameraForward);

    ECA::Event event2{"onMouseClicked", &channel, nullptr, 0};
    Core::EventBus::instance().publish(event2);
    lawManager.tick();

    check(world.getOwnedObjects().size() == 1, "Basic cube object spawned in InFront placement mode");
    if (!world.getOwnedObjects().empty()) {
        Object* obj = world.getOwnedObjects()[0].get();
        check(obj != nullptr, "Spawned object pointer is valid");
        check(obj->getShapeKind() == Object::ShapeKind::Cube, "Object is ShapeKind::Cube");
        check(nearVec3(obj->getPosition(), glm::vec3(0.0f, 2.0f, -2.0f)),
              "InFront mode placed object at cameraPos + forward * 2.0 (0, 2, -2)");
    }

    // 6. Test Placement Mode: "CursorSnap" -- against the surface under the
    //    cursor, pushed out along its normal by the shape's own half extent so
    //    it RESTS on the surface instead of sinking halfway into it.
    std::cout << "\n[Test 3] Triggering event in Placement Mode \"CursorSnap\" (surface placement)..." << std::endl;
    channel.placementMode = "CursorSnap";
    channel.gridSnap = false;
    channel.cursorSpawnScale = glm::vec3(1.0f);              // half extent 0.5
    channel.cursorHitPos = glm::vec3(10.0f, 3.0f, -7.0f);
    channel.cursorHitNormal = glm::vec3(0.0f, 1.0f, 0.0f);   // floor
    channel.updatePlacement(player.cameraPos, player.cameraForward);

    ECA::Event event3{"onMouseClicked", &channel, nullptr, 0};
    Core::EventBus::instance().publish(event3);
    lawManager.tick();

    check(world.getOwnedObjects().size() == 2, "Second basic cube spawned in CursorSnap placement mode");
    if (world.getOwnedObjects().size() >= 2) {
        Object* obj = world.getOwnedObjects()[1].get();
        check(obj != nullptr, "Second spawned object pointer is valid");
        check(obj->getShapeKind() == Object::ShapeKind::Cube, "Second object is ShapeKind::Cube");
        check(nearVec3(obj->getPosition(), glm::vec3(10.0f, 3.5f, -7.0f)),
              "CursorSnap rests the shape ON the surface: hit (10,3,-7) + normal * half extent 0.5");
    }

    // 6b. Grid snap is ORTHOGONAL to the mode -- it rounds whatever the mode
    //     produced. Proven here in InFront mode, which the old code could not
    //     snap at all because snapping only existed inside CursorSnap.
    std::cout << "\n[Test 3b] Grid snap applies to InFront placement too..." << std::endl;
    channel.placementMode = "InFront";
    player.cameraPos = glm::vec3(0.4f, 2.2f, 0.0f);
    player.cameraForward = glm::vec3(0.0f, 0.0f, -1.0f);
    channel.inFrontDistance = 2.3f;          // raw: (0.4, 2.2, -2.3)
    channel.gridSnap = true;
    channel.gridSnapSize = 1.0f;             // snapped: (0, 2, -2)
    channel.updatePlacement(player.cameraPos, player.cameraForward);

    ECA::Event event3b{"onMouseClicked", &channel, nullptr, 0};
    Core::EventBus::instance().publish(event3b);
    lawManager.tick();

    check(world.getOwnedObjects().size() == 3, "Cube spawned with grid snap in InFront mode");
    if (world.getOwnedObjects().size() >= 3) {
        check(nearVec3(world.getOwnedObjects()[2].get()->getPosition(), glm::vec3(0.0f, 2.0f, -2.0f)),
              "Grid snap rounded an InFront placement (0.4, 2.2, -2.3) to (0, 2, -2)");
    }
    channel.gridSnap = false;

    // 7. Test Placement Mode: "ManualDistance" -- measured from a FROZEN
    //    anchor, not the live camera, so the shape stays where it was put.
    std::cout << "\n[Test 4] Triggering event in Placement Mode \"ManualDistance\"..." << std::endl;
    channel.placementMode = "ManualDistance";
    channel.manualAnchorValid = true;
    channel.manualAnchorPos = glm::vec3(0.0f, 2.0f, 0.0f);
    channel.manualAnchorRight = glm::vec3(1.0f, 0.0f, 0.0f);
    channel.manualAnchorUp = glm::vec3(0.0f, 1.0f, 0.0f);
    channel.manualAnchorForward = glm::vec3(0.0f, 0.0f, -1.0f);
    channel.manualOffset = glm::vec3(0.0f, 0.0f, 12.0f);   // 12 along anchor forward
    channel.updatePlacement(player.cameraPos, player.cameraForward);                              // -> (0, 2, -12)

    ECA::Event event4{"onMouseClicked", &channel, nullptr, 0};
    Core::EventBus::instance().publish(event4);
    lawManager.tick();

    check(world.getOwnedObjects().size() == 4, "Fourth basic cube spawned in ManualDistance placement mode");
    if (world.getOwnedObjects().size() >= 4) {
        Object* obj = world.getOwnedObjects()[3].get();
        check(obj != nullptr, "Fourth spawned object pointer is valid");
        check(obj->getShapeKind() == Object::ShapeKind::Cube, "Fourth object is ShapeKind::Cube");
        check(nearVec3(obj->getPosition(), glm::vec3(0.0f, 2.0f, -12.0f)),
              "ManualDistance placed object at anchor + offset (0, 2, -12)");
    }

    // The anchor is the whole point of the mode: turning the camera must not
    // drag the placement along. The old scalar-distance version failed this.
    std::cout << "\n[Test 4b] Moving the camera must not move an anchored placement..." << std::endl;
    player.cameraPos = glm::vec3(50.0f, 40.0f, 30.0f);
    player.cameraForward = glm::vec3(1.0f, 0.0f, 0.0f);
    channel.updatePlacement(player.cameraPos, player.cameraForward);
    check(nearVec3(channel.cursorSpawnPos, glm::vec3(0.0f, 2.0f, -12.0f)),
          "Anchored placement stayed at (0, 2, -12) after the camera moved");

    // 7c. Shape kind and colour come from the author's live selection, the way
    //     the tool read getCurrentShapeKind()/getCurrentColor() every click.
    //     The concept's template still says Cube; the override wins.
    std::cout << "\n[Test 4c] Shape kind and colour follow the live selection..." << std::endl;
    channel.placementMode = "InFront";
    player.cameraPos = glm::vec3(0.0f, 0.0f, 0.0f);
    player.cameraForward = glm::vec3(0.0f, 0.0f, -1.0f);
    channel.inFrontDistance = 3.0f;
    channel.activeShapeKind = static_cast<int>(Object::ShapeKind::Sphere);
    channel.activeColor = glm::vec3(1.0f, 0.0f, 0.0f);
    channel.updatePlacement(player.cameraPos, player.cameraForward);

    ECA::Event event4c{"onMouseClicked", &channel, nullptr, 0};
    Core::EventBus::instance().publish(event4c);
    lawManager.tick();

    check(world.getOwnedObjects().size() == 5, "Fifth object spawned with an overridden shape kind");
    if (world.getOwnedObjects().size() >= 5) {
        Object* obj = world.getOwnedObjects()[4].get();
        check(obj->getShapeKind() == Object::ShapeKind::Sphere,
              "Spawned a Sphere from activeShapeKind, overriding the concept's Cube template");
        // Every face the object actually HAS, not a fixed six: a Sphere is one
        // smooth surface with one face, so colouring six slots would assert
        // against slots the shape does not own. The face slots are the object's
        // own colour — the per-face TEXTURES moved to Material with the rest of
        // the paint system, and a material is shared by name, so an object's
        // own selected colour is faceColors, which is what "color" reads back.
        const int faces = std::min(obj->getFaces() > 0 ? obj->getFaces() : 6, 6);
        bool allRed = faces > 0;
        for (int f = 0; f < faces; ++f) {
            allRed = allRed &&
                     std::fabs(obj->faceColors[f][0] - 1.0f) < 1e-5f &&
                     std::fabs(obj->faceColors[f][1] - 0.0f) < 1e-5f &&
                     std::fabs(obj->faceColors[f][2] - 0.0f) < 1e-5f;
        }
        check(allRed, "Every face took activeColor (1, 0, 0), as the tool coloured them");

        // And it reads back through the property surface a law would use.
        PropertyValue live;
        check(PropertyPath::parse("color").getValue(*obj, live) == PropertyPath::PathResult::Ok &&
              nearVec3(std::get<glm::vec3>(live), glm::vec3(1.0f, 0.0f, 0.0f)),
              "The spawned object's \"color\" property reads back the selected red");
    }
    // Put the selection back so the persistence case below stays a cube.
    channel.activeShapeKind = static_cast<int>(Object::ShapeKind::Cube);
    channel.activeColor = glm::vec3(1.0f, 1.0f, 1.0f);

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

    // The overrides are law text and must survive the round trip too --
    // without them the reloaded law silently reverts to the concept's Cube.
    const ActionNode* reloadedAction = reloadedLaw->actionModel();
    check(reloadedAction != nullptr &&
          reloadedAction->spawnShapeKindPath.toString() == "activeShapeKind",
          "Reloaded law retained its shape-kind override path");
    check(reloadedAction != nullptr &&
          reloadedAction->spawnColorPath.toString() == "activeColor",
          "Reloaded law retained its colour override path");

    // Trigger the reloaded law: surface placement with grid snap ON, i.e. both
    // of the things the old code could not combine.
    channel.placementMode = "CursorSnap";
    channel.cursorSpawnScale = glm::vec3(1.0f);
    channel.cursorHitPos = glm::vec3(4.24f, 1.76f, -3.12f);
    channel.cursorHitNormal = glm::vec3(0.0f, 1.0f, 0.0f);   // raw: (4.24, 2.26, -3.12)
    channel.gridSnap = true;
    channel.gridSnapSize = 0.5f;                              // snapped: (4.0, 2.5, -3.0)
    channel.updatePlacement(player.cameraPos, player.cameraForward);

    ECA::Event event5{"onMouseClicked", &channel, nullptr, 0};
    Core::EventBus::instance().publish(event5);
    reloadedManager.tick();

    check(world.getOwnedObjects().size() == 6,
          "Reloaded law executed successfully on event, spawning a sixth object");
    if (world.getOwnedObjects().size() >= 6) {
        Object* sixth = world.getOwnedObjects()[5].get();
        check(sixth->getShapeKind() == Object::ShapeKind::Cube,
              "Sixth spawned object shape kind is ShapeKind::Cube (Basic Cube)");
        check(nearVec3(sixth->getPosition(), glm::vec3(4.0f, 2.5f, -3.0f)),
              "Reloaded law combined surface offset and grid snap: (4.0, 2.5, -3.0)");
    }

    std::cout << "\n============================================================" << std::endl;
    if (g_failures > 0) {
        std::cout << "FAILED: " << g_failures << " of " << g_checks
                  << " checks failed for Basic Cube 3D Custom Shape Generator Law!" << std::endl;
    } else {
        std::cout << "SUCCESS! All " << g_checks << " checks passed for Basic Cube 3D Custom Shape Generator Law!" << std::endl;
    }
    std::cout << "============================================================" << std::endl;

    // Generate final test save containing all the spawned objects
    dump_test_save("basic_cube_law_test_final", world, lawManager, player);

    return g_failures > 0 ? 1 : 0;
}
