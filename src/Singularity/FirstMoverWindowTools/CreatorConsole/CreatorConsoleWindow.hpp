#pragma once

#include "CreatorConsoleState.hpp"

class ZoneManager;
class Person;
class Ourverse;

namespace Core { class Engine; }

namespace Rendering {

    // Main window rendering function
    void renderCreatorConsoleWindow(bool* open, Person* player, Object* selected, ZoneManager& zoneMgr, Core::Engine* engine);

    // Hardcoded preview rendering for First Mover 3D tools
    void renderCreatorConsole3DPreviews(Person* player, Object* selected);

} // namespace Rendering
