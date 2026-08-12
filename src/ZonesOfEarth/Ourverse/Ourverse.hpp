#pragma once
#include <vector>
#include <string>
#include <memory>
#include <unordered_set>
#include <map>

#include "../Zone/Zone.hpp"
#include "../HomesOfEarth/Home.hpp"
#include "Relation/Relation.hpp"
#include "ConstructedBeing/Object/Object.hpp"
#include "ConstructedBeing/Object/Formation/Formation.hpp"
#include "Person/Body/BodyPart/BodyPart.hpp"
#include "../Physics/Physics.hpp"
#include "ConstructedBeing/Singular/Singular.hpp"

class Ourverse : public Singular {
public:


    std::vector<Zone> zones;
    std::vector<Home> homes;
    std::vector<std::shared_ptr<Relation>> relations;

    void addZone(Zone zone);
    void addHome(Home home);
    // void relate(Relation relation);
    void relate(const std::shared_ptr<Relation>& relation);
    void display() const;
    void renderModeUI();

    void updateObjectCollisions(glm::vec3& position, const Object& obj, const glm::mat4& transform) const;

    void onUpdate(float deltaTime = 0.016f);

    void setCamera(glm::vec3* cam) { cameraPos = cam; }
    glm::vec3* getCamera() const { return cameraPos; }

    void addOwnedObject(std::shared_ptr<Object> obj) { ownedObjects.push_back(std::move(obj)); }
    const std::vector<std::shared_ptr<Object>>& getOwnedObjects() const { return ownedObjects; }

    // Mutable access (use with caution)
    std::vector<std::shared_ptr<Object>>& getOwnedObjectsMutable() { return ownedObjects; }

    // Remove all objects spawned dynamically (keep baseline 0 and 1)
    void clearDynamicObjects();

    // Singular interface
    std::string getIdentifier() const override { return "Ourverse"; }

    // --- UI/Creator State ---
    enum class PerspectiveMode { FirstPerson = 0, SecondPerson, ThirdPerson };
    enum class Mode3D { None = -1, FacePaint = 0, FaceBrush, BrushCreate, Pottery, Rotation, Selection, Morph, Combine, Sculpt, Graph };
    enum class ToolTarget3D { WorldObjects = 0 };
    enum class CreatorSection { Paint = 0, Create3D, Character, World, Assets, Relations, Zones };

    PerspectiveMode currentPerspective = PerspectiveMode::FirstPerson;
    
    // UI toggles
    bool showIntegrationUI = false;
    bool showChatWindow    = true;
    bool showKeymapWindow  = false;
    bool showToolbar       = true;
    bool showDebugCoordinates = false;
    bool showRelationManager = false;
    bool showLawAuthor = false;
    bool showCreationConsole = false;
    bool characterDesignLocked = false;
    bool use2DPressureSimulation = false;
    bool cursorToolsOpen = false;

    // Creator / toolbar
    std::string  currentToolId = "category.tool.brush";
    float        currentColor[3] = {1.0f, 0.9f, 0.2f};
    Mode3D       current3DMode   = Mode3D::None;
    ToolTarget3D current3DTarget = ToolTarget3D::WorldObjects;
    CreatorSection creatorSection = CreatorSection::Create3D;
    BodyPart* selectedCharacterPart = nullptr;
    
    // 3D selection
    Object* selectedObject3D = nullptr;
    Formation selectedFormation3D;
    int morphVertexIndex = -1;
    bool fieldHandleDragging = false;
    bool blendHandleDragging = false;
    int patchCtrlIndex = -1;

    // Combine/Sculpt tool state
    Object* combineOperandA = nullptr;
    int   combineOp = 2;
    float combineBlend = 0.15f;
    Object* clayGrabbed = nullptr;
    Object* clayTarget  = nullptr;

    // Node graph state
    std::vector<int> graphSelPath;
    bool graphHasSel = false;
    bool graphDragging = false;
    glm::vec2 graphSelScreen{0.0f};
    std::vector<int> graphPanelPath;
    bool graphScrubbing = false;
    std::vector<int> graphCtxPath;
    std::map<std::vector<int>, glm::vec2> graphManualOffset;

    // Straight line tool state
    bool  straightLineMode      = false;
    bool  drawingStraightLine   = false;
    float straightLineStartX    = 0.0f;
    float straightLineStartY    = 0.0f;
    float straightLineEndX      = 0.0f;
    float straightLineEndY      = 0.0f;

    // Shared 2D tool drag state
    bool toolDragActive = false;
    int toolDragType = 0; // Tool::Type is not available here, use int
    glm::vec2 toolDragStart = glm::vec2(0.0f);
    glm::vec2 toolDragCurrent = glm::vec2(0.0f);
    std::vector<glm::vec2> toolDragPoints;

    void setSelectedObject3D(Object* obj);
    void selectObject3D(Object* obj, bool extendSelection = false);
    void clearSelection3D();
    std::unordered_set<std::string> getSelectedObjectIds3D() const;
    void syncSelectedFormationRelations(const Zone& zone);

    // UI methods
    void renderCreatorToolbar();
    void renderCreatorSectionTabs();
    void renderPaintConsole(Zone& zone);
    void render3DConsole();
    void renderCharacterConsole();
    void renderWorldConsole();
    void renderAssetsConsole(Zone& zone);
    void renderRelationsConsole(ZoneManager& zoneMgr);
    void renderCreatorStatusBar();
    void renderSectionButton(CreatorSection section, const char* label);
    void render3DModeButton(Mode3D mode, const char* label);
    void renderPlacementInspector();
    void renderSelectionInspector();
    void set3DMode(Mode3D mode);

    // Graph UI
    void renderNodeGraph();
    void renderNodePanel();

private:
    void buildProperties() override {}

    glm::vec3* cameraPos = nullptr;
    std::vector<std::shared_ptr<Object>> ownedObjects;
};

struct InteractionEvent {
    std::string description;
    std::time_t timestamp;
    Object* other;
    // ... further relational or symbolic data
};
