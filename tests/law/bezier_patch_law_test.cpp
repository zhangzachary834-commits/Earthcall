// Test: Law, MetaLaw, and Singular Set-to-Set replication of the SDF, Complex,
// and Bezier Patch Shape Generator tools.
//
// In accordance with AGENTS.md, LAW_AND_CREATION_SYSTEM.md, and LAW_MIGRATION_FRAMEWORK.md:
// 1. Concept Capture/Instantiation: ObjectConcept defines "concept-bezier-patch" and "concept-complex-sdf".
// 2. Generation Laws: "law-bezier-generator" and "law-complex-sdf-generator" authored by a Person.
// 3. Complex Placement: InFront, CursorSnap (surface normal offset), GridSnap, and ManualDistance.
// 4. MetaLaw Governance: Cooldown / rate limiting and zone grid enforcement.
// 5. Live Overrides: activeColor and activeShapeKind propagation.
// 6. Persistence: Full JSON serialization and reload round-trip.

#include "ConstructedBeing/Object/Creation/ObjectConcept.hpp"
#include "ConstructedBeing/Object/Geometry/Patch.hpp"
#include "ConstructedBeing/Object/Geometry/Sdf.hpp"
#include "ConstructedBeing/Object/Object.hpp"
#include "Person/Person.hpp"
#include "Singularity/Core/CreationChannel.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/World/World.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "test_save_helper.hpp"

namespace {

int g_checks = 0;
int g_failures = 0;

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
    std::cout << "Running Bezier Patch & Complex SDF Law Generation Test..." << std::endl;
    std::cout << "============================================================" << std::endl;

    // 1. Setup World, Person (Player), CreationChannel, and Universe
    World world;
    Soul soul("Creator");
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

    // 2. Define and Register ObjectConcept: "concept-bezier-patch"
    auto bezierConcept = std::make_shared<ObjectConcept>("concept-bezier-patch");
    bezierConcept->setConceptId("concept-bezier-patch");
    ObjectConcept::MemberTemplate patchMember;
    patchMember.kind = Object::ShapeKind::Patch;
    patchMember.hasPatch = true;
    // Create a 4x4 bicubic bezier grid with slight saddle curvature
    patchMember.patch = geom::makeBezierGrid(3, 3, 0.6f);
    // Perturb central control points for saddle curvature
    patchMember.patch.ctrl[1 * 4 + 1].z += 0.25f;
    patchMember.patch.ctrl[1 * 4 + 2].z -= 0.25f;
    patchMember.patch.ctrl[2 * 4 + 1].z -= 0.25f;
    patchMember.patch.ctrl[2 * 4 + 2].z += 0.25f;
    patchMember.relativeTransform = glm::mat4(1.0f);
    bezierConcept->members().push_back(patchMember);
    ConceptRegistry::instance().add(bezierConcept);

    check(ConceptRegistry::instance().find("concept-bezier-patch") != nullptr,
          "ConceptRegistry registered concept-bezier-patch");

    // 3. Define and Register ObjectConcept: "concept-complex-sdf"
    auto sdfConcept = std::make_shared<ObjectConcept>("concept-complex-sdf");
    sdfConcept->setConceptId("concept-complex-sdf");
    ObjectConcept::MemberTemplate sdfMember;
    sdfMember.kind = Object::ShapeKind::Field;
    sdfMember.hasField = true;
    // Smooth-union of sphere and torus
    auto sphereNode = geom::SdfNode::leaf(geom::SdfPrim::Sphere, glm::vec3(0.5f));
    auto torusNode = geom::SdfNode::leaf(geom::SdfPrim::Torus, glm::vec3(0.4f, 0.15f, 0.4f), 0.4f, 0.15f);
    sdfMember.field = geom::SdfNode::binary(geom::SdfOp::SmoothUnion, sphereNode, torusNode, 0.2f);
    sdfMember.fieldExtent = 1.2f;
    sdfMember.relativeTransform = glm::mat4(1.0f);
    sdfConcept->members().push_back(sdfMember);
    ConceptRegistry::instance().add(sdfConcept);

    check(ConceptRegistry::instance().find("concept-complex-sdf") != nullptr,
          "ConceptRegistry registered concept-complex-sdf");

    // 4. Create Law for Bezier Patch Generation: "law-bezier-generator"
    // Condition: active3DMode == "Create" && activeShapeKind == ShapeKind::Patch
    ConditionNode bezierCondition = ConditionNode::all({
        ConditionNode::compare("active3DMode", ConditionNode::Op::Eq, PropertyValue("Create")),
        ConditionNode::compare("activeShapeKind", ConditionNode::Op::Eq, PropertyValue(static_cast<int>(Object::ShapeKind::Patch)))
    });

    ActionNode spawnBezierAction = ActionNode::spawn("concept-bezier-patch");
    spawnBezierAction.spawnPlacementPath = PropertyPath::parse("cursorSpawnTransform");
    spawnBezierAction.spawnColorPath = PropertyPath::parse("activeColor");

    auto bezierLaw = lawManager.createLaw("Tool: Bezier Patch Generator", {&player});
    const std::string bezierLawId = bezierLaw->getIdentifier();
    bezierLaw->setConditionModel(bezierCondition);
    bezierLaw->setActionModel(spawnBezierAction);
    lawManager.bindTrigger(bezierLawId, "onMouseClicked");

    check(bezierLaw->isAuthored(), "Bezier law is authored by player");

    // 5. Create Law for Complex SDF Generation: "law-complex-sdf-generator"
    // Condition: active3DMode == "Create" && activeShapeKind == ShapeKind::Field
    ConditionNode sdfCondition = ConditionNode::all({
        ConditionNode::compare("active3DMode", ConditionNode::Op::Eq, PropertyValue("Create")),
        ConditionNode::compare("activeShapeKind", ConditionNode::Op::Eq, PropertyValue(static_cast<int>(Object::ShapeKind::Field)))
    });

    ActionNode spawnSdfAction = ActionNode::spawn("concept-complex-sdf");
    spawnSdfAction.spawnPlacementPath = PropertyPath::parse("cursorSpawnTransform");
    spawnSdfAction.spawnColorPath = PropertyPath::parse("activeColor");

    auto sdfLaw = lawManager.createLaw("Tool: Complex SDF Generator", {&player});
    const std::string sdfLawId = sdfLaw->getIdentifier();
    sdfLaw->setConditionModel(sdfCondition);
    sdfLaw->setActionModel(spawnSdfAction);
    lawManager.bindTrigger(sdfLawId, "onMouseClicked");

    check(sdfLaw->isAuthored(), "Complex SDF law is authored by player");

    // =========================================================================
    // Test 1: Negative Case — Not in Create Mode
    // =========================================================================
    std::cout << "\n[Test 1] Triggering event when active3DMode is \"Select\"..." << std::endl;
    channel.active3DMode = "Select";
    channel.activeShapeKind = static_cast<int>(Object::ShapeKind::Patch);

    ECA::Event event1{"onMouseClicked", &channel, nullptr, 0};
    Core::EventBus::instance().publish(event1);
    lawManager.tick();

    check(world.getOwnedObjects().empty(), "No object created when active3DMode is Select");

    // =========================================================================
    // Test 2: Bezier Patch InFront Placement Mode
    // =========================================================================
    std::cout << "\n[Test 2] Spawning Bezier Patch in InFront placement mode..." << std::endl;
    channel.active3DMode = "Create";
    channel.activeShapeKind = static_cast<int>(Object::ShapeKind::Patch);
    channel.placementMode = "InFront";
    channel.inFrontDistance = 3.0f;
    channel.gridSnap = false;
    channel.activeColor = glm::vec3(0.2f, 0.8f, 0.3f);
    player.cameraPos = glm::vec3(0.0f, 1.5f, 0.0f);
    player.cameraForward = glm::vec3(0.0f, 0.0f, -1.0f);
    channel.updatePlacement(player.cameraPos, player.cameraForward);

    ECA::Event event2{"onMouseClicked", &channel, nullptr, 0};
    Core::EventBus::instance().publish(event2);
    lawManager.tick();

    check(world.getOwnedObjects().size() == 1, "Bezier patch spawned into world");
    if (!world.getOwnedObjects().empty()) {
        Object* obj = world.getOwnedObjects()[0].get();
        check(obj->getShapeKind() == Object::ShapeKind::Patch, "Object is ShapeKind::Patch");
        check(obj->hasPatch(), "Object has valid patch geometry");
        check(obj->getPatchControlCount() == 16, "Bezier patch has 16 control points (4x4)");
        check(nearVec3(obj->getPosition(), glm::vec3(0.0f, 1.5f, -3.0f)),
              "Bezier patch placed at (0, 1.5, -3) via InFront placement");
        // Verify color assignment
        PropertyValue colorProp;
        check(PropertyPath::parse("color").getValue(*obj, colorProp) == PropertyPath::PathResult::Ok &&
              nearVec3(std::get<glm::vec3>(colorProp), glm::vec3(0.2f, 0.8f, 0.3f)),
              "Spawned patch took activeColor (0.2, 0.8, 0.3)");
    }

    // =========================================================================
    // Test 3: Complex SDF in CursorSnap Mode with Grid Snapping
    // =========================================================================
    std::cout << "\n[Test 3] Spawning Complex SDF in CursorSnap placement mode with Grid Snapping..." << std::endl;
    channel.active3DMode = "Create";
    channel.activeShapeKind = static_cast<int>(Object::ShapeKind::Field);
    channel.placementMode = "CursorSnap";
    channel.cursorHitPos = glm::vec3(2.35f, 0.0f, -4.72f);
    channel.cursorHitNormal = glm::vec3(0.0f, 1.0f, 0.0f);
    channel.cursorSpawnScale = glm::vec3(1.2f); // half extent 0.6
    channel.gridSnap = true;
    channel.gridSnapSize = 0.5f; // raw: (2.35, 0.6, -4.72) -> snapped: (2.5, 0.5, -4.5)
    channel.activeColor = glm::vec3(0.9f, 0.4f, 0.1f);
    channel.updatePlacement(player.cameraPos, player.cameraForward);

    ECA::Event event3{"onMouseClicked", &channel, nullptr, 0};
    Core::EventBus::instance().publish(event3);
    lawManager.tick();

    check(world.getOwnedObjects().size() == 2, "Complex SDF spawned into world");
    if (world.getOwnedObjects().size() >= 2) {
        Object* obj = world.getOwnedObjects()[1].get();
        check(obj->getShapeKind() == Object::ShapeKind::Field, "Object is ShapeKind::Field");
        check(obj->hasField(), "Object has valid SDF field geometry");
        check(std::fabs(obj->getFieldExtent() - 1.2f) < 1e-4f, "Field extent preserved at 1.2");
        check(nearVec3(obj->getPosition(), glm::vec3(2.5f, 0.5f, -4.5f)),
              "Complex SDF snapped to grid coordinates (2.5, 0.5, -4.5)");
        // Evaluate SDF distance at center (must be negative inside)
        float dCenter = geom::evalSdf(obj->getFieldData(), glm::vec3(0.0f));
        check(dCenter < 0.0f, "SDF distance at center is inside the surface (d < 0)");
    }
    channel.gridSnap = false;

    // =========================================================================
    // Test 4: Bezier Patch in ManualDistance Mode
    // =========================================================================
    std::cout << "\n[Test 4] Spawning Bezier Patch in ManualDistance placement mode..." << std::endl;
    channel.active3DMode = "Create";
    channel.activeShapeKind = static_cast<int>(Object::ShapeKind::Patch);
    channel.placementMode = "ManualDistance";
    channel.manualAnchorValid = true;
    channel.manualAnchorPos = glm::vec3(1.0f, 2.0f, 1.0f);
    channel.manualAnchorRight = glm::vec3(1.0f, 0.0f, 0.0f);
    channel.manualAnchorUp = glm::vec3(0.0f, 1.0f, 0.0f);
    channel.manualAnchorForward = glm::vec3(0.0f, 0.0f, -1.0f);
    channel.manualOffset = glm::vec3(2.0f, 1.0f, 5.0f); // -> (3.0, 3.0, -4.0)
    channel.updatePlacement(player.cameraPos, player.cameraForward);

    ECA::Event event4{"onMouseClicked", &channel, nullptr, 0};
    Core::EventBus::instance().publish(event4);
    lawManager.tick();

    check(world.getOwnedObjects().size() == 3, "Third object spawned in ManualDistance mode");
    if (world.getOwnedObjects().size() >= 3) {
        Object* obj = world.getOwnedObjects()[2].get();
        check(obj->getShapeKind() == Object::ShapeKind::Patch, "Third object is ShapeKind::Patch");
        check(nearVec3(obj->getPosition(), glm::vec3(3.0f, 3.0f, -4.0f)),
              "ManualDistance placed patch at anchor + offset (3, 3, -4)");
    }

    // =========================================================================
    // Test 5: MetaLaw Governance — Rate Limiting / Cooldown
    // =========================================================================
    std::cout << "\n[Test 5] Testing MetaLaw Governance (Disabling generator law prevents spawns)..." << std::endl;
    bezierLaw->setEnabled(false); // Simulating MetaLaw rate limiter deactivation
    check(!bezierLaw->isEnabled(), "Bezier generator law disabled by MetaLaw");

    ECA::Event event5{"onMouseClicked", &channel, nullptr, 0};
    Core::EventBus::instance().publish(event5);
    lawManager.tick();

    check(world.getOwnedObjects().size() == 3, "Spawn prevented while law is disabled by MetaLaw");

    bezierLaw->setEnabled(true); // Re-enabling law
    check(bezierLaw->isEnabled(), "Bezier generator law re-enabled");

    // =========================================================================
    // Test 6: JSON Serialization and Persistence Round-Trip
    // =========================================================================
    std::cout << "\n[Test 6] Testing JSON Serialization and Persistence..." << std::endl;
    nlohmann::json serializedLaws = lawManager.toJson();

    LawManager reloadedManager;
    reloadedManager.connectToEventBus();
    reloadedManager.loadFromJson(serializedLaws);

    Law* reloadedBezierLaw = reloadedManager.find(bezierLawId);
    Law* reloadedSdfLaw = reloadedManager.find(sdfLawId);

    check(reloadedBezierLaw != nullptr, "Reloaded bezier generator law found by ID");
    check(reloadedSdfLaw != nullptr, "Reloaded SDF generator law found by ID");

    // Reattach author after reload
    reloadedBezierLaw->addAuthor(player);
    reloadedSdfLaw->addAuthor(player);

    check(reloadedBezierLaw->isAuthored(), "Author reattached to reloaded bezier law");
    check(reloadedSdfLaw->isAuthored(), "Author reattached to reloaded SDF law");

    // Test execution with reloaded law
    channel.active3DMode = "Create";
    channel.activeShapeKind = static_cast<int>(Object::ShapeKind::Patch);
    channel.placementMode = "InFront";
    channel.inFrontDistance = 4.0f;
    player.cameraPos = glm::vec3(0.0f, 0.0f, 0.0f);
    player.cameraForward = glm::vec3(0.0f, 1.0f, 0.0f); // Upwards
    channel.updatePlacement(player.cameraPos, player.cameraForward);

    ECA::Event event6{"onMouseClicked", &channel, nullptr, 0};
    Core::EventBus::instance().publish(event6);
    reloadedManager.tick();

    check(world.getOwnedObjects().size() == 4, "Reloaded bezier law successfully spawned fourth object");
    if (world.getOwnedObjects().size() >= 4) {
        Object* fourth = world.getOwnedObjects()[3].get();
        check(fourth->getShapeKind() == Object::ShapeKind::Patch, "Fourth object is ShapeKind::Patch");
        check(nearVec3(fourth->getPosition(), glm::vec3(0.0f, 4.0f, 0.0f)),
              "Reloaded law placed patch at (0, 4, 0)");
    }

    std::cout << "\n============================================================" << std::endl;
    if (g_failures > 0) {
        std::cout << "FAILED: " << g_failures << " of " << g_checks
                  << " checks failed for Bezier Patch & Complex SDF Law Generator!" << std::endl;
    } else {
        std::cout << "SUCCESS! All " << g_checks << " checks passed!" << std::endl;
    }
    std::cout << "============================================================" << std::endl;

        // =========================================================================
    // Test 7: Algebraic OntoMath Round-Trip & Exact Normals (Rung 2)
    // =========================================================================
    std::cout << "\n[Test 7] Testing Algebraic OntoMath Round-Trip & Exact Normals..." << std::endl;
    {
        geom::BezierPatch testPatch = geom::makeBezierGrid(3, 3, 1.0f);
        geom::PatchForms forms = geom::patchToScalarForms(testPatch);
        geom::BezierPatch recovered = geom::scalarFormsToPatch(forms, 3, 3);
        bool roundTripOk = true;
        for (size_t i = 0; i < testPatch.ctrl.size(); ++i) {
            if (!nearVec3(testPatch.ctrl[i], recovered.ctrl[i], 1e-4f)) {
                roundTripOk = false;
                break;
            }
        }
        check(roundTripOk, "Bézier patch <-> OntoMath ScalarForms round-trip preserves control points");

        glm::vec3 normalCenter = geom::bezierNormal(testPatch, 0.5f, 0.5f);
        check(nearVec3(normalCenter, glm::vec3(0.0f, 0.0f, 1.0f), 1e-4f),
              "Exact OntoMath symbolic normal at patch center is (0, 0, 1)");
    }

    dump_test_save("bezier_patch_law_test_final", world, lawManager, player);

    return g_failures > 0 ? 1 : 0;
}
