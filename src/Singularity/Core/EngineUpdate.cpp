#include "Engine.hpp"
#include "../../ZonesOfEarth/Ourverse/Ourverse.hpp"
#include "../Screen/Camera.hpp"
#include "../Input/KeyboardHandler.hpp"
#include "../Input/MouseHandler.hpp"
#include "../../Person/Person.hpp"
#include "../../ZonesOfEarth/ZoneManager.hpp"
#include "../../ZonesOfEarth/Physics/Physics.hpp"
#include "../../ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include <imgui.h>

extern ZoneManager mgr;

namespace Core {
    void Engine::fuseObjects(Object* A, Object* B) {}
    void Engine::blendRail(const Object* o, glm::vec3& start, glm::vec3& dir, float& length) const {}
    
    void Engine::update(float dt) {
        if (!_keyboardHandler || !_mouseHandler || !_camera || !_player) return;

        // Update input handlers
        if (_mainMenu.isOpen()) {
            _mainMenu.processInput(_window);
        }
        
        _keyboardHandler->update();
        _mouseHandler->update();

        // Update camera front from mouse handler
        _camera->front = _mouseHandler->calculateCameraFront();

        // Check if any text input is active (ImGui)
        bool anyTextInputActive = ImGui::IsAnyItemActive() || ImGui::IsWindowFocused();

        // Player movement
        const bool canMove = _mouseHandler->isCursorLocked() && !_mainMenu.isOpen() && !anyTextInputActive;
        const bool flying  = Physics::getFlying();
        _player->stepMovement(dt, _window, _camera.get(), &mgr, flying, canMove);

        // Update world (physics etc.)
        mgr.active().world().update(dt);
        mgr.active().applyFormationRelations();

        // Advance time
        _worldTime += static_cast<double>(dt);
        Universe::instance().setClock(_worldTime, static_cast<double>(dt));
    }
}
