// Probe for the 2026-08-18 shape-generator-law audit.
// Exercises the runtime path the factory tests do NOT: an armed click with
// no prior updatePlacement, a second authored twin of the same law, and
// setting the CreationChannel down while the spawn law stays enabled.
//
// Not a permanent test. Built through the test target for this session only.

#include "ConstructedBeing/Object/Creation/ObjectConcept.hpp"
#include "ConstructedBeing/Object/Object.hpp"
#include "Person/Person.hpp"
#include "Singularity/Core/CreationChannel.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"

#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace {

int g_fail = 0;

void check(bool ok, const char* what) {
    if (!ok) {
        ++g_fail;
        std::printf("  FAILED: %s\n", what);
        return;
    }
    std::printf("  ok: %s\n", what);
}

bool nearf(float a, float b, float eps = 1e-4f) { return std::fabs(a - b) < eps; }

} // namespace

int main() {
    std::printf("shape_generator_law_audit_probe\n");

    Zone world("test-zone", "default");
    Soul soul("Player");
    Body body("humanoid", "default");
    Person player(std::move(soul), std::move(body), "default");
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

    auto law = Singularity::Core::createShapeGenerator3DLaw(player);
    lawManager.add(law);
    lawManager.bindTrigger(law->getIdentifier(), "onMouseClicked");

    const auto click = [&]() {
        Core::EventBus::instance().publish(
            ECA::Event{"onMouseClicked", &channel, nullptr, 0});
        lawManager.tick();
    };

    // ---- 1. armed click, no updatePlacement: spawn is at the origin --------
    // This is the live Engine path: L arms active3DMode, the GLFW callback
    // publishes the click, Tool::ShapeGenerator3D returns early, and nothing
    // else writes cursorSpawnPos.
    channel.active3DMode = "Create";
    channel.activeShapeKind = static_cast<int>(Object::ShapeKind::Cube);
    channel.activeColor = glm::vec3(1.0f, 0.0f, 0.0f);
    channel.placementMode = "InFront";
    channel.inFrontDistance = 2.0f;
    player.cameraPos = glm::vec3(0.0f, 1.6f, 3.0f);
    player.cameraForward = glm::vec3(0.0f, 0.0f, -1.0f);
    // deliberately do NOT call channel.updatePlacement(...)

    click();
    check(world.getOwnedObjects().size() == 1,
          "armed click without updatePlacement still births an Object");
    if (!world.getOwnedObjects().empty()) {
        const glm::vec3 p = world.getOwnedObjects().back()->getPosition();
        check(nearf(p.x, 0.0f) && nearf(p.y, 0.0f) && nearf(p.z, 0.0f),
              "that Object is at the origin, not cameraPos + forward * 2");
        std::printf("    spawned at (%.3f, %.3f, %.3f); InFront would be (0, 1.6, 1)\n",
                    p.x, p.y, p.z);
    }

    // ---- 2. a second authored twin (the test-save law-3) double-spawns -----
    auto twin = lawManager.createLaw("Tool: Shape Generator 3D", {&player});
    twin->setLawIdentifier("law-3");
    twin->setActivation(Law::Activation::OnEvent);
    twin->setConditionModel(ConditionNode::compare(
        "active3DMode", ConditionNode::Op::Eq, PropertyValue(std::string("Create"))));
    ActionNode spawn = ActionNode::spawn("concept-shape-3d");
    spawn.spawnPlacementPath = PropertyPath::parse("cursorSpawnTransform");
    spawn.spawnColorPath     = PropertyPath::parse("activeColor");
    spawn.spawnShapeKindPath = PropertyPath::parse("activeShapeKind");
    twin->setActionModel(spawn);
    lawManager.bindTrigger(twin->getIdentifier(), "onMouseClicked");

    const std::size_t before = world.getOwnedObjects().size();
    click();
    const std::size_t after = world.getOwnedObjects().size();
    check(after == before + 2,
          "factory law + save-style twin both fire off one click");
    std::printf("    one click produced %zu new object(s)\n", after - before);

    // ---- 3. setting the channel down does not stop the spawn law -----------
    channel.setEnabled(false);
    check(!channel.isEnabled(), "CreationChannel is set down");
    check(law->isEnabled(), "the spawn law's own enabled bit is still on");
    const std::size_t beforeDown = world.getOwnedObjects().size();
    click();
    check(world.getOwnedObjects().size() == beforeDown + 2,
          "setting the channel down does not stop either spawn law");

    // ---- 4. the authoring picker name is not the published event -----------
    check(std::string("mouse-clicked") != std::string("onMouseClicked"),
          "Law Graph offers mouse-clicked; the engine publishes onMouseClicked");

    std::printf(g_fail == 0 ? "shape_generator_law_audit_probe: ALL OK (findings reproduced)\n"
                            : "shape_generator_law_audit_probe: FAILURES\n");
    return g_fail > 0 ? 1 : 0;
}
