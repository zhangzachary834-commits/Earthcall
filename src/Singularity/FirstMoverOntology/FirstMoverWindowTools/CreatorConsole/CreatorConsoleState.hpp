#pragma once

#include <glm/glm.hpp>
#include <vector>
#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/Tool.hpp"
#include "ConstructedBeing/Object/Object.hpp"
class BodyPart;

#include <imgui.h>

class ZoneManager;
class Person;

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
        // None until a Person arms a tool (console 3D tab, F4/F5, or a
        // mode button). Defaulting to BrushCreate made the developer
        // bypass fire on every click from boot, even with the console
        // never opened — the opposite of "tools run because they were
        // selected", which is the reason dispatch left render.
        Mode3D current3DMode = Mode3D::None;
        ToolTarget3D current3DTarget = ToolTarget3D::WorldObjects;
        ObjectTypes::ShapeKind currentShapeKind = ObjectTypes::ShapeKind::Cube;
        glm::vec3 createColor = glm::vec3(1.0f);
        bool wireframe = false;

        PolyhedronParams polyhedron;
        BrushParams brush;

        // Combine / Clay State
        int combineOp = 0;
        float combineBlend = 0.0f;
        // BENEATH THE KERNEL: per-gesture pick operands. They name Objects
        // the Person already holds; the next click recomputes them. Not a
        // being's standing state — the live tool is @creation-channel.activeTool.
        // Stale after ZoneManager::loadState (those Objects die). Callers of
        // forgetStaleObjectHandles: AssetsConsole::loadWorld,
        // DeveloperToolsWindow observation load.
        Object* combineOperandA = nullptr;
        Object* clayGrabbed = nullptr;
        Object* clayTarget = nullptr;

        // Morph State
        int morphVertexIndex = -1;
        int patchCtrlIndex = -1;
        bool fieldHandleDragging = false;
        bool blendHandleDragging = false;

        // Live 3D selection (Select mode). Not a being's standing state —
        // HighlightSystem mirrors it for the renderer. Same stale-on-load
        // latch as combineOperandA — forgetStaleObjectHandles.
        Object* selectedObject3D = nullptr;

        // Pottery / Rotate / Face Brush — the tools already read these
        // through Engine getters. Chrome lives here so collapsing the
        // console does not invent a second copy on Engine.
        int potteryTool = 1; // 0 Chisel, 1 Expand
        float potteryStrength = 0.2f;
        int rotationAxisMode = 0; // Free XY, X, Y, Z, Authoritative
        float rotationSensitivity = 1.0f;
        float rotationSmoothness = 8.0f;
        int faceBrushType = 0;
        float faceBrushRadius = 0.05f;
        float faceBrushSoftness = 0.3f;
        float faceBrushUOffset = 0.0f;
        float faceBrushVOffset = 0.0f;
        int faceBrushUAxis = 0;
        int faceBrushVAxis = 1;
        bool faceBrushInvertU = false;
        bool faceBrushInvertV = false;
        bool advancedFacePaint = false;
        glm::vec2 lastBrushUV{-1.0f, -1.0f};
        int lastBrushFace = -1;
        Object* lastBrushObject = nullptr;

        // Rotate drag latch (used to live as dummy Engine getters that
        // always returned false, so Rotate never started).
        bool rotateDragging = false;
        double rotateLastCursorX = 0.0;
        double rotateLastCursorY = 0.0;

        // Paint State
        Tool currentTool = Tool(Tool::Type::Brush);
        float currentColor[3] = {1.0f, 1.0f, 1.0f};
        bool use2DPressureSimulation = false;
        bool useLegacy2DTools = false;

        // Character State
        bool characterDesignLocked = false;
        BodyPart* selectedCharacterPart = nullptr;

        // World State
        bool cursorToolsOpen = false;
        bool showLawAuthor = false;
    };

    CreatorConsoleState& getCreatorConsoleState();
    // Drop Object* the tools hold if those beings are no longer in any Zone.
    // loadState replaces every Zone; observation replaces one. Without this,
    // Select/Morph/Combine/Clay keep a pointer into freed memory and switching
    // worlds goes funky. Safe to call after a refused load (live objects stay).
    void forgetStaleObjectHandles(ZoneManager& mgr, Person* player = nullptr);
    
    // Shared styling helpers
    void pushActiveButtonStyle(bool active, const ImVec4& color, const ImVec4& hoverColor);
    void popActiveButtonStyle(bool active);
    void sameLineEvery(int index, int perRow);

} // namespace Rendering
