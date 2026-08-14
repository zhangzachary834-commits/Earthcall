#include "DeveloperToolsWindow.hpp"

#include "imgui.h"
#include "GLFW/glfw3.h"

#include "OurVerse/Tool.hpp"
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
        static bool lawClickDownLast = false;
        bool lawClickDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        if (lawClickDown && !lawClickDownLast) {
            ECA::Event ev{"onMouseClicked", channel, nullptr, 0};
            Core::EventBus::instance().publish(ev);
        }
        lawClickDownLast = lawClickDown;
    }

    if (ImGui::Begin("Developer: 3D Create Tool", open)) {
        ImGui::TextWrapped(
            "Restored pre-law spawn tool. Left-click below spawns directly, "
            "authored by the CreationChannel First Mover.");
        ImGui::Separator();

        ImGui::TextColored(
            channel->active3DMode == "Create" ? ImVec4(0.3f, 0.9f, 0.3f, 1.0f)
                                               : ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
            "Law mode (press L): %s",
            channel->active3DMode == "Create" ? "ARMED -- click fires the law" : "off");

        ImGui::Separator();

        static int shapeIndex = 0;
        const int kShapeKindCount = static_cast<int>(sizeof(kDevToolShapeKinds) / sizeof(kDevToolShapeKinds[0]));
        if (ImGui::BeginCombo("Shape", shapeKindLabel(channel->activeShapeKind))) {
            for (int i = 0; i < kShapeKindCount; ++i) {
                bool selected = channel->activeShapeKind == static_cast<int>(kDevToolShapeKinds[i]);
                if (ImGui::Selectable(shapeKindLabel(static_cast<int>(kDevToolShapeKinds[i])), selected)) {
                    channel->activeShapeKind = static_cast<int>(kDevToolShapeKinds[i]);
                    shapeIndex = i;
                }
                if (selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        (void)shapeIndex;

        ImGui::ColorEdit3("Colour", &channel->activeColor.x);

        const char* placementModes[] = {"InFront", "CursorSnap", "ManualDistance"};
        int placementIndex = 0;
        for (int i = 0; i < 3; ++i) if (channel->placementMode == placementModes[i]) placementIndex = i;
        if (ImGui::Combo("Placement", &placementIndex, placementModes, 3)) {
            channel->placementMode = placementModes[placementIndex];
            channel->manualAnchorValid = false;
        }

        if (channel->placementMode == "InFront") {
            ImGui::SliderFloat("Distance", &channel->inFrontDistance, 0.5f, 10.0f);
        } else if (channel->placementMode == "ManualDistance") {
            ImGui::SliderFloat3("Offset (right/up/fwd)", &channel->manualOffset.x, -10.0f, 10.0f);
            if (ImGui::Button("Reset Anchor")) channel->manualAnchorValid = false;
        }

        ImGui::Checkbox("Grid Snap", &channel->gridSnap);
        if (channel->gridSnap) {
            ImGui::SliderFloat("Grid Size", &channel->gridSnapSize, 0.1f, 5.0f);
        }

        ImGui::Separator();
        ImGui::TextWrapped("Click anywhere in the 3D view to spawn.");
    }
    ImGui::End();

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
                SaveContext ctx;
                ctx.camera = engine->getCamera();
                ctx.mouseHandler = engine->getMouseHandler();
                static float dummyColor[3] = {1.0f, 1.0f, 1.0f};
                ctx.currentColor = dummyColor;
                ctx.currentTool = nullptr; // Engine might need a current tool, but test save loading usually doesn't strictly depend on it unless Tool state is serialized
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
