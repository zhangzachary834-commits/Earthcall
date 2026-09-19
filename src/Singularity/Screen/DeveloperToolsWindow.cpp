#include "DeveloperToolsWindow.hpp"

#include "imgui.h"
#include "GLFW/glfw3.h"

#include "Singularity/Core/Engine.hpp"
#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/CreatorConsole/CreatorConsoleState.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/SaveContext.hpp"
#include "json.hpp"
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
extern ZoneManager mgr;

namespace Rendering {

namespace {
struct TestSaveRow {
    std::string filename;
    int objectCount = -1;
};

int countSavedObjects(const std::filesystem::path& path) {
    try {
        std::ifstream in(path);
        if (!in) return -1;
        nlohmann::json j;
        in >> j;
        int n = 0;
        if (j.contains("zones") && j["zones"].is_array()) {
            for (const auto& z : j["zones"]) {
                if (z.contains("world") && z["world"].contains("objects") &&
                    z["world"]["objects"].is_array()) {
                    n += static_cast<int>(z["world"]["objects"].size());
                }
            }
        }
        if (n == 0 && j.contains("objects") && j["objects"].is_array()) {
            n = static_cast<int>(j["objects"].size());
        }
        return n;
    } catch (...) {
        return -1;
    }
}

std::vector<TestSaveRow> scanTestSaves() {
    std::vector<TestSaveRow> rows;
    if (!std::filesystem::exists("saves/tests")) return rows;
    std::error_code ec;
    auto it = std::filesystem::directory_iterator(
        "saves/tests", std::filesystem::directory_options::skip_permission_denied, ec);
    if (ec) return rows;
    for (const auto& entry : it) {
        if (entry.path().extension() != ".json") continue;
        TestSaveRow row;
        row.filename = entry.path().filename().string();
        row.objectCount = countSavedObjects(entry.path());
        rows.push_back(std::move(row));
    }
    return rows;
}
} // namespace

void renderDeveloperToolsWindow(bool* open, GLFWwindow* window, Core::Engine* engine) {
    if (!window || !engine) return;
    if (!open || !*open) return;

    // Placement sensing and the L-key that arms the shape-generator law
    // used to live here, above the `*open` return. They now run in
    // Rendering::stepCreationTools from Engine::update — this window is
    // the test-save loader, not a first-mover step hiding in a render.

    if (ImGui::Begin("Developer: Test World Saves", open)) {
        static std::vector<TestSaveRow> testSaves;
        static bool scanned = false;
        if (!scanned) {
            testSaves = scanTestSaves();
            scanned = true;
        }

        ImGui::TextWrapped(
            "Observe a test dump in-world. Home is not replaced. You are moved "
            "into a Zone named test.<save> facing the loaded beings.");
        ImGui::TextDisabled("Grave / Toggle Dev Mode. *_final.json files have spawned objects.");

        if (ImGui::Button("Refresh Test Saves")) {
            scanned = false;
        }

        ImGui::Separator();

        if (testSaves.empty()) {
            ImGui::TextDisabled("No JSON dumps in saves/tests/. Run the law tests to dump them.");
        }

        for (const auto& row : testSaves) {
            ImGui::PushID(row.filename.c_str());
            if (ImGui::Button("Observe")) {
                SaveContext ctx;
                ctx.camera = engine->getCamera();
                ctx.mouseHandler = engine->getMouseHandler();
                static float dummyColor[3] = {1.0f, 1.0f, 1.0f};
                ctx.currentColor = dummyColor;
                ctx.person = engine->getPerson();
                ctx.lawManager = engine->getLawManager();
                ctx.worldTime = engine->worldTimePtr();
                ctx.unpackForAuthoring = false;
                mgr.loadTestObservation("saves/tests/" + row.filename, ctx);
                forgetStaleObjectHandles(mgr, engine->getPerson());
            }
            ImGui::SameLine();
            ImGui::TextUnformatted(row.filename.c_str());
            if (row.objectCount >= 0) {
                ImGui::SameLine();
                if (row.objectCount == 0) {
                    ImGui::TextDisabled("(law seed, no objects)");
                } else {
                    ImGui::TextDisabled("(%d object%s)", row.objectCount,
                                        row.objectCount == 1 ? "" : "s");
                }
            }
            ImGui::PopID();
        }

        const std::string& report = mgr.getSaveLoadState().lastLoadReport;
        if (!report.empty()) {
            ImGui::Separator();
            ImGui::TextWrapped("%s", report.c_str());
        }
    }
    ImGui::End();
}

} // namespace Rendering
