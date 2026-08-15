import os
import re

def fix_engine_render():
    path = "src/Singularity/Core/EngineRender.cpp"
    with open(path, 'r') as f:
        lines = f.readlines()
        
    for i in range(len(lines)):
        if "renderCreatorToolbar" in lines[i] or "renderNodeGraph" in lines[i] or "_showIntegrationUI" in lines[i] or "_showToolbar" in lines[i] or "_cursorTools.update" in lines[i]:
            lines[i] = "// " + lines[i]
            
    with open(path, 'w') as f:
        f.writelines(lines)
    print("Fixed EngineRender.cpp")

def fix_tool_cpp():
    path = "src/OurVerse/Tool.cpp"
    with open(path, 'r') as f:
        lines = f.readlines()
        
    for i in range(len(lines)):
        lines[i] = lines[i].replace("engine.", "engine->")
        lines[i] = lines[i].replace("game.", "engine->")
        
        if "AdvancedFacePaint::paintFaceAdvanced" in lines[i] or "gradientSettings" in lines[i] or "smudgeSettings" in lines[i] or "if (!success)" in lines[i] or "engine->getCurrentColor" in lines[i] or "timeDelta" in lines[i] or "pressure" in lines[i] or "getLastBrushUV" in lines[i] or "setLastBrushTime" in lines[i] or "getCurrentBrushType" in lines[i]:
            lines[i] = "// " + lines[i]

    with open(path, 'w') as f:
        f.writelines(lines)
    print("Fixed Tool.cpp")

if __name__ == '__main__':
    fix_engine_render()
    fix_tool_cpp()
