#include "Engine.hpp"
#include "../../ZonesOfEarth/Ourverse/Ourverse.hpp"
#include "../Screen/Camera.hpp"
#include "../Input/KeyboardHandler.hpp"
#include "../Input/MouseHandler.hpp"
#include "../Input/LocomotionChannel.hpp"
#include "../Input/InteractionChannel.hpp"
#include "Singularity/FirstMoverWindowTools/CreationTools.hpp"
#include "../../Person/Person.hpp"
#include "../../ZonesOfEarth/ZoneManager.hpp"
#include "../../ZonesOfEarth/Physics/Physics.hpp"
#include "../../ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "../../ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include <imgui.h>

extern ZoneManager mgr;

namespace Core {
    void Engine::fuseObjects(Object* A, Object* B) {}
    void Engine::blendRail(const Object* o, glm::vec3& start, glm::vec3& dir, float& length) const {}
    
    void Engine::update(float dt) {
        if (!_keyboardHandler || !_mouseHandler || !_camera || !_player || !_lawManager) return;

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

        // Vessel movement — Input first mover, not Person.
        const bool canMove = _mouseHandler->isCursorLocked() && !_mainMenu.isOpen() && !anyTextInputActive;
        const bool flying  = Physics::getFlying();
        if (auto* locomotion = Singularity::Input::LocomotionChannel::find(*_lawManager)) {
            locomotion->step(*_player, *_camera, _window, mgr, dt, flying, canMove);
        }

        // Creation first mover — sense placement, honour L, push the
        // console's live selection onto the channel, actuate the armed
        // tool. Used to run inside render3DConsole / DeveloperToolsWindow,
        // so collapsing the console froze every 3D tool.
        Rendering::stepCreationTools(_window, this, mgr, dt, _creatorConsoleOpen);

        // Interaction first mover — pick the being under the pointer, publish
        // the click/scroll/focus edges, drive hover. Stepped here and not from
        // a render function, for the reason above it: a channel that only runs
        // while a window is on screen is a channel that freezes when the
        // window collapses.
        //
        // WantCaptureMouse is the foreign-surface veto: while an ImGui panel
        // owns the pointer, the world must see no pointer at all, or the
        // Person clicks a menu and a button behind it fires too.
        if (auto* interaction = Singularity::Input::InteractionChannel::find(*_lawManager)) {
            interaction->step(_window, *_camera, mgr, ImGui::GetIO().WantCaptureMouse);
        }

        // Update world (physics etc.)
        mgr.active().world().update(dt);
        mgr.active().applyFormationRelations();

        // Advance time
        _worldTime += static_cast<double>(dt);
        Universe::instance().setClock(_worldTime, static_cast<double>(dt));
    }
}
