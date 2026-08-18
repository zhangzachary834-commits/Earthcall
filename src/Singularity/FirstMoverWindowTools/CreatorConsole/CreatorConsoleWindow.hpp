#pragma once

#include "CreatorConsoleState.hpp"

class ZoneManager;
class Person;
class Object;
class Ourverse;
struct GLFWwindow;
namespace Core { class Engine; }

namespace Rendering {

    // Main window rendering function
    void renderCreatorConsoleWindow(bool* open, Person* player, Object* selected, ZoneManager& zoneMgr, GLFWwindow* window = nullptr, Core::Engine* engine = nullptr);

    // Hardcoded preview rendering for First Mover 3D tools
    void renderCreatorConsole3DPreviews(Person* player, Object* selected);

} // namespace Rendering
