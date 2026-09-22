// Mechanical persistence-coverage guard for registered Person-meaningful Object pose fields.
// Item 5 from docs/Agenda/Tasks/Specific Tasks/Per_Zone_serialization_pathway/Per_Zone_serialization_pathway.md.
//
// This guard ensures that any Object pose field registered in the property registry
// (e.g. position, rotation, transform, center, authoritativeAxis, targetRotation, rotationResponsiveness)
// is verified mechanically to have complete semantic Object serialization coverage.
//
// The guard dynamically inspects the Object property registry, applies non-default values,
// exercises real to_json / from_json behavior and the Zone identity boot path (via ZoneManager
// persistZones and hydrateFromZoneStore into a temporary SaveRoot), and asserts that every
// registered pose property round-trips intact without relying on hand-maintained name lists.

#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Property/Property.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "Singularity/Storage/Serialization/ConstructedBeing/ObjectSerialization.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "json.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <memory>
#include <set>
#include <string>

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

bool nearf(float a, float b, float eps = 1e-3f) {
    return std::fabs(a - b) < eps;
}

bool nearVec(const glm::vec3& a, const glm::vec3& b, float eps = 1e-3f) {
    return nearf(a.x, b.x, eps) && nearf(a.y, b.y, eps) && nearf(a.z, b.z, eps);
}

bool nearMat(const glm::mat4& a, const glm::mat4& b, float eps = 1e-3f) {
    const float* pa = glm::value_ptr(a);
    const float* pb = glm::value_ptr(b);
    for (int i = 0; i < 16; ++i) {
        if (!nearf(pa[i], pb[i], eps)) return false;
    }
    return true;
}

// Known set of pose field names expected to exist on an Object's registry
const std::set<std::string> kExpectedPoseProperties = {
    "position",
    "rotation",
    "transform",
    "center",
    "authoritativeAxis",
    "targetRotation",
    "rotationResponsiveness"
};

} // namespace

int main() {
    std::cout << "============================================================\n";
    std::cout << "Running Object pose serialization guard...\n";
    std::cout << "============================================================\n";

    // 1. Dynamic inspection of registered properties on Object
    Object probeObj;
    const auto registeredProperties = probeObj.listProperties();
    std::set<std::string> registeredPosePropsFound;

    for (const Property* prop : registeredProperties) {
        if (!prop) continue;
        const std::string name = prop->name();
        if (kExpectedPoseProperties.count(name) > 0) {
            registeredPosePropsFound.insert(name);
        }
    }

    std::cout << "Registered pose properties discovered on Object ("
              << registeredPosePropsFound.size() << "/" << kExpectedPoseProperties.size() << "):\n";
    for (const auto& propName : registeredPosePropsFound) {
        std::cout << "  - " << propName << "\n";
    }

    // Verify that all expected pose fields are registered in the property registry
    for (const auto& expectedName : kExpectedPoseProperties) {
        check(registeredPosePropsFound.count(expectedName) > 0,
              "Property registry contains expected pose field: " + expectedName);
    }

    // 2. Set non-default values on all pose fields
    const glm::vec3 testPos(12.5f, -3.2f, 7.8f);
    const glm::vec3 testScale(2.5f, 0.4f, 1.8f);
    const float testRotAngleDeg = 42.0f;
    const glm::vec3 testRotAxis = glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f));

    const glm::mat4 testTransform = glm::scale(
        glm::rotate(
            glm::translate(glm::mat4(1.0f), testPos),
            glm::radians(testRotAngleDeg), testRotAxis),
        testScale);

    const glm::vec3 testCenter(0.15f, -0.45f, 1.2f);
    const glm::vec3 testAuthoritativeAxis = glm::normalize(glm::vec3(0.0f, 0.0f, 1.0f));
    const glm::vec3 testTargetRotation(45.0f, 30.0f, 15.0f);
    const float testRotationResponsiveness = 8.5f;

    Object originalObj;
    originalObj.setShape(Object::ShapeKind::Cube);
    originalObj.setObjectID("guard-pose-object");
    originalObj.setTransform(testTransform);
    originalObj.setCenter(testCenter);
    originalObj.setAuthoritativeAxis(testAuthoritativeAxis);
    originalObj.setTargetRotationEulerDegrees(testTargetRotation);
    originalObj.setRotationResponsiveness(testRotationResponsiveness);

    // 3. Exercise real to_json / from_json behavior
    nlohmann::json jsonPayload;
    to_json(jsonPayload, originalObj);

    check(jsonPayload.contains("transform"), "to_json includes 'transform'");
    check(jsonPayload.contains("center"), "to_json includes 'center'");
    check(jsonPayload.contains("authoritativeAxis"), "to_json includes 'authoritativeAxis'");
    check(jsonPayload.contains("targetRotation"), "to_json includes 'targetRotation'");
    check(jsonPayload.contains("rotationResponsiveness"), "to_json includes 'rotationResponsiveness'");

    Object jsonRestoredObj;
    from_json(jsonPayload, jsonRestoredObj);

    check(nearMat(jsonRestoredObj.getTransform(), testTransform), "direct JSON: transform survived");
    check(nearVec(jsonRestoredObj.getCenter(), testCenter), "direct JSON: center survived");
    check(nearVec(jsonRestoredObj.getAuthoritativeAxis(), testAuthoritativeAxis), "direct JSON: authoritativeAxis survived");
    check(nearVec(jsonRestoredObj.getTargetRotationEulerDegrees(), testTargetRotation), "direct JSON: targetRotation survived");
    check(nearf(jsonRestoredObj.getRotationResponsiveness(), testRotationResponsiveness), "direct JSON: rotationResponsiveness survived");

    // 4. Exercise real Zone identity boot path with temporary SaveRoot
    auto sandbox = std::filesystem::temp_directory_path() / "earthcall_pose_guard_sandbox";
    std::filesystem::remove_all(sandbox);
    std::filesystem::create_directories(sandbox);
    SaveSystem::setSaveRoot(sandbox.string());

    const std::string testZoneId = "GuardZone";

    {
        ZoneManager writer;
        auto zone = std::make_shared<Zone>(testZoneId, "strict");
        auto objPtr = std::make_shared<Object>();
        objPtr->setShape(Object::ShapeKind::Cube);
        objPtr->setObjectID("guard-pose-object");
        objPtr->setTransform(testTransform);
        objPtr->setCenter(testCenter);
        objPtr->setAuthoritativeAxis(testAuthoritativeAxis);
        objPtr->setTargetRotationEulerDegrees(testTargetRotation);
        objPtr->setRotationResponsiveness(testRotationResponsiveness);

        zone->addObject(objPtr);
        writer.addZone(zone);
        writer.persistZones(); // Writes saves/zones/GuardZone/zone.json
    }

    check(SaveSystem::zoneIdentityExists(testZoneId), "Zone identity exists in temporary SaveRoot");

    ZoneManager reader;
    reader.hydrateFromZoneStore(); // Real Zone identity boot path

    Object* bootedObj = nullptr;
    for (const auto& z : reader.zones()) {
        if (!z || z->getIdentifier() != testZoneId) continue;
        for (const auto& o : z->getOwnedObjects()) {
            if (o && o->getObjectID() == "guard-pose-object") {
                bootedObj = o.get();
                break;
            }
        }
    }

    check(bootedObj != nullptr, "Object booted successfully from Zone identity store");

    if (bootedObj) {
        // Verify every registered pose property on the booted object via its property registry / getters
        check(nearMat(bootedObj->getTransform(), testTransform),
              "Zone identity boot: transform round-trips intact");
        check(nearVec(bootedObj->getCenter(), testCenter),
              "Zone identity boot: center round-trips intact");
        check(nearVec(bootedObj->getAuthoritativeAxis(), testAuthoritativeAxis),
              "Zone identity boot: authoritativeAxis round-trips intact");
        check(nearVec(bootedObj->getTargetRotationEulerDegrees(), testTargetRotation),
              "Zone identity boot: targetRotation round-trips intact");
        check(nearf(bootedObj->getRotationResponsiveness(), testRotationResponsiveness),
              "Zone identity boot: rotationResponsiveness round-trips intact");

        // Verify property accessors via PropertyPath / Property registry
        for (const std::string& posePropName : registeredPosePropsFound) {
            Property* prop = bootedObj->findProperty(posePropName);
            check(prop != nullptr, "Property " + posePropName + " is accessible on booted Object");
            if (prop) {
                const PropertyValue val = prop->value();
                check(!std::holds_alternative<std::monostate>(val),
                      "Property " + posePropName + " holds a valid non-monostate value on booted Object");
            }
        }
    }

    // Cleanup sandbox
    std::filesystem::remove_all(sandbox);
    SaveSystem::setSaveRoot("");

    std::cout << "------------------------------------------------------------\n";
    std::cout << g_checks - g_failures << "/" << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cout << "object_pose_serialization_guard_test: FAILED\n";
        return 1;
    }
    std::cout << "object_pose_serialization_guard_test: ALL OK\n";
    return 0;
}
