// Guards the live path for "load test saves to be visible and experienced
// in-world by a real Person at runtime."
//
// The Developer: Test World Saves window used to call loadState after a
// dummy "Test Zone" switch. loadState clears every Zone (Home included),
// and LocomotionChannel then snaps the camera back onto person.position,
// so the Person never sees the dump. This test calls the same office the
// window now calls — ZoneManager::loadTestObservation — and asserts the
// live path: Home survives, the dump's objects are in the active Zone,
// and the Person is settled looking at them.

#include "ConstructedBeing/Object/Object.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Singularity/Input/Mouse/MouseHandler.hpp"
#include "Singularity/Screen/Camera.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/SaveContext.hpp"
#include "ZonesOfEarth/World/World.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "test_save_helper.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <memory>
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

std::shared_ptr<Object> makeCube(const std::string& id, const glm::vec3& p) {
    auto obj = std::make_shared<Object>();
    obj->setShape(Object::ShapeKind::Cube);
    obj->setObjectID(id);
    obj->setTransform(glm::translate(glm::mat4(1.0f), p));
    return obj;
}

Person makePlayer() {
    Soul soul("Player");
    Body body("humanoid", "default");
    Person player(std::move(soul), std::move(body), "default");
    player.position = glm::vec3(10.0f, 0.0f, 10.0f);
    player.cameraPos = glm::vec3(10.0f, player.getBody().getEyeHeight(), 10.0f);
    player.cameraForward = glm::vec3(0.0f, 0.0f, -1.0f);
    return player;
}

} // namespace

int main() {
    std::cout << "============================================================\n";
    std::cout << "Running test observation load (in-world witness path)...\n";
    std::cout << "============================================================\n";

    auto sandbox = std::filesystem::temp_directory_path() / "earthcall_test_observation";
    std::filesystem::remove_all(sandbox);
    std::filesystem::create_directories(sandbox);
    const std::string dumpPath = (sandbox / "visible_cube.json").string();

    const glm::vec3 cubePos(0.0f, 2.0f, -2.0f);
    {
        World dumpWorld;
        dumpWorld.addObject(makeCube("witness-cube", cubePos));
        Person dumpPlayer = makePlayer();
        LawManager dumpLaws;
        dump_test_save("visible_cube", dumpWorld, dumpLaws, dumpPlayer, dumpPath);
        check(std::filesystem::exists(dumpPath), "dump_test_save wrote the observation fixture");
    }

    ZoneManager live;
    auto home = std::make_shared<Zone>("Home", "default");
    home->setOwner("Player");
    auto precious = makeCube("home-precious", glm::vec3(4.0f, 0.5f, 4.0f));
    home->world().addObject(precious);
    precious->addZoneDesignation(home->name());
    live.addZone(home);

    Person player = makePlayer();
    Core::Camera camera;
    camera.pos = player.cameraPos;
    camera.front = player.cameraForward;
    MouseHandler mouse;
    LawManager laws;
    float color[3] = {1.0f, 1.0f, 1.0f};
    double worldTime = 0.0;
    SaveContext ctx;
    ctx.camera = &camera;
    ctx.mouseHandler = &mouse;
    ctx.currentColor = color;
    ctx.player = &player;
    ctx.lawManager = &laws;
    ctx.worldTime = &worldTime;

    live.loadTestObservation(dumpPath, ctx);

    bool homeStillHere = false;
    std::shared_ptr<Object> preciousStill;
    std::shared_ptr<Zone> observation;
    for (const auto& z : live.zones()) {
        if (!z) continue;
        if (z->name() == "Home") {
            homeStillHere = true;
            for (const auto& obj : z->world().getOwnedObjects()) {
                if (obj && obj->getIdentifier() == "home-precious") preciousStill = obj;
            }
        }
        if (z->name() == "test.visible_cube") observation = z;
    }
    check(homeStillHere, "Home is still a Zone after observation load");
    check(preciousStill != nullptr, "Home's precious object was not erased");
    check(observation != nullptr, "observation Zone test.visible_cube exists");

    bool cubeInObservation = false;
    if (observation) {
        for (const auto& obj : observation->world().getOwnedObjects()) {
            if (obj && obj->getIdentifier() == "witness-cube") cubeInObservation = true;
        }
    }
    check(cubeInObservation, "dumped cube is in the observation Zone");
    check(live.active().getIdentifier() == "test.visible_cube",
          "active Zone is the observation Zone (the render path draws this world)");

    bool cubeInActive = false;
    glm::vec3 loadedPos(0.0f);
    for (const auto& obj : live.active().world().getOwnedObjects()) {
        if (obj && obj->getIdentifier() == "witness-cube") {
            cubeInActive = true;
            loadedPos = obj->getPosition();
        }
    }
    check(cubeInActive, "dumped cube is in the active world the Person sees");
    check(nearf(loadedPos.x, cubePos.x) && nearf(loadedPos.y, cubePos.y) &&
              nearf(loadedPos.z, cubePos.z),
          "loaded cube kept its position");

    const float eyeH = player.getBody().getEyeHeight();
    const glm::vec3 expectedPerson = camera.pos - glm::vec3(0.0f, eyeH, 0.0f);
    check(glm::distance(player.position, expectedPerson) < 1e-3f,
          "Person.position matches camera so locomotion will not snap the view back");
    check(glm::distance(player.cameraPos, camera.pos) < 1e-3f,
          "Person.cameraPos matches the Camera the render path uses");

    const glm::vec3 toCube = glm::normalize(loadedPos - camera.pos);
    const float facing = glm::dot(glm::normalize(camera.front), toCube);
    check(facing > 0.5f, "camera faces the loaded cube (Person can see it)");
    check(glm::distance(camera.pos, loadedPos) > 1.0f,
          "camera stands back from the cube rather than sitting inside it");

    const std::string& report = live.getSaveLoadState().lastLoadReport;
    check(report.find("Home is still here") != std::string::npos,
          "load report says Home survived");
    check(report.find("object") != std::string::npos,
          "load report names the objects so failure is loud");

    const std::filesystem::path fixture("saves/tests/basic_cube_law_test_final.json");
    if (std::filesystem::exists(fixture)) {
        std::cout << "\n[fixture] also observing saves/tests/basic_cube_law_test_final.json\n";
        live.loadTestObservation(fixture.string(), ctx);
        check(live.active().getIdentifier() == "test.basic_cube_law_test_final",
              "fixture observation Zone is active");
        check(!live.active().world().getOwnedObjects().empty(),
              "fixture dumped objects into the world a Person would see");
        bool homeAfterFixture = false;
        for (const auto& z : live.zones()) {
            if (z && z->name() == "Home") homeAfterFixture = true;
        }
        check(homeAfterFixture, "Home still present after observing the real fixture");
        const glm::vec3 expectedAfter = camera.pos - glm::vec3(0.0f, eyeH, 0.0f);
        check(glm::distance(player.position, expectedAfter) < 1e-3f,
              "Person remains settled on the camera after the fixture load");
    } else {
        std::cout << "  skip: saves/tests/basic_cube_law_test_final.json is not on disk "
                     "(gitignored); synthetic dump covered the live path.\n";
    }

    std::filesystem::remove_all(sandbox);

    std::cout << "------------------------------------------------------------\n";
    std::cout << g_checks - g_failures << "/" << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cout << "test_observation_load_test: FAILED\n";
        return 1;
    }
    std::cout << "test_observation_load_test: ALL OK\n";
    return 0;
}
