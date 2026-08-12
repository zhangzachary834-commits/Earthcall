import os
import re

def fix_ourverse_includes():
    files_to_fix = [
        "src/ZonesOfEarth/Ourverse/OurverseNodeGraph.cpp",
        "src/ZonesOfEarth/Ourverse/OurverseSaveLoad.cpp",
        "src/ZonesOfEarth/Ourverse/OurverseUI.cpp",
        "src/OurVerse/Tool.cpp",
        "src/Singularity/Core/EngineRender.cpp"
    ]
    for path in files_to_fix:
        if os.path.exists(path):
            with open(path, "r") as f:
                content = f.read()
            content = content.replace('#include "Engine.hpp"', '#include "../../Singularity/Core/Engine.hpp"')
            with open(path, "w") as f:
                f.write(content)

def fix_tool_cpp():
    path = "src/OurVerse/Tool.cpp"
    with open(path, "r") as f:
        lines = f.readlines()
        
    for i in range(len(lines)):
        broken_words = [
            "dir /",
            "AdvancedFacePaint",
            "getUseStrokeInterpolation",
            "getFaceBrushRadius",
            "getFaceBrushSoftness",
            "getBrushOpacity",
            "getBrushSpacing",
            "timeDelta"
        ]
        if any(w in lines[i] for w in broken_words) and not lines[i].strip().startswith("//"):
            lines[i] = "// " + lines[i]

    with open(path, "w") as f:
        f.writelines(lines)
        
if __name__ == '__main__':
    fix_ourverse_includes()
    fix_tool_cpp()
