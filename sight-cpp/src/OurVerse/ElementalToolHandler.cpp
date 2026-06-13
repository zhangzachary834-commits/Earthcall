#include "ElementalToolHandler.hpp"
#include "imgui.h"
#include "Tool.hpp"
#include "Core/Game.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "Rendering/BrushSystem.hpp"
#include <string>

namespace {

void activateDesignTool(Zone& zone, Tool::Type type) {
    zone.setDesignTool(type);
    if (!zone.getBrushSystem()) {
        zone.initializeBrushSystem();
    }

    switch (type) {
        case Tool::Type::Airbrush:
            zone.setBrushType(BrushSystem::BrushType::Airbrush);
            break;
        case Tool::Type::Pencil:
            zone.setBrushType(BrushSystem::BrushType::Normal);
            zone.setBrushRadius(0.035f);
            zone.setBrushOpacity(1.0f);
            zone.setBrushFlow(1.0f);
            zone.setBrushSpacing(0.012f);
            break;
        case Tool::Type::Pen:
            zone.setBrushType(BrushSystem::BrushType::Normal);
            zone.setBrushRadius(0.055f);
            zone.setBrushOpacity(1.0f);
            zone.setBrushFlow(1.0f);
            zone.setBrushSpacing(0.008f);
            break;
        case Tool::Type::Marker:
            zone.setBrushType(BrushSystem::BrushType::Normal);
            zone.setBrushRadius(0.13f);
            zone.setBrushOpacity(0.55f);
            zone.setBrushFlow(0.75f);
            zone.setBrushSpacing(0.02f);
            break;
        case Tool::Type::Chalk:
            zone.setBrushType(BrushSystem::BrushType::Chalk);
            break;
        case Tool::Type::Spray:
            zone.setBrushType(BrushSystem::BrushType::Spray);
            break;
        case Tool::Type::Smudge:
            zone.setBrushType(BrushSystem::BrushType::Smudge);
            break;
        case Tool::Type::Clone:
            zone.setBrushType(BrushSystem::BrushType::Clone);
            zone.setCloneActive(true);
            zone.setCloneOffset(glm::vec2(-0.08f, -0.08f));
            break;
        case Tool::Type::Brush:
        case Tool::Type::Eraser:
        case Tool::Type::Delete:
        case Tool::Type::MagicEraser:
            zone.setBrushType(BrushSystem::BrushType::Normal);
            zone.setCloneActive(false);
            break;
        default:
            zone.setCloneActive(false);
            break;
    }
}

} // namespace

ElementalToolHandler::ElementalToolHandler(ZoneManager* mgr) {
    // Constructor
    _mgr = mgr;
}

ElementalToolHandler::~ElementalToolHandler() {
    // Destructor
}

void ElementalToolHandler::tool_status_update(Core::Game* game, GLFWwindow* window) {
    (void)game;
    (void)window;
    static bool showPaint = true;
    
    if (ImGui::Begin(u8"🎨 Professional 2D Design", &showPaint)) {
        Zone& zone = _mgr->active();
        
        // Ensure design system is initialized
        if (!zone.getDesignSystem()) {
            zone.initializeDesignSystem();
        }
        
        // Tool Categories
        if (ImGui::BeginTabBar("DesignTools")) {
            
            // Drawing Tools Tab
            if (ImGui::BeginTabItem("🖌 Drawing")) {
                ImGui::BeginGroup();
                
                // Drawing Tools
                if (ImGui::Button(u8"🖌 Brush")) { 
                    activateDesignTool(zone, Tool::Type::Brush);
                }
                ImGui::SameLine();
                if (ImGui::Button(u8"✏️ Pencil")) { 
                    activateDesignTool(zone, Tool::Type::Pencil);
                }
                ImGui::SameLine();
                if (ImGui::Button(u8"🖊 Pen")) { 
                    activateDesignTool(zone, Tool::Type::Pen);
                }
                
                if (ImGui::Button(u8"💨 Airbrush")) { 
                    activateDesignTool(zone, Tool::Type::Airbrush);
                }
                ImGui::SameLine();
                if (ImGui::Button(u8"🖼 Chalk")) { 
                    activateDesignTool(zone, Tool::Type::Chalk);
                }
                ImGui::SameLine();
                if (ImGui::Button(u8"🎨 Spray")) { 
                    activateDesignTool(zone, Tool::Type::Spray);
                }
                
                if (ImGui::Button(u8"👆 Smudge")) { 
                    activateDesignTool(zone, Tool::Type::Smudge);
                }
                ImGui::SameLine();
                if (ImGui::Button(u8"📋 Clone")) { 
                    activateDesignTool(zone, Tool::Type::Clone);
                }
                
                ImGui::EndGroup();
                ImGui::EndTabItem();
            }
            
            // Erasing Tools Tab
            if (ImGui::BeginTabItem("🧽 Erasing")) {
                ImGui::BeginGroup();
                
                if (ImGui::Button(u8"🧽 Eraser")) { 
                    activateDesignTool(zone, Tool::Type::Eraser);
                }
                ImGui::SameLine();
                if (ImGui::Button(u8"⌫ Delete")) {
                    activateDesignTool(zone, Tool::Type::Delete);
                }
                ImGui::SameLine();
                if (ImGui::Button(u8"✨ Magic Eraser")) { 
                    activateDesignTool(zone, Tool::Type::MagicEraser);
                }
                
                ImGui::EndGroup();
                ImGui::EndTabItem();
            }
            
            // Selection Tools Tab
            if (ImGui::BeginTabItem("⬜ Selection")) {
                ImGui::BeginGroup();
                
                if (ImGui::Button(u8"⬜ Selection")) { 
                    activateDesignTool(zone, Tool::Type::Selection);
                }
                ImGui::SameLine();
                if (ImGui::Button(u8"🔗 Lasso")) { 
                    activateDesignTool(zone, Tool::Type::Lasso);
                }
                
                if (ImGui::Button(u8"🪄 Magic Wand")) { 
                    activateDesignTool(zone, Tool::Type::MagicWand);
                }
                ImGui::SameLine();
                if (ImGui::Button(u8"📦 Marquee")) { 
                    activateDesignTool(zone, Tool::Type::Marquee);
                }
                
                ImGui::EndGroup();
                ImGui::EndTabItem();
            }
            
            // Shape Tools Tab
            if (ImGui::BeginTabItem("🔷 Shapes")) {
                ImGui::BeginGroup();
                
                if (ImGui::Button(u8"⬜ Rectangle")) { 
                    activateDesignTool(zone, Tool::Type::Rectangle);
                }
                ImGui::SameLine();
                if (ImGui::Button(u8"⭕ Ellipse")) { 
                    activateDesignTool(zone, Tool::Type::Ellipse);
                }
                ImGui::SameLine();
                if (ImGui::Button(u8"🔷 Polygon")) { 
                    activateDesignTool(zone, Tool::Type::Polygon);
                }
                
                if (ImGui::Button(u8"➖ Line")) { 
                    activateDesignTool(zone, Tool::Type::Line);
                }
                ImGui::SameLine();
                if (ImGui::Button(u8"➡️ Arrow")) { 
                    activateDesignTool(zone, Tool::Type::Arrow);
                }
                ImGui::SameLine();
                if (ImGui::Button(u8"⭐ Star")) { 
                    activateDesignTool(zone, Tool::Type::Star);
                }
                
                if (ImGui::Button(u8"❤️ Heart")) { 
                    activateDesignTool(zone, Tool::Type::Heart);
                }
                ImGui::SameLine();
                if (ImGui::Button(u8"🔶 Custom")) { 
                    activateDesignTool(zone, Tool::Type::CustomShape);
                }
                
                ImGui::EndGroup();
                ImGui::EndTabItem();
            }
            
            // Text Tools Tab
            if (ImGui::BeginTabItem("T Text")) {
                ImGui::BeginGroup();
                
                if (ImGui::Button(u8"T Text")) { 
                    activateDesignTool(zone, Tool::Type::Text);
                }
                ImGui::SameLine();
                if (ImGui::Button(u8"T↕️ Vertical")) { 
                    activateDesignTool(zone, Tool::Type::TextVertical);
                }
                ImGui::SameLine();
                if (ImGui::Button(u8"T〰️ Path")) { 
                    activateDesignTool(zone, Tool::Type::TextPath);
                }
                
                ImGui::EndGroup();
                ImGui::EndTabItem();
            }
            
            // Utility Tools Tab
            if (ImGui::BeginTabItem("🔧 Utility")) {
                ImGui::BeginGroup();
                
                if (ImGui::Button(u8"🎯 Color Picker")) { 
                    activateDesignTool(zone, Tool::Type::ColorPicker);
                }
                ImGui::SameLine();
                if (ImGui::Button(u8"💉 Eyedropper")) { 
                    activateDesignTool(zone, Tool::Type::Eyedropper);
                }

                ImGui::EndGroup();
                ImGui::EndTabItem();
            }
            
            ImGui::EndTabBar();
        }

        ImGui::Separator();
        
        // Color and Properties Panel
        ImGui::BeginGroup();
        ImGui::Text("Color & Properties:");
        ImGui::SameLine();
        
        glm::vec3 zoneColor = zone.getCurrentColor();
        float currentColor[3] = {zoneColor.r, zoneColor.g, zoneColor.b};
        if (ImGui::ColorEdit3("##MainColor", currentColor, ImGuiColorEditFlags_NoInputs)) {
            zone.setDrawColor(currentColor[0], currentColor[1], currentColor[2]);
        }
        
        // Layer Management
        ImGui::Separator();
        ImGui::Text("Layer Management:");
        if (ImGui::Button("Add Layer")) {
            zone.addDesignLayer();
        }
        ImGui::SameLine();
        if (ImGui::Button("Remove Layer")) {
            zone.removeDesignLayer(zone.getActiveDesignLayer());
        }
        
        // Show current tool status
        const DesignSystem* designSystem = zone.getDesignSystem();
        std::string currentToolName = designSystem ? Tool(designSystem->getCurrentTool()).getTypeName() : "Brush";
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Current Tool: %s", currentToolName.c_str());
        
        ImGui::EndGroup();
    }
    ImGui::End();
}
