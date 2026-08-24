class ElementalToolHandler;
#include "ZonesOfEarth/Ourverse/Ourverse.hpp"
#include "ConstructedBeing/Singular/Object/Formation/Menu/Menu.hpp"
#include <glm/glm.hpp>
#pragma once

#include <GLFW/glfw3.h>
#include <memory>

class Person;
class LawManager;
class KeyboardHandler;
class MouseHandler;
class CursorTools;
class Chat;

namespace Core { class Camera; }


namespace Core {
    enum class PotteryTool { Pinch, Pull, Push, Smooth, Flatten, Sharpen, Expand };
    enum class RotationAxisMode { Free, X, Y, Z, FreeXY, AuthoritativeAxis };



// The WebGPU device/surface/renderer, when built with -DEARTHCALL_WEBGPU.
// Deliberately only forward-declared: defining it here would drag webgpu.h into
// every translation unit that includes Engine.hpp, and the default OpenGL build
// must not need the WebGPU headers at all. Null in the OpenGL build.
struct WebGpuBackend;

// Centralised application driver. Responsible for window/context creation,
// main-loop timing, and global shutdown.  (Pure skeleton – implementation
// in Engine.cpp.)
//
// Which GPU backend this drives is a COMPILE-TIME choice, not a runtime one:
// a window is created either with a GL context or with GLFW_NO_API for WebGPU,
// and that cannot be changed afterwards. `make` builds the OpenGL binary and
// `make webgpu-app` builds the WebGPU one from the same sources, so the two can
// be run side by side and compared.
class Engine {
public:
    // Singleton accessor (simple for now; can be replaced later)
    static Engine& instance();

    // Lifecycle -----------------------------------------------------------
    bool init(int argc = 0, char** argv = nullptr);
    void initLogic();
    void run();
    void tick(float dt);
    void update(float dt);
    void shutdown();

    // Accessors -----------------------------------------------------------
    GLFWwindow* window() const { return _window; }
    bool running()   const { return _running; }
    
    // Core systems migrating from Game
    Person* getPlayer();
    Camera* getCamera();
    LawManager* getLawManager();
    KeyboardHandler* getKeyboardHandler();
    MouseHandler* getMouseHandler();
    CursorTools* getCursorTools();
    void ensureCursorUnlocked();
    bool getAdvanced2DBrush() const { return true; }

    bool isMouseLeftPressedLast() const { return _mouseLeftPressedLast; }
    void setMouseLeftPressedLast(bool v) { _mouseLeftPressedLast = v; }

    bool& devToolsWindowOpen() { return _devToolsWindowOpen; }
        
    glm::vec2 get2DToolDragStart() const { return _dragStart; }
    void update2DToolDrag(glm::vec2 pos) { _dragCurrent = pos; }
    template<typename T> void begin2DToolDrag(T type, glm::vec2 pos) { _is2DToolDragging = true; _dragStart = pos; _dragCurrent = pos; }
    void end2DToolDrag() { _is2DToolDragging = false; }
    template<typename T> bool is2DToolDragging(T type) const { return _is2DToolDragging; }
    bool getUseLegacy2DTools() const { return _useLegacy2DTools; }
    
    Ourverse& getWorld() { return _world; }
    Menu& getMainMenu() { return _mainMenu; }
    
    void fuseObjects(Object* A, Object* B);
    void blendRail(const Object* o, glm::vec3& start, glm::vec3& dir, float& length) const;
    bool handleFieldGizmos(Object* o, bool pressEdge, bool mouseDown, double winX, double winY);
    void registerCallbacks();

    void setCurrentColor(int index, float val) {} // dummy for now
    
        
    
    double getWorldTime() const { return _worldTime; }
    void setWorldTime(double t) { _worldTime = t; }
    double* worldTimePtr() { return &_worldTime; }

private:
    Engine() = default;                       // use instance()
    ~Engine();
    Engine(const Engine&)            = delete;
    Engine& operator=(const Engine&) = delete;

    GLFWwindow*     _window  = nullptr;
    bool            _running = false;
    WebGpuBackend*  _webgpu  = nullptr; // owned; only allocated in the WebGPU build
    
    // Systems migrated from Game
    std::unique_ptr<Person> _player;
    std::unique_ptr<Camera> _camera;
    std::unique_ptr<LawManager> _lawManager;
        std::unique_ptr<KeyboardHandler> _keyboardHandler;
    std::unique_ptr<MouseHandler> _mouseHandler;
    std::unique_ptr<CursorTools> _cursorTools;
    std::unique_ptr<Chat> _chat;
    std::unique_ptr<::ElementalToolHandler> _elementalToolHandler;
    double _worldTime = 0.0;

    bool _mouseLeftPressedLast = false;
    bool _devToolsWindowOpen = false;
    bool _creationConsoleOpen = false;
    bool _creatorConsoleOpen = false;
    bool _showKeymapWindow = false;
    bool _showChatWindow = false;
    bool _showImGuiDemo = false;
    bool _is2DToolDragging = false;
    glm::vec2 _dragStart = {0,0};
    glm::vec2 _dragCurrent = {0,0};
    bool _useLegacy2DTools = false;
    
        // Callbacks
    GLFWcursorposfun       _prevCursorPosCallback       = nullptr;
    GLFWwindowfocusfun     _prevFocusCallback           = nullptr;
    GLFWframebuffersizefun _prevFramebufferSizeCallback = nullptr;
    GLFWkeyfun             _prevKeyCallback             = nullptr;

    static void onCursorPos(GLFWwindow* win, double xpos, double ypos);
    static void onWindowFocus(GLFWwindow* win, int focused);
    static void onFramebufferSize(GLFWwindow* win, int width, int height);
    static void onKey(GLFWwindow* win, int key, int scancode, int action, int mods);

        // Polyhedron and Tool
    void buildCurrentPolyhedron() {} // dummy
    int _polyhedron = 0; // dummy for now, wait we need drawingStraightLine
    bool _drawingStraightLine = false;
    float _straightLineStartX = 0;
    float _straightLineStartY = 0;
    
    
public:
    // Missing rotation state — bodies in Engine.cpp, chrome on CreatorConsoleState
    bool getRotateDragging() const;
    double getRotateLastCursorX() const;
    double getRotateLastCursorY() const;
    void setRotateLastCursor(double x, double y);
    void setRotateDragging(bool);
    bool isAdvancedFacePaintEnabled() const;
    void* getCurrentGradientSettings() const { return nullptr; }
    void* getCurrentSmudgeSettings() const { return nullptr; }
    float getCurrentColor(int) const;
    int _currentTool = 0;
    float _straightLineEndX = 0;
    float _straightLineEndY = 0;
    float getRotationToolSensitivity() const;
    float getRotationToolSmoothness() const;
    RotationAxisMode getRotationAxisMode() const;
    
    // Missing Pottery state
    PotteryTool getCurrentPotteryTool() const;
    float getPotteryStrength() const;
    
    // Missing 2D state
    const std::vector<glm::vec2>& get2DToolDragPoints() const { static std::vector<glm::vec2> v; return v; }
    
    Ourverse _world;
    int _patchCtrlIndex = 0;
    float _currentColor[4] = {1,1,1,1};
    struct DummyBrush { bool showCursor=false; bool cursorVisible=false; float previewSize=1.0f; }; DummyBrush _brush;
        glm::vec3 rotation;
    struct DummyFaceBrush { float radius=1.0f; bool soft=false; float softness=0.0f; }; DummyFaceBrush _faceBrush;
    float _cubeAngle = 0;
    double getCursorX() const;
    double getCursorY() const;
    float getFaceBrushUOffset() const;
    float getFaceBrushVOffset() const;
    void setBrushCursorPos(float, float) {}
    void setBrushCursorVisible(bool) {}
    float getCurrentPressure() const { return 1.0f; }
    bool getUsePressureSimulation() const { return false; }
    void render();
    void renderNametags();
    void onFramebufferSize(int width, int height);
    Menu _mainMenu;
    
    // Combine tool state
    Object* _combineOperandA = nullptr;
    int _combineOp = 2;
    float _combineBlend = 0.15f;
    Object* _clayGrabbed = nullptr;
    Object* _clayTarget = nullptr;
    bool _fieldHandleDragging = false;
    bool _blendHandleDragging = false;

    enum class PerspectiveMode { FirstPerson = 0, SecondPerson, ThirdPerson };
    PerspectiveMode _currentPerspective = PerspectiveMode::FirstPerson;

    
};

} // namespace Core 