// Test: the floor is a floor, and nothing else is conscripted into being one.
//
// Guards a shipped bug ("the self-lifting floor"): spawning a second object into
// a Zone's world sent the entire world climbing at 30 m/s, and every object
// spawned afterwards was teleported up to the rising floor the instant it was
// born. Two faults compounded, and this test holds both closed, plus the third
// fault the first one was hiding.
//
//  1. World::update fell back to `groundIdx = 1` when no object carried
//     `baseline=ground`. A Zone's world is seeded with NOTHING (EngineInit's
//     baseline cube+ground go to the Ourverse's list), so index 1 was simply the
//     second being a Person spawned -- made the floor without mark or consent.
//
//  2. Physics::integrate clamps the point it is handed, and updateBodies handed
//     it the object's transform ORIGIN while groundY names the floor's TOP
//     surface. Every object therefore buried its centre in the floor -- and the
//     being that DEFINED groundY sat, necessarily, half its own height below its
//     own top, so the clamp lifted it by half a height every substep, which
//     raised groundY, which lifted it again. Unbounded: +0.5 per substep.
//
//  3. The collision loop skipped `i == 1` as "the ground" on the same index
//     assumption, so in a Zone's world the second being a Person spawned passed
//     through everything. Fault 1 hid this one.

#include "ConstructedBeing/Object/Object.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include "ZonesOfEarth/World/World.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>

namespace {

int g_checks = 0;
int g_failures = 0;

// Reports through the exit code rather than assert(): a Release build defines
// NDEBUG and would compile assert() out entirely.
void check(bool condition, const std::string& description) {
    ++g_checks;
    if (!condition) {
        ++g_failures;
        std::cout << "  FAILED: " << description << std::endl;
        return;
    }
    std::cout << "  ok: " << description << std::endl;
}

std::shared_ptr<Object> makeCube(const glm::vec3& p) {
    auto obj = std::make_shared<Object>();
    obj->setShape(Object::ShapeKind::Cube);
    const glm::mat4 t = glm::translate(glm::mat4(1.0f), p);
    obj->setTransform(t);
    obj->updateCollisionZone(t);
    return obj;
}

void freshPhysics() {
    Physics::resetRigidBodies();
    Physics::clearBonds();
    Physics::setLegacyEngineEnabled(true);
}

void step(World& w, int frames) {
    for (int i = 0; i < frames; ++i) w.update(1.0f / 60.0f);
}

float highestY(const World& w) {
    float highest = -1e9f;
    for (const auto& obj : w.objects())
        if (obj) highest = std::max(highest, obj->getPosition().y);
    return highest;
}

// A unit cube spawned repeatedly at the same spot, exactly as clicking the 3D
// create tool without moving the camera does (InFront placement is a pure
// function of the camera, so every click lands on the identical transform).
const glm::vec3 kSpawn(0.0f, 1.6f, 1.0f);

// ---------------------------------------------------------------------
void testNoBeingIsConscriptedAsTheFloor() {
    std::cout << "\n[1] An empty world's second being is not the ground" << std::endl;
    freshPhysics();
    World world;

    // Four clicks, a beat of physics between each -- the reported reproduction.
    for (int click = 0; click < 4; ++click) {
        world.addObject(makeCube(kSpawn));
        step(world, 30);
    }
    const float afterClicks = highestY(world);
    check(afterClicks < 5.0f,
          "four coincident spawns stay near the floor (highest y = " +
              std::to_string(afterClicks) + ", was 46.60 before the fix)");

    // The runaway was unbounded in TIME, not just in count: it climbed for as
    // long as the world ran. Three more seconds must change nothing.
    step(world, 180);
    const float afterTime = highestY(world);
    check(std::fabs(afterTime - afterClicks) < 0.5f,
          "and do not climb over the next 3 seconds (y = " + std::to_string(afterTime) +
              ", was +30 m/s before the fix)");
}

// ---------------------------------------------------------------------
void testObjectsRestOnTheFloorNotInIt() {
    std::cout << "\n[2] An object rests its BOTTOM on groundY, not its centre" << std::endl;
    freshPhysics();
    World world;
    world.addObject(makeCube(glm::vec3(0.0f, 6.0f, 0.0f)));
    step(world, 120);

    // No baseline ground -> the floor is the y=0 plane. A unit cube resting on
    // it has its centre at +0.5. Clamping the centre would have parked it at 0.0
    // with half the cube underground.
    const float y = world.objects()[0]->getPosition().y;
    check(std::fabs(y - 0.5f) < 0.05f,
          "unit cube settles with centre at 0.5 above the y=0 plane (got " +
              std::to_string(y) + ")");
}

// ---------------------------------------------------------------------
void testTaggedGroundHoldsStill() {
    std::cout << "\n[3] A being tagged baseline=ground neither rises nor falls" << std::endl;
    freshPhysics();
    World world;
    auto ground = makeCube(glm::vec3(0.0f, 0.0f, 0.0f));
    ground->setAttribute("baseline", "ground");
    world.addObject(ground);
    world.addObject(makeCube(glm::vec3(0.0f, 6.0f, 0.0f)));
    step(world, 180);

    check(std::fabs(ground->getPosition().y) < 1e-3f,
          "the ground stays at y=0 (got " + std::to_string(ground->getPosition().y) +
              "); it lifted itself before the fix, and fell forever if merely "
              "exempted from its own clamp");

    // Ground centre 0, unit height -> its top surface, and so groundY, is 0.5.
    // A unit cube resting on THAT has its centre at 1.0.
    const float y = world.objects()[1]->getPosition().y;
    check(std::fabs(y - 1.0f) < 0.05f,
          "a cube rests on the ground's top surface, centre at 1.0 (got " +
              std::to_string(y) + ")");
}

// ---------------------------------------------------------------------
void testSecondBeingIsNotAGhost() {
    std::cout << "\n[4] The second being spawned still collides" << std::endl;
    freshPhysics();
    World world;
    world.addObject(makeCube(glm::vec3(0.0f, 0.5f, 0.0f)));
    world.addObject(makeCube(glm::vec3(0.3f, 0.5f, 0.0f))); // index 1: overlaps the first
    const float before =
        std::fabs(world.objects()[0]->getPosition().x - world.objects()[1]->getPosition().x);
    step(world, 60);
    const float after =
        std::fabs(world.objects()[0]->getPosition().x - world.objects()[1]->getPosition().x);

    check(after > before + 0.3f,
          "two overlapping unit cubes push apart (|dx| " + std::to_string(before) + " -> " +
              std::to_string(after) + "); index 1 was skipped as \"the ground\" before the fix");
}

} // namespace

int main() {
    std::cout << "============================================================" << std::endl;
    std::cout << "Ground plane test: the floor is a floor" << std::endl;
    std::cout << "============================================================" << std::endl;

    testNoBeingIsConscriptedAsTheFloor();
    testObjectsRestOnTheFloorNotInIt();
    testTaggedGroundHoldsStill();
    testSecondBeingIsNotAGhost();

    std::cout << "\n------------------------------------------------------------" << std::endl;
    std::cout << (g_failures == 0 ? "PASSED" : "FAILED") << ": " << (g_checks - g_failures)
              << "/" << g_checks << " checks" << std::endl;
    return g_failures == 0 ? 0 : 1;
}
