#include "DeveloperToolsWindow.hpp"

#include "imgui.h"
#include "GLFW/glfw3.h"

#include "Singularity/Core/Engine.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/SaveContext.hpp"
#include <filesystem>
#include <vector>
#include <string>
extern ZoneManager mgr;

namespace Rendering {

void renderDeveloperToolsWindow(bool* open, GLFWwindow* window, Core::Engine* engine) {
    if (!window || !engine) return;
    if (!open || !*open) return;

    // Placement sensing and the L-key that arms the shape-generator law
    // used to live here, above the `*open` return. They now run in
    // Rendering::stepCreationTools from Engine::update — this window is
    // the test-save loader, not a first-mover step hiding in a render.

    if (ImGui::Begin("Developer: Test World Saves", open)) {
        static std::vector<std::string> testSaves;
        static bool scanned = false;
        if (!scanned) {
            testSaves.clear();
            if (std::filesystem::exists("saves/tests")) {
                std::error_code ec;
                auto it = std::filesystem::directory_iterator("saves/tests", std::filesystem::directory_options::skip_permission_denied, ec);
                if (!ec) {
                    for (const auto& entry : it) {
                        if (entry.path().extension() == ".json") {
                            testSaves.push_back(entry.path().filename().string());
                        }
                    }
                }
            }
            scanned = true;
        }

        if (ImGui::Button("Refresh Test Saves")) {
            scanned = false;
        }
        
        ImGui::Separator();
        
        for (const auto& saveName : testSaves) {
            if (ImGui::Button(("Load " + saveName).c_str())) {
                // Find or create "Test Zone"
                size_t testZoneIndex = static_cast<size_t>(-1);
                for (size_t i = 0; i < mgr.zones().size(); ++i) {
                    if (mgr.zones()[i]->getIdentifier() == "Test Zone") {
                        testZoneIndex = i;
                        break;
                    }
                }
                if (testZoneIndex == static_cast<size_t>(-1)) {
                    auto testZone = std::make_shared<Zone>("Test Zone", "test", Zone::Scope::Local);
                    mgr.addZone(testZone);
                    testZoneIndex = mgr.zones().size() - 1;
                }
                mgr.switchTo(testZoneIndex);

                SaveContext ctx;
                ctx.camera = engine->getCamera();
                ctx.mouseHandler = engine->getMouseHandler();
                static float dummyColor[3] = {1.0f, 1.0f, 1.0f};
                ctx.currentColor = dummyColor;
                ctx.player = engine->getPlayer();
                ctx.lawManager = engine->getLawManager();
                double t = 0;
                ctx.worldTime = &t;
                ctx.unpackForAuthoring = false;
                
                mgr.loadState("saves/tests/" + saveName, ctx);
            }
        }
    }
    ImGui::End();
}

} // namespace Rendering
