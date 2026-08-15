import os
import re

def fix_engine_hpp():
    path = "src/Singularity/Core/Engine.hpp"
    with open(path, 'r') as f:
        content = f.read()
    
    # Add AdvancedFacePaint structs
    if "namespace AdvancedFacePaint {" not in content:
        content = content.replace("class Engine {", "namespace AdvancedFacePaint { struct GradientSettings; struct SmudgeSettings; }\nclass Engine {")
    
    # Fix return types for advanced face paint
    content = content.replace("void* getCurrentGradientSettings() const { return nullptr; }", "AdvancedFacePaint::GradientSettings* getCurrentGradientSettings() const { return nullptr; }")
    content = content.replace("void* getCurrentSmudgeSettings() const { return nullptr; }", "AdvancedFacePaint::SmudgeSettings* getCurrentSmudgeSettings() const { return nullptr; }")

    # Add missing method
    content = content.replace("bool getUsePressureSimulation() const { return false; }", "bool getUsePressureSimulation() const { return false; }\n    double getLastBrushTime() const { return 0.0; }")

    # Remove Chat from Engine.hpp
    content = re.sub(r'class Chat;\n', '', content)
    content = re.sub(r'std::unique_ptr<Chat> _chat;\n', '', content)

    with open(path, 'w') as f:
        f.write(content)
    print("Fixed Engine.hpp")
    
def fix_engine_cpp():
    path = "src/Singularity/Core/Engine.cpp"
    with open(path, 'r') as f:
        content = f.read()
        
    if "OurVerse/ElementalToolHandler.hpp" not in content:
        content = content.replace('#include "Singularity/Core/Engine.hpp"', '#include "Singularity/Core/Engine.hpp"\n#include "OurVerse/ElementalToolHandler.hpp"')
    
    # Remove any remaining _chat usages
    content = content.replace('_chat = std::make_unique<Chat>();', '')
    
    with open(path, 'w') as f:
        f.write(content)
    print("Fixed Engine.cpp")

def fix_engine_render():
    path = "src/Singularity/Core/EngineRender.cpp"
    with open(path, 'r') as f:
        content = f.read()
        
    content = content.replace("_showKeymapWindow", "_world.showKeymapWindow")
    content = content.replace("_showDebugCoordinates", "_world.showDebugCoordinates")
    content = content.replace("_showChatWindow", "_world.showChatWindow")
    content = content.replace("_patchCtrlIndex", "_world.patchCtrlIndex")
    content = content.replace("_straightLineEndX", "_world.straightLineEndX")
    content = content.replace("_straightLineEndY", "_world.straightLineEndY")
    
    with open(path, 'w') as f:
        f.write(content)
    print("Fixed EngineRender.cpp")

def fix_tool_cpp():
    path = "src/OurVerse/Tool.cpp"
    with open(path, 'r') as f:
        content = f.read()
        
    content = content.replace("game.", "engine->")
    content = content.replace("engine.", "engine->")
    
    with open(path, 'w') as f:
        f.write(content)
    print("Fixed Tool.cpp")

if __name__ == '__main__':
    fix_engine_hpp()
    fix_engine_cpp()
    fix_engine_render()
    fix_tool_cpp()
