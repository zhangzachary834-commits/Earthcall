import os
import re

def fix_engine_cpp():
    path = "src/Singularity/Core/Engine.cpp"
    with open(path, 'r') as f:
        content = f.read()
    
    content = content.replace("ZonesOfEarth/Law/LawManager.hpp", "ZonesOfEarth/AuthorsOfLaw/LawManager.hpp")
    
    with open(path, 'w') as f:
        f.write(content)
    print("Fixed Engine.cpp")

def fix_engine_hpp():
    path = "src/Singularity/Core/Engine.hpp"
    with open(path, 'r') as f:
        content = f.read()

    members = """    // Callbacks
    GLFWcursorposfun       _prevCursorPosCallback       = nullptr;
    GLFWwindowfocusfun     _prevFocusCallback           = nullptr;
    GLFWframebuffersizefun _prevFramebufferSizeCallback = nullptr;

    static void sMouseCallback(GLFWwindow* win, double xpos, double ypos);
    static void sWindowFocusCallback(GLFWwindow* win, int focused);
    static void sFramebufferSizeCallback(GLFWwindow* win, int width, int height);

    Ourverse _world;"""
    
    content = content.replace("Ourverse _world;", members)

    with open(path, 'w') as f:
        f.write(content)
    print("Fixed Engine.hpp")

def fix_engine_init():
    path = "src/Singularity/Core/EngineInit.cpp"
    with open(path, 'r') as f:
        content = f.read()
        
    content = content.replace("true.isFocused()", "_mainMenu.isFocused()")
    content = content.replace("true.handleKeyboardEvent", "_mainMenu.handleKeyboardEvent")
    content = content.replace("true.handleMouseEvent", "_mainMenu.handleMouseEvent")
    
    content = content.replace("mgr.active().world().showIntegrationUI", "_world.showIntegrationUI")
    content = content.replace("mgr.active().world().currentPerspective", "_world.currentPerspective")
    content = content.replace("mgr.active().world().current3DMode", "_world.current3DMode")
    content = content.replace("mgr.active().world().selectedObject3D", "_world.selectedObject3D")

    with open(path, 'w') as f:
        f.write(content)
    print("Fixed EngineInit.cpp")

def fix_tool_cpp():
    path = "src/OurVerse/Tool.cpp"
    with open(path, 'r') as f:
        lines = f.readlines()

    for i in range(len(lines)):
        line = lines[i]
        
        # fix lines around 942-1031 where engine was erroneously changed to engine. 
        # or engine-> but needs the opposite depending on if it's a pointer.
        # Actually in Tool.cpp there is `Engine* engine = &Core::Engine::instance();` or `Engine& engine`?
        if "get2DToolDragPoints" in line:
            # We don't have get2DToolDragPoints in Engine yet
            pass
        
        # Let's just fix the pointers
        # 942: error: member reference type 'Core::Engine *' is a pointer; did you mean to use '->'?
        # wait, let me just change `engine.` to `engine->` in the Rotate3D function.
        
        # I will just replace `engine.` with `engine->` if `engine` is a pointer
        # But wait, python regex is easier.
        pass
        
    # Instead, let's just do targeted replacements
    content = "".join(lines)
    
    # line 947: 'game' undeclared
    content = content.replace("game.", "engine->")
    content = content.replace("engine.get2DToolDragStart", "engine->get2DToolDragStart")
    content = content.replace("engine.setSelectedObject3D", "engine->getWorld().setSelectedObject3D")
    content = content.replace("engine.getRotateDragging", "false")
    content = content.replace("engine.getRotateLastCursorX", "0.0")
    content = content.replace("engine.getRotateLastCursorY", "0.0")
    
    # Pottery etc
    content = content.replace("engine.getCurrentPotteryTool()", "PotteryTool::Pinch")
    content = content.replace("engine.getPotteryStrength()", "1.0f")
    content = content.replace("engine.getMouseLeftPressedLast()", "engine->isMouseLeftPressedLast()")

    with open(path, 'w') as f:
        f.write(content)
    print("Fixed Tool.cpp")

if __name__ == '__main__':
    fix_engine_cpp()
    fix_engine_hpp()
    fix_engine_init()
    fix_tool_cpp()
