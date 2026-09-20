// Real runtime path witness test for EarthcallAPI.
//
// Unlike earthcall_api_test (which constructs a standalone EarthcallAPI and
// manually attaches a local ZoneManager), this test boots the engine through
// BootedEngineHarness (mirroring Engine::initLogic) and exercises the live
// global singleton Integration::getEarthcallAPI().

#include "support/test_harness.hpp"
#include "Singularity/Foreign/API/EarthcallAPI.hpp"
#include "Singularity/Foreign/API/SecurityManager.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"

#include <cassert>
#include <iostream>
#include <string>

int main() {
    std::cout << "=== Running EarthcallAPI Real Path Witness Test ===" << std::endl;

    TestSupport::RealSaveTreeGuard guard(TestSupport::GuardCurrentRoot);
    TestSupport::BootedEngineHarness harness;

    // Retrieve global EarthcallAPI singleton used by Web/WASM/Python bridges
    auto& api = Integration::getEarthcallAPI();

    // Grant WORLD_ACCESS permission to foreign caller identity
    Integration::SecurityManager::instance().grantPermission(
        Integration::PermissionType::WORLD_ACCESS, "earthcall_api"
    );
    assert(api.hasPermission("world_access"));

    // Verify initial active zone state
    auto& activeZone = harness.zones.active();
    const std::size_t initialObjCount = activeZone.getOwnedObjects().size();

    // Exercise createObject via live getEarthcallAPI() singleton
    bool created = api.createObject("real_path_rock", glm::vec3(5.0f, 10.0f, 15.0f));
    assert(created && "createObject on live getEarthcallAPI() must succeed");

    assert(activeZone.getOwnedObjects().size() == initialObjCount + 1 &&
           "Live active zone must now contain the created object");

    Object* createdObj = activeZone.getOwnedObjects().back().get();
    assert(createdObj != nullptr);
    assert(createdObj->getObjectType() == "real_path_rock");
    assert(createdObj->getPosition().x == 5.0f);
    assert(createdObj->getPosition().y == 10.0f);
    assert(createdObj->getPosition().z == 15.0f);

    const std::string createdId = createdObj->getIdentifier();

    // Exercise modifyObject via live getEarthcallAPI() singleton
    bool modified = api.modifyObject(createdId, glm::vec3(20.0f, 25.0f, 30.0f), glm::vec3(2.0f, 2.0f, 2.0f));
    assert(modified && "modifyObject on live getEarthcallAPI() must succeed");
    assert(createdObj->getPosition().x == 20.0f);

    // Exercise deleteObject via live getEarthcallAPI() singleton
    bool deleted = api.deleteObject(createdId);
    assert(deleted && "deleteObject on live getEarthcallAPI() must succeed");
    assert(activeZone.getOwnedObjects().size() == initialObjCount &&
           "Live active zone object count must return to initial state after delete");

    std::cout << "earthcall_api_real_path_test: ALL OK" << std::endl;
    return 0;
}
