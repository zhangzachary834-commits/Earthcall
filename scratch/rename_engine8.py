import os

def fix_engine_hpp():
    path = "src/Singularity/Core/Engine.hpp"
    with open(path, 'r') as f:
        content = f.read()

    members = """    bool _mouseLeftPressedLast = false;
    bool _is2DToolDragging = false;
    glm::vec2 _dragStart = {0,0};
    glm::vec2 _dragCurrent = {0,0};
    bool _useLegacy2DTools = false;"""
    
    content = content.replace("    bool _mouseLeftPressedLast = false;\n    bool _is2DToolDragging = false;", members)
    
    with open(path, 'w') as f:
        f.write(content)
    print("Fixed Engine.hpp")

def fix_engine_init_cpp():
    path = "src/Singularity/Core/EngineInit.cpp"
    with open(path, 'r') as f:
        content = f.read()

    includes = """#include "Person/Person.hpp"
#include "Singularity/Input/KeyboardHandler.hpp"
#include "Singularity/Input/MouseHandler.hpp"
"""
    if '#include "Person/Person.hpp"' not in content:
        content = includes + content
        
    with open(path, 'w') as f:
        f.write(content)
    print("Fixed EngineInit.cpp")

if __name__ == '__main__':
    fix_engine_hpp()
    fix_engine_init_cpp()
