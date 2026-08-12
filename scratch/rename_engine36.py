import os

def fix_tool_cpp():
    path = "src/OurVerse/Tool.cpp"
    with open(path, 'r') as f:
        lines = f.readlines()
        
    in_ref_func = False
    for i in range(len(lines)):
        if "Core::Engine& engine" in lines[i]:
            in_ref_func = True
        elif "Core::Engine* engine" in lines[i]:
            in_ref_func = False
            
        if in_ref_func:
            lines[i] = lines[i].replace("engine->", "engine.")
        else:
            lines[i] = lines[i].replace("engine.", "engine->")
            # Just in case we messed up standard dot accesses like engine.getWorld() 
            # where it should be engine->getWorld()
            
    with open(path, 'w') as f:
        f.writelines(lines)
    print("Fixed Tool.cpp")

def fix_engine_render():
    path = "src/Singularity/Core/EngineRender.cpp"
    with open(path, 'r') as f:
        lines = f.readlines()
        
    for i in range(324, 495):
        if not lines[i].strip().startswith("//"):
            lines[i] = "// " + lines[i]
            
    with open(path, 'w') as f:
        f.writelines(lines)
    print("Fixed EngineRender.cpp")

def fix_engine_update():
    path = "src/Singularity/Core/EngineUpdate.cpp"
    with open(path, 'r') as f:
        lines = f.readlines()
        
    for i in range(170, 511):
        if not lines[i].strip().startswith("//"):
            lines[i] = "// " + lines[i]
            
    with open(path, 'w') as f:
        f.writelines(lines)
    print("Fixed EngineUpdate.cpp")

def fix_polyhedron_settings():
    path = "src/Singularity/Core/PolyhedronSettings.cpp"
    if os.path.exists(path):
        with open(path, 'r') as f:
            content = f.read()
        content = content.replace("ConstructedBeing/Object/Tool/PolyhedronSettings.hpp", "OurVerse/Tool/PolyhedronSettings.hpp")
        with open(path, 'w') as f:
            f.write(content)
        print("Fixed PolyhedronSettings.cpp")

if __name__ == '__main__':
    fix_tool_cpp()
    fix_engine_render()
    fix_engine_update()
    fix_polyhedron_settings()
