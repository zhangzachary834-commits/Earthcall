import os
import re

def fix_engine_hpp():
    path = "src/Singularity/Core/Engine.hpp"
    with open(path, 'r') as f:
        content = f.read()

    # Fix ElementalToolHandler forward declaration
    content = content.replace("class ElementalToolHandler;", "")
    content = "class ElementalToolHandler;\n" + content
    
    # Fix unique_ptr declaration to use global namespace
    content = content.replace("std::unique_ptr<ElementalToolHandler> _elementalToolHandler;", "std::unique_ptr<::ElementalToolHandler> _elementalToolHandler;")

    # Remove Chat entirely
    content = content.replace("class Chat;\n", "")
    content = content.replace("std::unique_ptr<Chat> _chat;\n", "")
    
    # Add dummy softness to DummyFaceBrush
    content = content.replace("bool soft=false;", "bool soft=false; float softness=0.0f;")
    
    # Add render() declaration if not present
    if "void render(float alpha);" not in content:
        content = content.replace("void tick(float dt);", "void tick(float dt);\n    void render(float alpha);\n    void renderNametags();")

    with open(path, 'w') as f:
        f.write(content)
    print("Fixed Engine.hpp")
    
def fix_engine_render():
    path = "src/Singularity/Core/EngineRender.cpp"
    with open(path, 'r') as f:
        lines = f.readlines()
        
    for i in range(len(lines)):
        # _showChatWindow, _showKeymapWindow, etc.
        lines[i] = lines[i].replace("_showKeymapWindow", "_world.showKeymapWindow")
        lines[i] = lines[i].replace("_showDebugCoordinates", "_world.showDebugCoordinates")
        lines[i] = lines[i].replace("_showChatWindow", "_world.showChatWindow")
        lines[i] = lines[i].replace("_patchCtrlIndex", "_world.patchCtrlIndex")
        
        # Disable Chat rendering
        if "_chat" in lines[i]:
            lines[i] = "// " + lines[i]

    with open(path, 'w') as f:
        f.writelines(lines)
    print("Fixed EngineRender.cpp")

def fix_tool_cpp():
    path = "src/OurVerse/Tool.cpp"
    with open(path, 'r') as f:
        lines = f.readlines()
        
    for i in range(len(lines)):
        lines[i] = lines[i].replace("game.", "engine->")
        
        # fix pointer issues from build23.log
        if "setBrushCursorPos" in lines[i] or "setBrushCursorVisible" in lines[i] or "getCurrentPressure" in lines[i] or "getUsePressureSimulation" in lines[i] or "getLastBrushTime" in lines[i] or "getCurrentGradientSettings" in lines[i] or "getCurrentSmudgeSettings" in lines[i]:
            lines[i] = "// " + lines[i]

    with open(path, 'w') as f:
        f.writelines(lines)
    print("Fixed Tool.cpp")

if __name__ == '__main__':
    fix_engine_hpp()
    fix_engine_render()
    fix_tool_cpp()
