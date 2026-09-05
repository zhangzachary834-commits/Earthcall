#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/Physics/DefaultPhysicsLaws.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"

#include <cstdio>
#include <memory>
#include <vector>

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

} // namespace

int main() {
    std::printf("Running physics toggle test...\n");

    LawManager lm;
    Physics::setLawManager(&lm);
    for (const auto& law : Physics::createDefaultPhysicsLaws()) {
        lm.add(law);
    }

    Law* gravityLaw = lm.find("physics-gravity");
    Law* collisionLaw = lm.find("physics-collision");

    check(gravityLaw != nullptr, "physics-gravity law found in LawManager");
    check(collisionLaw != nullptr, "physics-collision law found in LawManager");

    check(Physics::isGravityEnabled(&lm), "gravity is enabled by default");
    check(Physics::isCollisionEnabled(&lm), "collision is enabled by default");

    // Test disabling gravity
    gravityLaw->setEnabled(false);
    check(!Physics::isGravityEnabled(&lm), "gravity helper returns false after disabling physics-gravity");

    // Test re-enabling gravity
    gravityLaw->setEnabled(true);
    check(Physics::isGravityEnabled(&lm), "gravity helper returns true after re-enabling physics-gravity");

    // Test disabling collision
    collisionLaw->setEnabled(false);
    check(!Physics::isCollisionEnabled(&lm), "collision helper returns false after disabling physics-collision");

    // Test physics simulation behavior with disabled gravity
    Physics::resetRigidBodies();
    auto cube = std::make_shared<Object>("test-cube");
    cube->setTransform(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 10.0f, 0.0f)));
    std::vector<std::shared_ptr<Object>> objects = { cube };

    gravityLaw->setEnabled(false);
    Physics::updateBodies(objects, 0.1f, 9.81f, 0.1f, 0.0f);
    glm::vec3 posDisabled = glm::vec3(cube->getTransform()[3]);
    check(std::abs(posDisabled.y - 10.0f) < 1e-4f, "object position Y unchanged when physics-gravity disabled");

    gravityLaw->setEnabled(true);
    Physics::updateBodies(objects, 0.1f, 9.81f, 0.1f, 0.0f);
    glm::vec3 posEnabled = glm::vec3(cube->getTransform()[3]);
    check(posEnabled.y < 10.0f, "object position Y fell when physics-gravity enabled");

    // Test collision enforcement helper with collision disabled vs enabled
    auto obstacle = std::make_shared<Object>("obstacle");
    obstacle->setTransform(glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)), glm::vec3(2.0f)));
    std::vector<std::shared_ptr<Object>> obstacleList = { obstacle };

    collisionLaw->setEnabled(false);
    glm::vec3 posPenetrating(0.0f, 0.0f, 0.0f);
    Physics::enforceCollisions(posPenetrating, obstacleList);
    check(glm::length(posPenetrating) < 1e-5f, "enforceCollisions does no correction when physics-collision disabled");

    std::printf(g_failures == 0 ? "physics_toggle_test: ALL OK\n"
                                : "physics_toggle_test: FAILURES\n");
    return g_failures > 0 ? 1 : 0;
}
