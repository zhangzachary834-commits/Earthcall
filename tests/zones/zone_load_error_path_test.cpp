// Guards the error reporting path for ZoneManager::loadState and loadTestObservation.
// Triggering specific error conditions in C++ (like file IO or parsing exceptions) can
// sometimes require complex mocking or fixture setup, leading to lower confidence
// in a quick test implementation. This test provides a deterministic way to trigger
// parse errors without full mocking, by simply writing an invalid JSON file to the sandbox.
// It also tests the inner stage failure by writing a valid JSON shell with bad content.

#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Singularity/Input/Mouse/MouseHandler.hpp"
#include "Singularity/Screen/Camera.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/SaveContext.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "json.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <filesystem>
#include <fstream>
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

} // namespace

int main() {
    std::cout << "============================================================\n";
    std::cout << "Running ZoneManager load error path test...\n";
    std::cout << "============================================================\n";

    auto sandbox = std::filesystem::temp_directory_path() / "earthcall_zone_load_error";
    std::filesystem::remove_all(sandbox);
    std::filesystem::create_directories(sandbox / "worlds");
    SaveSystem::setSaveRoot(sandbox.string());

    Soul soul("Player");
    Body body("humanoid", "default");
    Person player(std::move(soul), std::move(body), "default");
    Core::Camera camera;
    MouseHandler mouse;
    LawManager laws;
    float color[3] = {1.0f, 1.0f, 1.0f};
    double worldTime = 1.0;
    SaveContext ctx;
    ctx.camera = &camera;
    ctx.mouseHandler = &mouse;
    ctx.currentColor = color;
    ctx.person = &player;
    ctx.lawManager = &laws;
    ctx.worldTime = &worldTime;

    const std::string badJsonPath = (sandbox / "worlds" / "bad.json").string();
    {
        std::ofstream out(badJsonPath);
        out << "{ \"invalid json\": ";
    }

    ZoneManager live;
    auto z = std::make_shared<Zone>("Sanctum of Beginnings", "default");
    live.addZone(z);

    live.loadState(badJsonPath, ctx);

    const std::string& report = live.getSaveLoadState().lastLoadReport;
    check(report.find("COULD NOT OPEN OR READ") != std::string::npos, "loadState handles JSON parse error gracefully");

    live.loadTestObservation(badJsonPath, ctx);
    const std::string& obsReport = live.getSaveLoadState().lastLoadReport;
    check(obsReport.find("COULD NOT OPEN OR READ") != std::string::npos, "loadTestObservation handles JSON parse error gracefully");

    const std::string badDataPath = (sandbox / "worlds" / "bad_data.json").string();
    {
        std::ofstream out(badDataPath);
        // Make 'physicsLaws' something that throws when iterated with range-based for
        // nlohmann json throws type_error when iterating a non-array/non-object if expected
        // Actually, if we make it a number, iterating it might throw.
        out << "{ \"physicsLaws\": 123, \"objects\": [] }";
    }

    live.loadState(badDataPath, ctx);
    const std::string& stageReport = live.getSaveLoadState().lastLoadReport;

    // In ZoneManager::loadState, there is a try-catch for the overall load.
    // If an exception is thrown, lastLoadReport becomes "LOAD FAILED: " + e.what()
    // Or if the stage throws, the stage catches it and logs to 'failures'.
    check(stageReport.find("FAILED stages:") != std::string::npos || stageReport.find("LOAD FAILED:") != std::string::npos, "loadState handles exception during loading stages or parsing");

    std::filesystem::remove_all(sandbox);
    SaveSystem::setSaveRoot("");

    std::cout << "------------------------------------------------------------\n";
    std::cout << g_checks - g_failures << "/" << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cout << "zone_load_error_path_test: FAILED\n";
        return 1;
    }
    std::cout << "zone_load_error_path_test: ALL OK\n";
    return 0;
}
