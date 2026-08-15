import os

def fix_engine_render():
    path = "src/Singularity/Core/EngineRender.cpp"
    with open(path, 'r') as f:
        lines = f.readlines()
        
    for i in range(len(lines)):
        # uncomment 466
        if i == 465 and lines[i].startswith("//"):
            lines[i] = lines[i].replace("// ", "", 1)
        # comment out polyhedron dummy usage
        if "_polyhedron.shapeKind" in lines[i] or "_polyhedron.shapeParams" in lines[i]:
            if not lines[i].strip().startswith("//"):
                lines[i] = "// " + lines[i]
        # comment out setPlayerEyeHeight
        if "setPlayerEyeHeight" in lines[i]:
            if not lines[i].strip().startswith("//"):
                lines[i] = "// " + lines[i]
            
    with open(path, 'w') as f:
        f.writelines(lines)
    print("Fixed EngineRender.cpp")

def fix_tool_cpp():
    path = "src/OurVerse/Tool.cpp"
    with open(path, 'r') as f:
        lines = f.readlines()
        
    in_ref_func = False
    for i in range(len(lines)):
        line = lines[i]
        if "Core::Engine& engine" in line:
            in_ref_func = True
        elif "Core::Engine* engine" in line:
            in_ref_func = False
        
        if in_ref_func:
            lines[i] = lines[i].replace("engine->", "engine.")
            
    with open(path, 'w') as f:
        f.writelines(lines)
    print("Fixed Tool.cpp")
    
def fix_engine_cpp():
    path = "src/Singularity/Core/Engine.cpp"
    with open(path, 'r') as f:
        content = f.read()
        
    if "ElementalToolHandler.hpp" not in content:
        content = content.replace('#include "Engine.hpp"', '#include "Engine.hpp"\n#include "../../OurVerse/ElementalToolHandler.hpp"')
        with open(path, 'w') as f:
            f.write(content)
        print("Fixed Engine.cpp")
        
if __name__ == '__main__':
    fix_engine_render()
    fix_tool_cpp()
    fix_engine_cpp()
