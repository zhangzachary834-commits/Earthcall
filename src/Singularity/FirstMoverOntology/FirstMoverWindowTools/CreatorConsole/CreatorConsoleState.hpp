#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <memory>
#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/Tool.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
class BodyPart;

#include <imgui.h>

class ZoneManager;
class Person;

namespace Rendering {

    enum class CreatorSection {
        Paint,
        Create3D,
        Concepts,
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

    enum class HistoryActionType {
        Spawn,
        Delete
    };

    struct HistoryRecord {
        HistoryActionType type;
        std::shared_ptr<Object> object;
        std::string description;
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
            if (customVertexCount < 3) customVertexCount = 3;
            for (int i = 0; i < customVertexCount; ++i) {
                float theta = 2.0f * 3.14159265f * static_cast<float>(i) / static_cast<float>(customVertexCount);
                float y = (i % 2 == 0) ? 0.5f : -0.5f;
                customVertices.push_back(glm::vec3(std::cos(theta) * 0.5f, y, std::sin(theta) * 0.5f));
            }
        }
    };

    struct CreatorConsoleState {
        CreatorSection currentSection = CreatorSection::Create3D;
        
        // 3D Create State
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
        Object* combineOperandA = nullptr;
        Object* clayGrabbed = nullptr;
        Object* clayTarget = nullptr;

        // Morph State
        int morphVertexIndex = -1;
        int patchCtrlIndex = -1;
        bool fieldHandleDragging = false;
        bool blendHandleDragging = false;

        // Live 3D selection (Select mode)
        Object* selectedObject3D = nullptr;

        // Pottery / Rotate / Face Brush
        int potteryTool = 1; // 0 Chisel, 1 Expand
        float potteryStrength = 0.2f;
        int rotationAxisMode = 0; // Free XY, X, Y, Z, Authoritative
        float rotationSensitivity = 1.0f;
        float rotationSmoothness = 8.0f;
        int faceBrushType = 0;
        float faceBrushRadius = 0.05f;
        float faceBrushSoftness = 0.3f;
        float faceBrushOpacity = 1.0f;
        float faceBrushFlow = 1.0f;
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

        // Rotate drag latch
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

        // 3D History Stack (Undo / Redo)
        std::vector<HistoryRecord> undoStack;
        std::vector<HistoryRecord> redoStack;

        void recordSpawn(std::shared_ptr<Object> obj, const std::string& desc = "Spawn Object") {
            if (!obj) return;
            undoStack.push_back({HistoryActionType::Spawn, std::move(obj), desc});
            redoStack.clear();
        }

        void recordDelete(std::shared_ptr<Object> obj, const std::string& desc = "Delete Object") {
            if (!obj) return;
            undoStack.push_back({HistoryActionType::Delete, std::move(obj), desc});
            redoStack.clear();
        }

        bool canUndo() const { return !undoStack.empty(); }
        bool canRedo() const { return !redoStack.empty(); }

        std::string nextUndoDesc() const {
            return undoStack.empty() ? "" : undoStack.back().description;
        }
        std::string nextRedoDesc() const {
            return redoStack.empty() ? "" : redoStack.back().description;
        }

        void performUndo(ZoneManager& zoneMgr);
        void performRedo(ZoneManager& zoneMgr);
    };

    CreatorConsoleState& getCreatorConsoleState();
    void forgetStaleObjectHandles(ZoneManager& mgr, Person* player = nullptr);
    
    // Shared styling helpers
    void pushActiveButtonStyle(bool active, const ImVec4& color, const ImVec4& hoverColor);
    void popActiveButtonStyle(bool active);
    void sameLineEvery(int index, int perRow);

    // Responsive grid helpers for resizable dockable sidebars
    float responsiveItemWidth(int columns, float minWidth = 60.0f);
    void responsiveSameLine(int index, int columns);

} // namespace Rendering
