#include "DeveloperToolsWindow.hpp"

#include "imgui.h"
#include "GLFW/glfw3.h"

#include "Singularity/FirstMoverWindowTools/Tool.hpp"
#include "Singularity/Core/Engine.hpp"
#include "Singularity/Core/CreationChannel.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
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

const char* shapeKindLabel(int kind) {
    switch (static_cast<Object::ShapeKind>(kind)) {
        case Object::ShapeKind::Cube:       return "Cube";
        case Object::ShapeKind::Sphere:     return "Sphere";
        case Object::ShapeKind::Cylinder:   return "Cylinder";
        case Object::ShapeKind::Cone:       return "Cone";
        case Object::ShapeKind::Ellipsoid:  return "Ellipsoid";
        case Object::ShapeKind::Ovoid:      return "Ovoid";
        case Object::ShapeKind::Paraboloid: return "Paraboloid";
        default:                            return "Unsupported";
    }
}

const Object::ShapeKind kDevToolShapeKinds[] = {
    Object::ShapeKind::Cube,   Object::ShapeKind::Sphere,     Object::ShapeKind::Cylinder,
    Object::ShapeKind::Cone,   Object::ShapeKind::Ellipsoid,  Object::ShapeKind::Ovoid,
    Object::ShapeKind::Paraboloid,
};

} // namespace

void renderDeveloperToolsWindow(bool* open, GLFWwindow* window, Core::Engine* engine) {
    if (!open || !*open || !window || !engine) return;

    auto* channel = findCreationChannel(engine);
    if (!channel) return;

    // The law path's activation input: a keyboard edge, not an ImGui button,
    // so it stays a genuinely different input from this window's spawn
    // button below. Deliberately not gated on `*open` -- arming the law path
    // shouldn't require this developer window to be visible.
    static bool lawModeKeyDownLast = false;
    bool lawModeKeyDown = glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS;
    if (lawModeKeyDown && !lawModeKeyDownLast) {
        channel->active3DMode = (channel->active3DMode == "Create") ? "" : "Create";
    }
    lawModeKeyDownLast = lawModeKeyDown;

    if (channel->active3DMode == "Create") {
        // Mode is active, tool logic executes when the global onMouseClicked event fires.
    }



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

    Tool::ShapeGenerator3D(window, engine, mgr, *channel);
}

} // namespace Rendering
