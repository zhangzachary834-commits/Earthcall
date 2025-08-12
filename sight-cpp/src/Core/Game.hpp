#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "Form/Object/Formation/Menu/Menu.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Ourverse/Ourverse.hpp"
#include "OurVerse/Chat.hpp"
#include <memory>
#include <array>
#include "OurVerse/Tool.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include "json.hpp"
#include <fstream>
#include "Person/Person.hpp"
#include "Person/AvatarManager.hpp"
#include "Person/Soul/Soul.hpp"
#include "Person/Body/Body.hpp"
#include "Perspective/KeyboardHandler.hpp"
#include "Perspective/MouseHandler.hpp"
#include "OurVerse/ElementalToolHandler.hpp"
#include "OurVerse/CursorTools.hpp"
#include "OurVerse/AdvancedFacePaint.hpp"

// Optional modern GL3 renderer prototype
#ifdef USE_GL3_RENDERER
#include "Rendering/GL/GL3Renderer.hpp"
#endif

// forward declare
namespace Core { class Game; }

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

    // Must be called after window exists to hook callbacks
    void registerCallbacks();

    // GLFW static adapters
    static void sMouseCallback(GLFWwindow* win, double xpos, double ypos);
    static void sWindowFocusCallback(GLFWwindow* win, int focused);
    static void sFramebufferSizeCallback(GLFWwindow* win, int width, int height);

    // Getters and Setters

    // Cursor Pos

    float getCursorX() const { return _mouseHandler.getCursorX(); }
    float getCursorY() const { return _mouseHandler.getCursorY(); }
    void setCursorX(float x) { _mouseHandler.setCursorX(x); }
    void setCursorY(float y) { _mouseHandler.setCursorY(y); }

    // Camera 
    glm::vec3 getCameraPos() const { return _cameraPos; }
    glm::vec3 getCameraFront() const { return _cameraFront; }
    glm::vec3 getCameraUp() const { return _cameraUp; }
    void setCameraPos(const glm::vec3& pos) { _cameraPos = pos; }
    void setCameraFront(const glm::vec3& front) { _cameraFront = front; }
    void setCameraUp(const glm::vec3& up) { _cameraUp = up; }
    
    // Camera viewport
    const GLint* getCameraViewport() const { return _cameraViewport; }
    void setCameraViewport(const GLint viewport[4]) { 
        _cameraViewport[0] = viewport[0]; 
        _cameraViewport[1] = viewport[1]; 
        _cameraViewport[2] = viewport[2]; 
        _cameraViewport[3] = viewport[3]; 
    }
    
    // Camera matrices
    const GLdouble* getCameraModelview() const { return _cameraModelview; }
    void setCameraModelview(const GLdouble modelview[16]) { 
        for(int i = 0; i < 16; ++i) _cameraModelview[i] = modelview[i]; 
    }
    
    const GLdouble* getCameraProjection() const { return _cameraProjection; }
    void setCameraProjection(const GLdouble projection[16]) { 
        for(int i = 0; i < 16; ++i) _cameraProjection[i] = projection[i]; 
    }

    // Placement Mode
    enum class BrushPlacementMode { InFront = 0, ManualDistance, CursorSnap };
    void setPlacementMode(BrushPlacementMode mode);
    BrushPlacementMode getPlacementMode() const { return _placementMode; }

    // Advanced 2D Brush
    bool getAdvanced2DBrush() const;
    void setAdvanced2DBrush(bool value);

    // Mouse state
    bool getMouseLeftPressedLast() const;
    void setMouseLeftPressedLast(bool value);

    // Manual placement
    glm::vec3 getManualOffset() const { return _manualOffset; }
    void setManualOffset(const glm::vec3& offset) { _manualOffset = offset; }
    
    bool getManualAnchorValid() const { return _manualAnchorValid; }
    void setManualAnchorValid(bool valid) { _manualAnchorValid = valid; }
    
    glm::vec3 getManualAnchorPos() const { return _manualAnchorPos; }
    void setManualAnchorPos(const glm::vec3& pos) { _manualAnchorPos = pos; }
    
    glm::vec3 getManualAnchorRight() const { return _manualAnchorRight; }
    void setManualAnchorRight(const glm::vec3& right) { _manualAnchorRight = right; }
    
    glm::vec3 getManualAnchorUp() const { return _manualAnchorUp; }
    void setManualAnchorUp(const glm::vec3& up) { _manualAnchorUp = up; }
    
    glm::vec3 getManualAnchorForward() const { return _manualAnchorForward; }
    void setManualAnchorForward(const glm::vec3& forward) { _manualAnchorForward = forward; }
    
    BrushPlacementMode getPrevPlacementMode() const { return _prevPlacementMode; }
    void setPrevPlacementMode(BrushPlacementMode mode) { _prevPlacementMode = mode; }

    // Current Color
    float getCurrentColor(int index) const { return _currentColor[index]; }
    void setCurrentColor(int index, float value) { _currentColor[index] = value; }

    // Brush settings getters/setters
    float getBrushSize() const { return _brushSize; }
    void setBrushSize(float size) { _brushSize = size; }
    
    glm::vec3 getBrushScale() const { return _brushScale; }
    void setBrushScale(const glm::vec3& scale) { _brushScale = scale; }
    
    glm::vec3 getBrushRotation() const { return _brushRotation; }
    void setBrushRotation(const glm::vec3& rotation) { _brushRotation = rotation; }
    
    bool getBrushGridSnap() const { return _brushGridSnap; }
    void setBrushGridSnap(bool snap) { _brushGridSnap = snap; }
    
    float getBrushGridSize() const { return _brushGridSize; }
    void setBrushGridSize(float size) { _brushGridSize = size; }
    
    // Current primitive getter/setter
    Object::GeometryType getCurrentPrimitive() const { return _currentPrimitive; }
    void setCurrentPrimitive(Object::GeometryType primitive) { _currentPrimitive = primitive; }
    
    // Polyhedron type getters/setters
    int getCurrentPolyhedronType() const { return _currentPolyhedronType; }
    void setCurrentPolyhedronType(int type) { _currentPolyhedronType = type; }
    
    // Custom polyhedron getters/setters
    bool getUseCustomPolyhedron() const { return _useCustomPolyhedron; }
    void setUseCustomPolyhedron(bool use) { _useCustomPolyhedron = use; }
    
    const std::vector<glm::vec3>& getCustomPolyhedronVertices() const { return _customPolyhedronVertices; }
    void setCustomPolyhedronVertices(const std::vector<glm::vec3>& vertices) { _customPolyhedronVertices = vertices; }
    
    const std::vector<std::vector<int>>& getCustomPolyhedronFaces() const { return _customPolyhedronFaces; }
    void setCustomPolyhedronFaces(const std::vector<std::vector<int>>& faces) { _customPolyhedronFaces = faces; }
    
    int getCustomPolyhedronVertexCount() const { return _customPolyhedronVertexCount; }
    void setCustomPolyhedronVertexCount(int count) { _customPolyhedronVertexCount = count; }
    
    int getCustomPolyhedronFaceCount() const { return _customPolyhedronFaceCount; }
    void setCustomPolyhedronFaceCount(int count) { _customPolyhedronFaceCount = count; }
    
    // Convex/concave polyhedron getters/setters
    int getCurrentConcaveType() const { return _currentConcaveType; }
    void setCurrentConcaveType(int type) { _currentConcaveType = type; }
    
    float getConcavityAmount() const { return _concavityAmount; }
    void setConcavityAmount(float amount) { _concavityAmount = amount; }
    
    float getSpikeLength() const { return _spikeLength; }
    void setSpikeLength(float length) { _spikeLength = length; }
    
    float getCraterDepth() const { return _craterDepth; }
    void setCraterDepth(float depth) { _craterDepth = depth; }
    
    // Face brush getters/setters
    float getFaceBrushRadius() const { return _faceBrushRadius; }
    void setFaceBrushRadius(float radius) { _faceBrushRadius = radius; }
    
    float getFaceBrushSoftness() const { return _faceBrushSoftness; }
    void setFaceBrushSoftness(float softness) { _faceBrushSoftness = softness; }
    
    float getFaceBrushUOffset() const { return _faceBrushUOffset; }
    void setFaceBrushUOffset(float offset) { _faceBrushUOffset = offset; }
    
    float getFaceBrushVOffset() const { return _faceBrushVOffset; }
    void setFaceBrushVOffset(float offset) { _faceBrushVOffset = offset; }
    
    int getFaceBrushUAxis() const { return _faceBrushUAxis; }
    void setFaceBrushUAxis(int axis) { _faceBrushUAxis = axis; }
    
    int getFaceBrushVAxis() const { return _faceBrushVAxis; }
    void setFaceBrushVAxis(int axis) { _faceBrushVAxis = axis; }
    
    bool getFaceBrushInvertU() const { return _faceBrushInvertU; }
    void setFaceBrushInvertU(bool invert) { _faceBrushInvertU = invert; }
    
    bool getFaceBrushInvertV() const { return _faceBrushInvertV; }
    void setFaceBrushInvertV(bool invert) { _faceBrushInvertV = invert; }
    
    // Pottery tool getters/setters
    enum class PotteryTool { Chisel = 0, Expand };
    PotteryTool getCurrentPotteryTool() const { return _currentPotteryTool; }
    void setCurrentPotteryTool(PotteryTool tool) { _currentPotteryTool = tool; }
    
    float getPotteryStrength() const { return _potteryStrength; }
    void setPotteryStrength(float strength) { _potteryStrength = strength; }
    
    // Keyboard handler access
    KeyboardHandler& getKeyboardHandler() { return _keyboardHandler; }
    const KeyboardHandler& getKeyboardHandler() const { return _keyboardHandler; }
    
    // Mouse handler access
    MouseHandler& getMouseHandler() { return _mouseHandler; }
    const MouseHandler& getMouseHandler() const { return _mouseHandler; }
    
    // Active zone getter
    // Zone& getActiveZone() { return mgr.active(); } 

    // Zone management
    // Need Zone menu to be added to the window to be able to select zones. 
    // Need to add a way to select, add or remove, zones in the world, add worlds, 2D screens, Formations, relations, homes, objects to zones, etc.

    // Duplicated accidental getters (kept above); removing redundant public aliases

    // Brush Pressure
    float getCurrentPressure() const { return _currentPressure; }
    void setCurrentPressure(float pressure) { _currentPressure = pressure; }
    bool getUsePressureSimulation() const { return _usePressureSimulation; }
    void setUsePressureSimulation(bool use) { _usePressureSimulation = use; }
    float getPressureSensitivity() const { return _pressureSensitivity; }
    void setPressureSensitivity(float sensitivity) { _pressureSensitivity = sensitivity; }
    float getLastBrushTime() const { return _lastBrushTime; }
    void setLastBrushTime(float t) { _lastBrushTime = t; }

    glm::vec2 getBrushCursorPos() const { return _brushCursorPos; }
    bool getBrushCursorVisible() const { return _brushCursorVisible; }
    void setBrushCursorPos(const glm::vec2& pos) { _brushCursorPos = pos; }
    void setBrushCursorVisible(bool visible) { _brushCursorVisible = visible; }

    // Cursor tools
    CursorTools& getCursorTools() { return _cursorTools; }
    const CursorTools& getCursorTools() const { return _cursorTools; }

    glm::vec2 getLastBrushUV() const { return _lastBrushUV; }
    void setLastBrushUV(const glm::vec2& uv) { _lastBrushUV = uv; }

    // Track last painted face/object to avoid cross-face interpolation artifacts
    int getLastBrushFace() const { return _lastBrushFace; }
    void setLastBrushFace(int face) { _lastBrushFace = face; }
    Object* getLastBrushObject() const { return _lastBrushObject; }
    void setLastBrushObject(Object* obj) { _lastBrushObject = obj; }

    // Clone tool
    bool getCloneToolActive() const { return _cloneToolActive; }
    void setCloneToolActive(bool active) { _cloneToolActive = active; }
    glm::vec2 getCloneSourceUV() const { return _cloneSourceUV; }
    void setCloneSourceUV(const glm::vec2& uv) { _cloneSourceUV = uv; }
    glm::vec2 getCloneOffset() const { return _cloneOffset; }
    void setCloneOffset(const glm::vec2& offset) { _cloneOffset = offset; }

    // Stroke interpolation
    bool getUseStrokeInterpolation() const { return _useStrokeInterpolation; }
    void setUseStrokeInterpolation(bool use) { _useStrokeInterpolation = use; }

    // Brush opacity
    float getBrushOpacity() const { return _brushOpacity; }
    void setBrushOpacity(float opacity) { _brushOpacity = opacity; }

    // Brush flow
    float getBrushFlow() const { return _brushFlow; }
    void setBrushFlow(float flow) { _brushFlow = flow; }

    // Advanced Face Paint System
    bool _useAdvancedFacePaint = false;
    bool _showAdvancedFacePaintPanel = false;
    
    // Advanced face paint settings
    AdvancedFacePaint::GradientSettings _currentGradientSettings;
    AdvancedFacePaint::SmudgeSettings _currentSmudgeSettings;
    
    // Advanced face paint getters/setters
    bool isAdvancedFacePaintEnabled() const { return _useAdvancedFacePaint; }
    void setAdvancedFacePaintEnabled(bool enabled) { _useAdvancedFacePaint = enabled; }
    
    bool isAdvancedFacePaintPanelVisible() const { return _showAdvancedFacePaintPanel; }
    void setAdvancedFacePaintPanelVisible(bool visible) { _showAdvancedFacePaintPanel = visible; }
    
    AdvancedFacePaint::GradientSettings* getCurrentGradientSettings() { return &_currentGradientSettings; }
    AdvancedFacePaint::SmudgeSettings* getCurrentSmudgeSettings() { return &_currentSmudgeSettings; }
    
    void setCurrentGradientSettings(const AdvancedFacePaint::GradientSettings& settings) { _currentGradientSettings = settings; }
    void setCurrentSmudgeSettings(const AdvancedFacePaint::SmudgeSettings& settings) { _currentSmudgeSettings = settings; }

    // For 3D face brush type switching (reuse BrushType defined in Game)
    enum class PublicBrushType { Normal=0, Airbrush, Chalk, Spray, Smudge, Clone };
    PublicBrushType getCurrentBrushType() const { return static_cast<PublicBrushType>(_currentBrushType); }

    // Brush spacing
    float getBrushSpacing() const { return _brushSpacing; }
    void setBrushSpacing(float spacing) { _brushSpacing = spacing; }

    // Brush density

private:
    enum class PerspectiveMode { FirstPerson = 0, SecondPerson, ThirdPerson };

    // Window handle (cached for input & dimensions)
    GLFWwindow* _window = nullptr;

#ifdef USE_GL3_RENDERER
    // Minimal GL3 triangle renderer to validate modern pipeline
    GL3Renderer _gl3Renderer;
    bool _gl3Initialized = false;
#endif

    // Camera --------------------------------------------------------------
    glm::vec3 _cameraPos   {0.0f, 0.0f, 3.0f};
    glm::vec3 _cameraFront {0.0f, 0.0f, -1.0f};
    glm::vec3 _cameraUp    {0.0f, 1.0f, 0.0f};

    float _cameraSpeed = 0.1f;
    // Camera rotation now handled by MouseHandler

    // Perspective & controls
    PerspectiveMode _currentPerspective = PerspectiveMode::FirstPerson;

    // Systems -------------------------------------------------------------
    Menu        _mainMenu;
    // Zone management handled by global ::mgr for now.
    Ourverse    _world;
    Chat        _chat;
    Soul        _playerSoul {"Player"};  // Soul for the player
    Body        _playerBody = Body::createBasicAvatar("Voxel");  // Body for the player
    Person      _player {_playerSoul, _playerBody};
    AvatarManager _avatarManager;  // New avatar management system
    KeyboardHandler _keyboardHandler;  // Keyboard input management
    MouseHandler _mouseHandler;  // Mouse input management
    ElementalToolHandler _elementalToolHandler;  // Elemental tool management
    CursorTools _cursorTools{};
    
    // Integration System
    bool _showIntegrationUI = false;

    bool _showChatWindow = true;
    bool _showAvatarDemo = false;  // Toggle for avatar demo
    bool _showKeymapWindow = false; // Toggle for keymap window

    // Toggle states - now handled by KeyboardHandler

    // Animation helpers
    float _cubeAngle = 0.0f;

    // Camera matrices for picking
    GLdouble _cameraModelview[16];
    GLdouble _cameraProjection[16];
    GLint    _cameraViewport[4] = {0,0,0,0};

    // Previous GLFW callbacks to forward events to ImGui (prevents toolbar freeze)
    GLFWcursorposfun      _prevCursorPosCallback = nullptr;
    GLFWwindowfocusfun    _prevFocusCallback     = nullptr;
    GLFWframebuffersizefun _prevFramebufferSizeCallback = nullptr;
    //GLFWmousebuttonfun    _prevMouseButtonCallback = nullptr;
    //GLFWscrollfun         _prevScrollCallback = nullptr;

    // Input state ---------------------------------------------------------
    bool _mouseLeftPressedLast = false;  // Still needed for mouse handling

    // Internal handlers ---------------------------------------------------
    void onFramebufferSize(int width, int height);

    enum class Mode3D { None = -1, FacePaint = 0, FaceBrush, BrushCreate, Pottery, Selection };

    // Creator/toolbar -----------------------------------------------------

    // Placement control for 3-D Brush ------------------------------------
    BrushPlacementMode _placementMode = BrushPlacementMode::InFront;
    glm::vec3          _manualOffset {0.0f, 0.0f, 2.0f}; // x (right), y (up), z (forward)
    bool               _manualAnchorValid = false;
    glm::vec3          _manualAnchorPos;
    glm::vec3          _manualAnchorRight, _manualAnchorUp, _manualAnchorForward;
    BrushPlacementMode _prevPlacementMode = BrushPlacementMode::InFront;

    bool _showToolbar = true;
    Tool _currentTool { Tool::Type::Brush };
    float _currentColor[3] = {1.0f, 0.9f, 0.2f};
    Mode3D _current3DMode = Mode3D::None;

    void renderCreatorToolbar();

    // Advanced primitive brush settings (subset)
    Object::GeometryType _currentPrimitive = Object::GeometryType::Cube;
    int _currentPolyhedronType = 4; // Default to tetrahedron
    
    // Custom polyhedron generation
    bool _useCustomPolyhedron = false;
    std::vector<glm::vec3> _customPolyhedronVertices;
    std::vector<std::vector<int>> _customPolyhedronFaces;
    int _customPolyhedronVertexCount = 4;
    int _customPolyhedronFaceCount = 4;
    
    // Convex/concave polyhedron variables
    int _currentConcaveType = 0;  // 0=Regular, 1=Concave, 2=Star, 3=Crater
    float _concavityAmount = 0.3f;
    float _spikeLength = 0.3f;
    float _craterDepth = 0.2f;
    
    float _brushSize = 1.0f;
    glm::vec3 _brushScale {1.0f};
    glm::vec3 _brushRotation {0.0f};
    bool _brushGridSnap = false;
    float _brushGridSize = 1.0f;

    // Pottery (sculpt) tool settings ------------------------------------
    PotteryTool _currentPotteryTool = PotteryTool::Expand; // default tool
    float _potteryStrength = 0.2f; // default visual strength

    float _faceBrushRadius = 0.1f; // relative to texture (0-1)
    float _faceBrushSoftness = 1.0f; // 1 = hard, 0 = very soft
    float _faceBrushUOffset = 0.0f; // manual UV offset controls
    float _faceBrushVOffset = 0.0f;
    int  _faceBrushUAxis = 1; // 0=X,1=Y,2=Z
    int  _faceBrushVAxis = 2; // must differ from U axis
    bool _faceBrushInvertU = false;
    bool _faceBrushInvertV = false;

    // Advanced brush features
    enum class BrushType { 
        Normal = 0, 
        Airbrush, 
        Chalk, 
        Spray, 
        Smudge, 
        Clone 
    };
    BrushType _currentBrushType = BrushType::Normal;
    
    // Brush dynamics
    float _brushOpacity = 1.0f;
    float _brushFlow = 1.0f;
    float _brushSpacing = 0.1f;
    float _brushDensity = 0.5f;
    float _brushStrength = 0.5f;
    
    // Pressure simulation
    bool _usePressureSimulation = false;
    float _pressureSensitivity = 1.0f;
    float _currentPressure = 1.0f;
    
    // Stroke interpolation
    bool _useStrokeInterpolation = true;
    glm::vec2 _lastBrushUV = glm::vec2(-1.0f, -1.0f);
    float _lastBrushTime = 0.0f;
    int _lastBrushFace = -1; // face index on which the last UV was recorded
    Object* _lastBrushObject = nullptr; // object pointer for the last brush UV
    
    // Clone tool
    bool _cloneToolActive = false;
    glm::vec2 _cloneSourceUV = glm::vec2(0.0f, 0.0f);
    glm::vec2 _cloneOffset = glm::vec2(0.0f, 0.0f);
    
    // Layer system
    bool _useLayers = false;
    int _activeLayer = 0;
    float _layerOpacity = 1.0f;
    int _blendMode = 0; // 0=Normal, 1=Multiply, 2=Screen, 3=Overlay, 4=Add, 5=Subtract
    
    // Brush presets
    struct BrushPreset {
        std::string name;
        BrushType type;
        float radius;
        float softness;
        float opacity;
        float flow;
        float spacing;
        float density;
        float strength;
    };

    // Small builder to create presets in a self-documenting way (fluent API)
    struct PresetBuilder {
        BrushPreset value;
        PresetBuilder(const std::string& presetName, BrushType brushType) {
            value.name = presetName;
            value.type = brushType;
            // Sensible defaults; callers can override as needed
            value.radius = 0.1f;
            value.softness = 1.0f;
            value.opacity = 1.0f;
            value.flow = 1.0f;
            value.spacing = 0.1f;
            value.density = 0.5f;
            value.strength = 0.5f;
        }
        PresetBuilder& radius(float v)   { value.radius = v; return *this; }
        PresetBuilder& softness(float v) { value.softness = v; return *this; }
        PresetBuilder& opacity(float v)  { value.opacity = v; return *this; }
        PresetBuilder& flow(float v)     { value.flow = v; return *this; }
        PresetBuilder& spacing(float v)  { value.spacing = v; return *this; }
        PresetBuilder& density(float v)  { value.density = v; return *this; }
        PresetBuilder& strength(float v) { value.strength = v; return *this; }
        BrushPreset build() const { return value; }
    };
    std::vector<BrushPreset> _brushPresets;
    int _currentPreset = 0;
    
    // Brush preview
    bool _showBrushPreview = true;
    float _brushPreviewSize = 1.0f;
    
    // Undo/Redo
    // Undo/Redo state - now handled by KeyboardHandler

    // Cursor position
    // Cursor position now handled by MouseHandler

    // Brush cursor
    bool _showBrushCursor = true;
    glm::vec2 _brushCursorPos = glm::vec2(0.0f, 0.0f);
    bool _brushCursorVisible = false;

    // 2D Brush System
    bool _useAdvanced2DBrush = false;
    bool _show2DBrushPanel = false;
    // 3D Selection state
    Object* _selectedObject3D = nullptr;

public:
    Object* getSelectedObject3D() const { return _selectedObject3D; }
    void setSelectedObject3D(Object* obj) { _selectedObject3D = obj; }
    
    // Straight line tool state
    bool _straightLineMode = false;
    bool _drawingStraightLine = false;
    float _straightLineStartX = 0.0f;
    float _straightLineStartY = 0.0f;

    // Save / load ----------------------------------------------------------
    std::vector<std::string> _saveFiles;
    bool _showLoadWindow = false;
    bool _showSaveWindow = false;
    bool _showSaveManager = false;
    char _customSaveName[256] = "";

    void saveState(const std::string& filename);
    void loadState(const std::string& filename);
    void saveStateWithLog(const std::string& customName="");
    void updateSaveFiles();
    void drawLoadWindow();
    void drawSaveWindow();
    void drawSaveManager();
    
    // Polyhedron generation
    void _generateCustomPolyhedron();

    // UI helpers
    bool isMenuOpen() const { return _mainMenu.isOpen(); }
    bool& getShowKeymapRef() { return _showKeymapWindow; }
};

} // namespace Core 