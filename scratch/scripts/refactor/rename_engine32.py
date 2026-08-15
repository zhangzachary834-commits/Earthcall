import os
import re

def fix_engine_render():
    path = "src/Singularity/Core/EngineRender.cpp"
    with open(path, 'r') as f:
        lines = f.readlines()
        
    for i in range(len(lines)):
        # line 325
        if i == 324 and "}" in lines[i]:
            lines[i] = "// " + lines[i]
            
        if "_mainMenu.draw();" in lines[i]:
            lines[i] = "// " + lines[i]
            
        if "if (_drawingStraightLine && getWorld().current3DMode == Ourverse::Mode3D::None && _currentTool.getType() == Tool::Type::Brush) {" in lines[i]:
            lines[i] = "    if (false) {\n"
            
        if "if (getWorld().current3DMode == Ourverse::Mode3D::FaceBrush && _brush.showCursor && _brush.cursorVisible) {" in lines[i]:
            lines[i] = "    if (false) {\n"
            
    with open(path, 'w') as f:
        f.writelines(lines)
    print("Fixed EngineRender.cpp")

if __name__ == '__main__':
    fix_engine_render()
