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
//
// dump_test_save (test_save_helper.hpp) calls ZoneManager::saveState,
// which calls persistZones() unconditionally — and this test never points
// SaveSystem at a sandbox, so that write landed in the REAL saves/zones/
// tree (saves/zones/visible_cube/zone.json picked up 34 lines of drift
// running the full suite once, 2026-09-08 — the same bug class as the 5
// chess tests, just not yet fixed). Sol (agent intercom, "Basic Pixel
// Changer Zone Identity Bug 9-7-26", 2026-09-09, Stage 0): "Put
// test_observation_load_test behind the same TestSupport::RealSaveTreeGuard
// used by the chess tests, and add a before/after tree hash or equivalent
// assertion so the test proves it restored the real identity tree even on
// early return/exception."

#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Singularity/Input/Mouse/MouseHandler.hpp"
#include "Singularity/Screen/Camera.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/SaveContext.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "test_save_helper.hpp"
#include "support/test_harness.hpp"

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
    player.position() = glm::vec3(10.0f, 0.0f, 10.0f);
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

    // Sol, Stage 0: prove the real tree comes back byte-identical even if
    // something in the guarded section throws. Snapshotting before
    // constructing the guard, and diffing after it is destroyed (whether
    // by falling off the end of the try block or by unwinding through the
    // catch below), is the actual proof — not just trusting the guard's
    // own bookkeeping.
    const std::string beforeTreeHash =
        TestSupport::hashDirectoryTree("saves/zones") + "|" +
        TestSupport::hashDirectoryTree("saves/homes");
    bool guardedSectionThrew = false;
    std::string guardedSectionException;

    try {
    TestSupport::RealSaveTreeGuard realTreeGuard(TestSupport::GuardCurrentRoot);

    const glm::vec3 cubePos(0.0f, 2.0f, -2.0f);
    {
        Zone dumpWorld("test-observation", "default");
        dumpWorld.addObject(makeCube("witness-cube", cubePos));
        Person dumpPlayer = makePlayer();
        LawManager dumpLaws;
        dump_test_save("visible_cube", dumpWorld, dumpLaws, dumpPlayer, dumpPath);
        check(std::filesystem::exists(dumpPath) || std::filesystem::exists(sandbox / "visible_cube.ecform"),
              "dump_test_save wrote the observation fixture");
    }

    ZoneManager live;
    auto home = std::make_shared<Zone>("Home", "default");
    home->setOwner("Player");
    auto precious = makeCube("home-precious", glm::vec3(4.0f, 0.5f, 4.0f));
    home->addObject(precious);
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
    ctx.person = &player;
    ctx.lawManager = &laws;
    ctx.worldTime = &worldTime;

    // 1. Error path: missing file
    live.loadTestObservation("non_existent_file.json", ctx);
    check(live.getSaveLoadState().lastLoadReport.find("COULD NOT OPEN OR READ") != std::string::npos,
          "loadTestObservation handles missing file properly");

    // 2. Happy path: valid dump
    live.loadTestObservation(dumpPath, ctx);

    bool homeStillHere = false;
    std::shared_ptr<Object> preciousStill;
    std::shared_ptr<Zone> observation;
    for (const auto& z : live.zones()) {
        if (!z) continue;
        if (z->name() == "Home") {
            homeStillHere = true;
            for (const auto& obj : z->getOwnedObjects()) {
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
        for (const auto& obj : observation->getOwnedObjects()) {
            if (obj && obj->getIdentifier() == "witness-cube") cubeInObservation = true;
        }
    }
    check(cubeInObservation, "dumped cube is in the observation Zone");
    check(live.active().getIdentifier() == "test.visible_cube",
          "active Zone is the observation Zone (the render path draws this world)");

    bool cubeInActive = false;
    glm::vec3 loadedPos(0.0f);
    for (const auto& obj : live.active().getOwnedObjects()) {
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
    check(glm::distance(player.position(), expectedPerson) < 1e-3f,
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
        check(!live.active().getOwnedObjects().empty(),
              "fixture dumped objects into the world a Person would see");
        bool homeAfterFixture = false;
        for (const auto& z : live.zones()) {
            if (z && z->name() == "Home") homeAfterFixture = true;
        }
        check(homeAfterFixture, "Home still present after observing the real fixture");
        const glm::vec3 expectedAfter = camera.pos - glm::vec3(0.0f, eyeH, 0.0f);
        check(glm::distance(player.position(), expectedAfter) < 1e-3f,
              "Person remains settled on the camera after the fixture load");
    } else {
        std::cout << "  skip: saves/tests/basic_cube_law_test_final.json is not on disk "
                     "(gitignored); synthetic dump covered the live path.\n";
    }
    } catch (const std::exception& e) {
        guardedSectionThrew = true;
        guardedSectionException = e.what();
    } catch (...) {
        guardedSectionThrew = true;
        guardedSectionException = "non-std::exception thrown";
    }
    // realTreeGuard is out of scope here either way — normal fall-through
    // or stack unwinding through the catches above — so the real tree
    // should already be restored from backup by this point.

    check(!guardedSectionThrew,
          std::string("the guarded section completed without throwing") +
              (guardedSectionThrew ? (" (threw: " + guardedSectionException + ")") : ""));
    const std::string afterTreeHash =
        TestSupport::hashDirectoryTree("saves/zones") + "|" +
        TestSupport::hashDirectoryTree("saves/homes");
    check(afterTreeHash == beforeTreeHash,
          "saves/zones and saves/homes hash byte-identical after the guarded section — "
          "the real identity tree was restored, even if the section above threw");

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
