import os
import re

def fix_engine_update():
    path = "src/Singularity/Core/EngineUpdate.cpp"
    with open(path, 'r') as f:
        lines = f.readlines()
        
    for i in range(len(lines)):
        # replace Mode3D with Ourverse::Mode3D if it's Mode3D::
        if "Mode3D::" in lines[i] and "Ourverse::Mode3D::" not in lines[i]:
            lines[i] = lines[i].replace("Mode3D::", "Ourverse::Mode3D::")
            
        # comment out stuff that doesn't exist anymore
        bad_words = [
            "updateChat", 
            "setCursorX", 
            "setCursorY",
            "_straightLineMode",
            "selectObject3D(",
            "_selectedObject3D",
            "_currentTool.getType()",
            "_currentTool.getTypeName()",
            "&_player, nullptr, static_cast<std::time_t>(_worldTime)"
        ]
        
        for word in bad_words:
            if word in lines[i] and not lines[i].strip().startswith("//"):
                lines[i] = "// " + lines[i]
                
    with open(path, 'w') as f:
        f.writelines(lines)
    print("Fixed EngineUpdate.cpp")

def fix_engine_hpp():
    path = "src/Singularity/Core/Engine.hpp"
    with open(path, 'r') as f:
        lines = f.readlines()
        
    for i in range(len(lines)):
        if "struct DummyBrush {" in lines[i]:
            # find next line
            if "glm::vec3 rotation;" not in lines[i+1]:
                lines[i] = lines[i] + "        glm::vec3 rotation;\n"
                
    with open(path, 'w') as f:
        f.writelines(lines)
    print("Fixed Engine.hpp")
    
def fix_engine_cpp():
    path = "src/Singularity/Core/Engine.cpp"
    with open(path, 'r') as f:
        content = f.read()
        
    if "ElementalToolHandler.hpp" not in content:
        content = content.replace('#include "Singularity/Core/Engine.hpp"', '#include "Singularity/Core/Engine.hpp"\n#include "OurVerse/ElementalToolHandler.hpp"')
        with open(path, 'w') as f:
            f.write(content)
        print("Fixed Engine.cpp")
        
if __name__ == '__main__':
    fix_engine_update()
    fix_engine_hpp()
    fix_engine_cpp()
