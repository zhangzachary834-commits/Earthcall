// Test: retired legacy physics toggle fossil (g_legacyEngineEnabled)
// Verifies that physics execution is governed by First-Mover law state after
// the legacy global toggle is removed.

#include "ZonesOfEarth/Physics/Physics.hpp"
#include "ZonesOfEarth/Physics/DefaultPhysicsLaws.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>

namespace {
int g_checks = 0;
int g_failures = 0;

void check(bool ok, const std::string& description) {
    ++g_checks;
    if (!ok) {
        ++g_failures;
        std::cout << "  FAILED: " << description << std::endl;
        return;
    }
    std::cout << "  ok: " << description << std::endl;
}

std::shared_ptr<Object> makeCube(float y) {
    auto cube = std::make_shared<Object>("test-cube");
    cube->setShape(Object::ShapeKind::Cube);
    cube->setTransform(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, y, 0.0f)));
    cube->updateCollisionZone(cube->getTransform());
    return cube;
}
} // namespace

int main() {
    std::cout << "============================================================" << std::endl;
    std::cout << "Legacy physics engine toggle fossil retired test" << std::endl;
    std::cout << "============================================================" << std::endl;

    LawManager lm;
    Physics::setLawManager(&lm);
    Physics::resetRigidBodies();
    Physics::clearBonds();

    for (const auto& law : Physics::createDefaultPhysicsLaws()) {
        law->setEnabled(false);
        lm.add(law);
    }

    check(!Physics::hasAnyActivePhysics(&lm),
          "First-Mover physics reports inactive when its laws are disabled");

    Zone world("legacy-toggle-retired-test-zone", "default");
    auto cube = makeCube(10.0f);
    world.addObject(cube);

    const float beforeDisabled = cube->getPosition().y;
    for (int i = 0; i < 30; ++i) world.update(1.0f / 60.0f);
    const float afterDisabled = cube->getPosition().y;
    check(std::fabs(afterDisabled - beforeDisabled) < 1e-4f,
          "Zone physics stays still while First-Mover physics laws are disabled");

    Law* gravityLaw = lm.find("physics-gravity");
    check(gravityLaw != nullptr, "physics-gravity First-Mover law is registered");
    if (gravityLaw) {
        gravityLaw->setEnabled(true);
        check(Physics::isGravityEnabled(&lm),
              "gravity governance becomes active through the First-Mover law");
        check(Physics::hasAnyActivePhysics(&lm),
              "physics reports active after enabling the First-Mover gravity law");
        for (int i = 0; i < 30; ++i) world.update(1.0f / 60.0f);
        check(cube->getPosition().y < afterDisabled - 0.1f,
              "Zone physics advances after the First-Mover gravity law is enabled");
    }

    Physics::setLawManager(nullptr);
    Physics::resetRigidBodies();
    Physics::clearBonds();

    std::cout << "\n------------------------------------------------------------" << std::endl;
    std::cout << (g_failures == 0 ? "PASSED" : "FAILED") << ": "
              << (g_checks - g_failures) << "/" << g_checks << " checks" << std::endl;
    return g_failures == 0 ? 0 : 1;
}
