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

#ifdef USE_GL3_RENDERER
#include "Rendering/GL/GL3Renderer.hpp"
#endif

#include "json.hpp"
#include <array>
#include <memory>

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

    // Mouse state
    bool getMouseLeftPressedLast() const;
    void setMouseLeftPressedLast(bool value);

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
    Object::PolyhedronData buildCurrentPolyhedron() const { return _polyhedron.build(); }

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
    void setSelectedObject3D(Object* obj) { _selectedObject3D = obj; }

    // UI helpers
    bool isMenuOpen() const { return _mainMenu.isOpen(); }
    bool& getShowKeymapRef() { return _showKeymapWindow; }

    // Save / load
    void saveState(const std::string& filename);
    void loadState(const std::string& filename);
    void saveStateWithLog(const std::string& customName = "");
    void updateSaveFiles();
    void drawLoadWindow();
    void drawSaveWindow();
    void drawSaveManager();

private:
    enum class PerspectiveMode { FirstPerson = 0, SecondPerson, ThirdPerson };
    enum class Mode3D { None = -1, FacePaint = 0, FaceBrush, BrushCreate, Pottery, Rotation, Selection };
    enum class ToolTarget3D { WorldObjects = 0, AvatarBodyParts };

    using BrushType = Core::BrushType;

    // Save serialization helper (shared by saveState & saveStateWithLog)
    nlohmann::json buildSaveJson() const;

    // Window handle (cached for input & dimensions)
    GLFWwindow* _window = nullptr;

#ifdef USE_GL3_RENDERER
    GL3Renderer _gl3Renderer;
    bool _gl3Initialized = false;
#endif

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
    Soul          _playerSoul {"Player"};
    Body          _playerBody = Body::createBasicAvatar("Voxel");
    Person        _player {_playerSoul, _playerBody};
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

    // Animation
    float _cubeAngle = 0.0f;

    // Forwarded GLFW callbacks (so ImGui still gets events)
    GLFWcursorposfun       _prevCursorPosCallback       = nullptr;
    GLFWwindowfocusfun     _prevFocusCallback           = nullptr;
    GLFWframebuffersizefun _prevFramebufferSizeCallback = nullptr;

    // Input state
    bool _mouseLeftPressedLast = false;

    // Creator / toolbar
    Tool         _currentTool { Tool::Type::Brush };
    float        _currentColor[3] = {1.0f, 0.9f, 0.2f};
    Mode3D       _current3DMode   = Mode3D::None;
    ToolTarget3D _current3DTarget = ToolTarget3D::WorldObjects;

    // 3D selection
    Object* _selectedObject3D = nullptr;

    // Straight line tool state
    bool  _straightLineMode      = false;
    bool  _drawingStraightLine   = false;
    float _straightLineStartX    = 0.0f;
    float _straightLineStartY    = 0.0f;

    // Layer system (unused externally; kept for future use)
    bool  _useLayers    = false;
    int   _activeLayer  = 0;
    float _layerOpacity = 1.0f;
    int   _blendMode    = 0; // 0=Normal, 1=Multiply, 2=Screen, 3=Overlay, 4=Add, 5=Subtract

    // Internal handlers ---------------------------------------------------
    void onFramebufferSize(int width, int height);
    void renderCreatorToolbar();
};

} // namespace Core
