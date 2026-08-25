#include "Person/Person.hpp"
#include "Singularity/Input/Keyboard/KeyboardHandler.hpp"
#include "Singularity/Input/Mouse/MouseHandler.hpp"
// GameInit.cpp – Game initialisation, GLFW callbacks
// Split from Game.cpp during refactor.

#include "Singularity/Core/Engine.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "../../Singularity/FirstMoverOntology/FirstMoverWindowTools/ElementalToolHandler.hpp"
#include "../../Singularity/FirstMoverOntology/FirstMoverWindowTools/CursorTools.hpp"
#include "../../../imgui/backends/imgui_impl_glfw.h"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Singularity/Screen/ShadingSystem.hpp"
#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/AdvancedFacePaint.hpp"
#include "ConstructedBeing/Singular/Object/Creation/ObjectConcept.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include "ZonesOfEarth/Physics/DefaultPhysicsLaws.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "Singularity/TransferPolicy.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ConstructedBeing/CategoryManager.hpp"
#include "Person/Body/BodyPart/BodyPart.hpp"
#include "Singularity/Screen/Camera.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/CreatorConsole/CreatorConsoleWindow.hpp"
#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/CreationTools.hpp"
#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/Chat.hpp"
#include "ZonesOfEarth/SaveContext.hpp"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <glm/glm.hpp>
#include <iostream>
#include "CreationChannel.hpp"
#include "Singularity/Input/Locomotion/LocomotionChannel.hpp"
#include "Singularity/Input/Interaction/InteractionChannel.hpp"
#include "Singularity/Input/Interaction/ControlPatterns.hpp"
#include <memory>

extern ZoneManager mgr;
extern MaterialManager materials;   // global Material beings (globals.cpp)
extern CategoryManager categories;

namespace Core {

void Engine::initLogic() {
    // These were declared on Engine but never constructed anywhere in the
    // post-"Game" refactor: every line below that touches _lawManager,
    // _player, _camera, _mouseHandler or _keyboardHandler was one call away
    // from a null-pointer deref, starting with connectToEventBus() two lines
    // down. Restoring Tool::ShapeGenerator3D as a developer tool (2026-08-13)
    // is the first thing that actually calls getPlayer()/getLawManager(), so
    // it is the first thing that hits this. Allocating them here is the
    // minimum fix to make the engine boot at all -- not a redesign.
    if (!_lawManager) _lawManager = std::make_unique<LawManager>();
    if (!_camera) _camera = std::make_unique<Camera>();
    if (!_mouseHandler) _mouseHandler = std::make_unique<MouseHandler>();
    if (!_keyboardHandler) _keyboardHandler = std::make_unique<KeyboardHandler>();
    if (!_player) {
        Soul soul("Player");
        Body body("humanoid", "default");
        _player = std::make_unique<Person>(std::move(soul), std::move(body), "default");
    }
    if (!_chat) _chat = std::make_unique<Chat>();
    if (!_cursorTools) _cursorTools = std::make_unique<CursorTools>();

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

    // Register first-mover LocomotionChannel (WASD / jump / vessel clips).
    // Must exist before playIdle below — the clips live on the channel now.
    Singularity::Input::LocomotionChannel::syncRegister(*_lawManager);

    // Register first-mover InteractionChannel (pointer / wheel / keys) and the
    // archetype control laws that read it. INTERACTION_AS_LAW.md: a GUI in
    // Earthcall is this channel's sense plus authored law text, and nothing
    // else. The patterns are registered first-wins, so a loaded world's edited
    // version of any of them survives.
    Singularity::Input::InteractionChannel::syncRegister(*_lawManager);
    Singularity::Input::syncRegisterControlPatterns(*_lawManager, ::categories, *_player);

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

    // Shape Generator 3D plus the rest of the Creator Console tools, as
    // first movers. The console remains the chrome; these are the named
    // beings it arms. See CreationChannel::syncRegisterCreatorTools.
    Singularity::Core::syncRegisterCreatorTools(*_lawManager, *_player);

    // The Universe: what continuous laws watch and quantified conditions
    // (ForAny/ForAll) range over — the active world's objects, the laws
    // themselves (so metalaws can quantify over laws), and the player.
    // Evaluated lazily each tick, well after zones exist.
    Universe::instance().setProvider([this](std::vector<Singular*>& beings) {
        // Active Zone first: Spawn/Create fall back to the first Zone in the
        // domain, which must be the one in front of the Person (the old
        // World bag was only the active world's).
        beings.push_back(&mgr.active());
        for (const auto& obj : mgr.active().getOwnedObjects()) {
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
        // other being. See docs/architecture/ontology/AUTHORED_CATEGORIES.md §9a.
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
        // Other Zones: governance geography — laws quantify over them
        // (ForAny Zone ...) and address them by name (@Home.owner) even
        // while unloaded. Active was already pushed as the Spawn womb.
        for (auto& zone : mgr.zones()) {
            if (zone.get() != &mgr.active()) beings.push_back(zone.get());
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
    mgr.addZone(std::make_shared<Zone>("Sanctum of Beginnings", "default"));
    mgr.addZone(std::make_shared<Zone>("Temple of Echoes", "default"));
    mgr.addZone(std::make_shared<Zone>("Cavern of Light", "default"));
    mgr.addZone(std::make_shared<Zone>("Character Architect Forge", "default"));
    mgr.ensureHomeZone(_player->getIdentifier());
    mgr.bindLive();
    // Home (and every other identity-stable Zone) lives in
    // saves/zones/<id>/, not inside a session/"world" file. Hydrate
    // after minting the boot Zones so an empty Sanctum/Home is filled
    // from the store rather than a second copy being born.
    mgr.hydrateFromZoneStore();
    _world.ensureGatheringZone(mgr);
    if (_lawManager) _world.registerMetalaws(*_lawManager);

    // Initialize elemental tool handler with zone manager
    _elementalToolHandler = std::make_unique<ElementalToolHandler>(&mgr);

    printf("[Init] Checkpoint B: zones added (%zu)\n", mgr.zones().size());
    // Debug: list zone names to validate memory integrity of strings
    for (size_t i = 0; i < mgr.zones().size(); ++i) {
        const auto& z = mgr.zones()[i];
        printf("[Init] Zone[%zu]: %s | Q=%zu D=%zu\n", i, z->name().c_str(), z->getQualities().size(), z->getDeletability().size());
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

    // Menu options answer or they are not on the menu. Dead Settings/Toolbar
    // entries were deleted rather than shown empty. Placement/Selection
    // inspectors point at the living Create3D console, not the pre-split ones.
    _mainMenu.addOption("Quick Save", GLFW_KEY_S, [this]() {
        SaveContext ctx;
        ctx.camera = getCamera();
        ctx.mouseHandler = getMouseHandler();
        ctx.currentColor = Rendering::getCreatorConsoleState().currentColor;
        ctx.player = getPlayer();
        ctx.lawManager = getLawManager();
        ctx.worldTime = &_worldTime;
        ctx.unpackForAuthoring = mgr.getSaveLoadState().unpackForAuthoring;
        mgr.saveStateWithLog("", ctx);
    });
    _mainMenu.addOption("Save As...", GLFW_KEY_A, [this]() { mgr.getSaveLoadState().showSaveWindow = true; });
    _mainMenu.addOption("Load", GLFW_KEY_L, [this]() { mgr.updateSaveFiles(); mgr.getSaveLoadState().showLoadWindow = true; });
    _mainMenu.addOption("Save Manager", GLFW_KEY_G, [this]() { mgr.getSaveLoadState().showManager = true; });
    _mainMenu.addOption("Toggle Chat", GLFW_KEY_H, [this]() {
        _showChatWindow = !_showChatWindow;
        if (_showChatWindow) ensureCursorUnlocked();
    });
    _mainMenu.addOption("Toggle ImGui Demo", GLFW_KEY_F3, [this]() {
        _showImGuiDemo = !_showImGuiDemo;
        if (_showImGuiDemo) ensureCursorUnlocked();
    });
    _mainMenu.addOption("3D Create", GLFW_KEY_F4, [this]() {
        _creatorConsoleOpen = true;
        Rendering::getCreatorConsoleState().currentSection = Rendering::CreatorSection::Create3D;
        Rendering::apply3DMode(Rendering::getCreatorConsoleState(),
                              _lawManager ? Singularity::Core::CreationChannel::find(*_lawManager) : nullptr,
                              Rendering::Mode3D::BrushCreate);
        ensureCursorUnlocked();
    });
    _mainMenu.addOption("3D Select", GLFW_KEY_F5, [this]() {
        _creatorConsoleOpen = true;
        Rendering::getCreatorConsoleState().currentSection = Rendering::CreatorSection::Create3D;
        Rendering::apply3DMode(Rendering::getCreatorConsoleState(),
                              _lawManager ? Singularity::Core::CreationChannel::find(*_lawManager) : nullptr,
                              Rendering::Mode3D::Selection);
        ensureCursorUnlocked();
    });
    _mainMenu.addOption("Toggle Dev Mode", GLFW_KEY_GRAVE_ACCENT, [this]() {
        _devToolsWindowOpen = !_devToolsWindowOpen;
        if (_devToolsWindowOpen) ensureCursorUnlocked();
    });
    _mainMenu.addOption("Toggle Creator Console", GLFW_KEY_F8, [this]() {
        _creatorConsoleOpen = !_creatorConsoleOpen;
        if (_creatorConsoleOpen) ensureCursorUnlocked();
    });
    
    // Main Tools submenu
    _mainMenu.addOption("Brush Tool", GLFW_KEY_B, [this]() {
        _creatorConsoleOpen = true;
        Rendering::getCreatorConsoleState().currentSection = Rendering::CreatorSection::Paint;
        _mainMenu.close();
        _keyboardHandler->setMenuOpen(false);
        _mouseHandler->setMenuOpen(false);
        if (_mouseHandler->isCursorLocked()) _mouseHandler->toggleCursorLock(_window);
    });
    _mainMenu.addOption("Move Tool", GLFW_KEY_V, [this]() {
        _creatorConsoleOpen = true;
        Rendering::getCreatorConsoleState().currentSection = Rendering::CreatorSection::Create3D;
        Rendering::apply3DMode(Rendering::getCreatorConsoleState(),
                              _lawManager ? Singularity::Core::CreationChannel::find(*_lawManager) : nullptr,
                              Rendering::Mode3D::Selection);
        _mainMenu.close();
        _keyboardHandler->setMenuOpen(false);
        _mouseHandler->setMenuOpen(false);
        if (_mouseHandler->isCursorLocked()) _mouseHandler->toggleCursorLock(_window);
    });
    _mainMenu.addOption("Controls / Keymap", GLFW_KEY_K, [this]() {
        _showKeymapWindow = !_showKeymapWindow;
        if (_showKeymapWindow) ensureCursorUnlocked();
    });
    _mainMenu.addOption("Character Architect Forge", GLFW_KEY_C, [this]() {
        _creatorConsoleOpen = true;
        Rendering::getCreatorConsoleState().currentSection = Rendering::CreatorSection::Character;
        const auto& zones = mgr.zones();
        for (size_t i = 0; i < zones.size(); ++i) {
            if (zones[i]->name().find("Character") != std::string::npos) {
                mgr.switchTo(i);
                break;
            }
        }
        _mainMenu.close();
        _keyboardHandler->setMenuOpen(false);
        _mouseHandler->setMenuOpen(false);
        if (_mouseHandler->isCursorLocked()) _mouseHandler->toggleCursorLock(_window);
    });

    _mainMenu.addOption("Quit",   GLFW_KEY_Q, [this]() { glfwSetWindowShouldClose(_window, 1); });
    printf("[Init] Checkpoint B3: after menu addOption(Quit)\n");

    // --------------------------------------------------------------
    // World baseline objects (spinning cube + ground)
    // --------------------------------------------------------------
    _world.setCamera(&_camera->pos);

    {
        std::shared_ptr<Object> cube = std::make_shared<Object>("object.baseline.cube");
        std::shared_ptr<Object> ground = std::make_shared<Object>("object.baseline.ground");
        // Tag these as baseline placeholders so we can safely special-case them later
        cube->setAttribute("baseline", "cube");
        ground->setAttribute("baseline", "ground");
        _world.addOwnedObject(std::move(cube));
        _world.addOwnedObject(std::move(ground));
    }

    printf("[Init] Checkpoint C: baseline objects created\n");


    // Ensure _player initial position matches _camera.pos
    glm::vec3 anchor = _camera->pos - glm::vec3(0.0f, _player->getBody().getEyeHeight(), 0.0f);
    _player->position() = anchor;
    // Route LocomotionChanged to the channel's clip libraries, then settle
    // the vessel into idle. Movement publishes the event that swaps idle
    // for a walk cycle (see Engine::update).
    if (auto* locomotion = Singularity::Input::LocomotionChannel::find(*_lawManager)) {
        locomotion->installRouting();
        locomotion->playIdle(*_player);
    }
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
        // Manage cursor lock based on menu state
        if (_mainMenu.isOpen()) {
            if (_mouseHandler->isCursorLocked()) {
                _mouseHandler->toggleCursorLock(_window); // unlock it
            }
        } else {
            if (!_mouseHandler->isCursorLocked()) {
                _mouseHandler->toggleCursorLock(_window); // lock it
            }
        }
    });
    _keyboardHandler->bindKey(GLFW_KEY_ESCAPE, "toggle_cursor_lock", [this]() {
        if (!_mainMenu.isOpen()) {
            _mouseHandler->toggleCursorLock(_window);
        }
    });
    _keyboardHandler->bindKey(GLFW_KEY_GRAVE_ACCENT, "toggle_dev_tools", [this]() {
        _devToolsWindowOpen = !_devToolsWindowOpen;
        if (_devToolsWindowOpen) ensureCursorUnlocked();
    });
    _keyboardHandler->bindKey(GLFW_KEY_F8, "toggle_creation_console", [this]() {
        _creatorConsoleOpen = !_creatorConsoleOpen;
        if (_creatorConsoleOpen) ensureCursorUnlocked();
    });
    _keyboardHandler->bindKey(GLFW_KEY_F9, "toggle_singular_set_to_set", [this]() {
        _creationConsoleOpen = !_creationConsoleOpen;
        if (_creationConsoleOpen) ensureCursorUnlocked();
    });
    _keyboardHandler->bindKey(GLFW_KEY_H, "toggle_chat", [this]() {
        _showChatWindow = !_showChatWindow;
        if (_showChatWindow) ensureCursorUnlocked();
    });
    _keyboardHandler->bindKey(GLFW_KEY_K, "toggle_keymap", [this]() {
        _showKeymapWindow = !_showKeymapWindow;
        if (_showKeymapWindow) ensureCursorUnlocked();
    });
    _keyboardHandler->bindKey(GLFW_KEY_1, "perspective_first_person", [this]() { _currentPerspective = PerspectiveMode::FirstPerson; });
    _keyboardHandler->bindKey(GLFW_KEY_2, "perspective_second_person", [this]() { _currentPerspective = PerspectiveMode::SecondPerson; });
    _keyboardHandler->bindKey(GLFW_KEY_3, "perspective_third_person", [this]() { _currentPerspective = PerspectiveMode::ThirdPerson; });
    _keyboardHandler->bindKey(GLFW_KEY_F, "toggle_flight", [this]() {
        Physics::toggleFlying();
    });
    _keyboardHandler->bindKey(GLFW_KEY_C, "switch_to_character_zone", [this]() {
        const auto& zones = mgr.zones();
        for (size_t i = 0; i < zones.size(); ++i) {
            if (zones[i]->name().find("Character") != std::string::npos) {
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
    _prevKeyCallback       = glfwSetKeyCallback(_window, &Engine::onKey);

    // Set up mouse button callback that forwards to ImGui first, then our handler
    glfwSetMouseButtonCallback(_window, [](GLFWwindow* window, int button, int action, int mods) {
        Engine* self = static_cast<Engine*>(glfwGetWindowUserPointer(window));
        if (self) {
            // Forward to ImGui first (critical for UI interactions)
            ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
            // Then handle game-specific mouse input
            self->_mouseHandler->handleMouseButton(button, action, mods);
            
            // Emit global onMouseClicked event
            if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && !ImGui::GetIO().WantCaptureMouse) {
                Singularity::Core::CreationChannel* channel = nullptr;
                if (self->getLawManager()) {
                    for (const auto& law : self->getLawManager()->getAll()) {
                        channel = dynamic_cast<Singularity::Core::CreationChannel*>(law.get());
                        if (channel) break;
                    }
                }
                ECA::Event ev{"onMouseClicked", channel, nullptr, std::time(nullptr)};
                Core::EventBus::instance().publish(ev);
            }
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

            // The wheel has no level to poll: what this callback reports is
            // all there is, so the interaction channel accumulates it here and
            // drains it on its step. Vetoed while an ImGui surface owns the
            // pointer, the same rule the buttons follow.
            if (self->getLawManager() && !ImGui::GetIO().WantCaptureMouse) {
                if (auto* interaction =
                        Singularity::Input::InteractionChannel::find(*self->getLawManager())) {
                    interaction->noteScroll(static_cast<float>(xoffset),
                                            static_cast<float>(yoffset));
                }
            }
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

void Engine::onKey(GLFWwindow* win, int key, int scancode, int action, int mods) {
    Engine* self = static_cast<Engine*>(glfwGetWindowUserPointer(win));
    if (self && self->_prevKeyCallback) {
        self->_prevKeyCallback(win, key, scancode, action, mods); // forward to ImGui
    } else {
        // Fallback direct call if no previous callback was stored
        ImGui_ImplGlfw_KeyCallback(win, key, scancode, action, mods);
    }
    
    if (self && self->_keyboardHandler) {
        if (action == GLFW_PRESS) {
            self->_keyboardHandler->handleKeyPress(key);
        } else if (action == GLFW_RELEASE) {
            self->_keyboardHandler->handleKeyRelease(key);
        }
    }

    // The key, as a law-addressable event aimed at whatever holds focus.
    // GLFW_REPEAT is deliberately not forwarded: a held key repeating through
    // this callback is a LEVEL, and republishing key-pressed on every repeat
    // would make an event out of a state. `keyDown` is the level.
    //
    // Suppressed while ImGui wants the keyboard, so typing in a text box does
    // not also fire the world's key-command laws.
    if (self && self->getLawManager() && !ImGui::GetIO().WantCaptureKeyboard &&
        (action == GLFW_PRESS || action == GLFW_RELEASE)) {
        if (auto* interaction =
                Singularity::Input::InteractionChannel::find(*self->getLawManager())) {
            const char* named = glfwGetKeyName(key, scancode);
            interaction->noteKey(named ? named : std::to_string(key), key,
                                 action == GLFW_PRESS);
        }
    }
}

} // namespace Core
