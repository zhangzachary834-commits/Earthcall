#include "ElementalToolHandler.hpp"
#include "imgui.h"
#include "Tool.hpp"
#include "Singularity/Core/Engine.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "Singularity/Screen/BrushSystem.hpp"

ElementalToolHandler::ElementalToolHandler(ZoneManager* mgr) {
    _mgr = mgr;
}

ElementalToolHandler::~ElementalToolHandler() {
}

void ElementalToolHandler::tool_status_update(Core::Engine* engine, GLFWwindow* window) {
    (void)engine;
    (void)window;
    static bool showPaint = true;
    
    if (ImGui::Begin(u8"🎨 Professional 2D Design (Disabled)", &showPaint)) {
        ImGui::Text("Design tools have been migrated to the Object paradigm and are currently undergoing maintenance.");
    }
    ImGui::End();
}
