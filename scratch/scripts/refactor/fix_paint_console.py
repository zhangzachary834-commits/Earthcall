import re

with open("src/Singularity/FirstMoverWindowTools/CreatorConsole/PaintConsole.cpp", "r") as f:
    content = f.read()

# Comment out zone. calls that are related to brush/design
methods_to_comment = [
    "getBrushSystem", "initializeBrushSystem", "setCloneActive", "setBrushType",
    "setBrushRadius", "setBrushOpacity", "setBrushFlow", "setBrushSpacing", "setCloneOffset",
    "setDesignTool", "setDrawColor", "setBrushDensity", "setBrushStrength", "setPressureSimulation",
    "setPressureSensitivity", "setStrokeInterpolation", "setUseLayers", "setActiveLayer",
    "setLayerOpacity", "setBlendMode", "addLayer", "deleteLayer", "getActiveDesignLayer",
    "getDesignLayerCount", "setActiveDesignLayer", "getDesignLayerOpacity", "setDesignLayerOpacity",
    "addDesignLayer", "removeDesignLayer", "undo", "redo", "clearHistory"
]

lines = content.split('\n')
new_lines = []
for line in lines:
    has_method = False
    for m in methods_to_comment:
        if f"zone.{m}" in line or f"zone->{m}" in line:
            has_method = True
            break
    if has_method:
        new_lines.append("// " + line)
    else:
        new_lines.append(line)

with open("src/Singularity/FirstMoverWindowTools/CreatorConsole/PaintConsole.cpp", "w") as f:
    f.write("\n".join(new_lines))
