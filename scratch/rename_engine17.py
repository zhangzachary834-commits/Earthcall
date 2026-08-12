import os

def fix_engine_init():
    path = "src/Singularity/Core/EngineInit.cpp"
    with open(path, 'r') as f:
        content = f.read()

    # fix true -> _mainMenu
    content = content.replace("true.isOpen()", "_mainMenu.isOpen()")
    content = content.replace("true.toggle()", "_mainMenu.toggle()")

    # fix callbacks
    content = content.replace("sMouseCallback", "onCursorPos")
    content = content.replace("sWindowFocusCallback", "onWindowFocus")
    content = content.replace("sFramebufferSizeCallback", "onFramebufferSize")
    
    # glfwSetFramebufferSizeCallback takes 2 args, but _prevFramebufferSizeCallback might be expecting 2
    # The error says "too few arguments to function call, expected 3, have 2"
    # glfwSetFramebufferSizeCallback signature is `GLFWframebuffersizefun glfwSetFramebufferSizeCallback(GLFWwindow* window, GLFWframebuffersizefun callback)`
    # In EngineInit.cpp line 327 maybe I have `glfwSetFramebufferSizeCallback(_window, onFramebufferSize, this)`? No.
    # Let me just replace the assignment:
    
    with open(path, 'w') as f:
        f.write(content)
    print("Fixed EngineInit.cpp")
    
def fix_engine_hpp():
    path = "src/Singularity/Core/Engine.hpp"
    with open(path, 'r') as f:
        content = f.read()
    
    # move the added properties to public
    # I'll just change "Ourverse _world" to "public: Ourverse _world;" 
    # since everything before it was in private.
    content = content.replace("    // Missing rotation state", "public:\n    // Missing rotation state")
    
    with open(path, 'w') as f:
        f.write(content)
    print("Fixed Engine.hpp")
    
def fix_tool_cpp():
    path = "src/OurVerse/Tool.cpp"
    with open(path, 'r') as f:
        content = f.read()
    
    # tool cpp game and pointers
    content = content.replace("game.isMouseLeftPressedLast", "engine->isMouseLeftPressedLast")
    content = content.replace("game.getRotateDragging", "engine->getRotateDragging")
    content = content.replace("game.setRotateLastCursor", "engine->setRotateLastCursor")
    
    content = content.replace("PotteryTool::Expand", "Core::PotteryTool::Expand")
    content = content.replace("RotationAxisMode::FreeXY", "Core::RotationAxisMode::FreeXY")
    content = content.replace("RotationAxisMode::AuthoritativeAxis", "Core::RotationAxisMode::AuthoritativeAxis")
    content = content.replace("RotationAxisMode::Free", "Core::RotationAxisMode::Free")
    content = content.replace("engine->setRotateDragging", "engine.setRotateDragging")

    with open(path, 'w') as f:
        f.write(content)
    print("Fixed Tool.cpp")
    
if __name__ == '__main__':
    fix_engine_init()
    fix_engine_hpp()
    fix_tool_cpp()
