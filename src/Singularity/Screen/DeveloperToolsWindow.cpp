#include "DeveloperToolsWindow.hpp"

#include "imgui.h"
#include "GLFW/glfw3.h"

#include "Singularity/Input/Interaction/InteractionChannel.hpp"
#include "Singularity/Screen/ScreenChannel.hpp"


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
    // Test observations are workshop output: tests write them under the
    // checkout's saves/tests (cwd), not the Person's data home.
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

void renderDeveloperToolsContent(GLFWwindow* window, Core::Engine* engine) {
    if (!window || !engine) return;

    if (ImGui::CollapsingHeader("Interaction Diagnostics", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (engine && engine->getLawManager()) {
            if (auto* chan = Singularity::Input::InteractionChannel::find(*engine->getLawManager())) {
                ImGui::Text("Hovered ID: %s", chan->hoveredId.c_str());
                ImGui::Text("Pressed ID: %s", chan->pressedId.c_str());
                ImGui::Text("Left Down: %s", chan->leftDown ? "true" : "false");
                ImGui::Text("Dragging: %s", chan->dragging ? "true" : "false");
                ImGui::Text("_liveLeftDown: %s", chan->liveLeftDown() ? "true" : "false");
                ImGui::Text("_pendingLeftEdges: %zu", chan->pendingLeftEdges().size());
                ImGui::Text("WantCaptureMouse: %s", ImGui::GetIO().WantCaptureMouse ? "true" : "false");

                int rawState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
                ImGui::Text("glfwGetMouseButton (RAW): %s", rawState == GLFW_PRESS ? "PRESS" : "RELEASE");
            } else {
                ImGui::TextDisabled("InteractionChannel not found.");
            }
        }
    }

    if (ImGui::CollapsingHeader("Routed Volume Transport (Northern Veil)", ImGuiTreeNodeFlags_DefaultOpen)) {
        // These write the ScreenChannel's registered properties (the same paths Law and the CLI reach); the renderer applies them next frame.
        auto* screen = engine->getLawManager() ? Singularity::Screen::ScreenChannel::find(*engine->getLawManager()) : nullptr;
        if (!screen) {
            ImGui::TextDisabled("ScreenChannel not found.");
        } else {
            ImGui::TextWrapped(
                "A/B the routed-ray transport on the live view. Only applies when four-curtain routed "
                "transport is active; the readouts below show whether it ran. Watch the Performance Metrics window for ms/frame.");
            ImGui::Checkbox("Routed transport  (off = Current renderer)", &screen->volumeRoutedTransportEnabled);
            ImGui::Checkbox("Coalesced single compute pass  (default on; off = original 8 passes/strip)", &screen->volumeRoutedCoalescedPasses);
            ImGui::Checkbox("Skip wave-empty tiles  (2a, default off)", &screen->volumeRoutedSkipEmptyTiles);
            ImGui::Checkbox("Single-member tiles to S1  (2b, default off; includes 2a)", &screen->volumeRoutedSingleMemberS1);
            if (screen->volumeRoutedSingleMemberS1 && !screen->volumeRoutedSkipEmptyTiles) {
                ImGui::TextDisabled("2b also skips wave-empty tiles, so it behaves as 2a + 2b regardless of the 2a box.");
            }
            ImGui::TextDisabled("2b: refusals come only from curtains the tile proof marks active (accepted contract); measured 2-12%% under Current, one device.");
            if (ImGui::Button("Reset to defaults")) {
                screen->volumeRoutedTransportEnabled = true;
                screen->volumeRoutedCoalescedPasses = true;
                screen->volumeRoutedSkipEmptyTiles = false;
                screen->volumeRoutedSingleMemberS1 = false;
            }
            ImGui::Text("Last frame: routed sets %d, routed pixels %d, working set %.1f MB",
                        screen->volumeRoutedSets, screen->volumeRoutedPixels,
                        screen->volumeRoutedWorkingSetBytes / (1024.0 * 1024.0));
        }
    }

    if (ImGui::CollapsingHeader("Test World Saves")) {
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
}

void renderDeveloperToolsWindow(bool* open, GLFWwindow* window, Core::Engine* engine) {
    if (!window || !engine) return;
    if (!open || !*open) return;

    if (ImGui::Begin("Developer Tools", open)) {
        renderDeveloperToolsContent(window, engine);
    }
    ImGui::End();
}

} // namespace Rendering
