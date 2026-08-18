#include "DeveloperToolsWindow.hpp"

#include "imgui.h"
#include "GLFW/glfw3.h"

#include "Singularity/Core/Engine.hpp"
#include "Singularity/Core/CreationChannel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/SaveContext.hpp"
#include <filesystem>
#include <vector>
#include <string>
extern ZoneManager mgr;

namespace Rendering {

namespace {

// CreationChannel is registered once (EngineInit.cpp, syncRegister) and never
// removed -- it is a First Mover, so LawManager::loadFromJson keeps it across
// every load. Finding it by dynamic_cast each frame, rather than caching the
// pointer, matches CreationChannel::syncRegister's own lookup and stays
// correct even if a save reload ever rebuilds the law list.
Singularity::Core::CreationChannel* findCreationChannel(Core::Engine* engine) {
    if (!engine || !engine->getLawManager()) return nullptr;
    for (const auto& law : engine->getLawManager()->getAll()) {
        auto* channel = dynamic_cast<Singularity::Core::CreationChannel*>(law.get());
        if (channel) return channel;
    }
    return nullptr;
}

} // namespace

void renderDeveloperToolsWindow(bool* open, GLFWwindow* window, Core::Engine* engine) {
    if (!window || !engine) return;

    auto* channel = findCreationChannel(engine);
    if (!channel) return;

    // The law path's activation input: a keyboard edge, not an ImGui button,
    // so it stays a genuinely different input from this window's panels below.
    // Deliberately not gated on `*open` -- arming the law path shouldn't
    // require this developer window to be visible. That is why it sits ABOVE
    // the `*open` return: the comment said so from the day it was written
    // (8c9a725b) while a `!*open` early return at the top of the function made
    // it false, so L did nothing unless the window happened to be open.
    //
    // Setting active3DMode to "Create" is the whole arming gesture: the law
    // ("Tool: Shape Generator 3D") conditions on exactly this property, and
    // the click itself is published globally from the GLFW mouse callback.
    static bool lawModeKeyDownLast = false;
    bool lawModeKeyDown = glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS;
    if (lawModeKeyDown && !lawModeKeyDownLast) {
        channel->active3DMode = (channel->active3DMode == "Create") ? "" : "Create";
    }
    lawModeKeyDownLast = lawModeKeyDown;

    if (!open || !*open) return;   // panels below are the window; arming is not

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
