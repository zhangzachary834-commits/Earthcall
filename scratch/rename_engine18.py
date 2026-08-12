import os

def fix_engine_cpp():
    path = "src/Singularity/Core/Engine.cpp"
    with open(path, 'r') as f:
        content = f.read()
    
    content = content.replace("ZonesOfEarth/AuthorsOfLaw/LawManager.hpp", "ZonesOfEarth/AuthorsOfLaw/Law.hpp")
    
    with open(path, 'w') as f:
        f.write(content)
    print("Fixed Engine.cpp")
    
def fix_tool_cpp():
    path = "src/OurVerse/Tool.cpp"
    with open(path, 'r') as f:
        lines = f.readlines()
        
    for i in range(len(lines)):
        line = lines[i]
        
        # fix pointers
        if "engine->getMouseHandler()->" in line and "Engine& engine" in "".join(lines[max(0, i-50):i]):
            lines[i] = line.replace("engine->", "engine.")
        
        # 626, 652, 909
        if "626" in str(i+1) or "652" in str(i+1) or "909" in str(i+1):
            lines[i] = line.replace("engine->", "engine.")
            
        # 242, 942, 1029, 1048, 1054, 1092
        if "242" in str(i+1) or "942" in str(i+1) or "1029" in str(i+1) or "1048" in str(i+1) or "1054" in str(i+1) or "1092" in str(i+1):
            lines[i] = line.replace("engine.", "engine->")

        # 947, 1027
        if "947" in str(i+1) or "1027" in str(i+1):
            lines[i] = line.replace("game.", "engine->")
            
        # Core::Core
        lines[i] = lines[i].replace("Core::Core::", "Core::")
        
        # 1040: called object type 'Object *' is not a function
        # Wait, if getSelectedObject3D() returned a value, but I replaced it with `getWorld().selectedObject3D()`, it's not a function.
        if "selectedObject3D()" in lines[i]:
            lines[i] = lines[i].replace("selectedObject3D()", "selectedObject3D")
            
    with open(path, 'w') as f:
        f.writelines(lines)
    print("Fixed Tool.cpp")
    
def fix_engine_init():
    path = "src/Singularity/Core/EngineInit.cpp"
    with open(path, 'r') as f:
        content = f.read()

    # glfwSetFramebufferSizeCallback
    # if it says expected 3 have 2, maybe I replaced `setCursorPosCallback` with `setFramebufferSizeCallback`?
    # Let me just restore the glfwSet... calls:
    # glfwSetCursorPosCallback(_window, onCursorPos);
    # glfwSetWindowFocusCallback(_window, onWindowFocus);
    # glfwSetFramebufferSizeCallback(_window, onFramebufferSize);
    content = content.replace("glfwSetFramebufferSizeCallback(_window, onFramebufferSize, this)", "glfwSetFramebufferSizeCallback(_window, onFramebufferSize)")
    content = content.replace("glfwSetCursorPosCallback(_window, onCursorPos, this)", "glfwSetCursorPosCallback(_window, onCursorPos)")
    content = content.replace("glfwSetWindowFocusCallback(_window, onWindowFocus, this)", "glfwSetWindowFocusCallback(_window, onWindowFocus)")

    # fix out of line definition of onFramebufferSize
    content = content.replace("void Engine::onFramebufferSize", "void Engine::onFramebufferSize")
    
    with open(path, 'w') as f:
        f.write(content)
    print("Fixed EngineInit.cpp")

if __name__ == '__main__':
    fix_engine_cpp()
    fix_tool_cpp()
    fix_engine_init()
