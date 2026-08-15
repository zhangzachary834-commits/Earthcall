// Scratch probe: reproduce the "cubes soar into the sky" glitch reported for the
// shape generator / 3D create tool.
//
// Faithful to the runtime path: EngineUpdate calls mgr.active().world().update(dt),
// and a freshly loaded zone world (saves/tests/shape_generator_3d_law.json ships
// "objects": []) contains NO baseline-ground object. World::update's ground search
// therefore falls back to `groundIdx = 1` -- literally the SECOND object in the
// list, which for an empty world is the second cube the Person spawns.
//
// groundY is then that cube's TOP (centre.y + halfHeight), while Physics::integrate
// clamps every object's CENTRE to groundY -- including the designated ground cube's
// own centre. So the "floor" lifts itself by half its height every physics substep
// and drags the whole world up with it.

#include "ConstructedBeing/Object/Object.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include "ZonesOfEarth/World/World.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <cstdio>
#include <memory>
#include <vector>

namespace {

std::shared_ptr<Object> makeCube(const glm::vec3& p) {
    auto obj = std::make_shared<Object>();
    obj->setShape(Object::ShapeKind::Cube);
    const glm::mat4 t = glm::translate(glm::mat4(1.0f), p);
    obj->setTransform(t);
    obj->updateCollisionZone(t);
    return obj;
}

void step(World& w, int frames) {
    for (int i = 0; i < frames; ++i) w.update(1.0f / 60.0f);
}

void report(const World& w, const char* label) {
    std::printf("  %-28s ", label);
    for (std::size_t i = 0; i < w.objects().size(); ++i)
        std::printf("[%zu]y=%8.2f ", i, w.objects()[i]->getPosition().y);
    std::printf("\n");
}

// The InFront placement the tool actually uses: camera at (0,1.6,3) looking -Z,
// inFrontDistance 2 -> every click without moving spawns at exactly (0,1.6,1).
const glm::vec3 kSpawn(0.0f, 1.6f, 1.0f);

void clickSequence(int clicks, int framesBetween) {
    Physics::resetRigidBodies();
    Physics::clearBonds();
    Physics::setLegacyEngineEnabled(true);

    World world; // a fresh zone world: empty, no baseline ground
    for (int c = 0; c < clicks; ++c) {
        world.addObject(makeCube(kSpawn));
        step(world, framesBetween);
        char label[64];
        std::snprintf(label, sizeof(label), "after click %d (+%df)", c + 1, framesBetween);
        report(world, label);
    }
}

} // namespace

int main() {
    std::printf("== one click at a time, 30 frames (0.5s) of physics between ==\n");
    clickSequence(4, 30);

    std::printf("\n== the same, but spawning at spread-out positions ==\n");
    {
        Physics::resetRigidBodies();
        Physics::clearBonds();
        Physics::setLegacyEngineEnabled(true);
        World world;
        for (int c = 0; c < 4; ++c) {
            world.addObject(makeCube(glm::vec3(2.0f * c, 1.6f, 1.0f)));
            step(world, 30);
            char label[64];
            std::snprintf(label, sizeof(label), "after click %d", c + 1);
            report(world, label);
        }
    }

    std::printf("\n== control: same world, but object[1] tagged baseline=ground ==\n");
    {
        Physics::resetRigidBodies();
        Physics::clearBonds();
        Physics::setLegacyEngineEnabled(true);
        World world;
        world.addObject(makeCube(glm::vec3(0.0f, 5.0f, 0.0f)));
        auto ground = makeCube(glm::vec3(0.0f, 0.0f, 0.0f));
        ground->setAttribute("baseline", "ground");
        world.addObject(ground);
        for (int c = 0; c < 3; ++c) {
            world.addObject(makeCube(kSpawn));
            step(world, 30);
            char label[64];
            std::snprintf(label, sizeof(label), "after click %d", c + 1);
            report(world, label);
        }
        std::printf("  (before the fix the tagged ground lifted itself just the same,\n"
                    "   because the clamp was on the centre; it is now kinematic)\n");
    }

    std::printf("\n== how fast does it climb? 4 cubes, sampled every 30 frames ==\n");
    {
        Physics::resetRigidBodies();
        Physics::clearBonds();
        Physics::setLegacyEngineEnabled(true);
        World world;
        for (int c = 0; c < 4; ++c) world.addObject(makeCube(kSpawn));
        for (int s = 0; s < 6; ++s) {
            step(world, 30);
            char label[64];
            std::snprintf(label, sizeof(label), "t=%.1fs", (s + 1) * 0.5);
            report(world, label);
        }
    }
    return 0;
}
