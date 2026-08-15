#pragma once

#include <glm/glm.hpp>
#include <vector>
#include "Singularity/FirstMoverWindowTools/Tool.hpp"
#include "ConstructedBeing/Object/Object.hpp"

#include <imgui.h>

namespace Rendering {

    enum class CreatorSection {
        Paint,
        Create3D,
        Character,
        World,
        Assets,
        Relations,
        Zones
    };

    enum class Mode3D {
        None,
        BrushCreate,
        Selection,
        FaceBrush,
        FacePaint,
        Pottery,
        Rotation,
        Morph,
        Combine,
        Sculpt,
        Clay,
        Graph
    };

    enum class ToolTarget3D {
        WorldObjects,
        SelectionOnly
    };

    struct BrushParams {
        float size = 1.0f;
        glm::vec3 scale = glm::vec3(1.0f);
        glm::vec3 rotation = glm::vec3(0.0f);
        bool gridSnap = false;
        float gridSize = 1.0f;
        bool useAdvanced2D = false;
    };

    struct PolyhedronParams {
        ObjectTypes::ShapeKind shapeKind = ObjectTypes::ShapeKind::Cube;
        ObjectTypes::ShapeParams shapeParams;
        int currentType = 4;
        int irregularType = 0;
        int irregularBaseSides = 4;
        float irregularHeight = 1.0f;
        float frustumTopScale = 0.5f;
        int concaveType = 0;
        float concavityAmount = 0.5f;
        float spikeLength = 0.5f;
        float craterDepth = 0.2f;
        bool useCustom = false;
        int customVertexCount = 8;
        int customFaceCount = 6;
        std::vector<glm::vec3> customVertices;

        void generateCustom() {
            customVertices.clear();
            // Stub for custom generation
        }
    };

    struct CreatorConsoleState {
        CreatorSection currentSection = CreatorSection::Create3D;
        
        // 3D Create State
        Mode3D current3DMode = Mode3D::BrushCreate;
        ToolTarget3D current3DTarget = ToolTarget3D::WorldObjects;
        ObjectTypes::ShapeKind currentShapeKind = ObjectTypes::ShapeKind::Cube;
        glm::vec3 createColor = glm::vec3(1.0f);
        bool wireframe = false;

        PolyhedronParams polyhedron;
        BrushParams brush;

        // Combine / Clay State
        int combineOp = 0;
        float combineBlend = 0.0f;
        Object* combineOperandA = nullptr;
        Object* clayGrabbed = nullptr;
        Object* clayTarget = nullptr;

        // Morph State
        int morphVertexIndex = -1;
        int patchCtrlIndex = -1;

        // Paint State
        Tool currentTool = Tool(Tool::Type::Brush);
        float currentColor[3] = {1.0f, 1.0f, 1.0f};
        bool use2DPressureSimulation = false;
        bool useLegacy2DTools = false;

        // Character State
        bool characterDesignLocked = false;

        // World State
        bool cursorToolsOpen = false;
        bool showLawAuthor = false;
    };

    CreatorConsoleState& getCreatorConsoleState();
    
    // Shared styling helpers
    void pushActiveButtonStyle(bool active, const ImVec4& color, const ImVec4& hoverColor);
    void popActiveButtonStyle(bool active);
    void sameLineEvery(int index, int perRow);

} // namespace Rendering
