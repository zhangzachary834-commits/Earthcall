#include "Person/Person.hpp"
#include "Singularity/Input/KeyboardHandler.hpp"
#include "Singularity/Input/MouseHandler.hpp"
// GameInit.cpp – Game initialisation, GLFW callbacks
// Split from Game.cpp during refactor.

#include "Singularity/Core/Engine.hpp"
#include "../../OurVerse/ElementalToolHandler.hpp"
#include "Singularity/Core/Engine.hpp"
#include "../../OurVerse/ElementalToolHandler.hpp"
#include "../../../imgui/backends/imgui_impl_glfw.h"
#include "ConstructedBeing/Object/Object.hpp"
#include "Singularity/Screen/ShadingSystem.hpp"
#include "OurVerse/AdvancedFacePaint.hpp"
#include "ConstructedBeing/Object/Creation/ObjectConcept.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include "ZonesOfEarth/Physics/DefaultPhysicsLaws.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "Singularity/TransferPolicy.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ConstructedBeing/CategoryManager.hpp"
#include "Person/Body/BodyPart/BodyPart.hpp"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <glm/glm.hpp>
#include <iostream>
#include "CreationChannel.hpp"
#include <memory>

extern ZoneManager mgr;
extern MaterialManager materials;   // global Material beings (globals.cpp)
extern CategoryManager categories;

namespace Core {

void Engine::initLogic() {
    // Laws listen from the first frame: every ECA::Event published anywhere
    // in the engine becomes a fact the law network can act on.
    _lawManager->connectToEventBus();
    (void)TransferPolicy::instance();   // the gate exists from the first frame

    // Collisions feed the relation graph (recordCollision) as well as the
    // ECA echo above; without this, PhysicsCollisionEvent had no listener
    // and collision history never reached the relation registry.
    Physics::setupPhysicsEventListeners();

    // Register core concepts like sound-emitters before laws are built
    ConceptRegistry::instance().registerCoreConcepts();

    // Register first-mover CreationChannel
    Singularity::Core::CreationChannel::syncRegister(*_lawManager);

    // Inject default physics laws (gravity and kinematics)
    for (const auto& law : Physics::createDefaultPhysicsLaws()) {
        law->setEnabled(!Physics::getLegacyEngineEnabled());
        _lawManager->add(law);
        // OnEvent laws need their triggers bound explicitly.
        // physics-acoustics is OnEvent with eventType = "contact-began"
        if (law->activation() == Law::Activation::OnEvent) {
            _lawManager->bindTrigger(law->getIdentifier(), law->ecaLoop().eventType);
        }
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
        for (const auto& law : _lawManager->getAll()) {
            if (law) beings.push_back(law.get());
        }
        // Relations are Singulars too: quantifiers (ForAny Relation...)
        // range over them like any other being.
        for (const auto& rel : mgr.active().formation().relations().getAll()) {
            if (rel) beings.push_back(rel.get());
        }
        // Materials are cross-zone shared beings (globals.cpp), and
        // Material.hpp has always claimed "the Law system can address
        // material.clay.baseColor". Until they were provided here that was
        // aspiration: buildProperties() registered the surface and nothing
        // could reach it. Providing them makes the claim true — laws read and
        // drive appearance, and quantifiers range over materials like any
        // other being. See docs/architecture/AUTHORED_CATEGORIES.md §9a.
        for (const auto& material : materials.getAll()) {
            if (material) beings.push_back(material.get());
        }
        for (const auto& category : ::categories.getAll()) {
            if (category) beings.push_back(category.get());
        }
        // The transfer gate is a legible being: laws govern set-to-set
        // access by writing @transfer-policy.gate.* properties.
        beings.push_back(&TransferPolicy::instance());
        beings.push_back(_player.get());
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
    mgr.ensureHomeZone(_player->getIdentifier());

    // Initialize elemental tool handler with zone manager
    _elementalToolHandler = std::make_unique<ElementalToolHandler>(&mgr);

    printf("[Init] Checkpoint B: zones added (%zu)\n", mgr.zones().size());
    // Debug: list zone names to validate memory integrity of strings
    for (size_t i = 0; i < mgr.zones().size(); ++i) {
        const auto& z = mgr.zones()[i];
        printf("[Init] Zone[%zu]: %s | Q=%zu D=%zu\n", i, z.name().c_str(), z.getQualities().size(), z.getDeletability().size());
    }
    fflush(stdout);

    printf("[Init] Menu reset before adding options\n");

    // Populate menu options
    printf("[Init] Checkpoint B1: before menu addOption(Resume)\n");
    _mainMenu.addOption("Resume World", GLFW_KEY_R, [this]() { 
        _mainMenu.close();
        _keyboardHandler->setMenuOpen(false);
        _mouseHandler->setMenuOpen(false);
    });
    printf("[Init] Checkpoint B2: after menu addOption(Resume)\n");

    // Enhanced main menu options (non-destructive; all previous features intact)
    _mainMenu.addOption("Quick Save", GLFW_KEY_S, [this]() {  });
    _mainMenu.addOption("Save As...", GLFW_KEY_A, [this]() { mgr.getSaveLoadState().showSaveWindow = true; });
    _mainMenu.addOption("Load", GLFW_KEY_L, [this]() { mgr.updateSaveFiles(); mgr.getSaveLoadState().showLoadWindow = true; });
    _mainMenu.addOption("Save Manager", GLFW_KEY_G, [this]() { mgr.getSaveLoadState().showManager = true; });
    _mainMenu.addOption("Toggle Chat", GLFW_KEY_H, [this]() { /* _world.showChatWindow = !_world.showChatWindow; */ });
    _mainMenu.addOption("Toggle Toolbar", GLFW_KEY_T, [this]() { /* _world.showToolbar = !_world.showToolbar; */ });
    _mainMenu.addOption("Settings", GLFW_KEY_F2, [this]() {  });
    _mainMenu.addOption("Toggle ImGui Demo", GLFW_KEY_F3, [this]() {  });
    _mainMenu.addOption("Toggle Placement Insp.", GLFW_KEY_F4, [this]() {  });
    _mainMenu.addOption("Toggle Selection Insp.", GLFW_KEY_F5, [this]() {  });
    _mainMenu.addOption("Toggle Dev Mode", GLFW_KEY_GRAVE_ACCENT, [this]() {  });
    
    // Main Tools submenu
    // _mainMenu.beginSubMenu("Main Tools");
    _mainMenu.addOption("Brush Tool", GLFW_KEY_B, [this]() {  });
    _mainMenu.addOption("Move Tool", GLFW_KEY_M, [this]() {  });
    _mainMenu.addOption("Controls / Keymap", GLFW_KEY_K, [this]() { /* _world.showKeymapWindow = true; */ });
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
    _world.setCamera(&_camera->pos);

    {
        std::shared_ptr<Object> cube = std::make_shared<Object>();
        std::shared_ptr<Object> ground = std::make_shared<Object>();
        // Tag these as baseline placeholders so we can safely special-case them later
        cube->setAttribute("baseline", "cube");
        ground->setAttribute("baseline", "ground");
        _world.addOwnedObject(std::move(cube));
        _world.addOwnedObject(std::move(ground));
    }

    printf("[Init] Checkpoint C: baseline objects created\n");


    // Ensure _player initial position matches _camera.pos
    glm::vec3 anchor = _camera->pos - glm::vec3(0.0f, _player->getBody().getEyeHeight(), 0.0f);
    _player->position = anchor;
    // Route LocomotionChanged events to their target Person (once), then settle
    // the avatar into a living idle by default; movement publishes the event
    // that swaps this for a walk cycle (see Engine::update).
    Person::installLocomotionRouting();
    _player->playIdleAutomation();
    _player->updatePose();

    // --------------------------------------------------------------
    // Register GLFW callbacks that need 'this' pointer via user pointer
    // --------------------------------------------------------------
    registerCallbacks();
    printf("[Init] Checkpoint D: callbacks registered\n");

    std::cout << "🔥 Earthcall Game initialised." << std::endl;

    // Initialize Advanced Face Paint System
    AdvancedFacePaint::initializeAdvancedPainter();

    // Initialize keyboard handler
    // Sync menu state between handlers
    _keyboardHandler->setMenuOpen(_mainMenu.isOpen());
    _mouseHandler->setMenuOpen(_mainMenu.isOpen());

    // Set up specific game callbacks
    _keyboardHandler->bindKey(GLFW_KEY_M, "toggle_menu", [this]() {
        _mainMenu.toggle();
        _keyboardHandler->setMenuOpen(_mainMenu.isOpen());
        _mouseHandler->setMenuOpen(_mainMenu.isOpen());
    });
    _keyboardHandler->bindKey(GLFW_KEY_ESCAPE, "toggle_cursor_lock", [this]() {
        _mouseHandler->toggleCursorLock(_window);
    });
    _keyboardHandler->bindKey(GLFW_KEY_H, "toggle_chat", [this]() { /* _world.showChatWindow = !_world.showChatWindow; */ });
    _keyboardHandler->bindKey(GLFW_KEY_I, "toggle_integration_ui", [this]() { /* _world.showIntegrationUI = !_world.showIntegrationUI; */ });
    _keyboardHandler->bindKey(GLFW_KEY_T, "toggle_toolbar", [this]() { /* _world.showToolbar = !_world.showToolbar; */ });
    _keyboardHandler->bindKey(GLFW_KEY_1, "perspective_first_person", [this]() { _currentPerspective = PerspectiveMode::FirstPerson; });
    _keyboardHandler->bindKey(GLFW_KEY_2, "perspective_second_person", [this]() { _currentPerspective = PerspectiveMode::SecondPerson; });
    _keyboardHandler->bindKey(GLFW_KEY_3, "perspective_third_person", [this]() { _currentPerspective = PerspectiveMode::ThirdPerson; });
    _keyboardHandler->bindKey(GLFW_KEY_F, "toggle_flight", [this]() {
        Physics::toggleFlying();
    });
    _keyboardHandler->bindKey(GLFW_KEY_C, "switch_to_character_zone", [this]() {
        const auto& zones = mgr.zones();
        for (size_t i = 0; i < zones.size(); ++i) {
            if (zones[i].name().find("Character") != std::string::npos) {
                mgr.switchTo(i);
                break;
            }
        }
    });

    // Debug toggles for gravity field visualization and law enable
    _keyboardHandler->bindKey(GLFW_KEY_F6, "toggle_gravity_viz", [this]() {
        bool v = Physics::getGravityVisualization();
        Physics::setGravityVisualization(!v);
    });
    _keyboardHandler->bindKey(GLFW_KEY_F7, "toggle_gravity_field", [this]() {
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
    _keyboardHandler->bindKey(GLFW_KEY_Z, "undo", [this]() {
        // Undo functionality temporarily disabled due to UI migration
    });
    _keyboardHandler->bindKey(GLFW_KEY_Y, "redo", [this]() {
        // Redo functionality temporarily disabled due to UI migration
    });

    // Camera movement bindings (these are handled in the update loop for continuous movement)
    _keyboardHandler->bindKey(GLFW_KEY_W, "camera_forward", [](){});
    _keyboardHandler->bindKey(GLFW_KEY_S, "camera_backward", [](){});
    _keyboardHandler->bindKey(GLFW_KEY_A, "camera_left", [](){});
    _keyboardHandler->bindKey(GLFW_KEY_D, "camera_right", [](){});
    _keyboardHandler->bindKey(GLFW_KEY_LEFT_SHIFT, "camera_down", [](){});
    _keyboardHandler->bindKey(GLFW_KEY_SPACE, "camera_up", [](){});
    _keyboardHandler->bindKey(GLFW_KEY_V, "camera_sprint", [](){});
    _keyboardHandler->bindKey(GLFW_KEY_LEFT_ALT, "camera_slow", [](){});

    // Manual offset controls
    _keyboardHandler->bindKey(GLFW_KEY_RIGHT, "manual_offset_right", [](){});
    _keyboardHandler->bindKey(GLFW_KEY_LEFT, "manual_offset_left", [](){});
    _keyboardHandler->bindKey(GLFW_KEY_PAGE_UP, "manual_offset_up", [](){});
    _keyboardHandler->bindKey(GLFW_KEY_PAGE_DOWN, "manual_offset_down", [](){});
    _keyboardHandler->bindKey(GLFW_KEY_UP, "manual_offset_forward", [](){});
    _keyboardHandler->bindKey(GLFW_KEY_DOWN, "manual_offset_backward", [](){});

}

// ---------------------------------------------------------------------------
// GLFW Callbacks
// ---------------------------------------------------------------------------

void Engine::registerCallbacks() {
    if (!_window) return;
    glfwSetWindowUserPointer(_window, this);

    // Store previous callbacks (likely ImGui's) so we can forward events
    _prevCursorPosCallback = glfwSetCursorPosCallback(_window, &Engine::onCursorPos);
    _prevFocusCallback     = glfwSetWindowFocusCallback(_window, &Engine::onWindowFocus);
    _prevFramebufferSizeCallback = glfwSetFramebufferSizeCallback(_window, &Engine::onFramebufferSize);

    // Set up mouse button callback that forwards to ImGui first, then our handler
    glfwSetMouseButtonCallback(_window, [](GLFWwindow* window, int button, int action, int mods) {
        Engine* self = static_cast<Engine*>(glfwGetWindowUserPointer(window));
        if (self) {
            // Forward to ImGui first (critical for UI interactions)
            ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
            // Then handle game-specific mouse input
            self->_mouseHandler->handleMouseButton(button, action, mods);
        }
    });

    // Set up scroll callback that forwards to ImGui first, then our handler
    glfwSetScrollCallback(_window, [](GLFWwindow* window, double xoffset, double yoffset) {
        Engine* self = static_cast<Engine*>(glfwGetWindowUserPointer(window));
        if (self) {
            // Forward to ImGui first (critical for UI scrolling)
            ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
            // Then handle game-specific scroll input
            self->_mouseHandler->handleMouseScroll(xoffset, yoffset);
        }
    });
}

void Engine::onCursorPos(GLFWwindow* win, double xpos, double ypos) {
    Engine* self = static_cast<Engine*>(glfwGetWindowUserPointer(win));
    // Forward to ImGui by calling its backend directly (same pattern as the
    // mouse-button and scroll callbacks). The previous code forwarded through a
    // runtime-saved function pointer (_prevCursorPosCallback); when ImGui is
    // initialised with install_callbacks=true that saved pointer could be stale
    // and crash (EXC_BAD_ACCESS in mouseMoved:). Calling the symbol directly is
    // link-time resolved and safe.
    ImGui_ImplGlfw_CursorPosCallback(win, xpos, ypos);
    if (self) self->_mouseHandler->handleMouseMove(xpos, ypos);
}

void Engine::onWindowFocus(GLFWwindow* win, int focused) {
    Engine* self = static_cast<Engine*>(glfwGetWindowUserPointer(win));
    if (self && self->_prevFocusCallback) {
        self->_prevFocusCallback(win, focused);
    }
    if (self) self->_mouseHandler->onWindowFocus(focused);
}

void Engine::onFramebufferSize(GLFWwindow* win, int width, int height) {
    Engine* self = static_cast<Engine*>(glfwGetWindowUserPointer(win));
    if (self && self->_prevFramebufferSizeCallback) {
        self->_prevFramebufferSizeCallback(win, width, height); // forward to ImGui (or whatever was there)
    }
    if (self) self->onFramebufferSize(width, height);
}

void Engine::onFramebufferSize(int /*width*/, int /*height*/) {
    // Nothing to do: render() passes the current framebuffer size to
    // beginFrame every frame, and the backend sets its own viewport from it.
    // (WebGPU additionally has to reconfigure its surface, which it does there.)
}

} // namespace Core
