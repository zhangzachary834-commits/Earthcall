// Authored Rotational Physics Laws Witness Test.
//
// WITNESS DOCUMENTATION:
// What the old test proved:
//   That Physics::createAuthoredRotationalLaws() factory function constructed 4 valid Law objects,
//   and that manually calling kinLaw->applyTo(*obj) integrated angularVelocity into rotation.
// What the old test COULD NOT prove:
//   That Physics::createAuthoredRotationalLaws() was registered in LawManager during production
//   engine initialization (Engine::initLogic()), or that an object carrying angularVelocity in a
//   booted engine would actually rotate during runtime LawManager::tick() execution.

#include "support/test_harness.hpp"
#include "ZonesOfEarth/Physics/AuthoredPhysicsLaws.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <set>
#include <string>
#include <cmath>

static void check(bool cond, const std::string& what) {
    if (!cond) {
        std::printf("  FAILED: %s\n", what.c_str());
        std::exit(1);
    }
    std::printf("  ok: %s\n", what.c_str());
}

int main() {
    std::printf("Running authored rotational physics laws test...\n");

    const char* expected[] = {
        "law-angular-kinematics",
        "law-angular-damping",
        "law-gravity-tilt",
        "law-rolling-coupling",
    };

    // --- Isolated Factory Unit Verification ---
    {
        std::printf("--- Isolated Factory Unit Verification ---\n");

        const auto laws = Physics::createAuthoredRotationalLaws();
        check(!laws.empty(), "the factory seeds at least one authored law");

        check(laws.size() == sizeof(expected) / sizeof(expected[0]),
              "exactly 4 authored rotational physics laws minted");

        std::set<std::string> ids;
        for (const auto& law : laws) {
            check(!law->isFirstMover(), law->getIdentifier() + " is an AUTHORED law (not FirstMover)");
            ids.insert(law->getIdentifier());
        }

        for (const char* id : expected) {
            check(ids.count(id) == 1, std::string(id) + " is present and unique");
        }

        // Verify property registration on Object
        auto obj = std::make_shared<Object>();
        check(obj->findProperty("angularVelocity") != nullptr, "Object registers 'angularVelocity' property");
        check(obj->findProperty("centerOfMass") != nullptr, "Object registers 'centerOfMass' property");
        check(obj->findProperty("momentOfInertia") != nullptr, "Object registers 'momentOfInertia' property");

        // Test setting and getting angularVelocity
        obj->findProperty("angularVelocity")->setValue(glm::vec3(10.0f, 20.0f, 30.0f));
        auto val = obj->findProperty("angularVelocity")->value();
        const auto* av = std::get_if<glm::vec3>(&val);
        check(av != nullptr && std::abs(av->x - 10.0f) < 1e-4f &&
              std::abs(av->y - 20.0f) < 1e-4f && std::abs(av->z - 30.0f) < 1e-4f,
              "angularVelocity property reads and writes correctly through RigidFormBridge");

        // Test setting and getting centerOfMass
        obj->findProperty("centerOfMass")->setValue(glm::vec3(0.1f, 0.5f, 0.0f));
        auto comVal = obj->findProperty("centerOfMass")->value();
        const auto* com = std::get_if<glm::vec3>(&comVal);
        check(com != nullptr && std::abs(com->x - 0.1f) < 1e-4f &&
              std::abs(com->y - 0.5f) < 1e-4f,
              "centerOfMass property reads and writes correctly through RigidFormBridge");

        // Test law-angular-kinematics application
        std::shared_ptr<Law> kinLaw = nullptr;
        for (const auto& l : laws) {
            if (l->getIdentifier() == "law-angular-kinematics") kinLaw = l;
        }
        check(kinLaw != nullptr, "found law-angular-kinematics");

        // Clear rotation and set angularVelocity = (0, 90, 0) deg/s
        obj->findProperty("rotation")->setValue(glm::vec3(0.0f));
        obj->findProperty("angularVelocity")->setValue(glm::vec3(0.0f, 90.0f, 0.0f));

        // Nothing enters the world without an author.
        auto authorObj = std::make_shared<Object>();
        kinLaw->addAuthor(*authorObj);

        // Apply law for dt = 0.5s: rotation.y should advance by ~45 deg
        Universe::instance().setClock(0.5, 0.5);
        auto res = kinLaw->applyTo(*obj);

        auto rotVal = obj->findProperty("rotation")->value();
        const auto* rot = std::get_if<glm::vec3>(&rotVal);
        std::printf("res = %s, rot = %p\n", Law::resultName(res), rot);
        if (rot) {
            std::printf("rot = (%.2f, %.2f, %.2f)\n", rot->x, rot->y, rot->z);
        }
        check(rot != nullptr && std::abs(rot->y - 45.0f) < 1e-2f,
              "law-angular-kinematics flows angularVelocity into rotation: 90 deg/s * 0.5s = 45 deg");
    }

    // --- Real Application Booted Path Verification ---
    {
        std::printf("--- Real Application Booted Path Verification ---\n");
        TestSupport::BootedEngineHarness harness("Player", "humanoid", false);

        // Verify all 4 rotational laws exist in LawManager on boot
        for (const char* id : expected) {
            Law* bootedLaw = harness.lawManager.find(id);
            check(bootedLaw != nullptr, std::string(id) + " is registered in LawManager on boot");
            if (bootedLaw) {
                check(bootedLaw->isAuthored(), std::string(id) + " is authored by player on boot");
            }
        }

        // Add a test object to active zone and verify rotation via LawManager::tick()
        auto testObj = std::make_shared<Object>();
        testObj->setObjectID("test-rotational-vessel");
        testObj->findProperty("rotation")->setValue(glm::vec3(0.0f));
        testObj->findProperty("angularVelocity")->setValue(glm::vec3(0.0f, 90.0f, 0.0f));

        harness.zones.active().addObject(testObj);

        // Advance clock by 0.5s and tick the booted LawManager
        Universe::instance().setClock(0.5, 0.5);
        harness.lawManager.tick();

        auto bootedRotVal = testObj->findProperty("rotation")->value();
        const auto* bootedRot = std::get_if<glm::vec3>(&bootedRotVal);
        check(bootedRot != nullptr && std::abs(bootedRot->y - 45.0f) < 1e-2f,
              "law-angular-kinematics updates object rotation on the real production LawManager::tick() path: 90 deg/s * 0.5s = 45 deg");
    }

    std::printf("Authored rotational physics laws test PASSED!\n");
    return 0;
}
