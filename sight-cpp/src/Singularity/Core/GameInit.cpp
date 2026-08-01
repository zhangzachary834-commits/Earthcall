// GameInit.cpp – Game initialisation, GLFW callbacks
// Split from Game.cpp during refactor.

#include "Game.hpp"
#include "Singularity/Core/Engine.hpp"
#include "../../../imgui/backends/imgui_impl_glfw.h"
#include "Form/Object/Object.hpp"
#include "Rendering/BrushSystem.hpp"
#include "Rendering/ShadingSystem.hpp"
#include "OurVerse/AdvancedFacePaint.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include "ZonesOfEarth/Physics/DefaultPhysicsLaws.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "Singularity/TransferPolicy.hpp"
#include "Person/Body/BodyPart/BodyPart.hpp"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <glm/glm.hpp>
#include <iostream>
#include <memory>

extern ZoneManager mgr;

namespace Core {

bool Game::init() {
    _window = Engine::instance().window();
    if (!_window) return false;

    // Laws listen from the first frame: every ECA::Event published anywhere
    // in the engine becomes a fact the law network can act on.
    _lawManager.connectToEventBus();
    (void)TransferPolicy::instance();   // the gate exists from the first frame

    // Collisions feed the relation graph (recordCollision) as well as the
    // ECA echo above; without this, PhysicsCollisionEvent had no listener
    // and collision history never reached the relation registry.
    Physics::setupPhysicsEventListeners();

    // Inject default physics laws (gravity and kinematics)
    for (const auto& law : Physics::createDefaultPhysicsLaws()) {
        law->setEnabled(!Physics::getLegacyEngineEnabled());
        _lawManager.add(law);
    }

    // The Universe: what continuous laws watch and quantified conditions
    // (ForAny/ForAll) range over — the active world's objects, the laws
    // themselves (so metalaws can quantify over laws), and the player.
    // Evaluated lazily each tick, well after zones exist.
    Universe::instance().setProvider([this](std::vector<Singular*>& beings) {
        // Provide the active World itself so laws can target it or spawn into it
        beings.push_back(&mgr.active().world());
        
        for (const auto& obj : mgr.active().world().getOwnedObjects()) {
            if (obj) beings.push_back(obj.get());
        }
        for (const auto& law : _lawManager.getAll()) {
            if (law) beings.push_back(law.get());
        }
        // Relations are Singulars too: quantifiers (ForAny Relation...)
        // range over them like any other being.
        for (const auto& rel : mgr.active().formation().relations().getAll()) {
            if (rel) beings.push_back(rel.get());
        }
        // The transfer gate is a legible being: laws govern set-to-set
        // access by writing @transfer-policy.gate.* properties.
        beings.push_back(&TransferPolicy::instance());
        beings.push_back(&_player);
        // ALL zones, not just the active one: zones are the governance
        // geography — laws quantify over them (ForAny Zone ...) and address
        // them by name (@Home.owner) even while unloaded.
        for (auto& zone : mgr.zones()) {
            beings.push_back(&zone);
        }
    });

    // The relation GRAPH — the edge view Related conditions query
    // ("is x related to y in type t?"): the active zone's Formation.
    Universe::instance().setRelationProvider([](std::vector<Relation*>& relations) {
        for (const auto& rel : mgr.active().formation().relations().getAll()) {
            if (rel) relations.push_back(rel.get());
        }
    });
    // ...and its write side: newborn relations (a concept's reborn
    // inter-member structure) join the same Formation.
    Universe::instance().setRelationRegistrar([](std::shared_ptr<Relation> relation) {
        mgr.active().formation().relations().add(std::move(relation));
    });

    // Init GL state – depth test already enabled in ShadingSystem::init()
    ShadingSystem::init();

    printf("[Init] Checkpoint A: after ShadingSystem::init()\n");

    // --------------------------------------------------------------
    // Setup zones & menu
    // --------------------------------------------------------------
    mgr.addZone(Zone("Sanctum of Beginnings", "default"));
    mgr.addZone(Zone("Temple of Echoes", "default"));
    mgr.addZone(Zone("Cavern of Light", "default"));
    mgr.addZone(Zone("Character Architect Forge", "default"));
    ensureHomeZone();

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
    _mainMenu.addOption("Save As...", GLFW_KEY_A, [this]() { _saveLoad.showSaveWindow = true; });
    _mainMenu.addOption("Load", GLFW_KEY_L, [this]() { updateSaveFiles(); _saveLoad.showLoadWindow = true; });
    _mainMenu.addOption("Save Manager", GLFW_KEY_G, [this]() { _saveLoad.showManager = true; });
    _mainMenu.addOption("Toggle Chat", GLFW_KEY_H, [this]() { _showChatWindow = !_showChatWindow; });
    _mainMenu.addOption("Toggle Toolbar", GLFW_KEY_T, [this]() { _showToolbar = !_showToolbar; });
    _mainMenu.addOption("Toggle Physics", GLFW_KEY_P, [this]() { _world.togglePhysics(); });
    _mainMenu.addOption("Toggle Legacy Engine", GLFW_KEY_E, [this]() {
        bool newState = !Physics::getLegacyEngineEnabled();
        Physics::setLegacyEngineEnabled(newState);
        for (const auto& law : _lawManager.getAll()) {
            if (law->name() == "physics: gravity" || law->name() == "physics: kinematics") {
                law->setEnabled(!newState);
            }
        }
    });
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

    // --------------------------------------------------------------
    // World baseline objects (spinning cube + ground)
    // --------------------------------------------------------------
    _world.setCamera(&_camera.pos);

    {
        std::shared_ptr<Object> cube = std::make_shared<Object>();
        std::shared_ptr<Object> ground = std::make_shared<Object>();
        // Tag these as baseline placeholders so we can safely special-case them later
        cube->setAttribute("baseline", "cube");
        ground->setAttribute("baseline", "ground");
        mgr.active().world().addObject(std::move(cube));
        mgr.active().world().addObject(std::move(ground));
    }

    printf("[Init] Checkpoint C: baseline objects created\n");

    // Physics default true
    _world.setMode(Ourverse::GameMode::Creative);

    // Ensure _player initial position matches _camera.pos
    glm::vec3 anchor = _camera.pos - glm::vec3(0.0f, _player.getBody().getEyeHeight(), 0.0f);
    _player.position = anchor;
    // Route LocomotionChanged events to their target Person (once), then settle
    // the avatar into a living idle by default; movement publishes the event
    // that swaps this for a walk cycle (see Game::update).
    Person::installLocomotionRouting();
    _player.playIdleAutomation();
    _player.updatePose();

    // --------------------------------------------------------------
    // Register GLFW callbacks that need 'this' pointer via user pointer
    // --------------------------------------------------------------
    registerCallbacks();
    printf("[Init] Checkpoint D: callbacks registered\n");

    std::cout << "🔥 Earthcall Game initialised." << std::endl;

    // Initialize custom polyhedron
    try {
        _polyhedron.generateCustom();
        printf("[Init] Checkpoint E: custom polyhedron generated\n");
    } catch (...) {
        printf("[Init] Warning: custom polyhedron generation failed, continuing with defaults.\n");
    }

    // Initialize default brush presets
    _brush.presets.clear();

    _brush.presets.push_back(BrushPresetBuilder("Soft Brush", BrushType::Normal)
        .radius(0.15f).softness(0.3f).opacity(0.7f).flow(0.8f).spacing(0.05f).density(0.5f).strength(0.5f).build());
    _brush.presets.push_back(BrushPresetBuilder("Hard Brush", BrushType::Normal)
        .radius(0.1f).softness(1.0f).opacity(1.0f).flow(1.0f).spacing(0.02f).density(0.5f).strength(0.5f).build());
    _brush.presets.push_back(BrushPresetBuilder("Airbrush", BrushType::Airbrush)
        .radius(0.2f).softness(0.5f).opacity(0.5f).flow(0.6f).spacing(0.1f).density(0.8f).strength(0.5f).build());
    _brush.presets.push_back(BrushPresetBuilder("Chalk", BrushType::Chalk)
        .radius(0.12f).softness(0.2f).opacity(0.9f).flow(0.7f).spacing(0.08f).density(0.5f).strength(0.5f).build());
    _brush.presets.push_back(BrushPresetBuilder("Smudge", BrushType::Smudge)
        .radius(0.18f).softness(0.4f).opacity(1.0f).flow(1.0f).spacing(0.03f).density(0.5f).strength(0.7f).build());
    _brush.presets.push_back(BrushPresetBuilder("Clone", BrushType::Clone)
        .radius(0.15f).softness(0.6f).opacity(0.8f).flow(1.0f).spacing(0.05f).density(0.5f).strength(0.5f).build());

    // Initialize Advanced Face Paint System
    AdvancedFacePaint::initializeAdvancedPainter();

    // Initialize default advanced face paint settings
    _advancedFacePaint.gradient = AdvancedFacePaint::GradientSettings();
    _advancedFacePaint.smudge = AdvancedFacePaint::SmudgeSettings();

    // Initialize keyboard handler
    _keyboardHandler.setGameInstance(this);

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
            Object* target = _selectedObject3D;
            if (!target) {
                if (_current3DTarget == ToolTarget3D::AvatarBodyParts) {
                    for (auto* part : _player.getBody().parts) {
                        if (part) {
                            target = part;
                            break;
                        }
                    }
                } else {
            const auto& objects = mgr.active().world().getOwnedObjects();
                for (const auto& up : objects) {
                    Object* obj = up.get();
                    if (obj) {
                            target = obj;
                        break;
                    }
                }
                }
            }
            if (target) {
                // For now, undo the last stroke on the first face
                target->undoStroke(0);
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

    return true;
}

// ---------------------------------------------------------------------------
// GLFW Callbacks
// ---------------------------------------------------------------------------

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
    // Forward to ImGui by calling its backend directly (same pattern as the
    // mouse-button and scroll callbacks). The previous code forwarded through a
    // runtime-saved function pointer (_prevCursorPosCallback); when ImGui is
    // initialised with install_callbacks=true that saved pointer could be stale
    // and crash (EXC_BAD_ACCESS in mouseMoved:). Calling the symbol directly is
    // link-time resolved and safe.
    ImGui_ImplGlfw_CursorPosCallback(win, xpos, ypos);
    if (self) self->_mouseHandler.handleMouseMove(xpos, ypos);
}

void Game::sWindowFocusCallback(GLFWwindow* win, int focused) {
    Game* self = static_cast<Game*>(glfwGetWindowUserPointer(win));
    if (self && self->_prevFocusCallback) {
        self->_prevFocusCallback(win, focused);
    }
    if (self) self->_mouseHandler.onWindowFocus(focused);
}

// --------------------------------------------------------------
// Manifesto: "Every Person has a Home they fully own." Idempotent:
// if the player already owns ANY zone (fresh boot or a loaded save),
// nothing happens; otherwise a Home zone is born, owned by them.
// Named plainly "Home" so laws address it as @Home.* without spaces.
// --------------------------------------------------------------
void Game::ensureHomeZone() {
    const std::string playerId = _player.getIdentifier();
    if (playerId.empty()) return;   // an unnamed person cannot hold title

    for (const auto& zone : mgr.zones()) {
        if (zone.owner() == playerId) return;   // already homed
    }
    // A save from before ownership existed may hold an unowned "Home" —
    // claim it instead of minting a name-twin (identifiers must stay unique).
    for (auto& zone : mgr.zones()) {
        if (zone.name() == "Home" && zone.owner().empty()) {
            zone.setOwner(playerId);
            return;
        }
    }

    Zone home("Home", "strict", 0.08f, 0.06f, 0.12f);
    home.setOwner(playerId);
    home.setQuality("kind", "home");
    mgr.addZone(std::move(home));
    printf("[Init] Home established for '%s' (zone count now %zu)\n",
           playerId.c_str(), mgr.zones().size());
}

void Game::sFramebufferSizeCallback(GLFWwindow* win, int width, int height) {
    Game* self = static_cast<Game*>(glfwGetWindowUserPointer(win));
    if (self && self->_prevFramebufferSizeCallback) {
        self->_prevFramebufferSizeCallback(win, width, height); // forward to ImGui (or whatever was there)
    }
    if (self) self->onFramebufferSize(width, height);
}

void Game::onFramebufferSize(int /*width*/, int /*height*/) {
    // Nothing to do: render() passes the current framebuffer size to
    // beginFrame every frame, and the backend sets its own viewport from it.
    // (WebGPU additionally has to reconfigure its surface, which it does there.)
}

} // namespace Core
