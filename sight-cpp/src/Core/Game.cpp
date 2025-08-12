#include "Game.hpp"
#include "Core/Engine.hpp"
#include "../../imgui/backends/imgui_impl_glfw.h"
#include "Form/Object/Object.hpp"
#include "Rendering/BrushSystem.hpp"
#include "Rendering/DesignSystem.hpp"
#include "Form/Object/Formation/Formations.hpp"
#include "OurVerse/Tool.hpp"
#include "OurVerse/Chat.hpp"
#include "OurVerse/AdvancedFacePaint.hpp"
#include "Person/Person.hpp"
#include "Person/AvatarManager.hpp"
#include "Rendering/ShadingSystem.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include "Rendering/HighlightSystem.hpp"
#include "ZonesOfEarth/Ourverse/Ourverse.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "Perspective/PerspectiveManager.hpp"
#include "Util/SaveSystem.hpp"
#include "Util/Serialization.hpp"
#include "Person/Body/BodyPart/BodyPart.hpp"
#include "Integration/IntegrationManager.hpp"

#include <GLFW/glfw3.h>
#include <OpenGL/glu.h>
#include <imgui.h>
#include "../../imgui/backends/imgui_impl_glfw.h"
#include "../../imgui/backends/imgui_impl_opengl2.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <memory>
#include <limits>
#include <ctime>
#include <filesystem>

extern ZoneManager mgr;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using glm::vec3;

namespace Core {

Game::Game()
    : _elementalToolHandler(&mgr) {}

Game::~Game() {
    printf("[Shutdown] Game dtor\n");
    
    // Cleanup Advanced Face Paint System
    AdvancedFacePaint::cleanupAdvancedPainter();
}

bool Game::init() {
    _window = Engine::instance().window();
    if (!_window) return false;

    // Init GL state – depth test already enabled in ShadingSystem::init()
    ShadingSystem::init();

    printf("[Init] Checkpoint A: after ShadingSystem::init()\n");

    // --------------------------------------------------------------
    // Setup zones & menu
    // --------------------------------------------------------------
    mgr.addZone(Zone("Sanctum of Beginnings"));
    mgr.addZone(Zone("Temple of Echoes"));
    mgr.addZone(Zone("Cavern of Light"));
    mgr.addZone(Zone("Character Architect Forge"));
    
    // Initialize elemental tool handler with zone manager
    _elementalToolHandler = ElementalToolHandler(&mgr);

    printf("[Init] Checkpoint B: zones added (%zu)\n", mgr.zones().size());
    // Debug: list zone names to validate memory integrity of strings
    for (size_t i = 0; i < mgr.zones().size(); ++i) {
        const auto& z = mgr.zones()[i];
        printf("[Init] Zone[%zu]: %s | Q=%zu D=%zu\n", i, z.name().c_str(), z.getQualities().size(), z.getDeletability().size());
    }
    fflush(stdout);

    // Reset menu to ensure a clean state in case any earlier corruption occurred
    _mainMenu = Menu();
    printf("[Init] Menu reset before adding options\n");

    // Populate menu options
    printf("[Init] Checkpoint B1: before menu addOption(Resume)\n");
    _mainMenu.addOption("Resume World", GLFW_KEY_R, [this]() { _mainMenu.close(); });
    printf("[Init] Checkpoint B2: after menu addOption(Resume)\n");

    // Enhanced main menu options (non-destructive; all previous features intact)
    _mainMenu.addOption("Quick Save", GLFW_KEY_S, [this]() { saveStateWithLog(); });
    _mainMenu.addOption("Save As...", GLFW_KEY_A, [this]() { _showSaveWindow = true; });
    _mainMenu.addOption("Load", GLFW_KEY_L, [this]() { updateSaveFiles(); _showLoadWindow = true; });
    _mainMenu.addOption("Save Manager", GLFW_KEY_G, [this]() { _showSaveManager = true; });
    _mainMenu.addOption("Toggle Chat", GLFW_KEY_H, [this]() { _showChatWindow = !_showChatWindow; });
    _mainMenu.addOption("Toggle Toolbar", GLFW_KEY_T, [this]() { _showToolbar = !_showToolbar; });
    _mainMenu.addOption("Toggle Physics", GLFW_KEY_P, [this]() { _world.togglePhysics(); });
    _mainMenu.addOption("Controls / Keymap", GLFW_KEY_K, [this]() { _showKeymapWindow = true; });
    _mainMenu.addOption("Character Architect Forge", GLFW_KEY_C, [this]() {
        const auto& zones = mgr.zones();
        for (size_t i = 0; i < zones.size(); ++i) {
            if (zones[i].name().find("Character") != std::string::npos) {
                mgr.switchTo(i);
                break;
            }
        }
    });

    _mainMenu.addOption("Quit",   GLFW_KEY_Q, [this]() { glfwSetWindowShouldClose(_window, 1); });
    printf("[Init] Checkpoint B3: after menu addOption(Quit)\n");

    // Continue to next phase for bisection

    // --------------------------------------------------------------
    // World baseline objects (spinning cube + ground)
    // --------------------------------------------------------------
    _world.setCamera(&_cameraPos);

    {
        std::unique_ptr<Object> cube(new Object());
        std::unique_ptr<Object> ground(new Object());
        // Tag these as baseline placeholders so we can safely special-case them later
        cube->setAttribute("baseline", "cube");
        ground->setAttribute("baseline", "ground");
        mgr.active().world().addObject(std::move(cube));
        mgr.active().world().addObject(std::move(ground));
    }

    // Defensive: ensure active world owns objects before any polyhedron generation that may depend on it

    printf("[Init] Checkpoint C: baseline objects created\n");

    // Physics default true
    _world.setMode(Ourverse::GameMode::Creative);

    // Ensure _player initial position matches _cameraPos
    glm::vec3 anchor = _cameraPos - glm::vec3(0.0f, _player.getBody().getEyeHeight(), 0.0f);
    _player.position = anchor;
    _player.updatePose();

    // --------------------------------------------------------------
    // Register GLFW callbacks that need 'this' pointer via user pointer
    // --------------------------------------------------------------
    registerCallbacks();
    printf("[Init] Checkpoint D: callbacks registered\n");

    // Continue to next phase for bisection

    std::cout << "🔥 Earthcall Game initialised." << std::endl;

    // Initialize custom polyhedron
    try {
        _generateCustomPolyhedron();
        printf("[Init] Checkpoint E: custom polyhedron generated\n");
    } catch (...) {
        printf("[Init] Warning: custom polyhedron generation failed, continuing with defaults.\n");
    }

    // Initialize default brush presets
    _brushPresets.clear();

    // Fluent builder for self-documenting initialization
    _brushPresets.push_back(PresetBuilder("Soft Brush", BrushType::Normal)
        .radius(0.15f).softness(0.3f).opacity(0.7f).flow(0.8f).spacing(0.05f).density(0.5f).strength(0.5f).build());
    _brushPresets.push_back(PresetBuilder("Hard Brush", BrushType::Normal)
        .radius(0.1f).softness(1.0f).opacity(1.0f).flow(1.0f).spacing(0.02f).density(0.5f).strength(0.5f).build());
    _brushPresets.push_back(PresetBuilder("Airbrush", BrushType::Airbrush)
        .radius(0.2f).softness(0.5f).opacity(0.5f).flow(0.6f).spacing(0.1f).density(0.8f).strength(0.5f).build());
    _brushPresets.push_back(PresetBuilder("Chalk", BrushType::Chalk)
        .radius(0.12f).softness(0.2f).opacity(0.9f).flow(0.7f).spacing(0.08f).density(0.5f).strength(0.5f).build());
    _brushPresets.push_back(PresetBuilder("Smudge", BrushType::Smudge)
        .radius(0.18f).softness(0.4f).opacity(1.0f).flow(1.0f).spacing(0.03f).density(0.5f).strength(0.7f).build());
    _brushPresets.push_back(PresetBuilder("Clone", BrushType::Clone)
        .radius(0.15f).softness(0.6f).opacity(0.8f).flow(1.0f).spacing(0.05f).density(0.5f).strength(0.5f).build());

    // Previous aggregate init kept for reference
    /*
    _brushPresets.push_back(BrushPreset{"Soft Brush", BrushType::Normal, 0.15f, 0.3f, 0.7f, 0.8f, 0.05f, 0.5f, 0.5f});
    _brushPresets.push_back(BrushPreset{"Hard Brush", BrushType::Normal, 0.1f, 1.0f, 1.0f, 1.0f, 0.02f, 0.5f, 0.5f});
    _brushPresets.push_back(BrushPreset{"Airbrush", BrushType::Airbrush, 0.2f, 0.5f, 0.5f, 0.6f, 0.1f, 0.8f, 0.5f});
    _brushPresets.push_back(BrushPreset{"Chalk", BrushType::Chalk, 0.12f, 0.2f, 0.9f, 0.7f, 0.08f, 0.5f, 0.5f});
    _brushPresets.push_back(BrushPreset{"Smudge", BrushType::Smudge, 0.18f, 0.4f, 1.0f, 1.0f, 0.03f, 0.5f, 0.7f});
    _brushPresets.push_back(BrushPreset{"Clone", BrushType::Clone, 0.15f, 0.6f, 0.8f, 1.0f, 0.05f, 0.5f, 0.5f});
    */

    // Previous verbose initialization kept for reference
    /*
    // Refactor and de-cultter. All these variables should be initialized with constructor instead of spending this many lines.
    // Soft Brush 
    BrushPreset softBrush;
    softBrush.name = "Soft Brush";
    softBrush.type = BrushType::Normal;
    softBrush.radius = 0.15f;
    softBrush.softness = 0.3f;
    softBrush.opacity = 0.7f;
    softBrush.flow = 0.8f;
    softBrush.spacing = 0.05f;
    softBrush.density = 0.5f;
    softBrush.strength = 0.5f;
    _brushPresets.push_back(softBrush);
    
    // Hard Brush
    BrushPreset hardBrush;
    hardBrush.name = "Hard Brush";
    hardBrush.type = BrushType::Normal;
    hardBrush.radius = 0.1f;
    hardBrush.softness = 1.0f;
    hardBrush.opacity = 1.0f;
    hardBrush.flow = 1.0f;
    hardBrush.spacing = 0.02f;
    hardBrush.density = 0.5f;
    hardBrush.strength = 0.5f;
    _brushPresets.push_back(hardBrush);
    
    // Airbrush
    BrushPreset airbrush;
    airbrush.name = "Airbrush";
    airbrush.type = BrushType::Airbrush;
    airbrush.radius = 0.2f;
    airbrush.softness = 0.5f;
    airbrush.opacity = 0.5f;
    airbrush.flow = 0.6f;
    airbrush.spacing = 0.1f;
    airbrush.density = 0.8f;
    airbrush.strength = 0.5f;
    _brushPresets.push_back(airbrush);
    
    // Chalk
    BrushPreset chalk;
    chalk.name = "Chalk";
    chalk.type = BrushType::Chalk;
    chalk.radius = 0.12f;
    chalk.softness = 0.2f;
    chalk.opacity = 0.9f;
    chalk.flow = 0.7f;
    chalk.spacing = 0.08f;
    chalk.density = 0.5f;
    chalk.strength = 0.5f;
    _brushPresets.push_back(chalk);
    
    // Smudge
    BrushPreset smudge;
    smudge.name = "Smudge";
    smudge.type = BrushType::Smudge;
    smudge.radius = 0.18f;
    smudge.softness = 0.4f;
    smudge.opacity = 1.0f;
    smudge.flow = 1.0f;
    smudge.spacing = 0.03f;
    smudge.density = 0.5f;
    smudge.strength = 0.7f;
    _brushPresets.push_back(smudge);
    
    // Clone
    BrushPreset clone;
    clone.name = "Clone";
    clone.type = BrushType::Clone;
    clone.radius = 0.15f;
    clone.softness = 0.6f;
    clone.opacity = 0.8f;
    clone.flow = 1.0f;
    clone.spacing = 0.05f;
    clone.density = 0.5f;
    clone.strength = 0.5f;
    _brushPresets.push_back(clone);
    */

    // Initialize Advanced Face Paint System
    AdvancedFacePaint::initializeAdvancedPainter();
    
    // Initialize default advanced face paint settings
    _currentGradientSettings = AdvancedFacePaint::GradientSettings();
    _currentSmudgeSettings = AdvancedFacePaint::SmudgeSettings();

    // Full pipeline restored (integration disabled)
    //return true;

    // Initialize integration system (TEMP DISABLED for isolation)
    // Integration::IntegrationManager::instance().init();

    // Initialize keyboard handler
    _keyboardHandler.setGameInstance(this);
    // Don't call setupGameBindings() as we're setting up specific bindings below
    
    // Initialize mouse handler
    _mouseHandler.setGameInstance(this);
    
    // Set up specific game callbacks
    _keyboardHandler.bindKey(GLFW_KEY_M, "toggle_menu", [this]() { 
        _mainMenu.toggle(); 
        _mouseHandler.setMenuOpen(_mainMenu.isOpen());
    });
    _keyboardHandler.bindKey(GLFW_KEY_ESCAPE, "toggle_cursor_lock", [this]() { 
        _mouseHandler.toggleCursorLock(_window);
    });
    _keyboardHandler.bindKey(GLFW_KEY_H, "toggle_chat", [this]() { _showChatWindow = !_showChatWindow; });
    _keyboardHandler.bindKey(GLFW_KEY_I, "toggle_integration_ui", [this]() { _showIntegrationUI = !_showIntegrationUI; });
    _keyboardHandler.bindKey(GLFW_KEY_T, "toggle_toolbar", [this]() { _showToolbar = !_showToolbar; });
    _keyboardHandler.bindKey(GLFW_KEY_1, "perspective_first_person", [this]() { _currentPerspective = PerspectiveMode::FirstPerson; });
    _keyboardHandler.bindKey(GLFW_KEY_2, "perspective_second_person", [this]() { _currentPerspective = PerspectiveMode::SecondPerson; });
    _keyboardHandler.bindKey(GLFW_KEY_3, "perspective_third_person", [this]() { _currentPerspective = PerspectiveMode::ThirdPerson; });
    _keyboardHandler.bindKey(GLFW_KEY_F, "toggle_flight", [this]() { 
        if (_world.getMode() != Ourverse::GameMode::Survival) {
            Physics::toggleFlying();
        }
    });
    _keyboardHandler.bindKey(GLFW_KEY_C, "switch_to_character_zone", [this]() {
        const auto& zones = mgr.zones();
        for (size_t i = 0; i < zones.size(); ++i) {
            if (zones[i].name().find("Character") != std::string::npos) {
                mgr.switchTo(i);
                break;
            }
        }
    });
    _keyboardHandler.bindKey(GLFW_KEY_O, "toggle_avatar_demo", [this]() {
        _showAvatarDemo = !_showAvatarDemo;
        if (_showAvatarDemo) {
            // Initialize demo avatars if not already created
            if (_avatarManager.getTotalAvatars() == 0) {
                _avatarManager.createAvatar("Demo Alice", "Voxel");
                _avatarManager.createAvatar("Demo Bob", "Voxel");
                _avatarManager.createChildAvatar("Demo Child");
                _avatarManager.createElderAvatar("Demo Elder");
                
                // Position them around the player
                Person* alice = _avatarManager.getAvatar("Demo Alice");
                Person* bob = _avatarManager.getAvatar("Demo Bob");
                Person* child = _avatarManager.getAvatar("Demo Child");
                Person* elder = _avatarManager.getAvatar("Demo Elder");
                
                if (alice) alice->position = _player.position + glm::vec3(3.0f, 0.0f, 0.0f);
                if (bob) bob->position = _player.position + glm::vec3(-3.0f, 0.0f, 0.0f);
                if (child) child->position = _player.position + glm::vec3(0.0f, 0.0f, 3.0f);
                if (elder) elder->position = _player.position + glm::vec3(0.0f, 0.0f, -3.0f);
            }
        }
    });
    // Debug toggles for gravity field visualization and law enable
    _keyboardHandler.bindKey(GLFW_KEY_F6, "toggle_gravity_viz", [this]() {
        bool v = Physics::getGravityVisualization();
        Physics::setGravityVisualization(!v);
    });
    _keyboardHandler.bindKey(GLFW_KEY_F7, "toggle_gravity_field", [this]() {
        // Toggle the first GravityField law if present; otherwise create one targeting all objects
        const auto& lawsRef = Physics::getLaws();
        Physics::PhysicsLaw* gf = nullptr;
        for (auto& L : const_cast<std::vector<Physics::PhysicsLaw>&>(lawsRef)) {
            if (L.type == Physics::LawType::GravityField) { gf = &L; break; }
        }
        if (gf) {
            gf->enabled = !gf->enabled;
        } else {
            Physics::PhysicsLaw newLaw; newLaw.name = "Gravity Field"; newLaw.type = Physics::LawType::GravityField; newLaw.enabled = true; newLaw.target.allObjects = true; Physics::addLaw(newLaw);
        }
    });
    _keyboardHandler.bindKey(GLFW_KEY_Z, "undo", [this]() {
        // Undo last stroke
        if (_current3DMode == Mode3D::FaceBrush) {
            // Find the object under cursor and undo its last stroke
            const auto& objects = mgr.active().world().getOwnedObjects();
            if (!objects.empty()) {
                for (const auto& up : objects) {
                    Object* obj = up.get();
                    if (obj) {
                        // For now, undo the last stroke on the first face
                        obj->undoStroke(0);
                        break;
                    }
                }
            }
        }
    });
    _keyboardHandler.bindKey(GLFW_KEY_Y, "redo", [this]() {
        // Redo last undone stroke
        if (_current3DMode == Mode3D::FaceBrush) {
            // Redo functionality would be implemented here
        }
    });
    
    // Camera movement bindings (these are handled in the update loop for continuous movement)
    _keyboardHandler.bindKey(GLFW_KEY_W, "camera_forward", [](){});
    _keyboardHandler.bindKey(GLFW_KEY_S, "camera_backward", [](){});
    _keyboardHandler.bindKey(GLFW_KEY_A, "camera_left", [](){});
    _keyboardHandler.bindKey(GLFW_KEY_D, "camera_right", [](){});
    _keyboardHandler.bindKey(GLFW_KEY_LEFT_SHIFT, "camera_down", [](){});
    _keyboardHandler.bindKey(GLFW_KEY_SPACE, "camera_up", [](){});
    _keyboardHandler.bindKey(GLFW_KEY_V, "camera_sprint", [](){});
    _keyboardHandler.bindKey(GLFW_KEY_LEFT_ALT, "camera_slow", [](){});
    
    // Manual offset controls
    _keyboardHandler.bindKey(GLFW_KEY_RIGHT, "manual_offset_right", [](){});
    _keyboardHandler.bindKey(GLFW_KEY_LEFT, "manual_offset_left", [](){});
    _keyboardHandler.bindKey(GLFW_KEY_PAGE_UP, "manual_offset_up", [](){});
    _keyboardHandler.bindKey(GLFW_KEY_PAGE_DOWN, "manual_offset_down", [](){});
    _keyboardHandler.bindKey(GLFW_KEY_UP, "manual_offset_forward", [](){});
    _keyboardHandler.bindKey(GLFW_KEY_DOWN, "manual_offset_backward", [](){});

    // Save / load ----------------------------------------------------------

    return true;
}

void Game::registerCallbacks() {
    if (!_window) return;
    glfwSetWindowUserPointer(_window, this);

    // Store previous callbacks (likely ImGui's) so we can forward events
    _prevCursorPosCallback = glfwSetCursorPosCallback(_window, &Game::sMouseCallback);
    _prevFocusCallback     = glfwSetWindowFocusCallback(_window, &Game::sWindowFocusCallback);
    _prevFramebufferSizeCallback = glfwSetFramebufferSizeCallback(_window, &Game::sFramebufferSizeCallback);
    
    // Set up mouse button callback that forwards to ImGui first, then our handler
    glfwSetMouseButtonCallback(_window, [](GLFWwindow* window, int button, int action, int mods) {
        Game* self = static_cast<Game*>(glfwGetWindowUserPointer(window));
        if (self) {
            // Forward to ImGui first (critical for UI interactions)
            ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
            // Then handle game-specific mouse input
            self->_mouseHandler.handleMouseButton(button, action, mods);
        }
    });
    
    // Set up scroll callback that forwards to ImGui first, then our handler
    glfwSetScrollCallback(_window, [](GLFWwindow* window, double xoffset, double yoffset) {
        Game* self = static_cast<Game*>(glfwGetWindowUserPointer(window));
        if (self) {
            // Forward to ImGui first (critical for UI scrolling)
            ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
            // Then handle game-specific scroll input
            self->_mouseHandler.handleMouseScroll(xoffset, yoffset);
        }
    });
}

void Game::sMouseCallback(GLFWwindow* win, double xpos, double ypos) {
    Game* self = static_cast<Game*>(glfwGetWindowUserPointer(win));
    if (self && self->_prevCursorPosCallback) {
        self->_prevCursorPosCallback(win, xpos, ypos); // forward to ImGui (or whatever was there)
    }
    if (self) self->_mouseHandler.handleMouseMove(xpos, ypos);
}

void Game::sWindowFocusCallback(GLFWwindow* win, int focused) {
    Game* self = static_cast<Game*>(glfwGetWindowUserPointer(win));
    if (self && self->_prevFocusCallback) {
        self->_prevFocusCallback(win, focused);
    }
    if (self) self->_mouseHandler.onWindowFocus(focused);
}

void Game::sFramebufferSizeCallback(GLFWwindow* win, int width, int height) {
    Game* self = static_cast<Game*>(glfwGetWindowUserPointer(win));
    if (self && self->_prevFramebufferSizeCallback) {
        self->_prevFramebufferSizeCallback(win, width, height); // forward to ImGui (or whatever was there)
    }
    if (self) self->onFramebufferSize(width, height);
}

void Game::onFramebufferSize(int width, int height) {
    // Update viewport and projection matrix when window is resized
    // This prevents 3D objects from shifting when the screen size changes
    glViewport(0, 0, width, height);
    
    // Force a render update to ensure the new viewport is used
    // The projection matrix will be recalculated in the next render call
    // based on the new aspect ratio
}

// onMouseMove functionality moved to MouseHandler

void Game::update(float dt) {
    // Update input handlers
    _keyboardHandler.update();
    _keyboardHandler.updateGameInput(_window);
    _mouseHandler.update();
    
    // Update camera front from mouse handler
    _cameraFront = _mouseHandler.calculateCameraFront();
    
    // Check if any text input is active (ImGui)
    bool anyTextInputActive = ImGui::IsAnyItemActive() || ImGui::IsWindowFocused();

    // Update integration system
    // Integration disabled for stability; re-enable after refactor
    // Integration::IntegrationManager::instance().update();

    // ----------------------------------------------------------------------------
    // Camera movement WASD + SHIFT/SPACE (continuous movement)
    // ----------------------------------------------------------------------------
    float actualSpeed = _cameraSpeed;
    if (glfwGetKey(_window, GLFW_KEY_V) == GLFW_PRESS) actualSpeed *= 2.5f; // sprint
    if (glfwGetKey(_window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) actualSpeed *= 0.3f; // slow (changed from M to avoid conflict)

    if (_mouseHandler.isCursorLocked() && !_mainMenu.isOpen() && !anyTextInputActive) {
        // Calculate movement vectors that ignore camera pitch so WASD behaves like Minecraft
        glm::vec3 forwardXZ = glm::normalize(glm::vec3(_cameraFront.x, 0.0f, _cameraFront.z));
        if (glm::length(forwardXZ) < 1e-3f) forwardXZ = glm::vec3(0.0f, 0.0f, -1.0f); // fallback
        glm::vec3 rightXZ   = glm::normalize(glm::cross(forwardXZ, _cameraUp));

        if (glfwGetKey(_window, GLFW_KEY_W) == GLFW_PRESS) _cameraPos += actualSpeed * forwardXZ;
        if (glfwGetKey(_window, GLFW_KEY_S) == GLFW_PRESS) _cameraPos -= actualSpeed * forwardXZ;
        if (glfwGetKey(_window, GLFW_KEY_A) == GLFW_PRESS) _cameraPos -= rightXZ * actualSpeed;
        if (glfwGetKey(_window, GLFW_KEY_D) == GLFW_PRESS) _cameraPos += rightXZ * actualSpeed;
        if (glfwGetKey(_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) _cameraPos -= actualSpeed * _cameraUp;
        if (glfwGetKey(_window, GLFW_KEY_SPACE) == GLFW_PRESS) _cameraPos += actualSpeed * _cameraUp;

        // Reset anchor if mode switched out of ManualDistance
        if(_placementMode != BrushPlacementMode::ManualDistance){ _manualAnchorValid = false; }

        // Manual offset tweak with keys when using ManualDistance - only when not typing
        if (_placementMode == BrushPlacementMode::ManualDistance && _current3DMode == Mode3D::BrushCreate && !anyTextInputActive) {
            float step = 0.1f;
            if (glfwGetKey(_window, GLFW_KEY_RIGHT) == GLFW_PRESS) _manualOffset.x += step;
            if (glfwGetKey(_window, GLFW_KEY_LEFT)  == GLFW_PRESS) _manualOffset.x -= step;
            if (glfwGetKey(_window, GLFW_KEY_PAGE_UP) == GLFW_PRESS) _manualOffset.y += step;
            if (glfwGetKey(_window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS) _manualOffset.y -= step;
            if (glfwGetKey(_window, GLFW_KEY_UP)   == GLFW_PRESS) _manualOffset.z += step;
            if (glfwGetKey(_window, GLFW_KEY_DOWN) == GLFW_PRESS) _manualOffset.z -= step;
        }
    }

    // Sync player anchor with camera position
    glm::vec3 anchor = _cameraPos - glm::vec3(0.0f, _player.getBody().getEyeHeight(), 0.0f);
    _player.position = anchor;
    _player.updatePose();

    // Update avatar system
    _avatarManager.updateAllAvatars(dt);

    // Simple cube rotation animation
    _cubeAngle += 50.0f * dt; // degrees/sec
    if (_cubeAngle > 360.0f) _cubeAngle -= 360.0f;

    // --------------------------------------------------------------
    // Creation Tools
    // --------------------------------------------------------------
    {
        bool overUI = ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) || ImGui::IsAnyItemActive() || ImGui::IsAnyItemHovered();
        if (!overUI) {
        bool mouseLeftNow = glfwGetMouseButton(_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        double xpos, ypos;
        glfwGetCursorPos(_window, &xpos, &ypos);
        int winW, winH; glfwGetWindowSize(_window,&winW,&winH);
        int fW, fH; glfwGetFramebufferSize(_window,&fW,&fH);
        float scaleX = static_cast<float>(fW)/winW;
        float scaleY = static_cast<float>(fH)/winH;
        float mx = static_cast<float>(xpos*scaleX);
        float my = static_cast<float>(ypos*scaleY);

        setCursorX(mx);
        setCursorY(my);

        // 2D Creation
        if (_current3DMode == Mode3D::None) {
            // Active Zone
            Zone& zone = mgr.active();
            
            Tool::Type currentToolType = _currentTool.getType();
                    
            Tool::use(_window, mgr, zone, currentToolType, *this);
            if (currentToolType == Tool::Type::Brush) {
                // Check for Shift key to enable straight line mode
                bool shiftPressed = glfwGetKey(_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || 
                                   glfwGetKey(_window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
                
                // Straight line mode (either from button or Shift+click)
                if (_straightLineMode || shiftPressed) {
                    // Overhaul this straight line feature so it is coded in Zone/BrushSystem instead
                    // It should treat points as straight line and as extension of existing brush instead of being separate tool. 
                    // This way we can account for both advanced brush and normal brush using the straight line feature.
                    if (mouseLeftNow && !_mouseLeftPressedLast) {
                        // Start straight line
                        _drawingStraightLine = true;
                        _straightLineStartX = mx;
                        _straightLineStartY = my;
                        zone.startStroke(mx, my);
                    } else if (_drawingStraightLine) {
                        // Update straight line preview on mouse move
                        // Clear previous stroke and redraw from start to current position
                        zone.endStroke(); // End any existing stroke
                        zone.startStroke(_straightLineStartX, _straightLineStartY);
                        zone.continueStroke(mx, my);
                    } else if (!mouseLeftNow && _mouseLeftPressedLast && _drawingStraightLine) {
                        // End straight line
                        zone.endStroke();
                        _drawingStraightLine = false;
                        if (shiftPressed) {
                            _straightLineMode = false; // Reset if it was triggered by Shift
                        }
                    }
                } else {                    
                    // Ensure design system is initialized
                    if (!zone.getDesignSystem()) {
                        zone.initializeDesignSystem();
                    }
                    
                    // Debug output to see what tool is selected
                    static Tool::Type lastToolType = Tool::Type::Brush;
                    if (currentToolType != lastToolType) {
                        printf("Tool changed to: %s (%s)\n", _currentTool.getTypeName().c_str(), _currentTool.getIcon().c_str());
                        lastToolType = currentToolType;
                    }

                    // Drawing tools (including Line)
                    if (currentToolType == Tool::Type::Brush) {
                        // Fall back to old brush system for now since DesignSystem isn't properly rendering
                        Tool::use(_window, mgr, zone, currentToolType, *this);
                    }

                    // Utility tools
                    else if (currentToolType == Tool::Type::ColorPicker || 
                             currentToolType == Tool::Type::Eyedropper || 
                             currentToolType == Tool::Type::Hand || 
                             currentToolType == Tool::Type::Zoom || 
                             currentToolType == Tool::Type::Crop || 
                             currentToolType == Tool::Type::Slice) {
                        
                        if (mouseLeftNow && !_mouseLeftPressedLast) {
                            switch (currentToolType) {
                                case Tool::Type::ColorPicker:
                                case Tool::Type::Eyedropper: {
                                    // Sample color at click position
                                    // For now, just set a random color
                                    float r = static_cast<float>(rand()) / RAND_MAX;
                                    float g = static_cast<float>(rand()) / RAND_MAX;
                                    float b = static_cast<float>(rand()) / RAND_MAX;
                                    zone.setDrawColor(r, g, b);
                                    break;
                                }
                                    
                                case Tool::Type::Hand:
                                    // Pan the view (would need camera/viewport system)
                                    printf("Hand tool: Pan view at (%.1f, %.1f)\n", mx, my);
                                    break;
                                    
                                case Tool::Type::Zoom:
                                    // Zoom in/out (would need camera/viewport system)
                                    printf("Zoom tool: Zoom at (%.1f, %.1f)\n", mx, my);
                                    break;
                                    
                                case Tool::Type::Crop:
                                    // Start crop selection
                                    printf("Crop tool: Start crop at (%.1f, %.1f)\n", mx, my);
                                    break;
                                    
                                case Tool::Type::Slice:
                                    // Start slice selection
                                    printf("Slice tool: Start slice at (%.1f, %.1f)\n", mx, my);
                                    break;
                                    
                                default:
                                    break;
                            }
                        }
                    }
                    // Legacy fallback for compatibility
                    else {
                        if (_useAdvanced2DBrush) {
                            if (mouseLeftNow && !_mouseLeftPressedLast) {
                                zone.startStroke(mx, my);
                            } else if (mouseLeftNow && _mouseLeftPressedLast) {
                                zone.continueStroke(mx, my);
                            } else if (!mouseLeftNow && _mouseLeftPressedLast) {
                                zone.endStroke();
                            }
                        } else {
                            if (mouseLeftNow && !_mouseLeftPressedLast) {
                                mgr.active().startStroke(mx, my);
                            } else if (mouseLeftNow && _mouseLeftPressedLast) {
                                mgr.active().continueStroke(mx, my);
                            } else if (!mouseLeftNow && _mouseLeftPressedLast) {
                                mgr.active().endStroke();
                            }
                        }
                    }
                }
            } else if (_currentTool.getType() == Tool::Type::Eraser) {
                Tool::use(_window, mgr, zone, currentToolType, *this);

            } else if (_currentTool.getType() == Tool::Type::Rectangle) {
                Tool::use(_window, mgr, zone, currentToolType, *this);
            }
        }

        /* 3D Creation */

        // Need to refactor this into Tool.cpp
        // Ensure every object created is owned by the current zone
        else if (_current3DMode == Mode3D::BrushCreate) {
            Tool::ShapeGenerator3D(_window, this, mgr);
        } else if (_current3DMode == Mode3D::Pottery) {
            Tool::Pottery3D(_window, this, mgr, dt);
        } else if (_current3DMode == Mode3D::Selection) {
            // 3D Selection: set selected object on single click
            if (mouseLeftNow && !_mouseLeftPressedLast) {
                glGetIntegerv(GL_VIEWPORT, _cameraViewport);
                glGetDoublev(GL_MODELVIEW_MATRIX, _cameraModelview);
                glGetDoublev(GL_PROJECTION_MATRIX, _cameraProjection);
                double winX = xpos * scaleX; double winY = ypos * scaleY;
                winY = _cameraViewport[3] - winY;
                GLdouble nearX,nearY,nearZ,farX,farY,farZ;
                gluUnProject(winX, winY, 0.0, _cameraModelview, _cameraProjection, _cameraViewport, &nearX,&nearY,&nearZ);
                gluUnProject(winX, winY, 1.0, _cameraModelview, _cameraProjection, _cameraViewport, &farX,&farY,&farZ);
                glm::vec3 rayO(nearX,nearY,nearZ); glm::vec3 rayDir = glm::normalize(glm::vec3(farX,farY,farZ)-rayO);
                float nearestT = 1e9f; Object* hitObj=nullptr;
                const auto& objects = mgr.active().world().getOwnedObjects();
                for (const auto& up : objects) {
                    Object* obj = up.get();
                    float t; int face; glm::vec2 uv;
                    if (obj->raycastFace(rayO, rayDir, t, face, uv)) {
                        if (t > 0.0f && t < nearestT) { nearestT = t; hitObj = obj; }
                    }
                }
                _selectedObject3D = hitObj;
            }
            if (_selectedObject3D) {
                ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
                ImGui::Begin("SelectionHUD", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs);
                ImGui::Text("Selected: %s", _selectedObject3D->getIdentifier().c_str());
                ImGui::End();
            }
        } else if (_current3DMode == Mode3D::FacePaint) {
            Tool::FacePaint(_window, this, mgr, dt);
        } else if (_current3DMode == Mode3D::FaceBrush) {
            Tool::FaceBrush(_window, this, mgr, dt);
        }

        _mouseLeftPressedLast = mouseLeftNow;
        } else {
            _mouseLeftPressedLast = false;
        }
    }

    // Update world (physics etc.)
    mgr.active().world().update(dt);
    // Sync highlight selection
    Rendering::HighlightSystem::setSelected(_selectedObject3D);

    // Extra collision samples around the player (simple capsule approximation)
    {
        constexpr float EYE_TO_FEET = 0.9f;
        constexpr float RADIUS = 0.3f;

        glm::vec3 rightVec = glm::normalize(glm::cross(_cameraFront, _cameraUp));
        glm::vec3 forwardXZ = glm::normalize(glm::vec3(_cameraFront.x, 0.0f, _cameraFront.z));
        if (glm::length(forwardXZ) < 1e-3f) forwardXZ = glm::vec3(0.0f,0.0f,1.0f);

        glm::vec3 offsets[5] = {
            glm::vec3(0),
            rightVec * RADIUS,
            -rightVec * RADIUS,
            forwardXZ * RADIUS,
            -forwardXZ * RADIUS
        };

        for (const auto& off : offsets) {
            glm::vec3 sampleEye  = _cameraPos + off;
            glm::vec3 sampleFeet = sampleEye  - glm::vec3(0.0f, EYE_TO_FEET, 0.0f);
            Physics::enforceCollisions(sampleEye,  mgr.active().world().getOwnedObjects());
            Physics::enforceCollisions(sampleFeet, mgr.active().world().getOwnedObjects());
            // Propagate any delta back to cameraPos
            glm::vec3 resolvedCenter = sampleEye - off;
            _cameraPos = resolvedCenter; // latest correction wins (small offsets)
        }
    }

    // Update body part world transforms based on (possibly corrected) player position
    _player.updatePose();

    // Per-bodypart collision refinement – single aggregate delta
    glm::vec3 totalDelta(0.0f);
    for (auto* part : _player.getBody().parts) {
        if (!part) continue;
        glm::vec3 pos = glm::vec3(part->getTransform()[3]);
        glm::vec3 corrected = pos;
        Physics::enforceCollisions(corrected, mgr.active().world().getOwnedObjects());
        totalDelta += (corrected - pos);
    }
    if (glm::length(totalDelta) > 1e-4f) {
        _cameraPos += totalDelta;
        _player.position += totalDelta;
        _player.updatePose();
    }

    // Final sync so avatar anchors exactly to camera for next frame
    _player.position = _cameraPos - glm::vec3(0.0f, _player.getBody().getEyeHeight(), 0.0f);
    _player.updatePose();

    // Process menu hotkeys (must be after potential cursor unlock to allow selection)
    _mainMenu.processInput(_window);
    _mouseHandler.setMenuOpen(_mainMenu.isOpen());
}

void Game::render() {
    if (!_window) return;

#ifdef USE_GL3_RENDERER
    // Initialize the GL3 renderer lazily once we have a window/context
    if (!_gl3Initialized) {
        // Note: ImGui GL3 is already initialized in Engine; here we prepare our own minimal program
        _gl3Initialized = _gl3Renderer.init(_window, "#version 330 core");
    }
#endif

    // Apply active zone theme colour
    mgr.active().applyTheme();

    int fbW, fbH;
    glfwGetFramebufferSize(_window, &fbW, &fbH);
    if (fbH == 0) fbH = 1;
    float aspect = static_cast<float>(fbW) / fbH;

    glViewport(0, 0, fbW, fbH);

    // Current active zone's 3-D world (accessible throughout render)
    auto& zoneWorld = mgr.active().world();
    zoneWorld.setCamera(&_cameraPos);

    // ------------------------------------------------------------------
    // Projection
    // ------------------------------------------------------------------
    float fov = 45.0f;
    float nearZ = 0.1f;
    float farZ  = 100.0f;
    float top   = tanf(fov * M_PI / 360.0f) * nearZ;
    float bottom = -top;
    float right  = top * aspect;
    float left   = -right;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(left, right, bottom, top, nearZ, farZ);

    // ------------------------------------------------------------------
    // Model-view (camera)
    // ------------------------------------------------------------------
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    vec3 eyePos   = _cameraPos;
    vec3 lookDir  = _cameraFront;
    const float CAMERA_DISTANCE = 4.0f;

    if (_currentPerspective == PerspectiveMode::ThirdPerson) {
        eyePos  = _cameraPos - _cameraFront * CAMERA_DISTANCE;
    } else if (_currentPerspective == PerspectiveMode::SecondPerson) {
        eyePos  = _cameraPos + _cameraFront * CAMERA_DISTANCE;
    }

    vec3 lookTarget = _cameraPos + lookDir;
    gluLookAt(eyePos.x, eyePos.y, eyePos.z,
              lookTarget.x, lookTarget.y, lookTarget.z,
              _cameraUp.x, _cameraUp.y, _cameraUp.z);

    // Update lighting position to follow camera
    ShadingSystem::update(_cameraPos);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // --------------------------------------------------------------
    // Update transforms for demo cube + ground (only if tags still indicate baseline)
    // --------------------------------------------------------------
    if (!zoneWorld.getOwnedObjects().empty()) {
        auto& owned = zoneWorld.getOwnedObjectsMutable();
        if (!owned.empty() && owned[0] && owned[0]->hasAttribute("baseline") && owned[0]->getAttribute("baseline") == std::string("cube")) {
            glm::mat4 cubeTransform = glm::rotate(glm::mat4(1.0f), glm::radians(_cubeAngle), glm::vec3(0.5f, 1.0f, 0.0f));
            owned[0]->setTransform(cubeTransform);
        }
        if (owned.size() > 1 && owned[1] && owned[1]->hasAttribute("baseline") && owned[1]->getAttribute("baseline") == std::string("ground")) {
            glm::mat4 groundTransform = glm::scale(glm::mat4(1.0f), glm::vec3(100.0f, 1.0f, 100.0f));
            owned[1]->setTransform(groundTransform);
        }
    }

    // --------------------------------------------------------------
    // Draw all owned objects except index 1 (ground placeholder)
    // --------------------------------------------------------------
    const auto& objects = zoneWorld.getOwnedObjects();
    for (size_t i = 0; i < objects.size(); ++i) {
        if (i == 1) continue; // skip ground placeholder
        glPushMatrix();
        glMultMatrixf(&objects[i]->getTransform()[0][0]);
        objects[i]->drawObject();
        objects[i]->drawHighlightOutline();
        glPopMatrix();
    }

    // Gravity field visualization (holographic arrows)
    if (Physics::getGravityVisualization()) {
        glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glLineWidth(1.5f);

        // Build a small sample grid around the camera
        int N = Physics::getGravityVisualizationDensity();
        float span = 6.0f; // world units across the grid
        glm::vec3 center = _cameraPos + _cameraFront * 4.0f;
        for (int xi = 0; xi < N; ++xi) {
            for (int yi = 0; yi < N; ++yi) {
                for (int zi = 0; zi < N; ++zi) {
                    float fx = (xi / (float)(N - 1)) - 0.5f;
                    float fy = (yi / (float)(N - 1)) - 0.5f;
                    float fz = (zi / (float)(N - 1)) - 0.5f;
                    glm::vec3 p = center + glm::vec3(fx, fy, fz) * span;
                    float G, eps; Physics::getGravityConstants(G, eps);
                    glm::vec3 a = Physics::sampleGravityField(p, mgr.active().world().getOwnedObjects(), G, eps);
                    float mag = glm::length(a);
                    if (mag < 1e-6f) continue;
                    glm::vec3 dir = a / mag;
                    float len = std::min(0.5f, 0.2f + 0.3f * logf(1.0f + mag));
                    glm::vec3 q = p + dir * len;
                    // Color by magnitude (teal to purple)
                    float t = glm::clamp(mag / 5.0f, 0.0f, 1.0f);
                    glm::vec3 col = glm::mix(glm::vec3(0.2f, 1.0f, 0.9f), glm::vec3(0.8f, 0.2f, 1.0f), t);
                    glColor4f(col.r, col.g, col.b, 0.5f);
                    glBegin(GL_LINES);
                    glVertex3f(p.x, p.y, p.z);
                    glVertex3f(q.x, q.y, q.z);
                    glEnd();
                }
            }
        }
        glPopAttrib();
    }

    // ------------------------------------------------------------------
    // Live preview ("hologram") for BrushCreate mode
    // ------------------------------------------------------------------
    if (_current3DMode == Mode3D::BrushCreate) {
        glm::vec3 previewPos;
        if (_placementMode == BrushPlacementMode::InFront) {
            previewPos = _cameraPos + _cameraFront * 2.0f;
        } else if (_placementMode == BrushPlacementMode::ManualDistance) {
            if(!_manualAnchorValid){
                _manualAnchorPos      = _cameraPos + _cameraFront * 2.0f;
                _manualAnchorRight    = glm::normalize(glm::cross(_cameraFront, _cameraUp));
                _manualAnchorUp       = _cameraUp;
                _manualAnchorForward  = _cameraFront;
                _manualAnchorValid    = true;
            }
            previewPos = _manualAnchorPos + _manualAnchorRight * _manualOffset.x + _manualAnchorUp * _manualOffset.y + _manualAnchorForward * _manualOffset.z;
        } else {
            // CursorSnap – approximate using same raycast as spawn (without altering state)
            double mx,my; glfwGetCursorPos(_window,&mx,&my);
            int winW,winH; glfwGetWindowSize(_window,&winW,&winH);
            int fW,fH; glfwGetFramebufferSize(_window,&fW,&fH);
            float sx = static_cast<float>(fW)/winW;
            float sy = static_cast<float>(fH)/winH;
            double winX = mx*sx; double winY = my*sy;
            winY = _cameraViewport[3] - winY;
            GLdouble nx,ny,nz,fx,fy,fz;
            gluUnProject(winX,winY,0.0,_cameraModelview,_cameraProjection,_cameraViewport,&nx,&ny,&nz);
            gluUnProject(winX,winY,1.0,_cameraModelview,_cameraProjection,_cameraViewport,&fx,&fy,&fz);
            glm::vec3 rayO(nx,ny,nz);
            glm::vec3 rayDir = glm::normalize(glm::vec3(fx,fy,fz)-rayO);
            float nearestT=1e9f; int hitAxis=-1; int hitSign=1; Object* hitObj=nullptr;
            bool hitIsCube=false;
            const auto& objs=zoneWorld.getOwnedObjects();
            for(const auto& up:objs){
                Object* obj=up.get();
                if(obj->getGeometryType()==Object::GeometryType::Cube){
                    glm::mat4 inv=glm::inverse(obj->getTransform());
                    glm::vec3 oL=glm::vec3(inv*glm::vec4(rayO,1.0f));
                    glm::vec3 dL=glm::normalize(glm::vec3(inv*glm::vec4(rayDir,0.0f)));
                    float tMin=-1e9f,tMax=1e9f; int axis=-1; int sign=1;
                    for(int a=0;a<3;++a){float o=oL[a],d=dL[a];float t1,t2;if(fabs(d)<1e-6f){if(o<-0.5f||o>0.5f){tMin=1e9f;break;}t1=-1e9f;t2=1e9f;}else{t1=(-0.5f-o)/d; t2=(0.5f-o)/d;} if(t1>t2) std::swap(t1,t2); if(t1>tMin){tMin=t1; axis=a; sign=(d>0?-1:1);} if(t2<tMax) tMax=t2; if(tMin>tMax){tMin=1e9f;break;}}
                    if(tMin<nearestT && tMin>0 && tMin<1e8f){nearestT=tMin; hitAxis=axis; hitSign=sign; hitObj=obj; hitIsCube=true;}
                }else{
                    glm::vec3 centerWorld=glm::vec3(obj->getTransform()*glm::vec4(0.0f,0.0f,0.0f,1.0f));
                    glm::vec3 colX=glm::vec3(obj->getTransform()[0]);
                    glm::vec3 colY=glm::vec3(obj->getTransform()[1]);
                    glm::vec3 colZ=glm::vec3(obj->getTransform()[2]);
                    float scaleX=glm::length(colX);
                    float scaleY=glm::length(colY);
                    float scaleZ=glm::length(colZ);
                    float radius=0.5f*std::max(scaleX,std::max(scaleY,scaleZ));
                    glm::vec3 oc=rayO-centerWorld;
                    float b=glm::dot(oc,rayDir);
                    float c=glm::dot(oc,oc)-radius*radius;
                    float h=b*b-c;
                    if(h>=0.0f){h=std::sqrt(h); float t=-b-h; if(t<0.0f) t=-b+h; if(t>0.0f && t<nearestT){nearestT=t; hitObj=obj; hitIsCube=false;}}
                }
            }
            if(nearestT<1e8f && hitObj){
                glm::vec3 hitPoint=rayO + rayDir*nearestT;
                glm::vec3 nWorld;
                if(hitIsCube){glm::vec3 nLocal(0.0f); nLocal[hitAxis]=static_cast<float>(hitSign); nWorld=glm::normalize(glm::vec3(hitObj->getTransform()*glm::vec4(nLocal,0.0f)));}
                else{glm::vec3 centerWorld=glm::vec3(hitObj->getTransform()*glm::vec4(0.0f,0.0f,0.0f,1.0f)); nWorld=glm::normalize(hitPoint-centerWorld);} 
                glm::vec3 half=glm::vec3(_brushScale.x*_brushSize,_brushScale.y*_brushSize,_brushScale.z*_brushSize)*0.5f;
                float offAmt=glm::dot(glm::abs(nWorld),half)+0.01f;
                previewPos = hitPoint + nWorld*offAmt;
            } else previewPos = _cameraPos + _cameraFront * 2.0f;
        }

        // Apply optional grid-snap just like the actual spawn logic
        if (_brushGridSnap && _brushGridSize > 1e-6f) {
            previewPos.x = std::round(previewPos.x / _brushGridSize) * _brushGridSize;
            previewPos.y = std::round(previewPos.y / _brushGridSize) * _brushGridSize;
            previewPos.z = std::round(previewPos.z / _brushGridSize) * _brushGridSize;
        }

        // Build transform: translate ➜ scale (rotation optional in future)
        glm::mat4 previewT = glm::translate(glm::mat4(1.0f), previewPos);
        glm::vec3 totalScale = glm::vec3(_brushScale.x * _brushSize,
                                         _brushScale.y * _brushSize,
                                         _brushScale.z * _brushSize);
        previewT = glm::scale(previewT, totalScale);

        // Render as translucent wireframe so it does not occlude view
        glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT | GL_CURRENT_BIT);
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glColor4f(1.0f, 1.0f, 1.0f, 0.5f);

        glPushMatrix();
        glMultMatrixf(&previewT[0][0]);
        // Draw primitive outline using same geometry type
        Object temp;
        temp.setGeometryType(_currentPrimitive);
        
        // Initialize polyhedron data for preview if needed
        if (_currentPrimitive == Object::GeometryType::Polyhedron) {
            if (_useCustomPolyhedron && !_customPolyhedronVertices.empty()) {
                // Use custom polyhedron for preview
                temp.setPolyhedronData(Object::PolyhedronData::createCustomPolyhedron(
                    _customPolyhedronVertices, _customPolyhedronFaces));
            } else {
                // Use concave variant for preview based on selection
                switch (_currentConcaveType) {
                    case 0: // Regular
                        temp.setPolyhedronData(Object::PolyhedronData::createRegularPolyhedron(_currentPolyhedronType));
                        break;
                    case 1: // Concave
                        temp.setPolyhedronData(Object::PolyhedronData::createConcavePolyhedron(_currentPolyhedronType, 0.5f, _concavityAmount));
                        break;
                    case 2: // Star
                        temp.setPolyhedronData(Object::PolyhedronData::createStarPolyhedron(_currentPolyhedronType, 0.5f, _spikeLength));
                        break;
                    case 3: // Crater
                        temp.setPolyhedronData(Object::PolyhedronData::createCraterPolyhedron(_currentPolyhedronType, 0.5f, _craterDepth));
                        break;
                    default:
                        temp.setPolyhedronData(Object::PolyhedronData::createRegularPolyhedron(_currentPolyhedronType));
                        break;
                }
            }
        }
        
        temp.drawObject();
        temp.drawHighlightOutline();
        glPopMatrix();

        glPopAttrib();
    }

    // Draw player avatar and nametag when not in first-person
    if (_currentPerspective != PerspectiveMode::FirstPerson) {
        _player.draw();
        _player.drawNametag();
    }

    // Draw demo avatars if enabled
    if (_showAvatarDemo) {
        for (auto* avatar : _avatarManager.getAllAvatars()) {
            avatar->draw();
            avatar->drawNametag();
        }
    }

    // 2-D overlays ---------------------------------------------------------
    // ------------------------------------------------------------------
    // Draw 2-D strokes for active zone
    // ------------------------------------------------------------------
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, fbW, fbH, 0, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    mgr.active().renderArt();

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopAttrib();

#ifdef USE_GL3_RENDERER
    // Draw a small GL3 triangle in the corner as a migration test without replacing legacy path
    // It validates VAO/VBO/shader pipeline works alongside ImGui.
    _gl3Renderer.render(fbW, fbH);
#endif

    // Brush cursor rendering for Face Brush tool
    if (_current3DMode == Mode3D::FaceBrush && _showBrushCursor && _brushCursorVisible) {
        glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0, fbW, fbH, 0, -1, 1);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        // Convert cursor position to screen coordinates (use actual mouse position for correct alignment)
        // float screenX = _brushCursorPos.x * fbW; // old: derived from UV, caused misalignment
        // float screenY = (1.0f - _brushCursorPos.y) * fbH; // old: derived from UV, caused misalignment
        float screenX = getCursorX();
        float screenY = getCursorY();
        float cursorSize = _faceBrushRadius * 100.0f * _brushPreviewSize;

        // Draw brush cursor circle
        glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
        glLineWidth(2.0f);
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 32; ++i) {
            float angle = 2.0f * M_PI * i / 32.0f;
            float x = screenX + cos(angle) * cursorSize;
            float y = screenY + sin(angle) * cursorSize;
            glVertex2f(x, y);
        }
        glEnd();

        // Draw inner circle for softness indication
        if (_faceBrushSoftness < 1.0f) {
            glColor4f(1.0f, 1.0f, 1.0f, 0.4f);
            float innerSize = cursorSize * _faceBrushSoftness;
            glBegin(GL_LINE_LOOP);
            for (int i = 0; i < 32; ++i) {
                float angle = 2.0f * M_PI * i / 32.0f;
                float x = screenX + cos(angle) * innerSize;
                float y = screenY + sin(angle) * innerSize;
                glVertex2f(x, y);
            }
            glEnd();
        }

        // Draw crosshair at center
        glColor4f(1.0f, 1.0f, 1.0f, 0.6f);
        glLineWidth(1.0f);
        glBegin(GL_LINES);
        glVertex2f(screenX - 5.0f, screenY);
        glVertex2f(screenX + 5.0f, screenY);
        glVertex2f(screenX, screenY - 5.0f);
        glVertex2f(screenX, screenY + 5.0f);
        glEnd();

        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopAttrib();
    }

    _mainMenu.draw();

    // Controls / Keymap window (simple quick reference)
    if (_showKeymapWindow) {
        ImGui::SetNextWindowSize(ImVec2(420, 420), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Controls / Keymap", &_showKeymapWindow, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Core");
            ImGui::Separator();
            ImGui::BulletText("M: Toggle Main Menu");
            ImGui::BulletText("Esc: Toggle Cursor Lock");
            ImGui::BulletText("H: Toggle Chat");
            ImGui::BulletText("T: Toggle Toolbar");
            ImGui::BulletText("I: Toggle Integration UI");
            ImGui::BulletText("1/2/3: Perspective Modes");
            ImGui::BulletText("F: Toggle Flight (non-Survival)");
            ImGui::BulletText("C: Character Architect Forge Zone");
            ImGui::Separator();
            ImGui::Text("Saves");
            ImGui::Separator();
            ImGui::BulletText("S: Quick Save");
            ImGui::BulletText("A: Save As...  L: Load  G: Save Manager");
            ImGui::Separator();
            ImGui::Text("Camera");
            ImGui::Separator();
            ImGui::BulletText("WASD: Move");
            ImGui::BulletText("Space: Up");
            ImGui::BulletText("Shift: Down");
            ImGui::BulletText("V: Sprint");
            ImGui::BulletText("Alt: Slow");
        }
        ImGui::End();
    }

    if (_showChatWindow) {
        _chat.renderUI(&_showChatWindow);
    }

    if (_showToolbar) {
        renderCreatorToolbar();
    }

    // Update cursor tools selection each frame
    _cursorTools.update(*this);

    // Integration System UI
    if (_showIntegrationUI) {
        // Integration disabled for stability; re-enable after refactor
        // Integration::IntegrationManager::instance().renderIntegrationUI();
    }

    // Avatar demo info panel
    if (_showAvatarDemo) {
        ImGui::Begin("Avatar System Demo", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        
        ImGui::Text("Avatar System Features:");
        ImGui::BulletText("Health, Energy, Mood, Experience");
        ImGui::BulletText("Body Part Damage & Healing");
        ImGui::BulletText("Clothing System");
        ImGui::BulletText("Inventory Management");
        ImGui::BulletText("Avatar Interactions");
        ImGui::BulletText("Animation System");
        ImGui::BulletText("AI Behavior");
        ImGui::BulletText("Customization Presets");
        
        ImGui::Separator();
        ImGui::Text("Controls:");
        ImGui::Text("O - Toggle Avatar Demo");
        ImGui::Text("H - Toggle Chat Window");
        ImGui::Text("T - Toggle Toolbar");
        
        ImGui::Separator();
        ImGui::Text("Demo Avatars: %d", _avatarManager.getTotalAvatars());
        ImGui::Text("Average Health: %.1f", _avatarManager.getAverageHealth());
        ImGui::Text("Average Level: %.1f", _avatarManager.getAverageLevel());
        ImGui::Text("Total Experience: %d", _avatarManager.getTotalExperience());
        
        if (ImGui::Button("Heal All Avatars")) {
            _avatarManager.healAllAvatars(50.0f);
        }
        if (ImGui::Button("Damage All Avatars")) {
            _avatarManager.damageAllAvatars(10.0f);
        }
        if (ImGui::Button("Restore All Avatars")) {
            _avatarManager.restoreAllAvatars();
        }
        
        ImGui::End();
    }

    // ------------------------------------------------------------------
    // Character Designer panel – active when in a zone containing
    // the word "Character" (i.e. Character Architect Forge)
    // ------------------------------------------------------------------
    if (mgr.active().name().find("Character") != std::string::npos) {
        static BodyPart* selectedPart = nullptr;
        static bool designLocked = false;
        static int selectedTab = 0;

        ImGui::Begin("Character Designer", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Checkbox("\xF0\x9F\x94\x92 Design Lock", &designLocked); // padlock icon
        ImGui::Separator();
        
        // Tab system for different customization areas
        if (ImGui::BeginTabBar("CharacterTabs")) {
            if (ImGui::BeginTabItem("Body Parts")) {
                ImGui::Text("Body Parts:");
                
                for (auto* part : _player.getBody().parts) {
                    if (!part) continue;
                    bool isSel = (part == selectedPart);
                    if (ImGui::Selectable(part->getName().c_str(), isSel)) {
                        selectedPart = part;
                    }
                }

                if (selectedPart) {
                    ImGui::Separator();
                    ImGui::BeginDisabled(designLocked);

                    ImGui::Text("Editing: %s", selectedPart->getName().c_str());

                    // Dimensions slider
                    glm::vec3 dims = selectedPart->getGeometry().getDimensions();
                    float dimArr[3] = {dims.x, dims.y, dims.z};
                    if (ImGui::SliderFloat3("Dimensions", dimArr, 0.05f, 1.0f, "%.2f") && !designLocked) {
                        selectedPart->getGeometry().setDimensions({dimArr[0], dimArr[1], dimArr[2]});
                        // Update collision zone to reflect new size
                        selectedPart->setTransform(selectedPart->getTransform());
                    }

                    // Color picker
                    float col[3] = {selectedPart->getColor()[0], selectedPart->getColor()[1], selectedPart->getColor()[2]};
                    if (ImGui::ColorEdit3("Color", col) && !designLocked) {
                        selectedPart->setColor(col[0], col[1], col[2]);
                    }

                    // Health and damage system
                    ImGui::Separator();
                    ImGui::Text("Health: %.1f/%.1f", selectedPart->getHealth(), selectedPart->getMaxHealth());
                    if (ImGui::Button("Heal Part")) {
                        selectedPart->heal(20.0f);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Damage Part")) {
                        selectedPart->takeDamage(10.0f);
                    }

                    ImGui::EndDisabled();
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Avatar Stats")) {
                ImGui::BeginDisabled(designLocked);
                
                ImGui::Text("Health: %.1f/%.1f", _player.state.health, _player.state.maxHealth);
                ImGui::Text("Energy: %.1f/%.1f", _player.state.energy, _player.state.maxEnergy);
                ImGui::Text("Mood: %.1f", _player.state.mood);
                ImGui::Text("Level: %d (XP: %.1f)", _player.state.level, _player.state.experience);
                
                ImGui::Separator();
                ImGui::Text("Skills:");
                for (auto& skill : _player.state.skills) {
                    ImGui::Text("%s: %.1f", skill.first.c_str(), skill.second);
                }
                
                ImGui::Separator();
                if (ImGui::Button("Add Experience")) {
                    _player.addExperience(50.0f);
                }
                ImGui::SameLine();
                if (ImGui::Button("Heal Avatar")) {
                    _player.modifyHealth(50.0f);
                }
                ImGui::SameLine();
                if (ImGui::Button("Restore Energy")) {
                    _player.modifyEnergy(50.0f);
                }
                
                ImGui::EndDisabled();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Appearance")) {
                ImGui::BeginDisabled(designLocked);
                
                // Hair style
                static char hairStyle[64] = "";
                strcpy(hairStyle, _player.state.hairStyle.c_str());
                if (ImGui::InputText("Hair Style", hairStyle, sizeof(hairStyle))) {
                    _player.setHairStyle(hairStyle);
                }
                
                // Eye color
                static char eyeColor[32] = "";
                strcpy(eyeColor, _player.state.eyeColor.c_str());
                if (ImGui::InputText("Eye Color", eyeColor, sizeof(eyeColor))) {
                    _player.setEyeColor(eyeColor);
                }
                
                // Skin tone
                static char skinTone[32] = "";
                strcpy(skinTone, _player.state.skinTone.c_str());
                if (ImGui::InputText("Skin Tone", skinTone, sizeof(skinTone))) {
                    _player.setSkinTone(skinTone);
                }
                
                // Height and weight
                static float height = _player.state.height;
                if (ImGui::SliderFloat("Height", &height, 0.5f, 2.5f, "%.2f m")) {
                    _player.setHeight(height);
                }
                
                static float weight = _player.state.weight;
                if (ImGui::SliderFloat("Weight", &weight, 30.0f, 150.0f, "%.1f kg")) {
                    _player.setWeight(weight);
                }
                
                // Body proportions
                static int proportions = static_cast<int>(_player.getBody().proportions);
                const char* propNames[] = {"Child", "Teen", "Adult", "Elder"};
                if (ImGui::Combo("Proportions", &proportions, propNames, 4)) {
                    _player.getBody().setProportions(static_cast<Body::Proportions>(proportions));
                }
                
                ImGui::EndDisabled();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Clothing")) {
                ImGui::BeginDisabled(designLocked);
                
                ImGui::Text("Equipped Clothing:");
                for (auto& item : _player.getBody().clothing) {
                    bool equipped = item.second.isEquipped;
                    if (ImGui::Checkbox(item.first.c_str(), &equipped)) {
                        if (equipped) {
                            _player.getBody().equipClothing(item.first);
                        } else {
                            _player.getBody().unequipClothing(item.first);
                        }
                    }
                    if (equipped) {
                        ImGui::SameLine();
                        ImGui::Text("(Protection: %.1f, Warmth: %.1f)", 
                                   item.second.protection, item.second.warmth);
                    }
                }
                
                ImGui::Separator();
                ImGui::Text("Total Protection: %.1f", _player.getBody().getTotalProtection());
                ImGui::Text("Total Warmth: %.1f", _player.getBody().getTotalWarmth());
                
                ImGui::EndDisabled();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Inventory")) {
                ImGui::BeginDisabled(designLocked);
                
                ImGui::Text("Inventory (%zu/%d items):", _player.inventory.size(), _player.maxInventorySize);
                
                for (size_t i = 0; i < _player.inventory.size(); ++i) {
                    ImGui::Text("%zu. %s", i + 1, _player.inventory[i].c_str());
                }
                
                ImGui::Separator();
                static char newItem[64] = "";
                static bool addItemPressed = false;
                if (ImGui::InputText("Add Item", newItem, sizeof(newItem))) {
                    addItemPressed = true;
                }
                if (addItemPressed && ImGui::IsKeyPressed(ImGuiKey_Enter)) {
                    if (_player.addToInventory(newItem)) {
                        newItem[0] = '\0'; // Clear input
                    }
                    addItemPressed = false;
                }
                
                ImGui::EndDisabled();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Presets")) {
                ImGui::BeginDisabled(designLocked);
                
                ImGui::Text("Available Presets:");
                _avatarManager.listPresets();
                
                ImGui::Separator();
                static char presetName[64] = "";
                static bool addPresetPressed = false;
                if (ImGui::InputText("Preset Name", presetName, sizeof(presetName))) {
                    addPresetPressed = true;
                }
                if (addPresetPressed && ImGui::IsKeyPressed(ImGuiKey_Enter)) {
                    _avatarManager.createPreset(presetName, &_player);
                    presetName[0] = '\0'; // Clear input
                    addPresetPressed = false;
                }
                
                if (ImGui::Button("Create Current Preset")) {
                    _avatarManager.createPreset("Current", &_player);
                }
                
                ImGui::EndDisabled();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();
    }

    // update camera matrices after gluLookAt in render() right after setting view:
    glGetIntegerv(GL_VIEWPORT, _cameraViewport);
    glGetDoublev(GL_MODELVIEW_MATRIX, _cameraModelview);
    glGetDoublev(GL_PROJECTION_MATRIX, _cameraProjection);
}

void Game::renderCreatorToolbar() {
    // ------------------------------------------------------------------
    // Small host window containing a menu to toggle individual tool panes
    // ------------------------------------------------------------------

    static bool showPaint  = true;
    static bool show3D     = true;
    static bool showWorld  = true;
    static bool showAssets = true;
    static bool showBonds  = true;
    static bool showCursor = true;

    ImGui::SetNextWindowSize(ImVec2(550, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin(u8"🛠 Earthcall Creator", nullptr,
                 ImGuiWindowFlags_MenuBar);

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Windows")) {
            ImGui::MenuItem("Paint",  nullptr, &showPaint);
            ImGui::MenuItem("3D",     nullptr, &show3D);
            ImGui::MenuItem("World",  nullptr, &showWorld);
            ImGui::MenuItem("Assets", nullptr, &showAssets);
            ImGui::MenuItem("Bonds",  nullptr, &showBonds);
            ImGui::MenuItem("Cursor Tools",  nullptr, &showCursor);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

#ifdef ImGuiConfigFlags_DockingEnable
    // Provide dockspace so tool windows can be docked/split like an IDE. Store it static so we can reference later.
    static ImGuiID dockspace_id = 0;
    if (dockspace_id == 0) dockspace_id = ImGui::GetID("CreatorDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f));
#endif

    ImGui::End(); // End host window

    // ------------------------------------------------------------------
    // Paint window
    // ------------------------------------------------------------------
#ifdef ImGuiConfigFlags_DockingEnable
    ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_FirstUseEver);
#endif
    if (showPaint) {
        if (ImGui::Begin(u8"🎨 Professional 2D Design", &showPaint)) {
            Zone& zone = mgr.active();
            
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
                        _currentTool = Tool(Tool::Type::Brush); 
                        zone.setDesignTool(Tool::Type::Brush);
                        _current3DMode = Mode3D::None; 
                    }
            ImGui::SameLine();
                    if (ImGui::Button(u8"✏️ Pencil")) { 
                        _currentTool = Tool(Tool::Type::Pencil); 
                        zone.setDesignTool(Tool::Type::Pencil);
                        _current3DMode = Mode3D::None; 
                    }
            ImGui::SameLine();
                    if (ImGui::Button(u8"🖊 Pen")) { 
                        _currentTool = Tool(Tool::Type::Pen); 
                        zone.setDesignTool(Tool::Type::Pen);
                        _current3DMode = Mode3D::None; 
                    }
                    
                    if (ImGui::Button(u8"💨 Airbrush")) { 
                        _currentTool = Tool(Tool::Type::Airbrush); 
                        zone.setDesignTool(Tool::Type::Airbrush);
                        _current3DMode = Mode3D::None; 
                    }
            ImGui::SameLine();
                    if (ImGui::Button(u8"🖼 Chalk")) { 
                        _currentTool = Tool(Tool::Type::Chalk); 
                        zone.setDesignTool(Tool::Type::Chalk);
                        _current3DMode = Mode3D::None; 
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"🎨 Spray")) { 
                        _currentTool = Tool(Tool::Type::Spray); 
                        zone.setDesignTool(Tool::Type::Spray);
                        _current3DMode = Mode3D::None; 
                    }
                    
                    if (ImGui::Button(u8"👆 Smudge")) { 
                        _currentTool = Tool(Tool::Type::Smudge); 
                        zone.setDesignTool(Tool::Type::Smudge);
                        _current3DMode = Mode3D::None; 
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"📋 Clone")) { 
                        _currentTool = Tool(Tool::Type::Clone); 
                        zone.setDesignTool(Tool::Type::Clone);
                        _current3DMode = Mode3D::None; 
                    }
                    
                    ImGui::EndGroup();
                    ImGui::EndTabItem();
                }
                
                // Erasing Tools Tab
                if (ImGui::BeginTabItem("🧽 Erasing")) {
                    ImGui::BeginGroup();
                    
                    if (ImGui::Button(u8"🧽 Eraser")) { 
                        _currentTool = Tool(Tool::Type::Eraser); 
                        zone.setDesignTool(Tool::Type::Eraser);
                        _current3DMode = Mode3D::None; 
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"✨ Magic Eraser")) { 
                        _currentTool = Tool(Tool::Type::MagicEraser); 
                        zone.setDesignTool(Tool::Type::MagicEraser);
                        _current3DMode = Mode3D::None; 
                    }
                    
                    ImGui::EndGroup();
                    ImGui::EndTabItem();
                }
                
                // Selection Tools Tab
                if (ImGui::BeginTabItem("⬜ Selection")) {
                    ImGui::BeginGroup();
                    
                    if (ImGui::Button(u8"⬜ Selection")) { 
                        _currentTool = Tool(Tool::Type::Selection); 
                        zone.setDesignTool(Tool::Type::Selection);
                        _current3DMode = Mode3D::None; 
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"🔗 Lasso")) { 
                        _currentTool = Tool(Tool::Type::Lasso); 
                        zone.setDesignTool(Tool::Type::Lasso);
                        _current3DMode = Mode3D::None; 
                    }
                    
                    if (ImGui::Button(u8"🪄 Magic Wand")) { 
                        _currentTool = Tool(Tool::Type::MagicWand); 
                        zone.setDesignTool(Tool::Type::MagicWand);
                        _current3DMode = Mode3D::None; 
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"📦 Marquee")) { 
                        _currentTool = Tool(Tool::Type::Marquee); 
                        zone.setDesignTool(Tool::Type::Marquee);
                        _current3DMode = Mode3D::None; 
                    }
                    
                    ImGui::EndGroup();
                    ImGui::EndTabItem();
                }
                
                // Shape Tools Tab
                if (ImGui::BeginTabItem("🔷 Shapes")) {
                    ImGui::BeginGroup();
                    
                    if (ImGui::Button(u8"⬜ Rectangle")) { 
                        _currentTool = Tool(Tool::Type::Rectangle); 
                        zone.setDesignTool(Tool::Type::Rectangle);
                        _current3DMode = Mode3D::None; 
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"⭕ Ellipse")) { 
                        _currentTool = Tool(Tool::Type::Ellipse); 
                        zone.setDesignTool(Tool::Type::Ellipse);
                        _current3DMode = Mode3D::None; 
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"🔷 Polygon")) { 
                        _currentTool = Tool(Tool::Type::Polygon); 
                        zone.setDesignTool(Tool::Type::Polygon);
                        _current3DMode = Mode3D::None; 
                    }
                    
                    if (ImGui::Button(u8"➖ Line")) { 
                        _currentTool = Tool(Tool::Type::Line); 
                        zone.setDesignTool(Tool::Type::Line);
                        _current3DMode = Mode3D::None; 
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"➡️ Arrow")) { 
                        _currentTool = Tool(Tool::Type::Arrow); 
                        zone.setDesignTool(Tool::Type::Arrow);
                        _current3DMode = Mode3D::None; 
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"⭐ Star")) { 
                        _currentTool = Tool(Tool::Type::Star); 
                        zone.setDesignTool(Tool::Type::Star);
                        _current3DMode = Mode3D::None; 
                    }
                    
                    if (ImGui::Button(u8"❤️ Heart")) { 
                        _currentTool = Tool(Tool::Type::Heart); 
                        zone.setDesignTool(Tool::Type::Heart);
                        _current3DMode = Mode3D::None; 
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"🔶 Custom")) { 
                        _currentTool = Tool(Tool::Type::CustomShape); 
                        zone.setDesignTool(Tool::Type::CustomShape);
                        _current3DMode = Mode3D::None; 
                    }
                    
                    ImGui::EndGroup();
                    ImGui::EndTabItem();
                }
                
                // Text Tools Tab
                if (ImGui::BeginTabItem("T Text")) {
                    ImGui::BeginGroup();
                    
                    if (ImGui::Button(u8"T Text")) { 
                        _currentTool = Tool(Tool::Type::Text); 
                        zone.setDesignTool(Tool::Type::Text);
                        _current3DMode = Mode3D::None; 
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"T↕️ Vertical")) { 
                        _currentTool = Tool(Tool::Type::TextVertical); 
                        zone.setDesignTool(Tool::Type::TextVertical);
                        _current3DMode = Mode3D::None; 
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"T〰️ Path")) { 
                        _currentTool = Tool(Tool::Type::TextPath); 
                        zone.setDesignTool(Tool::Type::TextPath);
                        _current3DMode = Mode3D::None; 
                    }
                    
                    ImGui::EndGroup();
                    ImGui::EndTabItem();
                }
                
                // Transform Tools Tab
                if (ImGui::BeginTabItem("🔄 Transform")) {
                    ImGui::BeginGroup();
                    
                    if (ImGui::Button(u8"✋ Move")) { 
                        _currentTool = Tool(Tool::Type::Move); 
                        zone.setDesignTool(Tool::Type::Move);
                        _current3DMode = Mode3D::None; 
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"🔍 Scale")) { 
                        _currentTool = Tool(Tool::Type::Scale); 
                        zone.setDesignTool(Tool::Type::Scale);
                        _current3DMode = Mode3D::None; 
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"🔄 Rotate")) { 
                        _currentTool = Tool(Tool::Type::Rotate); 
                        zone.setDesignTool(Tool::Type::Rotate);
                        _current3DMode = Mode3D::None; 
                    }
                    
                    if (ImGui::Button(u8"📐 Skew")) { 
                        _currentTool = Tool(Tool::Type::Skew); 
                        zone.setDesignTool(Tool::Type::Skew);
                        _current3DMode = Mode3D::None; 
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"🔀 Distort")) { 
                        _currentTool = Tool(Tool::Type::Distort); 
                        zone.setDesignTool(Tool::Type::Distort);
                        _current3DMode = Mode3D::None; 
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"🏗️ Perspective")) { 
                        _currentTool = Tool(Tool::Type::Perspective); 
                        zone.setDesignTool(Tool::Type::Perspective);
                        _current3DMode = Mode3D::None; 
                    }
                    
                    ImGui::EndGroup();
                    ImGui::EndTabItem();
                }
                
                // Effects Tools Tab
                if (ImGui::BeginTabItem("🎨 Effects")) {
                    ImGui::BeginGroup();
                    
                    if (ImGui::Button(u8"🌫️ Blur")) { 
                        _currentTool = Tool(Tool::Type::Blur); 
                        zone.setDesignTool(Tool::Type::Blur);
                        _current3DMode = Mode3D::None; 
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"🔪 Sharpen")) { 
                        _currentTool = Tool(Tool::Type::Sharpen); 
                        zone.setDesignTool(Tool::Type::Sharpen);
                        _current3DMode = Mode3D::None; 
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"📻 Noise")) { 
                        _currentTool = Tool(Tool::Type::Noise); 
                        zone.setDesignTool(Tool::Type::Noise);
                        _current3DMode = Mode3D::None; 
                    }
                    
                    if (ImGui::Button(u8"🏛️ Emboss")) { 
                        _currentTool = Tool(Tool::Type::Emboss); 
                        zone.setDesignTool(Tool::Type::Emboss);
                        _current3DMode = Mode3D::None; 
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"💡 Glow")) { 
                        _currentTool = Tool(Tool::Type::Glow); 
                        zone.setDesignTool(Tool::Type::Glow);
                        _current3DMode = Mode3D::None; 
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"👤 Shadow")) { 
                        _currentTool = Tool(Tool::Type::Shadow); 
                        zone.setDesignTool(Tool::Type::Shadow);
                        _current3DMode = Mode3D::None; 
                    }
                    
                    if (ImGui::Button(u8"🌈 Gradient")) { 
                        _currentTool = Tool(Tool::Type::Gradient); 
                        zone.setDesignTool(Tool::Type::Gradient);
                        _current3DMode = Mode3D::None; 
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"🔲 Pattern")) { 
                        _currentTool = Tool(Tool::Type::Pattern); 
                        zone.setDesignTool(Tool::Type::Pattern);
                        _current3DMode = Mode3D::None; 
                    }
                    
                    ImGui::EndGroup();
                    ImGui::EndTabItem();
                }
                
                // Utility Tools Tab
                if (ImGui::BeginTabItem("🔧 Utility")) {
                    ImGui::BeginGroup();
                    
                    if (ImGui::Button(u8"🎯 Color Picker")) { 
                        _currentTool = Tool(Tool::Type::ColorPicker); 
                        zone.setDesignTool(Tool::Type::ColorPicker);
                        _current3DMode = Mode3D::None; 
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"💉 Eyedropper")) { 
                        _currentTool = Tool(Tool::Type::Eyedropper); 
                        zone.setDesignTool(Tool::Type::Eyedropper);
                        _current3DMode = Mode3D::None; 
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"✋ Hand")) { 
                        _currentTool = Tool(Tool::Type::Hand); 
                        zone.setDesignTool(Tool::Type::Hand);
                        _current3DMode = Mode3D::None; 
                    }
                    
                    if (ImGui::Button(u8"🔍 Zoom")) { 
                        _currentTool = Tool(Tool::Type::Zoom); 
                        zone.setDesignTool(Tool::Type::Zoom);
                        _current3DMode = Mode3D::None; 
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"✂️ Crop")) { 
                        _currentTool = Tool(Tool::Type::Crop); 
                        zone.setDesignTool(Tool::Type::Crop);
                        _current3DMode = Mode3D::None; 
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"🔪 Slice")) { 
                        _currentTool = Tool(Tool::Type::Slice); 
                        zone.setDesignTool(Tool::Type::Slice);
                        _current3DMode = Mode3D::None; 
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
            if (ImGui::ColorEdit3("##MainColor", _currentColor, ImGuiColorEditFlags_NoInputs)) {
                zone.setDrawColor(_currentColor[0], _currentColor[1], _currentColor[2]);
            }
            
            // Layer Management
            ImGui::Separator();
            ImGui::Text("Layer Management:");
            if (ImGui::Button("Add Layer")) {
                zone.addDesignLayer();
            }
            ImGui::SameLine();
            if (ImGui::Button("Remove Layer")) {
                zone.removeDesignLayer(0); // Remove active layer
            }
            
            // Legacy compatibility
            ImGui::Separator();
            ImGui::Checkbox("Use Advanced 2D Brush", &_useAdvanced2DBrush);
            if (_useAdvanced2DBrush) {
                ImGui::SameLine();
                if (ImGui::Button("Advanced Settings")) {
                    _show2DBrushPanel = !_show2DBrushPanel;
                }
            }
            
            // Show current tool status
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Current Tool: %s", Tool(_currentTool).getTypeName().c_str());
            
            ImGui::EndGroup();
        }
        ImGui::End();
    }

    // ------------------------------------------------------------------
    // 2D Advanced Brush Panel
    // ------------------------------------------------------------------
    if (_show2DBrushPanel && _useAdvanced2DBrush) {
        if (ImGui::Begin("Advanced 2D Brush", &_show2DBrushPanel)) {
            Zone& zone = mgr.active();
            
            // Ensure brush system is initialized
            if (!zone.getBrushSystem()) {
                zone.initializeBrushSystem();
            }
            
            BrushSystem* brushSystem = zone.getBrushSystem();
            
            if (brushSystem) {
                // Brush Type
                const char* brushTypes[] = {"Normal", "Airbrush", "Chalk", "Spray", "Smudge", "Clone"};
                int currentType = static_cast<int>(brushSystem->getBrushType());
                if (ImGui::Combo("Brush Type", &currentType, brushTypes, 6)) {
                    zone.setBrushType(static_cast<BrushSystem::BrushType>(currentType));
                }
                
                ImGui::Text("Brush System Status: Active");
                ImGui::Text("Active Layer: %d", brushSystem->getActiveLayer());
                ImGui::Text("Layer Count: %d", brushSystem->getLayerCount());
                
                // Basic Settings
                ImGui::Separator();
                ImGui::Text("Basic Settings:");
                float radius = brushSystem->getRadius();
                if (ImGui::SliderFloat("Radius", &radius, 0.01f, 2.0f, "%.3f")) {
                    zone.setBrushRadius(radius);
                }
                
                float opacity = brushSystem->getOpacity();
                if (ImGui::SliderFloat("Opacity", &opacity, 0.0f, 3.0f, "%.2f")) {
                    zone.setBrushOpacity(opacity);
                }
                
                float flow = brushSystem->getFlow();
                if (ImGui::SliderFloat("Flow", &flow, 0.0f, 3.0f, "%.2f")) {
                    zone.setBrushFlow(flow);
                }
                
                // Advanced Dynamics
                ImGui::Separator();
                ImGui::Text("Advanced Dynamics:");
                float spacing = brushSystem->getSpacing();
                if (ImGui::SliderFloat("Spacing", &spacing, 0.01f, 2.0f, "%.3f")) {
                    zone.setBrushSpacing(spacing);
                }
                
                float density = brushSystem->getDensity();
                if (ImGui::SliderFloat("Density", &density, 0.1f, 5.0f, "%.2f")) {
                    zone.setBrushDensity(density);
                }
                
                float strength = brushSystem->getStrength();
                if (ImGui::SliderFloat("Strength", &strength, 0.0f, 5.0f, "%.2f")) {
                    zone.setBrushStrength(strength);
                }
                
                // Pressure Simulation
                ImGui::Separator();
                ImGui::Text("Pressure Simulation:");
                bool usePressure = brushSystem->getUseLayers(); // Using layers as proxy for pressure
                if (ImGui::Checkbox("Enable Pressure", &usePressure)) {
                    zone.setPressureSimulation(usePressure);
                }
                
                // Stroke Interpolation
                ImGui::Separator();
                ImGui::Text("Stroke Settings:");
                bool useInterpolation = true; // Default to true
                if (ImGui::Checkbox("Stroke Interpolation", &useInterpolation)) {
                    zone.setStrokeInterpolation(useInterpolation);
                }
                
                // Layer System
                ImGui::Separator();
                ImGui::Text("Layer System:");
                bool useLayers = brushSystem->getUseLayers();
                if (ImGui::Checkbox("Use Layers", &useLayers)) {
                    zone.setUseLayers(useLayers);
                }
                
                if (useLayers) {
                    int layerCount = brushSystem->getLayerCount();
                    ImGui::Text("Layers: %d", layerCount);
                    
                    if (ImGui::Button("Add Layer")) {
                        zone.addLayer();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Delete Layer")) {
                        zone.deleteLayer(brushSystem->getActiveLayer());
                    }
                    
                    int activeLayer = brushSystem->getActiveLayer();
                    if (ImGui::SliderInt("Active Layer", &activeLayer, 0, std::max(0, layerCount - 1))) {
                        zone.setActiveLayer(activeLayer);
                    }
                }
                
                // Clone Tool
                if (currentType == 5) { // Clone
                    ImGui::Separator();
                    ImGui::Text("Clone Tool:");
                    bool cloneActive = brushSystem->getCloneActive();
                    if (ImGui::Checkbox("Clone Active", &cloneActive)) {
                        zone.setCloneActive(cloneActive);
                    }
                    
                    if (cloneActive) {
                        static glm::vec2 cloneOffset(0.0f, 0.0f);
                        if (ImGui::SliderFloat2("Clone Offset", &cloneOffset.x, -1.0f, 1.0f, "%.2f")) {
                            zone.setCloneOffset(cloneOffset);
                        }
                        
                        if (ImGui::Button("Set Source Point")) {
                            // TODO: Set source point from current mouse position
                        }
                    }
                }
                
                // Undo/Redo
                ImGui::Separator();
                ImGui::Text("History:");
                if (ImGui::Button("Undo (Ctrl+Z)")) {
                    zone.undo();
                }
                ImGui::SameLine();
                if (ImGui::Button("Redo (Ctrl+Y)")) {
                    zone.redo();
                }
                ImGui::SameLine();
                if (ImGui::Button("Clear History")) {
                    zone.clearHistory();
                }
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: Brush System failed to initialize!");
                if (ImGui::Button("Retry Initialization")) {
                    zone.initializeBrushSystem();
                }
            }
        }
        ImGui::End();
    }

    // ------------------------------------------------------------------
    // 3D window
    // ------------------------------------------------------------------
#ifdef ImGuiConfigFlags_DockingEnable
    ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_FirstUseEver);
#endif
    if (show3D) {
        if (ImGui::Begin(u8"🔳 3D", &show3D)) {
            int modeIdx = static_cast<int>(_current3DMode);
            const char* modeNames[] = {"Face Fill", "Face Brush", "Shape Generator", "Pottery", "Selection"};
            if (ImGui::Combo("SubMode", &modeIdx, modeNames, IM_ARRAYSIZE(modeNames))) {
                _current3DMode = static_cast<Mode3D>(modeIdx);
            }

            // Advanced Face Paint Options (only in Face Fill mode)
            if (_current3DMode == Mode3D::FacePaint) {
                ImGui::Separator();
                ImGui::TextUnformatted("🎨 Advanced Face Paint Options");
                
                // Enable/disable advanced face paint
                if (ImGui::Checkbox("Enable Advanced Face Paint", &_useAdvancedFacePaint)) {
                    // Initialize advanced painter if enabled
                    if (_useAdvancedFacePaint) {
                        AdvancedFacePaint::initializeAdvancedPainter();
                    }
                }
                
                if (_useAdvancedFacePaint) {
                    ImGui::Indent();
                    
                    // Gradient Options
                    if (ImGui::CollapsingHeader("Gradient Options", ImGuiTreeNodeFlags_DefaultOpen)) {
                        AdvancedFacePaint::GradientSettings& gradSettings = _currentGradientSettings;
                        
                        // Gradient Type
                        const char* gradientTypes[] = {"Linear", "Radial", "Angular", "Diamond", "Noise", "Custom"};
                        int gradTypeIdx = static_cast<int>(gradSettings.type);
                        if (ImGui::Combo("Gradient Type", &gradTypeIdx, gradientTypes, IM_ARRAYSIZE(gradientTypes))) {
                            gradSettings.type = static_cast<AdvancedFacePaint::GradientType>(gradTypeIdx);
                        }
                        
                        // Colors
                        ImGui::ColorEdit4("Start Color", &gradSettings.startColor.x);
                        ImGui::ColorEdit4("End Color", &gradSettings.endColor.x);
                        
                        // Points
                        ImGui::SliderFloat2("Start Point", &gradSettings.startPoint.x, 0.0f, 1.0f, "%.2f");
                        ImGui::SliderFloat2("End Point", &gradSettings.endPoint.x, 0.0f, 1.0f, "%.2f");
                        
                        // Angle
                        ImGui::SliderFloat("Angle", &gradSettings.angle, 0.0f, 360.0f, "%.1f°");
                        
                        // Noise settings (for noise gradient type)
                        if (gradSettings.type == AdvancedFacePaint::GradientType::Noise) {
                            ImGui::SliderFloat("Noise Scale", &gradSettings.noiseScale, 0.1f, 10.0f, "%.2f");
                            ImGui::SliderInt("Noise Octaves", &gradSettings.noiseOctaves, 1, 8);
                            ImGui::SliderFloat("Noise Persistence", &gradSettings.noisePersistence, 0.1f, 1.0f, "%.2f");
                            ImGui::SliderFloat("Noise Lacunarity", &gradSettings.noiseLacunarity, 1.0f, 4.0f, "%.2f");
                        }
                        
                        // Alpha settings
                        ImGui::Checkbox("Use Alpha", &gradSettings.useAlpha);
                        if (gradSettings.useAlpha) {
                            ImGui::SliderFloat("Alpha Blend", &gradSettings.alphaBlend, 0.0f, 1.0f, "%.2f");
                        }
                    }
                    
                    // Smudge Options
                    if (ImGui::CollapsingHeader("Smudge Options", ImGuiTreeNodeFlags_DefaultOpen)) {
                        AdvancedFacePaint::SmudgeSettings& smudgeSettings = _currentSmudgeSettings;
                        
                        // Smudge Type
                        const char* smudgeTypes[] = {"Normal", "Directional", "Radial", "Spiral", "Noise", "Custom"};
                        int smudgeTypeIdx = static_cast<int>(smudgeSettings.type);
                        if (ImGui::Combo("Smudge Type", &smudgeTypeIdx, smudgeTypes, IM_ARRAYSIZE(smudgeTypes))) {
                            smudgeSettings.type = static_cast<AdvancedFacePaint::SmudgeType>(smudgeTypeIdx);
                        }
                        
                        // Basic smudge settings
                        ImGui::SliderFloat("Strength", &smudgeSettings.strength, 0.0f, 1.0f, "%.2f");
                        ImGui::SliderFloat("Radius", &smudgeSettings.radius, 0.01f, 1.0f, "%.2f");
                        ImGui::SliderFloat("Softness", &smudgeSettings.softness, 0.1f, 2.0f, "%.2f");
                        ImGui::SliderFloat("Pressure", &smudgeSettings.pressure, 0.1f, 2.0f, "%.2f");
                        
                        // Direction (for directional smudge)
                        if (smudgeSettings.type == AdvancedFacePaint::SmudgeType::Directional) {
                            ImGui::SliderFloat2("Direction", &smudgeSettings.direction.x, -1.0f, 1.0f, "%.2f");
                            ImGui::SliderFloat("Directional Strength", &smudgeSettings.directionalStrength, 0.0f, 1.0f, "%.2f");
                        }
                        
                        // Speed and turbulence (for spiral smudge)
                        if (smudgeSettings.type == AdvancedFacePaint::SmudgeType::Spiral) {
                            ImGui::SliderFloat("Speed", &smudgeSettings.speed, 0.1f, 5.0f, "%.2f");
                            ImGui::SliderFloat("Turbulence", &smudgeSettings.turbulence, 0.01f, 1.0f, "%.2f");
                            ImGui::SliderFloat("Spiral Turns", &smudgeSettings.spiralTurns, 0.5f, 5.0f, "%.2f");
                        }
                        
                        // Noise settings (for noise smudge)
                        if (smudgeSettings.type == AdvancedFacePaint::SmudgeType::Noise) {
                            ImGui::SliderFloat("Noise Intensity", &smudgeSettings.noiseIntensity, 0.0f, 1.0f, "%.2f");
                            ImGui::SliderFloat("Noise Scale", &smudgeSettings.noiseScale, 0.1f, 10.0f, "%.2f");
                        }
                        
                        // Pressure simulation
                        ImGui::Checkbox("Use Pressure", &smudgeSettings.usePressure);
                    }
                    
                    // Preview and Apply
                    ImGui::Separator();
                    ImGui::TextUnformatted("Preview & Apply");
                    
                    if (ImGui::Button("Preview Gradient")) {
                        // Show gradient preview (would render in a separate window)
                        _showAdvancedFacePaintPanel = true;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Preview Smudge")) {
                        // Show smudge preview
                        _showAdvancedFacePaintPanel = true;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Apply to Selected Face")) {
                        // Apply current settings to selected face
                        if (_selectedObject3D) {
                            // This would apply the current gradient and smudge settings
                            // to the selected face of the selected object
                        }
                    }
                    
                    ImGui::Unindent();
                }
            }

            ImGui::Separator();
            int primitiveIdx = static_cast<int>(_currentPrimitive);
            const char* primitiveNames[] = {"Cube", "Sphere", "Cylinder", "Cone", "Polyhedron"};
            if (ImGui::Combo("Shape", &primitiveIdx, primitiveNames, IM_ARRAYSIZE(primitiveNames))) {
                _currentPrimitive = static_cast<Object::GeometryType>(primitiveIdx);
            }

            // Enhanced Polyhedron Generator (only in Shape Generator mode)
            if (_currentPrimitive == Object::GeometryType::Polyhedron && _current3DMode == Mode3D::BrushCreate) {
                ImGui::Separator();
                ImGui::TextUnformatted("🔷 Polyhedron Generator");
                
                // Basic polyhedron types
                ImGui::TextUnformatted("Regular Polyhedrons:");
                if (ImGui::Button("Tetrahedron (4)")) {
                    _currentPolyhedronType = 4;
                }
                ImGui::SameLine();
                if (ImGui::Button("Octahedron (8)")) {
                    _currentPolyhedronType = 8;
                }
                ImGui::SameLine();
                if (ImGui::Button("Dodecahedron (12)")) {
                    _currentPolyhedronType = 12;
                }
                ImGui::SameLine();
                if (ImGui::Button("Icosahedron (20)")) {
                    _currentPolyhedronType = 20;
                }
                
                // Advanced polyhedron options
                ImGui::Separator();
                ImGui::TextUnformatted("Advanced Options:");
                
                // Custom face count slider
                static int customFaceCount = 4;
                if (ImGui::SliderInt("Custom Face Count", &customFaceCount, 3, 50)) {
                    _currentPolyhedronType = customFaceCount;
                }
                
                // Random polyhedron generator
                if (ImGui::Button("🎲 Random Polyhedron")) {
                    _currentPolyhedronType = 4 + (rand() % 17); // 4-20 faces
                }
                ImGui::SameLine();
                if (ImGui::Button("🎲 Random Complex")) {
                    _currentPolyhedronType = 8 + (rand() % 13); // 8-20 faces
                }
                
                // Polyhedron presets
                ImGui::Separator();
                ImGui::TextUnformatted("Quick Presets:");
                if (ImGui::Button("Simple (4-8)")) {
                    _currentPolyhedronType = 4 + (rand() % 5);
                }
                ImGui::SameLine();
                if (ImGui::Button("Medium (8-12)")) {
                    _currentPolyhedronType = 8 + (rand() % 5);
                }
                ImGui::SameLine();
                if (ImGui::Button("Complex (12-20)")) {
                    _currentPolyhedronType = 12 + (rand() % 9);
                }
                
                // Display current selection
                ImGui::Separator();
                ImGui::Text("Selected: %d faces", _currentPolyhedronType);
                
                // Polyhedron info
                const char* polyhedronNames[] = {
                    "Unknown", "Unknown", "Unknown", "Unknown", "Tetrahedron",
                    "Unknown", "Unknown", "Unknown", "Octahedron", "Unknown",
                    "Unknown", "Unknown", "Dodecahedron", "Unknown", "Unknown",
                    "Unknown", "Unknown", "Unknown", "Unknown", "Icosahedron"
                };
                
                if (_currentPolyhedronType >= 4 && _currentPolyhedronType <= 20) {
                    ImGui::Text("Type: %s", polyhedronNames[_currentPolyhedronType]);
                }
                
                // Performance warning for complex polyhedrons
                if (_currentPolyhedronType > 12) {
                    ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "⚠ Complex polyhedron - may affect performance");
                }
                
                // Convex/Concave Polyhedron Controls
                ImGui::Separator();
                ImGui::TextUnformatted("🔷 Convex/Concave Variants:");
                
                static int concaveType = 0;
                const char* concaveTypes[] = {"Regular", "Concave", "Star", "Crater"};
                if (ImGui::Combo("Variant", &concaveType, concaveTypes, IM_ARRAYSIZE(concaveTypes))) {
                    _currentConcaveType = concaveType;
                }
                
                if (concaveType == 1) { // Concave
                    static float concavity = 0.3f;
                    if (ImGui::SliderFloat("Concavity", &concavity, 0.1f, 0.8f, "%.2f")) {
                        _concavityAmount = concavity;
                    }
                } else if (concaveType == 2) { // Star
                    static float spikeLength = 0.3f;
                    if (ImGui::SliderFloat("Spike Length", &spikeLength, 0.1f, 1.0f, "%.2f")) {
                        _spikeLength = spikeLength;
                    }
                } else if (concaveType == 3) { // Crater
                    static float craterDepth = 0.2f;
                    if (ImGui::SliderFloat("Crater Depth", &craterDepth, 0.1f, 0.5f, "%.2f")) {
                        _craterDepth = craterDepth;
                    }
                }
                
                // Custom polyhedron generation
                ImGui::Separator();
                ImGui::TextUnformatted("Custom Polyhedron:");
                ImGui::Checkbox("Use Custom Polyhedron", &_useCustomPolyhedron);
                
                if (_useCustomPolyhedron) {
                    if (ImGui::SliderInt("Vertex Count", &_customPolyhedronVertexCount, 3, 20)) {
                        // Regenerate custom polyhedron
                        _generateCustomPolyhedron();
                    }
                    if (ImGui::SliderInt("Face Count", &_customPolyhedronFaceCount, 3, 20)) {
                        // Regenerate custom polyhedron
                        _generateCustomPolyhedron();
                    }
                    
                    if (ImGui::Button("🔄 Regenerate Custom")) {
                        _generateCustomPolyhedron();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("💾 Save Custom")) {
                        // TODO: Save custom polyhedron to file
                        ImGui::OpenPopup("Custom Polyhedron Saved");
                    }
                    
                    ImGui::Text("Custom: %d vertices, %d faces", _customPolyhedronVertexCount, _customPolyhedronFaceCount);
                }
            } else if (_currentPrimitive == Object::GeometryType::Polyhedron) {
                // Simple polyhedron controls for other modes
                ImGui::Separator();
                ImGui::TextUnformatted("Polyhedron Type:");
                if (ImGui::Button("Tetrahedron")) {
                    _currentPolyhedronType = 4;
                }
                ImGui::SameLine();
                if (ImGui::Button("Octahedron")) {
                    _currentPolyhedronType = 8;
                }
                ImGui::SameLine();
                if (ImGui::Button("Dodecahedron")) {
                    _currentPolyhedronType = 12;
                }
                ImGui::SameLine();
                if (ImGui::Button("Icosahedron")) {
                    _currentPolyhedronType = 20;
                }
                ImGui::Text("Selected: %d faces", _currentPolyhedronType);
            }

            ImGui::SliderFloat("Uniform Size", &_brushSize, 0.1f, 10.0f, "%.2f");

            // Pottery specific controls
            if (_current3DMode == Mode3D::Pottery) {
                ImGui::Separator();
                ImGui::TextUnformatted("Pottery Tool:");
                bool isChisel = _currentPotteryTool == PotteryTool::Chisel;
                if (ImGui::RadioButton("Chisel", isChisel)) _currentPotteryTool = PotteryTool::Chisel;
                ImGui::SameLine();
                bool isExpand = _currentPotteryTool == PotteryTool::Expand;
                if (ImGui::RadioButton("Expand", isExpand)) _currentPotteryTool = PotteryTool::Expand;
                ImGui::SliderFloat("Strength", &_potteryStrength, 0.01f, 2.0f, "%.2f");
            }

            // Placement mode controls
            ImGui::Separator();
            int placeIdx = static_cast<int>(_placementMode);
            const char* placeNames[] = {"In Front", "Manual Distance", "Cursor Snap"};
            if (ImGui::Combo("Placement", &placeIdx, placeNames, IM_ARRAYSIZE(placeNames))) {
                _placementMode = static_cast<BrushPlacementMode>(placeIdx);
            }
            if (_placementMode == BrushPlacementMode::ManualDistance && _prevPlacementMode != BrushPlacementMode::ManualDistance) {
                // Capture anchor from current camera state
                _manualAnchorPos      = _cameraPos + _cameraFront * 2.0f;
                _manualAnchorRight    = glm::normalize(glm::cross(_cameraFront, _cameraUp));
                _manualAnchorUp       = _cameraUp;
                _manualAnchorForward  = _cameraFront;
                _manualAnchorValid    = true;
            }
            _prevPlacementMode = _placementMode;
            if (_placementMode == BrushPlacementMode::ManualDistance) {
                ImGui::SliderFloat3("Offset XYZ", &_manualOffset.x, -20.0f, 20.0f, "%.2f");
                ImGui::TextUnformatted("X = right, Y = up, Z = forward");
            }

            // In 3D window UI, inside if (show3D) -> when mode FaceBrush selected
            if (_current3DMode == Mode3D::FaceBrush) {
                ImGui::Separator();
                
                // Brush Type Selection
                ImGui::Text("Brush Type:");
                const char* brushTypeNames[] = {"Normal", "Airbrush", "Chalk", "Spray", "Smudge", "Clone"};
                int brushTypeIdx = static_cast<int>(_currentBrushType);
                if (ImGui::Combo("##BrushType", &brushTypeIdx, brushTypeNames, IM_ARRAYSIZE(brushTypeNames))) {
                    _currentBrushType = static_cast<BrushType>(brushTypeIdx);
                }
                
                // Brush Presets
                ImGui::Separator();
                ImGui::Text("Brush Presets:");
                if (ImGui::Button("Save Preset")) {
                    // Save current brush settings as preset
                    BrushPreset preset;
                    preset.name = "Custom " + std::to_string(_brushPresets.size() + 1);
                    preset.type = _currentBrushType;
                    preset.radius = _faceBrushRadius;
                    preset.softness = _faceBrushSoftness;
                    preset.opacity = _brushOpacity;
                    preset.flow = _brushFlow;
                    preset.spacing = _brushSpacing;
                    preset.density = _brushDensity;
                    preset.strength = _brushStrength;
                    _brushPresets.push_back(preset);
                }
                ImGui::SameLine();
                if (ImGui::Button("Load Preset") && !_brushPresets.empty()) {
                    // Load selected preset
                    if (_currentPreset >= 0 && _currentPreset < static_cast<int>(_brushPresets.size())) {
                        const BrushPreset& preset = _brushPresets[_currentPreset];
                        _currentBrushType = preset.type;
                        _faceBrushRadius = preset.radius;
                        _faceBrushSoftness = preset.softness;
                        _brushOpacity = preset.opacity;
                        _brushFlow = preset.flow;
                        _brushSpacing = preset.spacing;
                        _brushDensity = preset.density;
                        _brushStrength = preset.strength;
                    }
                }
                
                if (!_brushPresets.empty()) {
                    std::vector<const char*> presetNames;
                    for (const auto& preset : _brushPresets) {
                        presetNames.push_back(preset.name.c_str());
                    }
                    ImGui::Combo("##PresetSelect", &_currentPreset, presetNames.data(), static_cast<int>(presetNames.size()));
                }
                
                // Basic Brush Settings
                ImGui::Separator();
                ImGui::Text("Basic Settings:");
                ImGui::SliderFloat("Brush Radius", &_faceBrushRadius, 0.01f, 2.0f, "%.2f");
                ImGui::SliderFloat("Softness", &_faceBrushSoftness, 0.0f, 2.0f, "%.2f");
                ImGui::SliderFloat("Opacity", &_brushOpacity, 0.0f, 1.0f, "%.2f");
                ImGui::SliderFloat("Flow", &_brushFlow, 0.0f, 1.0f, "%.2f");
                
                // Advanced Brush Dynamics
                ImGui::Separator();
                ImGui::Text("Advanced Dynamics:");
                ImGui::SliderFloat("Spacing", &_brushSpacing, 0.01f, 0.5f, "%.2f");
                ImGui::SliderFloat("Density", &_brushDensity, 0.1f, 1.0f, "%.2f");
                ImGui::SliderFloat("Strength", &_brushStrength, 0.0f, 1.0f, "%.2f");
                
                // Pressure Simulation
                ImGui::Separator();
                ImGui::Text("Pressure Simulation:");
                ImGui::Checkbox("Enable Pressure", &_usePressureSimulation);
                if (_usePressureSimulation) {
                    ImGui::SliderFloat("Sensitivity", &_pressureSensitivity, 0.1f, 5.0f, "%.2f");
                    ImGui::SliderFloat("Current Pressure", &_currentPressure, 0.1f, 1.0f, "%.2f");
                }
                
                // Stroke Interpolation
                ImGui::Separator();
                ImGui::Text("Stroke Settings:");
                ImGui::Checkbox("Stroke Interpolation", &_useStrokeInterpolation);
                ImGui::Checkbox("Show Brush Cursor", &_showBrushCursor);
                ImGui::Checkbox("Show Brush Preview", &_showBrushPreview);
                
                // Clone Tool Settings
                if (_currentBrushType == BrushType::Clone) {
                    ImGui::Separator();
                    ImGui::Text("Clone Tool:");
                    ImGui::Checkbox("Clone Active", &_cloneToolActive);
                    if (_cloneToolActive) {
                        ImGui::SliderFloat2("Clone Offset", &_cloneOffset.x, -1.0f, 1.0f, "%.2f");
                        if (ImGui::Button("Set Source Point")) {
                            _cloneSourceUV = _brushCursorPos;
                        }
                    }
                }
                
                // Layer System
                ImGui::Separator();
                ImGui::Text("Layer System:");
                ImGui::Checkbox("Use Layers", &_useLayers);
                if (_useLayers) {
                    ImGui::SliderInt("Active Layer", &_activeLayer, 0, 10);
                    ImGui::SliderFloat("Layer Opacity", &_layerOpacity, 0.0f, 1.0f, "%.2f");
                    
                    const char* blendModeNames[] = {"Normal", "Multiply", "Screen", "Overlay", "Add", "Subtract"};
                    ImGui::Combo("Blend Mode", &_blendMode, blendModeNames, IM_ARRAYSIZE(blendModeNames));
                    
                    if (ImGui::Button("Add Layer")) {
                        // Add layer to current object (would need object reference)
                        const auto& objects = mgr.active().world().getOwnedObjects();
                        for (const auto& up : objects) {
                            Object* obj = up.get();
                            obj->addTextureLayer(0); // Add to first face for now
                            break;
                        }
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Delete Layer")) {
                        // Delete current layer
                        const auto& objects = mgr.active().world().getOwnedObjects();
                        for (const auto& up : objects) {
                            Object* obj = up.get();
                            obj->deleteTextureLayer(0, _activeLayer); // Delete from first face
                            break;
                        }
                    }
                }
                
                // UV Controls
                ImGui::Separator();
                ImGui::Text("UV Controls:");
                ImGui::SliderFloat("U Offset", &_faceBrushUOffset, -2.0f, 2.0f, "%.2f");
                ImGui::SliderFloat("V Offset", &_faceBrushVOffset, -2.0f, 2.0f, "%.2f");
                const char* axisNames[] = {"X","Y","Z"};
                ImGui::Combo("Axis 1", &_faceBrushUAxis, axisNames, 3);
                ImGui::Combo("Axis 2", &_faceBrushVAxis, axisNames, 3);
                if(_faceBrushVAxis == _faceBrushUAxis) {
                    ImGui::TextColored(ImVec4(1,0,0,1), "Axis 1 and Axis 2 must differ!");
                }
                ImGui::Checkbox("Invert Axis 1", &_faceBrushInvertU);
                ImGui::SameLine();
                ImGui::Checkbox("Invert Axis 2", &_faceBrushInvertV);
                
                // Undo/Redo
                ImGui::Separator();
                ImGui::Text("History:");
                if (ImGui::Button("Undo (Ctrl+Z)")) {
                    // Undo last stroke
                    const auto& objects = mgr.active().world().getOwnedObjects();
                    for (const auto& up : objects) {
                        Object* obj = up.get();
                        obj->undoStroke(0); // Undo on first face
                        break;
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Redo (Ctrl+Y)")) {
                    // Redo last undone stroke
                    // Redo functionality would be implemented here
                    // For now, just a placeholder
                }
                ImGui::SameLine();
                if (ImGui::Button("Clear History")) {
                    // Clear stroke history
                    const auto& objects = mgr.active().world().getOwnedObjects();
                    for (const auto& up : objects) {
                        Object* obj = up.get();
                        obj->clearStrokeHistory(0); // Clear on first face
                        break;
                    }
                }
            }
        }
        ImGui::End();
    }

    // ------------------------------------------------------------------
    // World window
    // ------------------------------------------------------------------
#ifdef ImGuiConfigFlags_DockingEnable
    ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_FirstUseEver);
#endif
    if (showWorld) {
        if (ImGui::Begin(u8"🌍 World", &showWorld)) {
            _world.renderModeUI();
        }
        ImGui::End();
    }

    // ------------------------------------------------------------------
    // Cursor Tools window
    // ------------------------------------------------------------------
#ifdef ImGuiConfigFlags_DockingEnable
    ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_FirstUseEver);
#endif
    if (showCursor) {
        bool open = true;
        _cursorTools.renderUI(open);
        if (!open) showCursor = false;
    }

    // ------------------------------------------------------------------
    // Assets window
    // ------------------------------------------------------------------
#ifdef ImGuiConfigFlags_DockingEnable
    ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_FirstUseEver);
#endif
    if (showAssets) {
        if (ImGui::Begin(u8"💾 Assets", &showAssets)) {
            if (ImGui::Button(u8"💾 Quick Save")) {
                saveStateWithLog();
            }
            ImGui::SameLine();
            if (ImGui::Button(u8"💾 Save As...")) {
                _showSaveWindow = true;
            }
            ImGui::SameLine();
            if (ImGui::Button(u8"📂 Load")) {
                updateSaveFiles();
                _showLoadWindow = true;
            }
            ImGui::SameLine();
            if (ImGui::Button(u8"📁 Save Manager")) {
                _showSaveManager = true;
            }
        }
        ImGui::End();
    }

    // ------------------------------------------------------------------
    // Bonds window
    // ------------------------------------------------------------------
#ifdef ImGuiConfigFlags_DockingEnable
    ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_FirstUseEver);
#endif
    if (showBonds) {
        if (ImGui::Begin(u8"🔗 Bonds", &showBonds)) {
            auto& zoneWorld = mgr.active().world();
            const auto& objs = zoneWorld.getOwnedObjects();

            // Object-level bond creation
            static int objAIdx = 0;
            static int objBIdx = 1;

            std::vector<std::string> labels;
            labels.reserve(objs.size());
            for (size_t i = 0; i < objs.size(); ++i) {
                char buf[32];
                snprintf(buf, sizeof(buf), "Obj %zu", i);
                labels.emplace_back(buf);
            }
            std::vector<const char*> cstrs;
            for (auto& s : labels) cstrs.push_back(s.c_str());

            if (!labels.empty()) {
                ImGui::Combo("Object A", &objAIdx, cstrs.data(), static_cast<int>(labels.size()));
                ImGui::Combo("Object B", &objBIdx, cstrs.data(), static_cast<int>(labels.size()));
                if (ImGui::Button("Create Bond") && objAIdx != objBIdx && objAIdx < labels.size() && objBIdx < labels.size()) {
                    Physics::addBond(objs[objAIdx].get(), objs[objBIdx].get());
                }
            } else {
                ImGui::TextUnformatted("No objects available.");
            }

            ImGui::Separator();

            // Auto bond rules by shape
            ImGui::Text("Auto Bond Rules (shape pairs):");
            static int shapeAIdx = 0;
            static int shapeBIdx = 1;
            const char* primitiveNames[] = {"Cube", "Sphere", "Cylinder", "Cone"};
            ImGui::Combo("Shape A", &shapeAIdx, primitiveNames, IM_ARRAYSIZE(primitiveNames));
            ImGui::Combo("Shape B", &shapeBIdx, primitiveNames, IM_ARRAYSIZE(primitiveNames));

            bool enabled = Physics::getAutoBond(static_cast<Object::GeometryType>(shapeAIdx), static_cast<Object::GeometryType>(shapeBIdx));
            if (ImGui::Checkbox("Bonded##Enabled", &enabled)) {
                Physics::setAutoBond(static_cast<Object::GeometryType>(shapeAIdx), static_cast<Object::GeometryType>(shapeBIdx), enabled);
            }

            ImGui::Separator();

            // Existing bonds list
            ImGui::Text("Existing Bonds:");
            const auto& bonds = Physics::getBonds();

            if (bonds.empty()) {
                ImGui::TextUnformatted("<none>");
            } else {
                static int selectedBond = -1;
                if (ImGui::BeginListBox("##BondList", ImVec2(-FLT_MIN, 120))) {
                    for (int i = 0; i < static_cast<int>(bonds.size()); ++i) {
                        int idxA = -1, idxB = -1;
                        for (size_t j = 0; j < objs.size(); ++j) {
                            if (objs[j].get() == bonds[i].a) idxA = static_cast<int>(j);
                            if (objs[j].get() == bonds[i].b) idxB = static_cast<int>(j);
                        }
                        char label[64];
                        snprintf(label, sizeof(label), "%d: Obj %d <-> Obj %d", i, idxA, idxB);
                        if (ImGui::Selectable(label, selectedBond == i)) {
                            selectedBond = i;
                        }
                    }
                    ImGui::EndListBox();
                }

                if (selectedBond >= 0 && selectedBond < static_cast<int>(bonds.size())) {
                    auto& bond = bonds[selectedBond];
                    float restLen = bond.restLength;
                    float strength = bond.strength;
                    if (ImGui::DragFloat("Rest Length", &restLen, 0.05f, 0.0f, 10.0f, "%.2f")) {
                        Physics::setBondParams(bond.a, bond.b, restLen, strength);
                    }
                    if (ImGui::DragFloat("Strength", &strength, 0.5f, 0.0f, 100.0f, "%.1f")) {
                        Physics::setBondParams(bond.a, bond.b, restLen, strength);
                    }
                    if (ImGui::Button("Remove Bond")) {
                        Physics::removeBond(bond.a, bond.b);
                        selectedBond = -1;
                    }
                }
            }
        }
        ImGui::End();
    }

    // Show save/load dialogs if requested
    drawLoadWindow();
    drawSaveWindow();
    drawSaveManager();
}

void Game::drawLoadWindow(){
    if(!_showLoadWindow) return;
    ImGui::SetNextWindowSize(ImVec2(500,400), ImGuiCond_FirstUseEver);
    if(ImGui::Begin("Load Game State", &_showLoadWindow, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)){
        ImGui::Text("Select a save file to load:");
        ImGui::Separator();
        
        // Get save metadata for better display
        auto saveMetadata = SaveSystem::getSaveMetadata(SaveSystem::SaveType::GAME);
        
        if(saveMetadata.empty()) {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No save files found.");
        } else {
            for(const auto& meta : saveMetadata) {
                // Format timestamp for display
                std::time_t time = meta.creationTime;
                std::tm* tm = std::localtime(&time);
                char timeStr[64];
                std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", tm);
                
                // Format file size
                std::string sizeStr;
                if(meta.fileSize < 1024) {
                    sizeStr = std::to_string(meta.fileSize) + " B";
                } else if(meta.fileSize < 1024*1024) {
                    sizeStr = std::to_string(meta.fileSize/1024) + " KB";
                } else {
                    sizeStr = std::to_string(meta.fileSize/(1024*1024)) + " MB";
                }
                
                // Create display string
                std::string displayText = meta.customLabel.empty() ? 
                    meta.filename : meta.customLabel;
                displayText += " (" + std::string(timeStr) + ", " + sizeStr + ")";
                
                if(ImGui::Selectable(displayText.c_str())){
                    loadState(meta.fullPath);
                _showLoadWindow = false;
            }
                
                // Show tooltip with full path
                if(ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Path: %s", meta.fullPath.c_str());
        }
            }
        }
        
        ImGui::Separator();
        
        // Add some management options
        if(ImGui::Button("Refresh")) {
            updateSaveFiles();
        }
        ImGui::SameLine();
        if(ImGui::Button("Clean Old Saves")) {
            SaveSystem::cleanupOldSaves(SaveSystem::SaveType::GAME, 10);
            updateSaveFiles();
        }
        ImGui::SameLine();
        if(ImGui::Button("Close")) {
            _showLoadWindow = false;
        }
    }
    ImGui::End();
}

void Game::shutdown() {
    // Automatically save game state upon shutdown
    saveStateWithLog();
}

void Game::updateSaveFiles() {
    _saveFiles = SaveSystem::listFiles(SaveSystem::SaveType::GAME);
}

void Game::saveState(const std::string& filename) {
    using json = nlohmann::json; json j;
    j["currentZone"] = mgr.currentIndex();
    json zonesJson = json::array();
    for (const auto& z : mgr.zones()) {
        json zj; zj["name"]=z.name();
        zj["r"]=z.r; zj["g"]=z.g; zj["b"]=z.b;
        json strokesJ = json::array();
        for(const auto& s : z.strokes){json sj; sj["color"]={s.r,s.g,s.b}; sj["points"]=s.points; strokesJ.push_back(sj);}
        zj["strokes"]=strokesJ;
        // Serialize the 3-D world owned by this zone
        zj["world"] = z.world();
        zonesJson.push_back(zj);
    }
    j["zones"] = zonesJson;
    auto& zoneWorld = mgr.active().world(); // legacy helper
    // Camera and player view
    j["cameraPos"]   = {_cameraPos.x, _cameraPos.y, _cameraPos.z};
    j["cameraFront"] = {_cameraFront.x, _cameraFront.y, _cameraFront.z};
    j["cameraUp"]    = {_cameraUp.x, _cameraUp.y, _cameraUp.z};
    j["yaw"]   = _mouseHandler.getYaw();
    j["pitch"] = _mouseHandler.getPitch();

    j["currentColor"] = {_currentColor[0], _currentColor[1], _currentColor[2]};
    j["currentTool"]  = static_cast<int>(_currentTool.getType());

    j["worldMode"]     = static_cast<int>(_world.getMode());
    j["worldPhysics"]   = _world.isPhysicsEnabled();
    // Save physics laws
    {
        nlohmann::json lawsJ = nlohmann::json::array();
        for (const auto& law : Physics::getLaws()) {
            nlohmann::json lj;
            lj["id"] = law.id; lj["name"]=law.name; lj["type"]=static_cast<int>(law.type); lj["enabled"]=law.enabled;
            lj["strength"]=law.strength; lj["damping"]=law.damping; lj["direction"] = {law.direction.x, law.direction.y, law.direction.z};
            const auto& t = law.target;
            nlohmann::json tj; tj["allObjects"]=t.allObjects; tj["limitByGeometry"]=t.limitByGeometry; tj["limitByObjectType"]=t.limitByObjectType; tj["limitByAttribute"]=t.limitByAttribute; tj["limitByTag"]=t.limitByTag; tj["limitByExplicitList"]=t.limitByExplicitList;
            tj["geometryTypes"] = nlohmann::json::array(); for(auto g: t.geometryTypes) tj["geometryTypes"].push_back(static_cast<int>(g));
            tj["objectTypes"] = t.objectTypes; tj["attributeKey"]=t.attributeKey; tj["attributeValue"]=t.attributeValue; tj["tag"]=t.tag; tj["objectIdentifiers"]=t.objectIdentifiers;
            lj["target"] = tj;
            lawsJ.push_back(lj);
        }
        j["physicsLaws"] = lawsJ;
    }
    j["flying"]        = Physics::getFlying();

    // Dynamic objects (skip baseline 0 & 1)
    // NOTE: We now serialize all objects per-zone under zj["world"] (authoritative source) (single source of truth).
    // The legacy top-level j["objects"] caused duplication on load and physics pushed overlapping copies apart (scatter).
    // Keeping this commented to avoid reintroducing the bug while preserving the old code for reference.
    // nlohmann::json objArr = nlohmann::json::array();
    // const auto& objs = zoneWorld.getOwnedObjects();
    // for(size_t i=2;i<objs.size();++i){ const auto& o = objs[i];
    //     // Reuse full object serialization (includes transform, geometryType, faceColors, faceTextures)
    //     json oj = *o;
    //     objArr.push_back(std::move(oj));
    // }
    // j["objects"] = objArr;

    std::ofstream out(filename); out<<j.dump(2);
}

void Game::saveStateWithLog(const std::string& customName) {
    using nlohmann::json;
    json j;
    
    // Build the save data
    j["currentZone"] = mgr.currentIndex();
    json zonesJson = json::array();
    for (const auto& z : mgr.zones()) {
        json zj; zj["name"]=z.name();
        zj["r"]=z.r; zj["g"]=z.g; zj["b"]=z.b;
        json strokesJ = json::array();
        for(const auto& s : z.strokes){json sj; sj["color"]={s.r,s.g,s.b}; sj["points"]=s.points; strokesJ.push_back(sj);}
        zj["strokes"]=strokesJ;
        // Serialize the 3-D world owned by this zone
        zj["world"] = z.world();
        zonesJson.push_back(zj);
    }
    j["zones"] = zonesJson;
    auto& zoneWorld = mgr.active().world(); // legacy helper
    // Camera and player view
    j["cameraPos"]   = {_cameraPos.x, _cameraPos.y, _cameraPos.z};
    j["cameraFront"] = {_cameraFront.x, _cameraFront.y, _cameraFront.z};
    j["cameraUp"]    = {_cameraUp.x, _cameraUp.y, _cameraUp.z};
    j["yaw"]   = _mouseHandler.getYaw();
    j["pitch"] = _mouseHandler.getPitch();

    j["currentColor"] = {_currentColor[0], _currentColor[1], _currentColor[2]};
    j["currentTool"]  = static_cast<int>(_currentTool.getType());

    j["worldMode"]     = static_cast<int>(_world.getMode());
    j["worldPhysics"]   = _world.isPhysicsEnabled();
    // Save physics laws
    {
        nlohmann::json lawsJ = nlohmann::json::array();
        for (const auto& law : Physics::getLaws()) {
            nlohmann::json lj;
            lj["id"] = law.id; lj["name"]=law.name; lj["type"]=static_cast<int>(law.type); lj["enabled"]=law.enabled;
            lj["strength"]=law.strength; lj["damping"]=law.damping; lj["direction"] = {law.direction.x, law.direction.y, law.direction.z};
            const auto& t = law.target;
            nlohmann::json tj; tj["allObjects"]=t.allObjects; tj["limitByGeometry"]=t.limitByGeometry; tj["limitByObjectType"]=t.limitByObjectType; tj["limitByAttribute"]=t.limitByAttribute; tj["limitByTag"]=t.limitByTag; tj["limitByExplicitList"]=t.limitByExplicitList;
            tj["geometryTypes"] = nlohmann::json::array(); for(auto g: t.geometryTypes) tj["geometryTypes"].push_back(static_cast<int>(g));
            tj["objectTypes"] = t.objectTypes; tj["attributeKey"]=t.attributeKey; tj["attributeValue"]=t.attributeValue; tj["tag"]=t.tag; tj["objectIdentifiers"]=t.objectIdentifiers;
            lj["target"] = tj;
            lawsJ.push_back(lj);
        }
        j["physicsLaws"] = lawsJ;
    }
    j["flying"]        = Physics::getFlying();

    // Dynamic objects (skip baseline 0 & 1)
    nlohmann::json objArr = nlohmann::json::array();
    const auto& objs = zoneWorld.getOwnedObjects();
    for(size_t i=2;i<objs.size();++i){ const auto& o = objs[i];
        // Reuse full object serialization (includes transform, geometryType, faceColors, faceTextures)
        json oj = *o;
        objArr.push_back(std::move(oj));
    }
    j["objects"] = objArr;

    // Use the new SaveSystem to write the file
    SaveSystem::writeJson(j, customName, SaveSystem::SaveType::GAME);
}

void Game::loadState(const std::string& filename){
    try {
        using json=nlohmann::json; std::ifstream in(filename); if(!in) { std::cerr<<"Could not open "<<filename<<"\n"; return; }
        json j; in>>j;
        // Reset physics registries to avoid stale velocities/bonds affecting freshly loaded objects
        Physics::resetRigidBodies();
        Physics::clearBonds();
        size_t currentZoneIdx = j.value("currentZone",0);
        auto& zonesVec = mgr.zones(); zonesVec.clear();
        if(j.contains("zones")) {
            for(const auto& zj: j["zones"]) {
                std::string name = zj.value("name","Untitled Zone");
                Zone z(name);
                z.r = zj.value("r",0.05f);
                z.g = zj.value("g",0.05f);
                z.b = zj.value("b",0.1f);
                if(zj.contains("strokes")) {
                    for(const auto& sj: zj["strokes"]) {
                        Zone::Stroke s; auto col = sj.value("color", std::vector<float>{1,1,1});
                        if(col.size()>=3){s.r=col[0]; s.g=col[1]; s.b=col[2];}
                        s.points = sj.value("points", std::vector<float>{});
                        z.strokes.push_back(std::move(s));
                    }
                }
                // Load 3-D world objects for this zone
                if(zj.contains("world")) {
                    from_json(zj["world"], z.world());
                }
                zonesVec.push_back(std::move(z));
            }
        }

        if(zonesVec.empty()) {
            zonesVec.push_back(Zone("Default Zone"));
        }
        mgr.switchTo(std::min(currentZoneIdx, zonesVec.size()-1));
        auto& zoneWorld = mgr.active().world(); // legacy helper

        // Load camera and player view
        if(j.contains("cameraPos")) {
            _cameraPos = glm::vec3(j["cameraPos"][0], j["cameraPos"][1], j["cameraPos"][2]);
        }
        if(j.contains("cameraFront")) {
            _cameraFront = glm::vec3(j["cameraFront"][0], j["cameraFront"][1], j["cameraFront"][2]);
        }
        if(j.contains("cameraUp")) {
            _cameraUp = glm::vec3(j["cameraUp"][0], j["cameraUp"][1], j["cameraUp"][2]);
        }
        _mouseHandler.setYaw(j.value("yaw", -90.0f));
        _mouseHandler.setPitch(j.value("pitch", 0.0f));

        if(j.contains("currentColor")) {
            _currentColor[0] = j["currentColor"][0];
            _currentColor[1] = j["currentColor"][1];
            _currentColor[2] = j["currentColor"][2];
        }
        _currentTool = Tool(static_cast<Tool::Type>(j.value("currentTool", static_cast<int>(Tool::Type::Brush))));

        _world.setMode(static_cast<Ourverse::GameMode>(j.value("worldMode", static_cast<int>(Ourverse::GameMode::Creative))));
        bool phys = j.value("worldPhysics", true);
        if(_world.isPhysicsEnabled()!=phys) _world.togglePhysics();
        Physics::setFlying(j.value("flying", false));
        // Load physics laws
        if (j.contains("physicsLaws")) {
            // Clear existing laws by removing all
            // No direct clear available; remove by id iteration
            // We rely on internal registry, so we rebuild:
            // Collect current ids
            std::vector<int> ids; for (const auto& L : Physics::getLaws()) ids.push_back(L.id);
            for (int id : ids) Physics::removeLaw(id);
            for (const auto& lj : j["physicsLaws"]) {
                Physics::PhysicsLaw law;
                law.name = lj.value("name", std::string("Law"));
                law.type = static_cast<Physics::LawType>(lj.value("type", 0));
                law.enabled = lj.value("enabled", true);
                law.strength = lj.value("strength", 9.81f);
                law.damping = lj.value("damping", 0.1f);
                auto dir = lj.value("direction", std::vector<float>{0, -1, 0}); if (dir.size()==3) law.direction = glm::vec3(dir[0],dir[1],dir[2]);
                const auto& tj = lj["target"];
                law.target.allObjects = tj.value("allObjects", true);
                law.target.limitByGeometry = tj.value("limitByGeometry", false);
                law.target.limitByObjectType = tj.value("limitByObjectType", false);
                law.target.limitByAttribute = tj.value("limitByAttribute", false);
                law.target.limitByTag = tj.value("limitByTag", false);
                law.target.limitByExplicitList = tj.value("limitByExplicitList", false);
                law.target.geometryTypes.clear();
                if (tj.contains("geometryTypes")) { for (const auto& gi : tj["geometryTypes"]) law.target.geometryTypes.push_back(static_cast<Object::GeometryType>(gi.get<int>())); }
                law.target.objectTypes.clear(); if (tj.contains("objectTypes")) { for (const auto& s : tj["objectTypes"]) law.target.objectTypes.push_back(s.get<std::string>()); }
                law.target.attributeKey = tj.value("attributeKey", std::string(""));
                law.target.attributeValue = tj.value("attributeValue", std::string(""));
                law.target.tag = tj.value("tag", std::string(""));
                law.target.objectIdentifiers.clear(); if (tj.contains("objectIdentifiers")) { for (const auto& s : tj["objectIdentifiers"]) law.target.objectIdentifiers.push_back(s.get<std::string>()); }
                Physics::addLaw(law);
            }
        }

        // Load dynamic objects (skip baseline 0 & 1)
        // NOTE: The authoritative per-zone serialization is zj["world"].
        // The legacy top-level j["objects"] duplicated the world objects on load, causing overlapping instances
        // that then scattered due to physics resolution. We keep the legacy loader commented out.
        // if(j.contains("objects")) {
        //     const auto& objArr = j["objects"];
        //     for(size_t i=0;i<objArr.size();++i){
        //         if(i<2) continue; // Skip baseline objects
        //         std::unique_ptr<Object> obj(new Object());
        //         from_json(objArr[i], *obj);
        //         zoneWorld.addObject(std::move(obj));
        //     }
        // }

    }
    catch(const std::exception& e) {
        std::cerr<<"Error loading state: "<<e.what()<<"\n";
    }
}

bool Game::getAdvanced2DBrush() const {
    return _useAdvanced2DBrush;
}

void Game::setAdvanced2DBrush(bool value) {
    _useAdvanced2DBrush = value;
}

bool Game::getMouseLeftPressedLast() const {
    return _mouseLeftPressedLast;
}

void Game::setMouseLeftPressedLast(bool value) {
    _mouseLeftPressedLast = value;
}

void Game::drawSaveWindow() {
    if(!_showSaveWindow) return;
    ImGui::SetNextWindowSize(ImVec2(400,200), ImGuiCond_FirstUseEver);
    if(ImGui::Begin("Save Game State", &_showSaveWindow, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)){
        ImGui::Text("Save your current game state:");
        ImGui::Separator();
        
        ImGui::Text("Save Name (optional):");
        ImGui::InputText("##SaveName", _customSaveName, sizeof(_customSaveName));
        
        ImGui::Separator();
        
        if(ImGui::Button("Save with Timestamp")) {
            saveStateWithLog("");
            _showSaveWindow = false;
        }
        ImGui::SameLine();
        if(ImGui::Button("Save with Custom Name")) {
            saveStateWithLog(_customSaveName);
            _showSaveWindow = false;
        }
        ImGui::SameLine();
        if(ImGui::Button("Cancel")) {
            _showSaveWindow = false;
        }
        
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Saves are stored in: saves/games/");
    }
    ImGui::End();
}

void Game::drawSaveManager() {
    if(!_showSaveManager) return;
    ImGui::SetNextWindowSize(ImVec2(600,500), ImGuiCond_FirstUseEver);
    if(ImGui::Begin("Save Manager", &_showSaveManager, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)){
        
        // Tabs for different save types
        if(ImGui::BeginTabBar("SaveTypes")) {
            
            // Game saves tab
            if(ImGui::BeginTabItem("Game Saves")) {
                auto gameSaves = SaveSystem::getSaveMetadata(SaveSystem::SaveType::GAME);
                
                ImGui::Text("Game Saves (%zu files)", gameSaves.size());
                ImGui::Separator();
                
                if(gameSaves.empty()) {
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No game saves found.");
                } else {
                    for(const auto& meta : gameSaves) {
                        std::time_t time = meta.creationTime;
                        std::tm* tm = std::localtime(&time);
                        char timeStr[64];
                        std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", tm);
                        
                        std::string sizeStr;
                        if(meta.fileSize < 1024) {
                            sizeStr = std::to_string(meta.fileSize) + " B";
                        } else if(meta.fileSize < 1024*1024) {
                            sizeStr = std::to_string(meta.fileSize/1024) + " KB";
                        } else {
                            sizeStr = std::to_string(meta.fileSize/(1024*1024)) + " MB";
                        }
                        
                        std::string displayText = meta.customLabel.empty() ? 
                            meta.filename : meta.customLabel;
                        displayText += " (" + std::string(timeStr) + ", " + sizeStr + ")";
                        
                        if(ImGui::Selectable(displayText.c_str())){
                            loadState(meta.fullPath);
                        }
                        
                        if(ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Path: %s", meta.fullPath.c_str());
                        }
                    }
                }
                
                ImGui::Separator();
                if(ImGui::Button("Clean Old Saves")) {
                    SaveSystem::cleanupOldSaves(SaveSystem::SaveType::GAME, 10);
                }
                
                ImGui::EndTabItem();
            }
            
            // Avatar saves tab
            if(ImGui::BeginTabItem("Avatar Saves")) {
                auto avatarSaves = SaveSystem::getSaveMetadata(SaveSystem::SaveType::AVATAR);
                
                ImGui::Text("Avatar Saves (%zu files)", avatarSaves.size());
                ImGui::Separator();
                
                if(avatarSaves.empty()) {
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No avatar saves found.");
                } else {
                    for(const auto& meta : avatarSaves) {
                        std::time_t time = meta.creationTime;
                        std::tm* tm = std::localtime(&time);
                        char timeStr[64];
                        std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", tm);
                        
                        std::string sizeStr;
                        if(meta.fileSize < 1024) {
                            sizeStr = std::to_string(meta.fileSize) + " B";
                        } else if(meta.fileSize < 1024*1024) {
                            sizeStr = std::to_string(meta.fileSize/1024) + " KB";
                        } else {
                            sizeStr = std::to_string(meta.fileSize/(1024*1024)) + " MB";
                        }
                        
                        std::string displayText = meta.customLabel.empty() ? 
                            meta.filename : meta.customLabel;
                        displayText += " (" + std::string(timeStr) + ", " + sizeStr + ")";
                        
                        if(ImGui::Selectable(displayText.c_str())){
                            // TODO: Load avatar save
                        }
                        
                        if(ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Path: %s", meta.fullPath.c_str());
                        }
                    }
                }
                
                ImGui::Separator();
                if(ImGui::Button("Clean Old Saves")) {
                    SaveSystem::cleanupOldSaves(SaveSystem::SaveType::AVATAR, 10);
                }
                
                ImGui::EndTabItem();
            }
            
            // Design saves tab
            if(ImGui::BeginTabItem("Design Saves")) {
                auto designSaves = SaveSystem::getSaveMetadata(SaveSystem::SaveType::DESIGN);
                
                ImGui::Text("Design Saves (%zu files)", designSaves.size());
                ImGui::Separator();
                
                if(designSaves.empty()) {
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No design saves found.");
                } else {
                    for(const auto& meta : designSaves) {
                        std::time_t time = meta.creationTime;
                        std::tm* tm = std::localtime(&time);
                        char timeStr[64];
                        std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", tm);
                        
                        std::string sizeStr;
                        if(meta.fileSize < 1024) {
                            sizeStr = std::to_string(meta.fileSize) + " B";
                        } else if(meta.fileSize < 1024*1024) {
                            sizeStr = std::to_string(meta.fileSize/1024) + " KB";
                        } else {
                            sizeStr = std::to_string(meta.fileSize/(1024*1024)) + " MB";
                        }
                        
                        std::string displayText = meta.customLabel.empty() ? 
                            meta.filename : meta.customLabel;
                        displayText += " (" + std::string(timeStr) + ", " + sizeStr + ")";
                        
                        if(ImGui::Selectable(displayText.c_str())){
                            // TODO: Load design save
                        }
                        
                        if(ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Path: %s", meta.fullPath.c_str());
                        }
                    }
                }
                
                ImGui::Separator();
                if(ImGui::Button("Clean Old Saves")) {
                    SaveSystem::cleanupOldSaves(SaveSystem::SaveType::DESIGN, 10);
                }
                
                ImGui::EndTabItem();
            }
            
            ImGui::EndTabBar();
        }
        
        ImGui::Separator();
        if(ImGui::Button("Close")) {
            _showSaveManager = false;
        }
    }
    ImGui::End();
}

void Game::_generateCustomPolyhedron() {
    _customPolyhedronVertices.clear();
    _customPolyhedronFaces.clear();
    
    // Generate vertices on a sphere
    float radius = 0.5f;
    for (int i = 0; i < _customPolyhedronVertexCount; ++i) {
        // Use spherical coordinates for even distribution
        float phi = acos(1.0f - 2.0f * (i + 0.5f) / _customPolyhedronVertexCount);
        float theta = M_PI * (1.0f + sqrt(5.0f)) * (i + 0.5f);
        
        float x = radius * sin(phi) * cos(theta);
        float y = radius * sin(phi) * sin(theta);
        float z = radius * cos(phi);
        
        _customPolyhedronVertices.push_back(glm::vec3(x, y, z));
    }
    
    // Generate faces using convex hull approximation
    // For simplicity, we'll create triangular faces
    int facesCreated = 0;
    for (int i = 0; i < _customPolyhedronVertexCount && facesCreated < _customPolyhedronFaceCount; ++i) {
        for (int j = i + 1; j < _customPolyhedronVertexCount && facesCreated < _customPolyhedronFaceCount; ++j) {
            for (int k = j + 1; k < _customPolyhedronVertexCount && facesCreated < _customPolyhedronFaceCount; ++k) {
                // Create a triangular face
                std::vector<int> face = {i, j, k};
                _customPolyhedronFaces.push_back(face);
                facesCreated++;
            }
        }
    }
    
    // If we need more faces, create some with more vertices
    while (facesCreated < _customPolyhedronFaceCount && _customPolyhedronVertexCount >= 4) {
        std::vector<int> face;
        for (int v = 0; v < 4 && v < _customPolyhedronVertexCount; ++v) {
            face.push_back(v);
        }
        _customPolyhedronFaces.push_back(face);
        facesCreated++;
    }
}

} // namespace Core 