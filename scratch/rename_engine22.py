import os
import re

def fix_tool_cpp():
    # Read the log file
    with open("build20.log", 'r') as f:
        log_content = f.read()

    # Find all line numbers in Tool.cpp that have "use of undeclared identifier 'game'"
    lines_with_game = re.findall(r'Tool\.cpp:(\d+):\d+: error: use of undeclared identifier \'game\'', log_content)
    # Find all line numbers in Tool.cpp that have "member reference type 'Core::Engine \*' is a pointer; did you mean to use '->'\?"
    lines_with_pointer = re.findall(r'Tool\.cpp:(\d+):\d+: error: member reference type \'Core::Engine \*\' is a pointer; did you mean to use \'->\'\?', log_content)

    path = "src/OurVerse/Tool.cpp"
    with open(path, 'r') as f:
        lines = f.readlines()
        
    for line_num in lines_with_game:
        idx = int(line_num) - 1
        lines[idx] = lines[idx].replace("game.", "engine->")
        
    for line_num in lines_with_pointer:
        idx = int(line_num) - 1
        lines[idx] = lines[idx].replace("engine.", "engine->")

    with open(path, 'w') as f:
        f.writelines(lines)
    print("Fixed Tool.cpp using log file")

def fix_engine_render():
    with open("build20.log", 'r') as f:
        log_content = f.read()
    
    # "error: use of undeclared identifier '_showKeymapWindow'" etc
    lines_with_world = re.findall(r'EngineRender\.cpp:(\d+):\d+: error: (?:use of undeclared identifier|invalid use of non-static data member) \'_?(showKeymapWindow|showDebugCoordinates|showChatWindow|patchCtrlIndex)\'', log_content)
    
    path = "src/Singularity/Core/EngineRender.cpp"
    with open(path, 'r') as f:
        lines = f.readlines()
        
    for line_num, var_name in lines_with_world:
        idx = int(line_num) - 1
        lines[idx] = lines[idx].replace(f"_{var_name}", f"_world.{var_name}")
        
    # member reference base type 'int' is not a structure or union
    lines_with_int = re.findall(r'EngineRender\.cpp:(\d+):\d+: error: member reference base type \'int\' is not a structure or union', log_content)
    for line_num in lines_with_int:
        idx = int(line_num) - 1
        if "_brush." in lines[idx]:
            lines[idx] = lines[idx].replace("_brush.", "_brush->")
        if "_faceBrush." in lines[idx]:
            lines[idx] = lines[idx].replace("_faceBrush.", "_faceBrush->")
        if "_currentColor[" in lines[idx]:
            lines[idx] = lines[idx].replace("_currentColor[", "_currentColor->[")

    with open(path, 'w') as f:
        f.writelines(lines)
    print("Fixed EngineRender.cpp using log file")

if __name__ == '__main__':
    fix_tool_cpp()
    fix_engine_render()
