import os
import re

def fix_tool_cpp():
    path = "src/OurVerse/Tool.cpp"
    with open(path, 'r') as f:
        content = f.read()

    content = content.replace("getMouseLeftPressedLast", "isMouseLeftPressedLast")
    
    with open(path, 'w') as f:
        f.write(content)
    print("Fixed Tool.cpp")

def fix_engine_render():
    path = "src/Singularity/Core/EngineRender.cpp"
    with open(path, 'r') as f:
        content = f.read()
        
    content = content.replace("PerspectiveMode::", "Ourverse::PerspectiveMode::")
    content = content.replace("_current3DMode", "getWorld().current3DMode")
    content = content.replace("Mode3D::", "Ourverse::Mode3D::")
    content = content.replace("_selectedObject3D", "getWorld().selectedObject3D")
    content = content.replace("_morphVertexIndex", "getWorld().morphVertexIndex")
    
    # Missing includes
    includes = """
#include "../Screen/Camera.hpp"
#include "../Input/KeyboardHandler.hpp"
#include "../Input/MouseHandler.hpp"
#include "../../Person/Person.hpp"
"""
    if "Camera.hpp" not in content:
        content = content.replace('#include "Engine.hpp"', '#include "Engine.hpp"\n' + includes)

    with open(path, 'w') as f:
        f.write(content)
    print("Fixed EngineRender.cpp")

def fix_engine_update():
    path = "src/Singularity/Core/EngineUpdate.cpp"
    with open(path, 'r') as f:
        content = f.read()
        
    content = content.replace("PerspectiveMode::", "Ourverse::PerspectiveMode::")
    content = content.replace("_current3DMode", "getWorld().current3DMode")
    content = content.replace("Mode3D::", "Ourverse::Mode3D::")
    content = content.replace("_selectedObject3D", "getWorld().selectedObject3D")
    content = content.replace("_morphVertexIndex", "getWorld().morphVertexIndex")
    
    content = re.sub(r'^.*_cubeAngle.*$', r'// \g<0>', content, flags=re.MULTILINE)
    content = re.sub(r'^.*mgr\..*$', r'// \g<0>', content, flags=re.MULTILINE)
    content = re.sub(r'^.*auto& mgr.*$', r'// \g<0>', content, flags=re.MULTILINE)
    
    # Missing includes
    includes = """
#include "../Screen/Camera.hpp"
#include "../Input/KeyboardHandler.hpp"
#include "../Input/MouseHandler.hpp"
#include "../../Person/Person.hpp"
"""
    if "Camera.hpp" not in content:
        content = content.replace('#include "Engine.hpp"', '#include "Engine.hpp"\n' + includes)

    with open(path, 'w') as f:
        f.write(content)
    print("Fixed EngineUpdate.cpp")

if __name__ == '__main__':
    fix_tool_cpp()
    fix_engine_render()
    fix_engine_update()
