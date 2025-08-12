#include "ZonesOfEarth/Ourverse/Ourverse.hpp"
#include "ZonesOfEarth/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "Form/Object/Object.hpp"
#include "Form/Object/Formation/Menu/Menu.hpp"
#include "OurVerse/Tool.hpp"
#include "OurVerse/Chat.hpp"
#include "Person/Person.hpp"
#include "Rendering/ShadingSystem.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"

// #include "Ourverse/OurverseArtTool.hpp"
// #include "Ourverse/OurverseArtTool.cpp"*/
#include <iostream>
#include <fstream>
#include <memory>

#include <GLFW/glfw3.h>
#include <cmath> // for tanf and M_PI
#include <OpenGL/glu.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../../imgui/imgui.h"
#include "../../imgui/backends/imgui_impl_glfw.h"
#include "../../imgui/backends/imgui_impl_opengl2.h"
#include "json.hpp" // nlohmann/json
#include <ctime>
#include <sstream>
#include <vector>
#include <algorithm>
#include <unistd.h> // getcwd
#include <limits.h>

Menu mainMenu;

glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float cameraSpeed = 0.1f;
float speedMultiplier = 1.0f;

bool cursorTogglePressed = false;

float lastFrontX;
float lastFrontY;
float lastFrontZ;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ---------------- Globals -------------------------
GLFWwindow *win;
ZoneManager mgr;

float yaw = -90.0f; // starts facing -Z
float pitch = 0.0f;

float lastX = 250.0f, lastY = 250.0f;
bool firstMouse = true;

bool cursorLocked = true;

int winWidth, winHeight;

bool drawMode = true;

Tool currentTool(Tool::Type::Brush);
float currentColor[3] = {1.0f, 0.9f, 0.2f};

// Current primitive shape and size used by the 3-D brush
Object::GeometryType currentPrimitive = Object::GeometryType::Cube;
float brushSize = 1.0f; // uniform scale factor when spawning primitives

// Advanced brush transform properties
glm::vec3 brushScale = glm::vec3(1.0f);      // Non-uniform scale multiplier on top of brushSize
glm::vec3 brushRotation = glm::vec3(0.0f);    // Euler rotation in degrees (XYZ)
bool       brushGridSnap = false;              // Snap placement to grid?
float      brushGridSize = 1.0f;               // Grid spacing when snapping

bool showToolbar = true; // Add this to control toolbar visibility

// Separate key press flags for toggles
bool escapePressedLastFrame = false;
bool tPressedLastFrame = false;
bool cPressedLastFrame = false; // design zone toggle
bool lPressedLastFrame = false; // design-lock toggle
bool fPressedLastFrame = false; // flight toggle
// Key press flag for chat toggle
// H key will open/close chat window

// Mouse state tracking
bool mouseLeftPressed = false;

// Store camera matrices each frame for accurate picking
GLdouble gCameraModelview[16];
GLdouble gCameraProjection[16];
GLint    gCameraViewport[4] = {0,0,0,0};

// Forward declaration so it can be used before definition
void LogStateSummary(const char* tag);

// Focus callback to fix ImGui/GLFW input bugs
void window_focus_callback(GLFWwindow* window, int focused) {
    if (focused) {
        mouseLeftPressed = false;
        ImGuiIO& io = ImGui::GetIO();
        for (int i = 0; i < IM_ARRAYSIZE(io.MouseDown); ++i) {
            io.MouseDown[i] = false;
        }
    }
}

// Function declarations
void ShowLayerPanel();
void UndoLastAction();

// ArtPiece userCreation("FirstSoulArt");

struct Stroke {
    std::vector<float> points;
    float r, g, b;
};
std::vector<Stroke> strokes;
Stroke currentStroke;

// Add a global flag for the layer window
bool showLayerWindow = false;

bool showLoadPopup = false;
char customSaveName[128] = "";

bool showAvatarLoadPopup = false;
char customAvatarName[128] = "";

std::vector<std::string> saveFiles;

// Avatar save tracking
std::vector<std::string> avatarFiles;

// -------------------------
// Chat system globals
// -------------------------
Chat chat;
bool showChatWindow = true;
bool hPressedLastFrame = false;

// Add a global Ourverse instance for world state
Ourverse world;

// ---------------------------------------------------------------------------------
// Perspective modes
// ---------------------------------------------------------------------------------

enum class PerspectiveMode {
    FirstPerson = 0,
    SecondPerson,
    ThirdPerson
};

PerspectiveMode currentPerspective = PerspectiveMode::FirstPerson;

// Distance and height offsets for the third/second person cameras
static const float CAMERA_DISTANCE = 4.0f;   // units behind/ in front of avatar
// Camera height is now derived from avatar hitbox --> player.body.getEyeHeight()
static const float CROSSHAIR_OFFSET = 1.0f;   // how far in front of head the crosshair projects

// ---------------------------------------------------------------------------------
// Player avatar
// ---------------------------------------------------------------------------------

Person player("Player", Body::createBasicAvatar("Voxel"), glm::vec3(0.0f, 0.0f, 0.0f));

// ---------------------------------------------------------------------------------
// 3D creation / editing modes
// ---------------------------------------------------------------------------------

enum class Mode3D {
    None = -1,
    FacePaint = 0,   // Fill entire face with a colour
    FaceBrush,       // Brush paint on faces with strokes
    BrushCreate,     // Free-form sculpting with a 3-D "shape generator"
    Pottery          // Lathe-style precision sculpting
};

// Brush placement behaviour for 3-D Brush
enum class BrushPlacementMode { InFront = 0, ManualDistance, CursorSnap };

BrushPlacementMode brushPlacementMode = BrushPlacementMode::InFront;
glm::vec3 manualOffset = glm::vec3(0.0f, 0.0f, 2.0f);
bool manualAnchorValid = false;
glm::vec3 manualAnchorPos;
glm::vec3 manualAnchorRight, manualAnchorUp, manualAnchorForward;

Mode3D current3DMode = Mode3D::None;

void UpdateSaveFiles() {
    saveFiles.clear();
    std::ifstream log("save_log.txt");
    std::string line;
    while (std::getline(log, line)) {
        if (!line.empty()) saveFiles.push_back(line);
    }
}

void UpdateAvatarFiles() {
    avatarFiles.clear();
    std::ifstream log("avatar_log.txt");
    std::string line;
    while (std::getline(log, line)) {
        if (!line.empty()) avatarFiles.push_back(line);
    }
}

void SaveAvatar(const std::string& filename);
void LoadAvatar(const std::string& filename);

void SaveState(const std::string& filename);
void LoadState(const std::string& filename);

void SaveStateWithLog(const std::string& customName = "");
void SaveAvatarWithLog(const std::string& customName = "");

void ShowLoadPopup() {
    std::cout << "[DEBUG] ShowLoadPopup saveFiles=" << saveFiles.size() << std::endl;
    ImGui::OpenPopup("Load Save");
    if (ImGui::BeginPopupModal("Load Save", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        UpdateSaveFiles();
        if (saveFiles.empty()) {
            ImGui::Text("No saves found.");
        } else {
            for (size_t i = 0; i < saveFiles.size(); ++i) {
                if (ImGui::Button(saveFiles[i].c_str())) {
                    LoadState(saveFiles[i]);
                    ImGui::CloseCurrentPopup();
                }
            }
        }
        if (ImGui::Button("Close")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void ShowAvatarLoadPopup() {
    ImGui::OpenPopup("Load Avatar");
    if (ImGui::BeginPopupModal("Load Avatar", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        UpdateAvatarFiles();
        if (avatarFiles.empty()) {
            ImGui::Text("No avatars found.");
        } else {
            for (size_t i = 0; i < avatarFiles.size(); ++i) {
                if (ImGui::Button(avatarFiles[i].c_str())) {
                    LoadAvatar(avatarFiles[i]);
                    ImGui::CloseCurrentPopup();
                }
            }
        }
        if (ImGui::Button("Close")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    if (!mainMenu.isOpen() && cursorLocked)
    {
        if (firstMouse)
        {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float sensitivity = 0.1f;
        float xoffset = (xpos - lastX) * sensitivity;
        float yoffset = (lastY - ypos) * sensitivity; // reversed: y ranges bottom to top

        lastX = xpos;
        lastY = ypos;

        yaw += xoffset;
        pitch += yoffset;

        // clamp pitch
        if (pitch > 89.0f)
            pitch = 89.0f;
        if (pitch < -89.0f)
            pitch = -89.0f;

        // convert yaw/pitch to direction vector
        glm::vec3 direction;
        direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction.y = sin(glm::radians(pitch));
        direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraFront = glm::normalize(direction);
    }
}

using json = nlohmann::json;

void RenderArtToolbar() {
    ImGui::Begin("🛠 Earthcall Creator", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    if (ImGui::BeginTabBar("CreatorTabs", ImGuiTabBarFlags_None)) {
        /* ------------------------------------------------------------------
         * Paint TAB
         * ----------------------------------------------------------------*/
        if (ImGui::BeginTabItem("🎨 Paint")) {
            // Primary painting tools
            if (ImGui::Button("🖌 Brush")) {
                currentTool = Tool(Tool::Type::Brush);
                current3DMode = Mode3D::None;
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Draw free-hand strokes (B)");
            ImGui::SameLine();
            if (ImGui::Button("🧽 Eraser")) {
                currentTool = Tool(Tool::Type::Eraser);
                current3DMode = Mode3D::None;
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Erase strokes under cursor (E)");
            ImGui::SameLine();
            if (ImGui::Button("🔲 Shape")) {
                currentTool = Tool(Tool::Type::Shape);
                current3DMode = Mode3D::None;
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Draw straight lines & primitives (L)");

            // Color & picker
            ImGui::Separator();
            ImGui::Text("Main Color:");
            ImGui::SameLine();
            if (ImGui::ColorEdit3("##MainColor", currentColor, ImGuiColorEditFlags_NoInputs)) {
                mgr.active().setDrawColor(currentColor[0], currentColor[1], currentColor[2]);
                current3DMode = Mode3D::None;
            }
            ImGui::SameLine();
            if (ImGui::Button("🎯 Pick")) {
                currentTool = Tool(Tool::Type::ColorPicker);
                current3DMode = Mode3D::None;
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Eyedropper – sample color under cursor (I)");
            ImGui::EndTabItem();
        }

        /* ------------------------------------------------------------------
         * Layers TAB
         * ----------------------------------------------------------------*/
        if (ImGui::BeginTabItem("🧅 Layers")) {
            if (ImGui::Button("Open Layer Panel")) {
                showLayerWindow = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("↩ Undo")) {
                UndoLastAction();
            }
            ImGui::EndTabItem();
        }

        /* ------------------------------------------------------------------
         * Assets / File TAB
         * ----------------------------------------------------------------*/
        if (ImGui::BeginTabItem("📂 Assets")) {
            if (ImGui::Button("💾 Save")) {
                SaveStateWithLog();
            }
            ImGui::SameLine();
            if (ImGui::Button("💾 Save As")) {
                ImGui::OpenPopup("Save As Popup");
            }
            ImGui::SameLine();
            if (ImGui::Button("📂 Load")) {
                showLoadPopup = true;
            }
            ImGui::Separator();
            if (ImGui::Button("👤 Save Avatar")) {
                ImGui::OpenPopup("Save Avatar As Popup");
            }
            ImGui::SameLine();
            if (ImGui::Button("👤 Load Avatar")) {
                showAvatarLoadPopup = true;
            }
            ImGui::EndTabItem();
        }

        /* ------------------------------------------------------------------
         * 3D Objects TAB
         * ----------------------------------------------------------------*/
        if (ImGui::BeginTabItem("🔳 3D")) {
            static const char* modeNames[] = {"Face Paint", "Face Brush", "3D Shape Generator", "Pottery"};
            int modeIdx = static_cast<int>(current3DMode);
            ImGui::Text("Sub-Mode:");
            ImGui::SameLine();
            if (ImGui::Combo("##3DModeCombo", &modeIdx, modeNames, IM_ARRAYSIZE(modeNames))) {
                current3DMode = static_cast<Mode3D>(modeIdx);
            }

            // Contextual hints / controls
            ImGui::Separator();
            switch (current3DMode) {
                case Mode3D::FacePaint:
                    ImGui::TextWrapped("Click on a cube face to paint.");
                    break;
                case Mode3D::FaceBrush:
                    ImGui::TextWrapped("Drag over faces to paint strokes.");
                    break;
                case Mode3D::BrushCreate:
                    ImGui::TextWrapped("Left-click to spawn primitives along your sight line – like a versatile 3-D shape generator.");

                    // Additional controls for primitive shape and size
                    {
                        int primitiveIdx = static_cast<int>(currentPrimitive);
                        const char* primitiveNames[] = {"Cube", "Sphere", "Cylinder", "Cone"};
                        if (ImGui::Combo("Shape", &primitiveIdx, primitiveNames, IM_ARRAYSIZE(primitiveNames))) {
                            currentPrimitive = static_cast<Object::GeometryType>(primitiveIdx);
                        }

                        ImGui::SliderFloat("Uniform Size", &brushSize, 0.1f, 10.0f, "%.2f");

                        // Placement controls
                        ImGui::Separator();
                        int placeIdx = static_cast<int>(brushPlacementMode);
                        const char* placeNames[] = {"In Front", "Manual Distance", "Cursor Snap"};
                        static BrushPlacementMode prevMode = brushPlacementMode;
                        if (ImGui::Combo("Placement", &placeIdx, placeNames, IM_ARRAYSIZE(placeNames))) {
                            brushPlacementMode = static_cast<BrushPlacementMode>(placeIdx);
                        }
                        if (brushPlacementMode == BrushPlacementMode::ManualDistance && prevMode != BrushPlacementMode::ManualDistance) {
                            manualAnchorPos      = cameraPos + cameraFront * 2.0f;
                            manualAnchorRight    = glm::normalize(glm::cross(cameraFront, cameraUp));
                            manualAnchorUp       = cameraUp;
                            manualAnchorForward  = cameraFront;
                            manualAnchorValid    = true;
                        }
                        prevMode = brushPlacementMode;
                        if (brushPlacementMode == BrushPlacementMode::ManualDistance) {
                            ImGui::SliderFloat3("Offset XYZ", &manualOffset.x, -20.0f, 20.0f, "%.2f");
                            ImGui::TextUnformatted("X = right, Y = up, Z = forward");
                        }

                        // Advanced transform foldout
                        if (ImGui::TreeNode("Advanced Transform")) {
                            ImGui::DragFloat3("Scale XYZ", &brushScale.x, 0.05f, 0.1f, 10.0f, "%.2f");
                            ImGui::DragFloat3("Rotation (deg)", &brushRotation.x, 1.0f, 0.0f, 360.0f, "%.0f");
                            ImGui::Checkbox("Grid Snap", &brushGridSnap);
                            if (brushGridSnap) {
                                ImGui::SliderFloat("Grid Size", &brushGridSize, 0.1f, 10.0f, "%.2f");
                            }
                            ImGui::TreePop();
                        }
                    }
                    break;
                case Mode3D::Pottery:
                    ImGui::TextWrapped("Coming soon: rotate a virtual wheel and sculpt with precision.");
                    break;
            }

            ImGui::EndTabItem();
        }

        /* ------------------------------------------------------------------
         * World / Game Mode TAB – delegates to world.renderModeUI()
         * ----------------------------------------------------------------*/
        if (ImGui::BeginTabItem("🌍 World")) {
            world.renderModeUI();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    /* -----------------------------------------------------------
     * Save As popup – reused from original implementation
     * ---------------------------------------------------------*/
    if (ImGui::BeginPopupModal("Save As Popup", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputText("Save Name", customSaveName, IM_ARRAYSIZE(customSaveName));
        if (ImGui::Button("Save")) {
            SaveStateWithLog(customSaveName);
            customSaveName[0] = '\0';
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (showLoadPopup) {
        ShowLoadPopup();
    }

    if (ImGui::BeginPopupModal("Save Avatar As Popup", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputText("Avatar Name", customAvatarName, IM_ARRAYSIZE(customAvatarName));
        if (ImGui::Button("Save")) {
            SaveAvatarWithLog(customAvatarName);
            customAvatarName[0] = '\0';
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (showAvatarLoadPopup) {
        ShowAvatarLoadPopup();
    }

    if (!ImGui::IsPopupOpen("Load Save") && !ImGui::IsPopupOpen("Load Avatar")) {
        showLoadPopup = false;
        showAvatarLoadPopup = false;
    }

    ImGui::End();

    // Render separate Layer window if requested
    if (showLayerWindow) {
        ShowLayerPanel();
    }
}

// Function implementations
void ShowLayerPanel() {
    ImGui::Begin("🧅 Layer Panel");
    ImGui::Text("Layer management coming soon...");
    ImGui::End();
}

void UndoLastAction() {
    // Remove the last stroke from the current zone
    auto& strokes = mgr.active().strokes;
    if (!strokes.empty()) {
        strokes.pop_back();
    }
}

// --------------------------------------------------------
// Simple debug helper – prints summary of important state
// --------------------------------------------------------
void LogStateSummary(const char* tag) {
    char cwdBuf[PATH_MAX];
    getcwd(cwdBuf, sizeof(cwdBuf));
    std::cout << "[DEBUG] " << tag << " | cwd=" << cwdBuf
              << " | camPos(" << cameraPos.x << "," << cameraPos.y << "," << cameraPos.z << ")"
              << " | objects=" << world.getOwnedObjects().size()
              << " | currentZone=" << mgr.currentIndex();
    std::cout << " | strokesPerZone=";
    for (size_t i=0;i<mgr.zones().size();++i) {
        std::cout << mgr.zones()[i].strokes.size();
        if (i+1<mgr.zones().size()) std::cout << ",";
    }
    std::cout << std::endl;
}

int main()
{
    glfwInit();

    if (!glfwInit())
    {
        std::cerr << "⚠️ Failed to initialize GLFW!" << std::endl;
        return -1;
    }

    winWidth = 2000;
    winHeight = 2000;

    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);
    win = glfwCreateWindow(winWidth, winHeight, "🌌 Earthcall: Sanctum of Beginnings", nullptr, nullptr);

    // Register focus callback to fix ImGui/GLFW input bugs
    glfwSetWindowFocusCallback(win, window_focus_callback);

    mgr.addZone(Zone("Sanctum of Beginnings"));
    mgr.addZone(Zone("Temple of Echoes"));
    mgr.addZone(Zone("Cavern of Light"));
    mgr.addZone(Zone("Character Architect Forge"));

    // --------------------------------------------------------------
    // Populate main menu with zone switching entries (F1..F12)
    // --------------------------------------------------------------
    const size_t maxHotkeys = 12; // up to F12
    for (size_t i = 0; i < mgr.zones().size() && i < maxHotkeys; ++i)
    {
        int hotkey = GLFW_KEY_F1 + static_cast<int>(i); // GLFW_KEY_F1 ..
        std::string label = "Enter " + mgr.zones()[i].name();
        mainMenu.addOption(label, hotkey, [&, i]() { mgr.switchTo(i); });
    }

    if (!win)
    {
        std::cerr << "⚠️ Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(win);

    glEnable(GL_DEPTH_TEST);
    // Initialize lighting and shading
    ShadingSystem::init();

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(win, true);
    ImGui_ImplOpenGL2_Init();

    // Chain ImGui + camera cursor callback once (instead of overwriting each frame)
    glfwSetCursorPosCallback(win, [](GLFWwindow* window, double xpos, double ypos){
        ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos); // forward to ImGui
        mouse_callback(window, xpos, ypos);                   // camera control
    });

    // (Projection matrix setup moved into the render loop)

    std::cout << "🔥 Earthcall engine booted. The world awaits..." << std::endl;
    LogStateSummary("Startup");

    // Register camera and objects with the world
    world.setCamera(&cameraPos);
    std::unique_ptr<Object> cube(new Object());
    std::unique_ptr<Object> ground(new Object());
    world.addOwnedObject(std::move(cube));
    world.addOwnedObject(std::move(ground));

    // Hide cursor
    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Lock cursor to window
    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    /* _________________________________________________


                                Menu

           ________________________________________________________ */  

    // Set up the menu options
    mainMenu.addOption("Resume World", GLFW_KEY_R, [&]()
                       { mainMenu.close(); });
    mainMenu.addOption("Save", GLFW_KEY_S, [&]() { /* TODO: Write the save logic */ });
    mainMenu.addOption("Quit", GLFW_KEY_Q, [&]()
                       { glfwSetWindowShouldClose(win, 1); });

    // ----------------------------------------------------
    //  Design-Lock  (true = read-only, false = editable)
    // ----------------------------------------------------
    bool designLocked = false;

    while (!glfwWindowShouldClose(win))
    {
        glfwPollEvents();

        // Start new ImGui frame early so all subsequent ImGui::Begin calls are valid
        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        int winW, winH;
        glfwGetWindowSize(win, &winW, &winH);
        if (winH == 0)
            winH = 1; // prevent divide-by-zero
        float aspect = static_cast<float>(winW) / winH;
        glViewport(0, 0, winW, winH);

        // ------------------------------------------------------------------
        // Perspective switching (keys 1,2,3)
        // ------------------------------------------------------------------
        if (glfwGetKey(win, GLFW_KEY_1) == GLFW_PRESS) {
            currentPerspective = PerspectiveMode::FirstPerson;
        }
        if (glfwGetKey(win, GLFW_KEY_2) == GLFW_PRESS) {
            currentPerspective = PerspectiveMode::SecondPerson;
        }
        if (glfwGetKey(win, GLFW_KEY_3) == GLFW_PRESS) {
            currentPerspective = PerspectiveMode::ThirdPerson;
        }

        // Flight toggle (F key) only allowed in Creative or Spectator
        bool fPressed = glfwGetKey(win, GLFW_KEY_F) == GLFW_PRESS;
        if (fPressed && !fPressedLastFrame) {
            if (world.getMode() != Ourverse::GameMode::Survival) {
                Physics::toggleFlying();
            }
        }
        fPressedLastFrame = fPressed;

        // Set the projection matrix
        float fov = 45.0f;
        float nearZ = 0.1f;
        float farZ = 100.0f;
        float top = tanf(fov * M_PI / 360.0f) * nearZ;
        float bottom = -top;
        float right = top * aspect;
        float left = -right;

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glFrustum(left, right, bottom, top, nearZ, farZ);

        // Set the clear color to a deep blue darkness
        glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // Update dynamic lighting (light follows camera)
        ShadingSystem::update(cameraPos);

        // ------------------------------------------------------------------
        // Compute and apply camera transform based on the current perspective
        // ------------------------------------------------------------------

        // Base head position (player anchor + head height)
        glm::vec3 headPos = cameraPos + glm::vec3(0.0f, player.body.getEyeHeight(), 0.0f);

        // Establish eye and target per-perspective
        glm::vec3 eyePos   = headPos;                 // default FPV eyes at head
        glm::vec3 lookDir  = cameraFront;             // initial view direction

        if (currentPerspective == PerspectiveMode::ThirdPerson) {
            eyePos  = headPos - cameraFront * CAMERA_DISTANCE; // camera behind the avatar
            lookDir = glm::normalize(headPos - eyePos);        // look toward head
        } else if (currentPerspective == PerspectiveMode::SecondPerson) {
            eyePos  = headPos + cameraFront * CAMERA_DISTANCE; // camera in front of avatar
            lookDir = glm::normalize(headPos - eyePos);        // look toward head
        }

        // Final look target: a point slightly in front of the head along view direction
        glm::vec3 lookTarget = headPos + lookDir * CROSSHAIR_OFFSET;

        // Update last front for menu hold
        glm::vec3 currentFront = glm::normalize(lookTarget - eyePos);

        if (!mainMenu.isOpen()) {
            gluLookAt(eyePos.x, eyePos.y, eyePos.z,
                      lookTarget.x, lookTarget.y, lookTarget.z,
                      cameraUp.x, cameraUp.y, cameraUp.z);

            // Capture camera matrices for picking before we multiply in object transforms
            glGetIntegerv(GL_VIEWPORT, gCameraViewport);
            glGetDoublev(GL_MODELVIEW_MATRIX, gCameraModelview);
            glGetDoublev(GL_PROJECTION_MATRIX, gCameraProjection);

            lastFrontX = currentFront.x;
            lastFrontY = currentFront.y;
            lastFrontZ = currentFront.z;
        } else {
            // Freeze view when menu open
            gluLookAt(eyePos.x, eyePos.y, eyePos.z,
                      eyePos.x + lastFrontX, eyePos.y + lastFrontY, eyePos.z + lastFrontZ,
                      cameraUp.x, cameraUp.y, cameraUp.z);
        }

        // Update cube transform (rotation)
        static float angle = 0.0f;
        angle += 0.5f;
        glm::mat4 cubeTransform = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.5f, 1.0f, 0.0f));
        world.getOwnedObjects()[0]->setTransform(cubeTransform); // cube is first

        // Update ground transform (identity, but scale to match ground plane)
        glm::mat4 groundTransform = glm::scale(glm::mat4(1.0f), glm::vec3(100.0f, 1.0f, 100.0f));
        world.getOwnedObjects()[1]->setTransform(groundTransform); // ground is second

        // Update world physics and collisions
        world.onUpdate();

        // Draw the ground plane
        glPushMatrix();
        glNormal3f(0.0f, 1.0f, 0.0f); // upward facing normal for lighting
        glColor3f(0.4f, 0.7f, 0.5f); // soft green
        glBegin(GL_QUADS);
        float groundSize = 100.0f;
        float groundY = 0.0f;
        glVertex3f(-groundSize, groundY, -groundSize);
        glVertex3f( groundSize, groundY, -groundSize);
        glVertex3f( groundSize, groundY,  groundSize);
        glVertex3f(-groundSize, groundY,  groundSize);
        glEnd();
        glPopMatrix();

        // Draw all cubes currently in the world (index 0 is the animated cube, 1 is ground)
        const auto& objects = world.getOwnedObjects();
        for (size_t i = 0; i < objects.size(); ++i) {
            // Skip ground (index 1) because it's drawn separately as a quad
            if (i == 1) continue;
            glPushMatrix();
            glMultMatrixf(&objects[i]->getTransform()[0][0]);
            objects[i]->drawObject();
            glPopMatrix();
        }

        // ------------------------------------------------------------------
        // Live preview ("hologram") for 3-D Brush placement
        // ------------------------------------------------------------------
        if (current3DMode == Mode3D::BrushCreate) {
            glm::vec3 previewPos;
            if (brushPlacementMode == BrushPlacementMode::InFront) {
                previewPos = cameraPos + cameraFront * 2.0f;
            } else if (brushPlacementMode == BrushPlacementMode::ManualDistance) {
                if(!manualAnchorValid){
                    manualAnchorPos      = cameraPos + cameraFront * 2.0f;
                    manualAnchorRight    = glm::normalize(glm::cross(cameraFront, cameraUp));
                    manualAnchorUp       = cameraUp;
                    manualAnchorForward  = cameraFront;
                    manualAnchorValid    = true;
                }
                previewPos = manualAnchorPos + manualAnchorRight * manualOffset.x + manualAnchorUp * manualOffset.y + manualAnchorForward * manualOffset.z;
            } else {
                // CursorSnap raycast
                double cx, cy; glfwGetCursorPos(win, &cx, &cy);
                GLdouble nearX, nearY, nearZ, farX, farY, farZ;
                double winY = gCameraViewport[3] - cy;
                gluUnProject(cx, winY, 0.0, gCameraModelview, gCameraProjection, gCameraViewport, &nearX, &nearY, &nearZ);
                gluUnProject(cx, winY, 1.0, gCameraModelview, gCameraProjection, gCameraViewport, &farX,  &farY,  &farZ);
                glm::vec3 rayO = glm::vec3(nearX, nearY, nearZ);
                glm::vec3 rayDir = glm::normalize(glm::vec3(farX, farY, farZ) - rayO);
                float nearestT = 1e9f; int hitAxis=-1; int hitSign=1; Object* hitObj=nullptr;
                const auto& objs = world.getOwnedObjects();
                for (const auto& up : objs) {
                    Object* obj = up.get();
                    glm::mat4 inv = glm::inverse(obj->getTransform());
                    glm::vec3 oL = glm::vec3(inv * glm::vec4(rayO, 1.0f));
                    glm::vec3 dL = glm::normalize(glm::vec3(inv * glm::vec4(rayDir, 0.0f)));
                    float tMin=-1e9,tMax=1e9; int axis=-1; int sign=1;
                    for(int a=0;a<3;++a){float o=oL[a],d=dL[a];float t1,t2;if(fabs(d)<1e-6){if(o<-0.5||o>0.5){tMin=1e9;break;}t1=-1e9;t2=1e9;}else{t1=(-0.5-o)/d; t2=(0.5-o)/d;} if(t1>tMin){tMin=t1; axis=a; sign=(d>0?-1:1);} if(t2<tMax) tMax=t2; if(tMin>tMax){tMin=1e9;break;}}
                    if(tMin<nearestT && tMin>0 && tMin<1e8){ nearestT = tMin; hitAxis=axis; hitSign=sign; hitObj=obj; }
                }
                if (nearestT < 1e8f && hitObj) {
                    glm::vec3 hitPoint = rayO + rayDir * nearestT;
                    glm::vec3 nLocal(0.0f); nLocal[hitAxis] = static_cast<float>(hitSign);
                    glm::vec3 nWorld = glm::normalize(glm::vec3(hitObj->getTransform() * glm::vec4(nLocal,0.0f)));
                    glm::vec3 half = glm::vec3(brushScale.x * brushSize, brushScale.y * brushSize, brushScale.z * brushSize) * 0.5f;
                    float offAmt = glm::dot(glm::abs(nWorld), half) + 0.01f;
                    spawnPos = hitPoint + nWorld * offAmt;
                } else {
                    spawnPos = cameraPos + cameraFront * 2.0f;
                }
            }

            if (brushGridSnap && brushGridSize > 0.0001f) {
                spawnPos.x = std::round(spawnPos.x / brushGridSize) * brushGridSize;
                spawnPos.y = std::round(spawnPos.y / brushGridSize) * brushGridSize;
                spawnPos.z = std::round(spawnPos.z / brushGridSize) * brushGridSize;
            }

            // Build transform: translate -> rotate -> scale (matches spawn logic)
            glm::mat4 previewT = glm::translate(glm::mat4(1.0f), spawnPos);
            previewT = glm::rotate(previewT, glm::radians(brushRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
            previewT = glm::rotate(previewT, glm::radians(brushRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
            previewT = glm::rotate(previewT, glm::radians(brushRotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

            glm::vec3 totalScale = glm::vec3(
                brushScale.x * brushSize,
                brushScale.y * brushSize,
                brushScale.z * brushSize);
            previewT = glm::scale(previewT, totalScale);

            glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT | GL_CURRENT_BIT);
            glDisable(GL_LIGHTING);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glColor4f(1.0f, 1.0f, 1.0f, 0.5f);

            glPushMatrix();
            glMultMatrixf(&previewT[0][0]);
            Object temp;
            temp.setGeometryType(currentPrimitive);
            temp.drawObject();
            glPopMatrix();

            glPopAttrib();
        }
        
        // ------------------------------------------------------------------
        // Draw the player avatar (skip in first-person to avoid visual clipping)
        // ------------------------------------------------------------------
        player.position = cameraPos; // keep avatar anchored to player logical position
        if (currentPerspective != PerspectiveMode::FirstPerson) {
            player.draw();
            player.drawNametag();
        }

        /* ________________________________________________

                                Menu

           ________________________________________________________ */

        if (glfwGetKey(win, GLFW_KEY_M) == GLFW_PRESS)
        {
            mainMenu.toggle(); // Open/close the menu with M
            firstMouse = true;
        }

        mainMenu.processInput(win);

        /* ________________________________________________________

                                Speed control

           ________________________________________________________ */

        // Reset to base
        speedMultiplier = 1.0f;

        // Define the momentary speed of the camera
        float actualSpeed = cameraSpeed * speedMultiplier;

        // Draw the cube before swapping buffers
        //world.getOwnedObjects()[0]->drawCube();

        if (!mainMenu.isOpen()) // Only process input if the menu is closed
        {
            // Only allow camera movement if cursor is locked
            if (cursorLocked) {
                // Move the camera with WASD
                // W = forward, S = backward, A = left, D = right
                // Use Shift to fly down
                // Use Space to fly up
                if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS)
                {
                    cameraPos += actualSpeed * cameraFront;
                }
                if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS)
                {
                    cameraPos -= actualSpeed * cameraFront;
                }
                if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS)
                {
                    cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * actualSpeed;
                }
                if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS)
                {
                    cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * actualSpeed;
                }

                // Anchor reset when not manual mode
                if(brushPlacementMode != BrushPlacementMode::ManualDistance){ manualAnchorValid = false; }

                // Manual offset adjustment for Brush placement
                if (brushPlacementMode == BrushPlacementMode::ManualDistance && current3DMode == Mode3D::BrushCreate) {
                    float step = 0.1f;
                    if (glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS) manualOffset.x += step;
                    if (glfwGetKey(win, GLFW_KEY_LEFT)  == GLFW_PRESS) manualOffset.x -= step;
                    if (glfwGetKey(win, GLFW_KEY_PAGE_UP) == GLFW_PRESS) manualOffset.y += step;
                    if (glfwGetKey(win, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS) manualOffset.y -= step;
                    if (glfwGetKey(win, GLFW_KEY_UP)   == GLFW_PRESS) manualOffset.z += step;
                    if (glfwGetKey(win, GLFW_KEY_DOWN) == GLFW_PRESS) manualOffset.z -= step;
                }

                // Hold Shift to fly down
                if (glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
                {
                    cameraPos -= actualSpeed * cameraUp;
                }

                // Hold V to sprint
                if (glfwGetKey(win, GLFW_KEY_V) == GLFW_PRESS)
                {
                    speedMultiplier = 2.5f;
                }

                // Fly Up (Space)
                if (glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS)
                    cameraPos += actualSpeed * cameraUp;

                // Slow Down (M Key)
                if (glfwGetKey(win, GLFW_KEY_M) == GLFW_PRESS)
                    speedMultiplier = 0.3f;
            }

            // Zone switching now handled via menu (F1-F12 entries)

            // Quick access to Character Design zone with C key
            bool cPressed = glfwGetKey(win, GLFW_KEY_C) == GLFW_PRESS;
            if (cPressed && !cPressedLastFrame) {
                // Iterate to find zone with "Character" in name
                for (size_t i = 0; i < mgr.zones().size(); ++i) {
                    if (mgr.zones()[i].name().find("Character") != std::string::npos) {
                        mgr.switchTo(i);
                        break;
                    }
                }
            }
            cPressedLastFrame = cPressed;

            // --------------------------------------------------
            // Design-Lock toggle with L key
            // --------------------------------------------------
            bool lPressed = glfwGetKey(win, GLFW_KEY_L) == GLFW_PRESS;
            if (lPressed && !lPressedLastFrame) {
                designLocked = !designLocked;
            }
            lPressedLastFrame = lPressed;

            // --------------------------------------------------
            // Chat window toggle with H key
            // --------------------------------------------------
            bool hPressed = glfwGetKey(win, GLFW_KEY_H) == GLFW_PRESS;
            if (hPressed && !hPressedLastFrame && !ImGui::IsAnyItemActive()) {
                showChatWindow = !showChatWindow;
            }
            hPressedLastFrame = hPressed;

            // Cursor lock/unlock with Escape
            bool escapePressed = glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS;
            if (escapePressed && !escapePressedLastFrame) {
                cursorLocked = !cursorLocked;
                if (cursorLocked) {
                    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                } else {
                    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                }
            }

            escapePressedLastFrame = escapePressed;
    
        }

        /* ________________________________________________________

                               Draw the Menu

        ________________________________________________________ */

        // Draw 2D overlay menu in orthographic projection
        glDisable(GL_DEPTH_TEST); // Disable depth test to draw overlay on top
        glViewport(0, 0, winW, winH);

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0, winW, winH, 0, -1, 1); // Top-left corner origin

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        // Draw the main menu in 2D overlay if open
        if (mainMenu.isOpen() && drawMode)
        {
            mainMenu.draw();
        }

        // Draw all art strokes in overlay coordinates
        mgr.active().renderArt();

        // Render the art toolbar
        if (showToolbar) {
            RenderArtToolbar();
        }

        // Render the chat window
        if (showChatWindow) {
            chat.renderUI();
        }

        // Character Designer panel (needs ImGui frame initialized)
        if (mgr.active().name().find("Character") != std::string::npos) {
            ImGui::Begin("Character Designer", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
            static BodyPart* selected = nullptr;
            ImGui::Checkbox("\xF0\x9F\x94\x92 Design Lock", &designLocked); // padlock icon
            ImGui::Separator();
            ImGui::Text("Body Parts:");
            for (auto* part : player.body.parts) {
                if (!part) continue;
                bool isSel = (part==selected);
                if (ImGui::Selectable(part->getName().c_str(), isSel)) {
                    selected = part;
                }
            }

            if (selected) {
                ImGui::Separator();
                ImGui::BeginDisabled(designLocked);
                ImGui::Text("Editing: %s", selected->getName().c_str());
                // Scale
                glm::vec3 dims = selected->getGeometry().getDimensions();
                float dimArr[3] = {dims.x,dims.y,dims.z};
                if (ImGui::SliderFloat3("Dimensions", dimArr, 0.05f, 1.0f) && !designLocked) {
                    selected->getGeometry().setDimensions({dimArr[0],dimArr[1],dimArr[2]});
                }
                // Color
                float col[3] = {selected->getColor()[0],selected->getColor()[1],selected->getColor()[2]};
                if(ImGui::ColorEdit3("Color", col) && !designLocked) {
                    selected->setColor(col[0],col[1],col[2]);
                }
                ImGui::EndDisabled();
            }
            ImGui::End();
        }

        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glEnable(GL_DEPTH_TEST); // Re-enable depth test

        /*if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS)
        {
            mgr.active().drawMode = true;
        }
        if (glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS)
        {
            mgr.active().drawMode = false;
        }
        if (glfwGetKey(win, GLFW_KEY_C) == GLFW_PRESS)
        {
            mgr.active().clearArt();
        }*/

        // RENDER -----------------------------------------------
        // mgr.active().applyTheme();
        // mgr.active().renderArt();

        glPushMatrix();
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        mgr.active().applyTheme();
        mgr.active().renderArt();
        glPopAttrib();
        glPopMatrix();

        // Render ImGui (NewFrame already called at loop start)

        // Block drawing if ImGui wants to capture the mouse
        if (!ImGui::GetIO().WantCaptureMouse) {
            // Handle mouse drawing
            bool mouseLeftCurrentlyPressed = glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
            float x = 0, y = 0;
            if (mouseLeftCurrentlyPressed || mouseLeftPressed) {
                double xpos, ypos;
                glfwGetCursorPos(win, &xpos, &ypos);
                x = xpos;
                y = ypos;
            }

            bool using3DMode = (current3DMode != Mode3D::None);

            if (!using3DMode) {
                // ----------------------- 2-D TOOLCHAIN -----------------------
                if (currentTool.getType() == Tool::Type::Brush) {
                    if (mouseLeftCurrentlyPressed && !mouseLeftPressed) {
                        mgr.active().startStroke(x, y);
                    } else if (mouseLeftCurrentlyPressed && mouseLeftPressed) {
                        mgr.active().continueStroke(x, y);
                    } else if (!mouseLeftCurrentlyPressed && mouseLeftPressed) {
                        mgr.active().endStroke();
                    }
                } else if (currentTool.getType() == Tool::Type::Eraser) {
                    if (mouseLeftCurrentlyPressed && !mouseLeftPressed) {
                        // Erase any stroke near the cursor
                        auto& zone = mgr.active();
                        float eraseRadius = 16.0f; // pixels
                        auto& strokes = zone.strokes;
                        for (auto it = strokes.begin(); it != strokes.end(); ) {
                            bool erased = false;
                            for (size_t i = 0; i + 1 < it->points.size(); i += 2) {
                                float px = it->points[i];
                                float py = it->points[i+1];
                                float dx = px - x;
                                float dy = py - y;
                                if (dx*dx + dy*dy < eraseRadius*eraseRadius) {
                                    it = strokes.erase(it);
                                    erased = true;
                                    break;
                                }
                            }
                            if (!erased) ++it;
                        }
                    }
                } else if (currentTool.getType() == Tool::Type::Shape) {
                    static float shapeStartX = 0, shapeStartY = 0;
                    static bool shapeDrawing = false;
                    if (mouseLeftCurrentlyPressed && !mouseLeftPressed) {
                        shapeStartX = x;
                        shapeStartY = y;
                        shapeDrawing = true;
                    } else if (!mouseLeftCurrentlyPressed && mouseLeftPressed && shapeDrawing) {
                        // Draw a straight line from shapeStart to (x, y)
                        mgr.active().startStroke(shapeStartX, shapeStartY);
                        mgr.active().continueStroke(x, y);
                        mgr.active().endStroke();
                        shapeDrawing = false;
                    }
                } else if (currentTool.getType() == Tool::Type::ColorPicker) {
                    if (mouseLeftCurrentlyPressed && !mouseLeftPressed) {
                        // Pick color from stroke under cursor
                        auto& zone = mgr.active();
                        float pickRadius = 12.0f;
                        for (const auto& stroke : zone.strokes) {
                            for (size_t i = 0; i + 1 < stroke.points.size(); i += 2) {
                                float px = stroke.points[i];
                                float py = stroke.points[i+1];
                                float dx = px - x;
                                float dy = py - y;
                                if (dx*dx + dy*dy < pickRadius*pickRadius) {
                                    currentColor[0] = stroke.r;
                                    currentColor[1] = stroke.g;
                                    currentColor[2] = stroke.b;
                                    mgr.active().setDrawColor(currentColor[0], currentColor[1], currentColor[2]);
                                    break;
                                }
                            }
                        }
                    }
                }
                // --------------------- END 2-D TOOLCHAIN --------------------
            } else {
                // ----------------------- 3-D TOOLCHAIN -----------------------
                if (current3DMode == Mode3D::BrushCreate) {
                    // Spawn cubes along the view direction
                    if (mouseLeftCurrentlyPressed && !mouseLeftPressed) {
                        glm::vec3 spawnPos;

                        if (brushPlacementMode == BrushPlacementMode::InFront) {
                            spawnPos = cameraPos + cameraFront * 2.0f;
                        } else if (brushPlacementMode == BrushPlacementMode::ManualDistance) {
                            glm::vec3 rightVec = glm::normalize(glm::cross(cameraFront, cameraUp));
                            spawnPos = cameraPos + rightVec * manualOffset.x + cameraUp * manualOffset.y + cameraFront * manualOffset.z;
                        } else {
                            // CursorSnap raycast from cursor
                            double cx = x; double cy = y;
                            GLdouble nearX, nearY, nearZ, farX, farY, farZ;
                            double winY = gCameraViewport[3] - cy;
                            gluUnProject(cx, winY, 0.0, gCameraModelview, gCameraProjection, gCameraViewport, &nearX, &nearY, &nearZ);
                            gluUnProject(cx, winY, 1.0, gCameraModelview, gCameraProjection, gCameraViewport, &farX,  &farY,  &farZ);
                            glm::vec3 rayO = glm::vec3(nearX, nearY, nearZ);
                            glm::vec3 rayDir = glm::normalize(glm::vec3(farX, farY, farZ) - rayO);

                            float nearestT = 1e9f; int hitAxis=-1; int hitSign=1; Object* hitObj=nullptr;
                            const auto& objs = world.getOwnedObjects();
                            for (const auto& up : objs) {
                                Object* obj = up.get();
                                glm::mat4 inv = glm::inverse(obj->getTransform());
                                glm::vec3 oL = glm::vec3(inv * glm::vec4(rayO, 1.0f));
                                glm::vec3 dL = glm::normalize(glm::vec3(inv * glm::vec4(rayDir, 0.0f)));
                                float tMin=-1e9,tMax=1e9; int axis=-1; int sign=1;
                                for(int a=0;a<3;++a){float o=oL[a],d=dL[a];float t1,t2;if(fabs(d)<1e-6){if(o<-0.5||o>0.5){tMin=1e9;break;}t1=-1e9;t2=1e9;}else{t1=(-0.5-o)/d; t2=(0.5-o)/d;} if(t1>tMin){tMin=t1; axis=a; sign=(d>0?-1:1);} if(t2<tMax) tMax=t2; if(tMin>tMax){tMin=1e9;break;}}
                                if(tMin<nearestT && tMin>0 && tMin<1e8){ nearestT = tMin; hitAxis=axis; hitSign=sign; hitObj=obj; }
                            }
                            if (nearestT < 1e8f && hitObj) {
                                glm::vec3 hitPoint = rayO + rayDir * nearestT;
                                glm::vec3 nLocal(0.0f); nLocal[hitAxis] = static_cast<float>(hitSign);
                                glm::vec3 nWorld = glm::normalize(glm::vec3(hitObj->getTransform() * glm::vec4(nLocal,0.0f)));
                                glm::vec3 half = glm::vec3(brushScale.x * brushSize, brushScale.y * brushSize, brushScale.z * brushSize) * 0.5f;
                                float offAmt = glm::dot(glm::abs(nWorld), half) + 0.01f;
                                spawnPos = hitPoint + nWorld * offAmt;
                            } else {
                                spawnPos = cameraPos + cameraFront * 2.0f;
                            }
                        }

                        // Optional grid snapping for precision placement
                        if (brushGridSnap && brushGridSize > 0.0001f) {
                            spawnPos.x = std::round(spawnPos.x / brushGridSize) * brushGridSize;
                            spawnPos.y = std::round(spawnPos.y / brushGridSize) * brushGridSize;
                            spawnPos.z = std::round(spawnPos.z / brushGridSize) * brushGridSize;
                        }

                        std::unique_ptr<Object> obj(new Object());
                        obj->setGeometryType(currentPrimitive);

                        // Build transform: translate -> rotate -> scale
                        glm::mat4 transform = glm::translate(glm::mat4(1.0f), spawnPos);

                        // Rotation (XYZ Euler)
                        transform = glm::rotate(transform, glm::radians(brushRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
                        transform = glm::rotate(transform, glm::radians(brushRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
                        transform = glm::rotate(transform, glm::radians(brushRotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

                        // Scale combines uniform brushSize and per-axis brushScale
                        glm::vec3 totalScale = glm::vec3(
                            brushScale.x * brushSize,
                            brushScale.y * brushSize,
                            brushScale.z * brushSize);
                        transform = glm::scale(transform, totalScale);

                        obj->setTransform(transform);

                        // Apply the currently selected color to all faces (works for all primitives)
                        for (int f = 0; f < 6; ++f) {
                            obj->setFaceColor(f, currentColor[0], currentColor[1], currentColor[2]);
                        }

                        world.addOwnedObject(std::move(obj));
                    }
                } else if (current3DMode == Mode3D::FacePaint) {
                    // Paint the face of the first cube hit by a ray
                    if (mouseLeftCurrentlyPressed && !mouseLeftPressed) {
                        float nearestT = 1e9f;
                        Object* hitObj = nullptr;
                        int hitFace = -1;

                        // Build picking ray from current mouse position
                        GLdouble nearX, nearY, nearZ;
                        GLdouble farX,  farY,  farZ;
                        double winY = gCameraViewport[3] - y;
                        gluUnProject(x, winY, 0.0, gCameraModelview, gCameraProjection, gCameraViewport, &nearX, &nearY, &nearZ);
                        gluUnProject(x, winY, 1.0, gCameraModelview, gCameraProjection, gCameraViewport, &farX,  &farY,  &farZ);

                        glm::vec3 rayO = glm::vec3(nearX, nearY, nearZ);
                        glm::vec3 rayFar = glm::vec3(farX,  farY,  farZ);
                        glm::vec3 rayD = glm::normalize(rayFar - rayO);

                        const auto& objects = world.getOwnedObjects();
                        for (const auto& uptr : objects) {
                            Object* obj = uptr.get();

                            // Inverse transform to bring ray into the cube's local space
                            glm::mat4 invM = glm::inverse(obj->getTransform());
                            glm::vec3 oLocal = glm::vec3(invM * glm::vec4(rayO, 1.0f));
                            glm::vec3 dLocal = glm::normalize(glm::vec3(invM * glm::vec4(rayD, 0.0f)));

                            // Robust ray-AABB intersection to determine entry face
                            const float INF = std::numeric_limits<float>::infinity();
                            float tEntry = -INF, tExit = INF;
                            int entryAxis = -1; // 0 x,1 y,2 z
                            int entrySign = 0;  // +1 or -1 to map face

                            for (int axis = 0; axis < 3; ++axis) {
                                float o = oLocal[axis];
                                float d = dLocal[axis];
                                float t1, t2;
                                if (fabs(d) < 1e-6f) {
                                    if (o < -0.5f || o > 0.5f) { tEntry = INF; break; } // parallel & outside
                                    else { t1 = -INF; t2 = INF; }
                                } else {
                                    t1 = (-0.5f - o) / d;
                                    t2 = ( 0.5f - o) / d;
                                }
                                if (t1 > t2) std::swap(t1, t2);
                                if (t1 > tEntry) { tEntry = t1; entryAxis = axis; entrySign = (d > 0 ? -1 : 1); }
                                if (t2 < tExit)  tExit  = t2;
                                if (tEntry > tExit) { tEntry = INF; break; }
                            }

                            if (tEntry < nearestT && tEntry > 0.0f && tEntry < INF) {
                                nearestT = tEntry;
                                hitObj = obj;
                                // Map axis/sign to face index
                                if (entryAxis == 0) hitFace = (entrySign > 0 ? 0 : 1); // +X / -X
                                else if (entryAxis == 1) hitFace = (entrySign > 0 ? 2 : 3); // +Y / -Y
                                else if (entryAxis == 2) hitFace = (entrySign > 0 ? 4 : 5); // +Z / -Z
                            }
                        }

                        if (hitObj && hitFace >= 0) {
                            hitObj->setFaceColor(hitFace, currentColor[0], currentColor[1], currentColor[2]);
                        }
                    }
                }
                // --------------------- END 3-D TOOLCHAIN --------------------
            }
            mouseLeftPressed = mouseLeftCurrentlyPressed;
        }
        
        ImGui::Render();
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(win);

        // Each frame, after input and before rendering:
        world.onUpdate();
    }

    // Cleanup ImGui
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(win);
    glfwTerminate();
    LogStateSummary("After LoadState");
    return 0;
}

void SaveState(const std::string& filename) {
    json j;
    // Camera
    j["cameraPos"] = {cameraPos.x, cameraPos.y, cameraPos.z};
    j["cameraFront"] = {cameraFront.x, cameraFront.y, cameraFront.z};
    j["cameraUp"] = {cameraUp.x, cameraUp.y, cameraUp.z};
    j["yaw"] = yaw;
    j["pitch"] = pitch;
    j["currentZone"] = mgr.currentIndex();
    j["currentColor"] = {currentColor[0], currentColor[1], currentColor[2]};
    j["currentTool"] = static_cast<int>(currentTool.getType());

    // World settings
    j["worldMode"]        = static_cast<int>(world.getMode());
    j["worldPhysics"]     = world.isPhysicsEnabled();

    // Zones and strokes
    json zonesArr = json::array();
    for (const auto& zone : mgr.zones()) {
        json zoneJ;
        zoneJ["strokes"] = json::array();
        for (const auto& stroke : zone.strokes) {
            json s;
            s["points"] = stroke.points;
            s["color"] = {stroke.r, stroke.g, stroke.b};
            zoneJ["strokes"].push_back(s);
        }
        zonesArr.push_back(zoneJ);
    }
    j["zones"] = zonesArr;

    // Dynamic objects (skip baseline cube & ground at indices 0 and 1)
    json objectsArr = json::array();
    const auto& objVec = world.getOwnedObjects();
    for (size_t i = 2; i < objVec.size(); ++i) {
        const auto& obj = objVec[i];
        json objJ;
        // Transform 16 floats column-major
        const glm::mat4& T = obj->getTransform();
        std::vector<float> tvals(16);
        const float* ptr = &T[0][0];
        for (int k=0;k<16;++k) tvals[k]=ptr[k];
        objJ["transform"] = tvals;
        // Face colors
        json faces = json::array();
        for (int f=0; f<6; ++f) {
            faces.push_back({obj->faceColors[f][0], obj->faceColors[f][1], obj->faceColors[f][2]});
        }
        objJ["faceColors"] = faces;
        objectsArr.push_back(objJ);
    }
    j["objects"] = objectsArr;

    std::ofstream out(filename);
    out << j.dump(2);
    LogStateSummary("After SaveState");
}

void LoadState(const std::string& filename) {
    std::ifstream in(filename);
    if (!in) {
        std::cout << "[DEBUG] LoadState could not open " << filename << std::endl;
        return;
    }
    std::cout << "[DEBUG] LoadState opened " << filename << std::endl;
    json j;
    in >> j;
    // Camera
    auto cam = j["cameraPos"];
    cameraPos = glm::vec3(cam[0], cam[1], cam[2]);
    auto front = j["cameraFront"];
    cameraFront = glm::vec3(front[0], front[1], front[2]);
    auto up = j["cameraUp"];
    cameraUp = glm::vec3(up[0], up[1], up[2]);
    yaw = j["yaw"];
    pitch = j["pitch"];
    auto col = j["currentColor"];
    currentColor[0] = col[0];
    currentColor[1] = col[1];
    currentColor[2] = col[2];
    currentTool = Tool(static_cast<Tool::Type>(j["currentTool"].get<int>()));

    // World settings
    if (j.contains("worldMode")) {
        world.setMode(static_cast<Ourverse::GameMode>(j["worldMode"].get<int>()));
    }
    if (j.contains("worldPhysics")) {
        bool enabled = j["worldPhysics"].get<bool>();
        if (world.isPhysicsEnabled() != enabled) world.togglePhysics();
    }

    // Zones and strokes
    if (j.contains("zones")) {
        auto zonesArr = j["zones"];
        auto& zonesVec = mgr.zones();
        size_t count = std::min(zonesArr.size(), zonesVec.size());
        for (size_t z = 0; z < count; ++z) {
            zonesVec[z].strokes.clear();
            for (const auto& s : zonesArr[z]["strokes"]) {
                Zone::Stroke stroke;
                stroke.points = s["points"].get<std::vector<float>>();
                auto c = s["color"];
                stroke.r = c[0]; stroke.g = c[1]; stroke.b = c[2];
                zonesVec[z].strokes.push_back(stroke);
            }
        }
    }

    // Dynamic objects
    if (j.contains("objects")) {
        world.clearDynamicObjects();
        for (const auto& objJ : j["objects"]) {
            std::unique_ptr<Object> newObj(new Object());
            if (objJ.contains("transform")) {
                std::vector<float> tvals = objJ["transform"].get<std::vector<float>>();
                if (tvals.size()==16) {
                    glm::mat4 T;
                    float* ptrT = &T[0][0];
                    for (int k=0;k<16;++k) ptrT[k]=tvals[k];
                    newObj->setTransform(T);
                }
            }
            if (objJ.contains("faceColors")) {
                auto faces = objJ["faceColors"];
                for (int f=0; f<6; ++f) {
                    auto c = faces[f];
                    newObj->setFaceColor(f, c[0], c[1], c[2]);
                }
            }
            world.addOwnedObject(std::move(newObj));
        }
    }

    // Restore current zone if possible
    size_t savedCurrent = j.value("currentZone", 0u);
    mgr.switchTo(savedCurrent);
    LogStateSummary("After LoadState");
}

void SaveAvatar(const std::string& filename) {
    json j;
    j["meta"]["filename"] = filename;
    // timestamp
    std::time_t t = std::time(nullptr);
    j["meta"]["timestamp"] = (long long)t;
    j["personName"] = player.getSoulName();

    json parts = json::array();
    for (auto* part : player.body.parts) {
        if (!part) continue;
        json p;
        p["name"] = part->getName();
        auto dims = part->getGeometry().getDimensions();
        p["dims"] = {dims.x, dims.y, dims.z};
        auto col = part->getColor();
        p["color"] = {col[0], col[1], col[2]};
        // 16 float transform
        const glm::mat4& T = part->getTransform();
        std::vector<float> tvals(16);
        const float* ptrT = &T[0][0];
        for(int k=0;k<16;++k) tvals[k]=ptrT[k];
        p["transform"] = tvals;
        parts.push_back(p);
    }
    j["parts"] = parts;

    std::ofstream out(filename);
    out << j.dump(2);
}

void LoadAvatar(const std::string& filename) {
    std::ifstream in(filename);
    if (!in) return;
    json j; in >> j;
    if (!j.contains("parts")) return;
    auto partsArr = j["parts"];
    // Map by name to existing parts
    for (auto* part : player.body.parts) {
        if (!part) continue;
        for (const auto& pj : partsArr) {
            if (pj["name"].get<std::string>() == part->getName()) {
                // dims
                auto d = pj["dims"];
                part->getGeometry().setDimensions({d[0], d[1], d[2]});
                // color
                auto c = pj["color"];
                part->setColor(c[0], c[1], c[2]);
                // transform optional
                if (pj.contains("transform")) {
                    std::vector<float> tvals = pj["transform"].get<std::vector<float>>();
                    if (tvals.size()==16) {
                        glm::mat4 T;
                        float* ptrT = &T[0][0];
                        for(int k=0;k<16;++k) ptrT[k]=tvals[k];
                        part->setTransform(T);
                    }
                }
                break;
            }
        }
    }
}

void SaveStateWithLog(const std::string& customName) {
    // Generate filename with timestamp and optional custom suffix
    std::string filename;
    std::time_t t = std::time(nullptr);
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream ts;
    ts << "save_" << (tm.tm_year+1900)
       << (tm.tm_mon+1)
       << (tm.tm_mday)
       << "_" << (tm.tm_hour)
       << (tm.tm_min)
       << (tm.tm_sec);

    if (!customName.empty()) {
        filename = ts.str() + "_" + customName + ".json";
    } else {
        filename = ts.str() + ".json";
    }

    // Perform actual save
    SaveState(filename);

    // Prepend to save log so the newest entry appears first
    std::ifstream in("save_log.txt");
    std::string oldLog((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    std::ofstream out("save_log.txt");
    out << filename << "\n" << oldLog;
    out.close();

    UpdateSaveFiles();
}

void SaveAvatarWithLog(const std::string& customName) {
    // Generate filename
    std::string filename;
    // Timestamp base name regardless of custom name
    std::time_t t = std::time(nullptr);
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream ts;
    ts << "avatar_" << (tm.tm_year+1900)
       << (tm.tm_mon+1)
       << (tm.tm_mday)
       << "_" << (tm.tm_hour)
       << (tm.tm_min)
       << (tm.tm_sec);

    if (!customName.empty()) {
        filename = ts.str() + "_" + customName + ".json";
    } else {
        filename = ts.str() + ".json";
    }

    SaveAvatar(filename);

    // Prepend to avatar log
    std::ifstream in("avatar_log.txt");
    std::string oldLog((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();
    std::ofstream out("avatar_log.txt");
    out << filename << "\n" << oldLog;
    out.close();
    UpdateAvatarFiles();
}