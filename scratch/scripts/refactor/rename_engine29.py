import os

def fix_engine_render():
    path = "src/Singularity/Core/EngineRender.cpp"
    with open(path, 'r') as f:
        lines = f.readlines()
        
    for i in range(len(lines)):
        if i == 465 or i == 469: # lines 466 and 470 in 1-based indexing
            if "}" in lines[i]:
                lines[i] = "// " + lines[i]
                
    with open(path, 'w') as f:
        f.writelines(lines)
    print("Fixed EngineRender.cpp")

def fix_engine_hpp():
    path = "src/Singularity/Core/Engine.hpp"
    with open(path, 'r') as f:
        lines = f.readlines()
        
    for i in range(len(lines)):
        if "void render(float alpha);" in lines[i]:
            lines[i] = lines[i].replace("void render(float alpha);", "void render();")
            
    with open(path, 'w') as f:
        f.writelines(lines)
    print("Fixed Engine.hpp")
    
if __name__ == '__main__':
    fix_engine_render()
    fix_engine_hpp()
