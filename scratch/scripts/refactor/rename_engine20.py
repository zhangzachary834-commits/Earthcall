import os
import re

def fix_engine_hpp():
    path = "src/Singularity/Core/Engine.hpp"
    with open(path, 'r') as f:
        content = f.read()
    
    if "void render(float" not in content:
        content = content.replace("Ourverse _world;", "Ourverse _world;\n    void render(float alpha);\n    void renderNametags();")
        
    # fix the advanced settings dummy return types
    content = content.replace("int getCurrentGradientSettings() const { return 0; }", "void* getCurrentGradientSettings() const { return nullptr; }")
    content = content.replace("int getCurrentSmudgeSettings() const { return 0; }", "void* getCurrentSmudgeSettings() const { return nullptr; }")
    
    # add other dummy vars
    content = content.replace("Ourverse _world;", "Ourverse _world;\n    int _patchCtrlIndex = 0;\n    float _currentColor[4] = {1,1,1,1};\n    int _brush = 0;\n    int _faceBrush = 0;\n    float _cubeAngle = 0;\n    double getCursorX() const { return 0; }\n    double getCursorY() const { return 0; }\n    float getFaceBrushUOffset() const { return 0; }\n    float getFaceBrushVOffset() const { return 0; }\n    void setBrushCursorPos(float, float) {}\n    void setBrushCursorVisible(bool) {}\n    float getCurrentPressure() const { return 1.0f; }\n    bool getUsePressureSimulation() const { return false; }")

    with open(path, 'w') as f:
        f.write(content)
    print("Fixed Engine.hpp")
    
def fix_engine_cpp():
    path = "src/Singularity/Core/Engine.cpp"
    with open(path, 'r') as f:
        content = f.read()
        
    content = content.replace('#include "Singularity/Network/Chat.hpp"', '')
    content = content.replace('_chat = std::make_unique<Chat>();', '')
    
    with open(path, 'w') as f:
        f.write(content)
    print("Fixed Engine.cpp")

def fix_engine_render():
    path = "src/Singularity/Core/EngineRender.cpp"
    with open(path, 'r') as f:
        content = f.read()
        
    content = content.replace("getWorld().setPlayerEyeHeight", "getWorld().playerEyeHeight") # dummy replace
    content = content.replace("_world.setPlayerEyeHeight", "_world.playerEyeHeight")
    
    with open(path, 'w') as f:
        f.write(content)
    print("Fixed EngineRender.cpp")

def fix_tool_cpp():
    path = "src/OurVerse/Tool.cpp"
    with open(path, 'r') as f:
        content = f.read()
        
    content = content.replace("game.", "engine->")
    
    with open(path, 'w') as f:
        f.write(content)
    print("Fixed Tool.cpp")

if __name__ == '__main__':
    fix_engine_hpp()
    fix_engine_cpp()
    fix_engine_render()
    fix_tool_cpp()
