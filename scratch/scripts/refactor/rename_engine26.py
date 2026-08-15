import os
import re

def fix_tool_cpp():
    path = "src/OurVerse/Tool.cpp"
    with open(path, 'r') as f:
        lines = f.readlines()
        
    for i in range(530, 595):
        if i < len(lines):
            lines[i] = lines[i].replace("engine->", "engine.")
            
    with open(path, 'w') as f:
        f.writelines(lines)
    print("Fixed Tool.cpp")
    
def fix_engine_render():
    path = "src/Singularity/Core/EngineRender.cpp"
    with open(path, 'r') as f:
        lines = f.readlines()
        
    for i in range(475, 490):
        if i < len(lines) and lines[i].strip() == "}":
            lines[i] = "// " + lines[i]
            break # only comment out the first one, which is the orphaned one
            
    with open(path, 'w') as f:
        f.writelines(lines)
    print("Fixed EngineRender.cpp")

if __name__ == '__main__':
    fix_tool_cpp()
    fix_engine_render()
