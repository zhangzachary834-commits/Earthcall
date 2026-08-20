#pragma once

#include "ZonesOfEarth/ZoneManager.hpp"
#include "imgui.h"

// Forward declarations
namespace Core { class Engine; }
struct GLFWwindow;

class ElementalToolHandler {
public:
    ElementalToolHandler(ZoneManager* mgr);
    ~ElementalToolHandler();

    void tool_status_update(Core::Engine* engine, GLFWwindow* window);

private:
    ZoneManager* _mgr;
};