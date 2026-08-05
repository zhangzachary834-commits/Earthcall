#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "Form/Object/Formation/Menu/Menu.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Ourverse/Ourverse.hpp"
#include "OurVerse/Chat.hpp"
#include "OurVerse/Tool.hpp"
#include "OurVerse/ElementalToolHandler.hpp"
#include "OurVerse/CursorTools.hpp"
#include "Person/Person.hpp"
#include "Person/AvatarManager.hpp"
#include "Person/Soul/Soul.hpp"
#include "Person/Body/Body.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "Perspective/KeyboardHandler.hpp"
#include "Perspective/MouseHandler.hpp"

#include "BrushSettings.hpp"
#include "CameraState.hpp"
#include "CloneToolState.hpp"
#include "FacePaintSettings.hpp"
#include "PlacementState.hpp"
#include "PolyhedronSettings.hpp"
#include "PotteryTool.hpp"
#include "RotationSettings.hpp"
#include "SaveLoadState.hpp"

#include "json.hpp"
#include <array>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>
#include <map>

class Object;

namespace Core {

class Game {
public:
    Game();
    ~Game();
    bool init();
    void update(float dt);
    void render();
    void shutdown();

    void registerCallbacks();

    // GLFW static adapters
    static void sMouseCallback(GLFWwindow* win, double xpos, double ypos);
    static void sWindowFocusCallback(GLFWwindow* win, int focused);
    static void sFramebufferSizeCallback(GLFWwindow* win, int width, int height);

    // Type aliases for external API compatibility ------------------------
    using BrushPlacementMode = Core::BrushPlacementMode;
    using PotteryTool        = Core::PotteryTool;
    using RotationAxisMode   = Core::RotationAxisMode;
    enum class PublicBrushType { Normal=0, Airbrush, Chalk, Spray, Smudge, Clone };

    // Cursor pos
    float getCursorX() const { return _mouseHandler.getCursorX(); }
    float getCursorY() const { return _mouseHandler.getCursorY(); }
    void setCursorX(float x) { _mouseHandler.setCursorX(x); }
    void setCursorY(float y) { _mouseHandler.setCursorY(y); }
    bool isCursorLocked() const { return _mouseHandler.isCursorLocked(); }

    // Rotate-tool drag state (owned here instead of function-local statics in
    // Tool::Rotate3D, so it can't leak across tool/object switches).
    bool   getRotateDragging() const { return _rotateDragging; }
    void   setRotateDragging(bool v) { _rotateDragging = v; }
    double getRotateLastCursorX() const { return _rotateLastCursorX; }
    double getRotateLastCursorY() const { return _rotateLastCursorY; }
    void   setRotateLastCursor(double x, double y) { _rotateLastCursorX = x; _rotateLastCursorY = y; }

    // Camera
    glm::vec3 getCameraPos()   const { return _camera.pos; }
    glm::vec3 getCameraFront() const { return _camera.front; }
    glm::vec3 getCameraUp()    const { return _camera.up; }
    void setCameraPos  (const glm::vec3& v) { _camera.pos   = v; }
    void setCameraFront(const glm::vec3& v) { _camera.front = v; }
    void setCameraUp   (const glm::vec3& v) { _camera.up    = v; }

    const GLint*    getCameraViewport()   const { return _camera.viewport; }
    const GLdouble* getCameraModelview()  const { return _camera.modelview; }
    const GLdouble* getCameraProjection() const { return _camera.projection; }
    void setCameraViewport(const GLint viewport[4]) {
        std::copy(viewport, viewport + 4, _camera.viewport);
    }
    void setCameraModelview(const GLdouble modelview[16]) {
        std::copy(modelview, modelview + 16, _camera.modelview);
    }
    void setCameraProjection(const GLdouble projection[16]) {
        std::copy(projection, projection + 16, _camera.projection);
    }

    // Placement
    void setPlacementMode(BrushPlacementMode mode);
    BrushPlacementMode getPlacementMode() const { return _placement.mode; }
    BrushPlacementMode getPrevPlacementMode() const { return _placement.prevMode; }
    void setPrevPlacementMode(BrushPlacementMode mode) { _placement.prevMode = mode; }

    glm::vec3 getManualOffset() const { return _placement.manualOffset; }
    void setManualOffset(const glm::vec3& v) { _placement.manualOffset = v; }
    bool getManualAnchorValid() const { return _placement.anchorValid; }
    void setManualAnchorValid(bool v) { _placement.anchorValid = v; }
    glm::vec3 getManualAnchorPos()     const { return _placement.anchorPos; }
    void setManualAnchorPos(const glm::vec3& v)     { _placement.anchorPos = v; }
    glm::vec3 getManualAnchorRight()   const { return _placement.anchorRight; }
    void setManualAnchorRight(const glm::vec3& v)   { _placement.anchorRight = v; }
    glm::vec3 getManualAnchorUp()      const { return _placement.anchorUp; }
    void setManualAnchorUp(const glm::vec3& v)      { _placement.anchorUp = v; }
    glm::vec3 getManualAnchorForward() const { return _placement.anchorForward; }
    void setManualAnchorForward(const glm::vec3& v) { _placement.anchorForward = v; }

    // Advanced 2D brush
    bool getAdvanced2DBrush() const;
    void setAdvanced2DBrush(bool value);

    bool getUseLegacy2DTools() const;
    void setUseLegacy2DTools(bool value);

    // Mouse state
    bool getMouseLeftPressedLast() const;
    void setMouseLeftPressedLast(bool value);

    // 2D tool drag state
    void begin2DToolDrag(Tool::Type type, const glm::vec2& position) {
        _toolDragActive = true;
        _toolDragType = type;
        _toolDragStart = position;
        _toolDragCurrent = position;
        _toolDragPoints.clear();
        _toolDragPoints.push_back(position);
    }
    void update2DToolDrag(const glm::vec2& position) {
        _toolDragCurrent = position;
        if (_toolDragActive) {
            _toolDragPoints.push_back(position);
        }
    }
    void end2DToolDrag() { _toolDragActive = false; }
    bool is2DToolDragging(Tool::Type type) const { return _toolDragActive && _toolDragType == type; }
    Tool::Type get2DToolDragType() const { return _toolDragType; }
    glm::vec2 get2DToolDragStart() const { return _toolDragStart; }
    glm::vec2 get2DToolDragCurrent() const { return _toolDragCurrent; }
    const std::vector<glm::vec2>& get2DToolDragPoints() const { return _toolDragPoints; }

    // Current color
    float getCurrentColor(int index) const { return _currentColor[index]; }
    void setCurrentColor(int index, float value) { _currentColor[index] = value; }

    // Brush
    float getBrushSize() const { return _brush.size; }
    void setBrushSize(float size) { _brush.size = size; }

    glm::vec3 getBrushScale() const { return _brush.scale; }
    void setBrushScale(const glm::vec3& v) { _brush.scale = v; }

    glm::vec3 getBrushRotation() const { return _brush.rotation; }
    void setBrushRotation(const glm::vec3& v) { _brush.rotation = v; }
    glm::mat4 buildBrushCreateTransform(const glm::vec3& position) const;
    float getBrushCreateSurfaceOffset(const glm::vec3& normal) const;

    bool getBrushGridSnap() const { return _brush.gridSnap; }
    void setBrushGridSnap(bool v) { _brush.gridSnap = v; }

    float getBrushGridSize() const { return _brush.gridSize; }
    void setBrushGridSize(float v) { _brush.gridSize = v; }

    float getBrushOpacity() const { return _brush.opacity; }
    void setBrushOpacity(float v) { _brush.opacity = v; }

    float getBrushFlow() const { return _brush.flow; }
    void setBrushFlow(float v) { _brush.flow = v; }

    float getBrushSpacing() const { return _brush.spacing; }
    void setBrushSpacing(float v) { _brush.spacing = v; }

    float getCurrentPressure() const { return _brush.currentPressure; }
    void setCurrentPressure(float v) { _brush.currentPressure = v; }

    bool getUsePressureSimulation() const { return _brush.usePressureSimulation; }
    void setUsePressureSimulation(bool v) { _brush.usePressureSimulation = v; }

    float getPressureSensitivity() const { return _brush.pressureSensitivity; }
    void setPressureSensitivity(float v) { _brush.pressureSensitivity = v; }

    float getLastBrushTime() const { return _strokeTracking.lastBrushTime; }
    void setLastBrushTime(float v) { _strokeTracking.lastBrushTime = v; }

    glm::vec2 getBrushCursorPos() const { return _brush.cursorPos; }
    bool getBrushCursorVisible() const { return _brush.cursorVisible; }
    void setBrushCursorPos(const glm::vec2& v) { _brush.cursorPos = v; }
    void setBrushCursorVisible(bool v) { _brush.cursorVisible = v; }

    glm::vec2 getLastBrushUV() const { return _strokeTracking.lastBrushUV; }
    void setLastBrushUV(const glm::vec2& v) { _strokeTracking.lastBrushUV = v; }
    int getLastBrushFace() const { return _strokeTracking.lastBrushFace; }
    void setLastBrushFace(int v) { _strokeTracking.lastBrushFace = v; }
    Object* getLastBrushObject() const { return _strokeTracking.lastBrushObject; }
    void setLastBrushObject(Object* obj) { _strokeTracking.lastBrushObject = obj; }

    bool getUseStrokeInterpolation() const { return _brush.useStrokeInterpolation; }
    void setUseStrokeInterpolation(bool v) { _brush.useStrokeInterpolation = v; }

    PublicBrushType getCurrentBrushType() const {
        return static_cast<PublicBrushType>(_brush.type);
    }

    // Clone tool
    bool getCloneToolActive() const { return _clone.active; }
    void setCloneToolActive(bool v) { _clone.active = v; }
    glm::vec2 getCloneSourceUV() const { return _clone.sourceUV; }
    void setCloneSourceUV(const glm::vec2& v) { _clone.sourceUV = v; }
    glm::vec2 getCloneOffset() const { return _clone.offset; }
    void setCloneOffset(const glm::vec2& v) { _clone.offset = v; }

    // Polyhedron
    Object::GeometryType getCurrentPrimitive() const { return _polyhedron.primitive; }
    void setCurrentPrimitive(Object::GeometryType v) { _polyhedron.primitive = v; }
    Object::ShapeKind getCurrentShapeKind() const { return _polyhedron.shapeKind; }
    void setCurrentShapeKind(Object::ShapeKind v) { _polyhedron.shapeKind = v; }
    const Object::ShapeParams& getCurrentShapeParams() const { return _polyhedron.shapeParams; }

    int getCurrentPolyhedronType() const { return _polyhedron.currentType; }
    void setCurrentPolyhedronType(int v) { _polyhedron.currentType = v; }

    bool getUseCustomPolyhedron() const { return _polyhedron.useCustom; }
    void setUseCustomPolyhedron(bool v) { _polyhedron.useCustom = v; }

    const std::vector<glm::vec3>& getCustomPolyhedronVertices() const { return _polyhedron.customVertices; }
    void setCustomPolyhedronVertices(const std::vector<glm::vec3>& v) { _polyhedron.customVertices = v; }

    const std::vector<std::vector<int>>& getCustomPolyhedronFaces() const { return _polyhedron.customFaces; }
    void setCustomPolyhedronFaces(const std::vector<std::vector<int>>& v) { _polyhedron.customFaces = v; }

    int getCustomPolyhedronVertexCount() const { return _polyhedron.customVertexCount; }
    void setCustomPolyhedronVertexCount(int v) { _polyhedron.customVertexCount = v; }

    int getCustomPolyhedronFaceCount() const { return _polyhedron.customFaceCount; }
    void setCustomPolyhedronFaceCount(int v) { _polyhedron.customFaceCount = v; }

    int getCurrentConcaveType() const { return _polyhedron.concaveType; }
    void setCurrentConcaveType(int v) { _polyhedron.concaveType = v; }

    float getConcavityAmount() const { return _polyhedron.concavityAmount; }
    void setConcavityAmount(float v) { _polyhedron.concavityAmount = v; }

    float getSpikeLength() const { return _polyhedron.spikeLength; }
    void setSpikeLength(float v) { _polyhedron.spikeLength = v; }

    float getCraterDepth() const { return _polyhedron.craterDepth; }
    void setCraterDepth(float v) { _polyhedron.craterDepth = v; }

    int getIrregularType() const { return _polyhedron.irregularType; }
    void setIrregularType(int v) { _polyhedron.irregularType = v; }

    int getIrregularBaseSides() const { return _polyhedron.irregularBaseSides; }
    void setIrregularBaseSides(int v) { _polyhedron.irregularBaseSides = v; }

    float getIrregularHeight() const { return _polyhedron.irregularHeight; }
    void setIrregularHeight(float v) { _polyhedron.irregularHeight = v; }

    float getFrustumTopScale() const { return _polyhedron.frustumTopScale; }
    void setFrustumTopScale(float v) { _polyhedron.frustumTopScale = v; }

    bool getApplyTruncation() const { return _polyhedron.applyTruncation; }
    void setApplyTruncation(bool v) { _polyhedron.applyTruncation = v; }

    float getTruncationAmount() const { return _polyhedron.truncationAmount; }
    void setTruncationAmount(float v) { _polyhedron.truncationAmount = v; }

    bool getApplyDual() const { return _polyhedron.applyDual; }
    void setApplyDual(bool v) { _polyhedron.applyDual = v; }

    // buildCurrentPolyhedron's GitHub implementation lives in GamePolyhedron.cpp.
    // After this merge it will be removed and Game returns the component's build directly.
    PolyhedronData buildCurrentPolyhedron() const { return _polyhedron.build(); }

    // Face brush
    float getFaceBrushRadius()   const { return _faceBrush.radius; }
    void setFaceBrushRadius(float v) { _faceBrush.radius = v; }
    float getFaceBrushSoftness() const { return _faceBrush.softness; }
    void setFaceBrushSoftness(float v) { _faceBrush.softness = v; }
    float getFaceBrushUOffset()  const { return _faceBrush.uOffset; }
    void setFaceBrushUOffset(float v) { _faceBrush.uOffset = v; }
    float getFaceBrushVOffset()  const { return _faceBrush.vOffset; }
    void setFaceBrushVOffset(float v) { _faceBrush.vOffset = v; }
    int  getFaceBrushUAxis()     const { return _faceBrush.uAxis; }
    void setFaceBrushUAxis(int v) { _faceBrush.uAxis = v; }
    int  getFaceBrushVAxis()     const { return _faceBrush.vAxis; }
    void setFaceBrushVAxis(int v) { _faceBrush.vAxis = v; }
    bool getFaceBrushInvertU()   const { return _faceBrush.invertU; }
    void setFaceBrushInvertU(bool v) { _faceBrush.invertU = v; }
    bool getFaceBrushInvertV()   const { return _faceBrush.invertV; }
    void setFaceBrushInvertV(bool v) { _faceBrush.invertV = v; }

    // Advanced face paint
    bool isAdvancedFacePaintEnabled() const { return _advancedFacePaint.enabled; }
    void setAdvancedFacePaintEnabled(bool v) { _advancedFacePaint.enabled = v; }
    bool isAdvancedFacePaintPanelVisible() const { return _advancedFacePaint.panelVisible; }
    void setAdvancedFacePaintPanelVisible(bool v) { _advancedFacePaint.panelVisible = v; }
    AdvancedFacePaint::GradientSettings* getCurrentGradientSettings() { return &_advancedFacePaint.gradient; }
    AdvancedFacePaint::SmudgeSettings*   getCurrentSmudgeSettings()   { return &_advancedFacePaint.smudge; }
    void setCurrentGradientSettings(const AdvancedFacePaint::GradientSettings& s) { _advancedFacePaint.gradient = s; }
    void setCurrentSmudgeSettings  (const AdvancedFacePaint::SmudgeSettings&   s) { _advancedFacePaint.smudge   = s; }

    // Pottery
    PotteryTool getCurrentPotteryTool() const { return _pottery.currentTool; }
    void setCurrentPotteryTool(PotteryTool v) { _pottery.currentTool = v; }
    float getPotteryStrength() const { return _pottery.strength; }
    void setPotteryStrength(float v) { _pottery.strength = v; }

    // Rotation tool
    RotationAxisMode getRotationAxisMode() const { return _rotation.axisMode; }
    void setRotationAxisMode(RotationAxisMode mode) { _rotation.axisMode = mode; }
    float getRotationToolSensitivity() const { return _rotation.sensitivity; }
    void setRotationToolSensitivity(float v) { _rotation.sensitivity = v; }
    float getRotationToolSmoothness() const { return _rotation.smoothness; }
    void setRotationToolSmoothness(float v) { _rotation.smoothness = v; }

    // Handlers
    KeyboardHandler& getKeyboardHandler() { return _keyboardHandler; }
    const KeyboardHandler& getKeyboardHandler() const { return _keyboardHandler; }
    MouseHandler& getMouseHandler() { return _mouseHandler; }
    const MouseHandler& getMouseHandler() const { return _mouseHandler; }
    CursorTools& getCursorTools() { return _cursorTools; }
    const CursorTools& getCursorTools() const { return _cursorTools; }

    // 3D selection
    Object* getSelectedObject3D() const { return _selectedObject3D; }
    void setSelectedObject3D(Object* obj);
    void selectObject3D(Object* obj, bool extendSelection = false);
    void clearSelection3D();
    Formation& getSelectedFormation3D() { return _selectedFormation3D; }
    const Formation& getSelectedFormation3D() const { return _selectedFormation3D; }
    std::unordered_set<std::string> getSelectedObjectIds3D() const;
    void syncSelectedFormationRelations(const Zone& zone);

    // UI helpers
    bool isMenuOpen() const { return _mainMenu.isOpen(); }
    bool& getShowKeymapRef() { return _showKeymapWindow; }

    // Save / load
    void saveState(const std::string& filename);
    void loadState(const std::string& filename);
    
    void setSaveDirectory(const std::string& dir);
    std::string getSaveDirectory() const;
    
    // FlatBuffer delta saves
    std::vector<uint8_t> buildSaveChunkFlatBuffer();
    void loadSaveChunkFlatBuffer(const std::vector<uint8_t>& buffer);
    // Manifesto: "Every Person has a Home they fully own." Idempotent —
    // creates the player's Home zone only if no zone they own exists yet.
    void ensureHomeZone();
    void saveStateWithLog(const std::string& customName = "");
    void updateSaveFiles();
    void drawLoadWindow();
    void drawSaveWindow();
    void drawSaveManager();

private:
    enum class PerspectiveMode { FirstPerson = 0, SecondPerson, ThirdPerson };
    enum class Mode3D { None = -1, FacePaint = 0, FaceBrush, BrushCreate, Pottery, Rotation, Selection, Morph, Combine, Sculpt, Graph };
    enum class ToolTarget3D { WorldObjects = 0, AvatarBodyParts };
    enum class CreatorSection { Paint = 0, Create3D, Character, World, Assets, Relations, Zones };

    using BrushType = Core::BrushType;

    // Save serialization helper (shared by saveState & saveStateWithLog)
    nlohmann::json buildSaveJson() const;

    // Window handle (cached for input & dimensions)
    GLFWwindow* _window = nullptr;

    // Subsystems / components --------------------------------------------
    CameraState         _camera;
    BrushSettings       _brush;
    PolyhedronSettings  _polyhedron;
    FaceBrushSettings   _faceBrush;
    AdvancedFacePaintState _advancedFacePaint;
    PlacementState      _placement;
    PotterySettings     _pottery;
    RotationSettings    _rotation;
    CloneToolState      _clone;
    StrokeTracking      _strokeTracking;
    SaveLoadState       _saveLoad;

    // Perspective & controls
    PerspectiveMode _currentPerspective = PerspectiveMode::FirstPerson;

    // Other systems -------------------------------------------------------
    Menu          _mainMenu;
    Ourverse      _world;
    Chat          _chat;
    // Laws live at engine lifetime (the EventBus has no unsubscribe, so the
    // connected manager must outlive all publishing). connectToEventBus() in
    // init(), tick() at end of update() — laws hear the frame's events.
    LawManager    _lawManager;
    // Accumulated seconds since the world began — the world clock laws read
    // via the reserved "time" paths (set on Universe each frame in update()).
    double        _worldTime = 0.0;
    Person        _player {Soul{"Player"}, Body::createBasicAvatar("Voxel"), "strict"};
    AvatarManager _avatarManager;
    KeyboardHandler _keyboardHandler;
    MouseHandler  _mouseHandler;
    ElementalToolHandler _elementalToolHandler;
    CursorTools   _cursorTools{};

    // UI toggles
    bool _showIntegrationUI = false;
    bool _showChatWindow    = true;
    bool _showAvatarDemo    = false;
    bool _showKeymapWindow  = false;
    bool _showToolbar       = true;
    bool _showDebugCoordinates = false;

    // Animation
    float _cubeAngle = 0.0f;

    // Player movement controller -----------------------------------------
    // A single authoritative per-frame resolve owns _camera.pos. Gravity,
    // jump and ground/support contact are integrated here; object collisions
    // only push the player horizontally. Body parts are posed *from* the
    // resolved position and never push the camera back (that competition was
    // the source of the ground/cube jitter).
    void  stepMovement(float dt);
    float _playerVelY        = 0.0f;  // vertical velocity (world units/sec)
    bool  _playerGrounded    = false; // resting on ground/support this frame
    bool  _jumpKeyDownLast   = false; // edge-trigger for jump
    bool  _playerWasMoving   = false; // previous frame's locomotion state, for the locomotion-started/-stopped edge

    // Rotate-tool drag state (see accessors above)
    bool   _rotateDragging    = false;
    double _rotateLastCursorX = 0.0;
    double _rotateLastCursorY = 0.0;

    // Forwarded GLFW callbacks (so ImGui still gets events)
    GLFWcursorposfun       _prevCursorPosCallback       = nullptr;
    GLFWwindowfocusfun     _prevFocusCallback           = nullptr;
    GLFWframebuffersizefun _prevFramebufferSizeCallback = nullptr;

    // Input state
    bool _mouseLeftPressedLast = false;
    bool _useLegacy2DTools = false;

    // Creator / toolbar
    Tool         _currentTool { Tool::Type::Brush };
    float        _currentColor[3] = {1.0f, 0.9f, 0.2f};
    Mode3D       _current3DMode   = Mode3D::None;
    ToolTarget3D _current3DTarget = ToolTarget3D::WorldObjects;
    CreatorSection _creatorSection = CreatorSection::Create3D;
    BodyPart* _selectedCharacterPart = nullptr;
    bool _showRelationManager = false;
    bool _showLawAuthor = false;
    bool _showCreationConsole = false;
    bool _characterDesignLocked = false;
    bool _use2DPressureSimulation = false;
    bool _cursorToolsOpen = false;

    // 3D selection
    Object* _selectedObject3D = nullptr;
    Formation _selectedFormation3D { Form::ShapeType::Cube, glm::vec3(1.0f) };
    int _morphVertexIndex = -1;   // selected vertex for the polyhedron Morph tool
    bool _fieldHandleDragging = false; // dragging a binary-field operand handle
    bool _blendHandleDragging = false; // dragging the floating blend/smoothness bead
    int _patchCtrlIndex = -1;     // selected control point for the Bezier patch Morph tool

    // Combine tool (in-scene boolean/blend): pick shape A, then shape B; the
    // result (A op B) replaces A in place and B is consumed into it.
    Object* _combineOperandA = nullptr; // first-picked operand, awaiting B
    int   _combineOp = 2;               // 0 Union, 1 Intersect, 2 Subtract, 3 SmoothUnion, 4 Blend(Morph)
    float _combineBlend = 0.15f;        // smoothness (SmoothUnion) / t (Blend)

    // Clay (Sculpt) tool: grab a shape, drag it; release while overlapping
    // another shape to fuse them (target op dragged), using _combineOp/_combineBlend.
    Object* _clayGrabbed = nullptr;     // shape currently being dragged
    Object* _clayTarget  = nullptr;     // overlap candidate to fuse into on release

    // Fuse B into A in place (A becomes A op B as an SDF field), consuming B.
    // Shared by the Combine (click A, click B) and Clay (drag-to-overlap) tools.
    void fuseObjects(Object* A, Object* B);

    // Floating blend/smoothness bead geometry for a binary field: a screen-aligned
    // rail above the shape; the bead position along it encodes the blend t in [0,1].
    void blendRail(const Object* o, glm::vec3& start, glm::vec3& dir, float& length) const;

    // In-scene field-refinement gizmos for a binary field (operand-B drag handle +
    // blend bead). Hit-tested/dragged here so Morph, Combine and Clay all share one
    // path. Returns true if a handle captured this frame's left-drag (the caller
    // should then NOT also pick/grab).
    bool handleFieldGizmos(Object* o, bool pressEdge, bool mouseDown, double winX, double winY);

    // Node-graph tool (Mode3D::Graph): the selected field's SdfNode tree rendered as
    // floating cards beside it in the scene; click a node to select, edit/restructure
    // it in the node panel. _graphSelPath is the child-index path from the root
    // (empty = root). Defined in GameNodeGraph.cpp.
    std::vector<int> _graphSelPath;     // path to the selected node (child indices)
    bool _graphHasSel = false;          // whether a node is selected
    bool _graphDragging = false;        // dragging a node to swap subtrees
    glm::vec2 _graphSelScreen{0.0f};    // selected card's screen pos (for the node panel)
    std::vector<int> _graphPanelPath;   // node the editor panel is currently placed for
    bool _graphScrubbing = false;       // dragging a card's blend strip to set t
    std::vector<int> _graphCtxPath;      // node the right-click context menu targets
    std::map<std::vector<int>, glm::vec2> _graphManualOffset; // hand-dragged card offsets (path -> screen delta)
    void renderNodeGraph();             // draws the floating graph + handles selection
    void renderNodePanel();             // edit panel for the selected node

    // Straight line tool state
    bool  _straightLineMode      = false;
    bool  _drawingStraightLine   = false;
    float _straightLineStartX    = 0.0f;
    float _straightLineStartY    = 0.0f;
    float _straightLineEndX      = 0.0f;
    float _straightLineEndY      = 0.0f;

    // Shared 2D tool drag state
    bool _toolDragActive = false;
    Tool::Type _toolDragType = Tool::Type::Brush;
    glm::vec2 _toolDragStart = glm::vec2(0.0f);
    glm::vec2 _toolDragCurrent = glm::vec2(0.0f);
    std::vector<glm::vec2> _toolDragPoints;

    // Layer system (unused externally; kept for future use)
    bool  _useLayers    = false;
    int   _activeLayer  = 0;
    float _layerOpacity = 1.0f;
    int   _blendMode    = 0; // 0=Normal, 1=Multiply, 2=Screen, 3=Overlay, 4=Add, 5=Subtract

    // Internal handlers ---------------------------------------------------
    void onFramebufferSize(int width, int height);
    void renderCreatorToolbar();
    void renderCreatorSectionTabs();
    void renderPaintConsole(Zone& zone);
    void render3DConsole();
    void renderCharacterConsole();
    void renderWorldConsole();
    void renderAssetsConsole(Zone& zone);
    void renderRelationsConsole(Zone& zone);
    void renderZonesConsole(ZoneManager& zoneMgr);
    void renderCreatorStatusBar();
    void renderSectionButton(CreatorSection section, const char* label);
    void renderPaintToolButton(Zone& zone, Tool::Type type, const char* label);
    void render3DModeButton(Mode3D mode, const char* label);
    void renderPrimitiveButton(Object::ShapeKind kind, const char* label);
    void renderPlacementInspector();
    void renderSelectionInspector();
    void setPaintTool(Zone& zone, Tool::Type type);
    void set3DMode(Mode3D mode);
};

} // namespace Core
