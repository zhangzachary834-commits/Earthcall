import os

def fix_engine_render():
    path = "src/Singularity/Core/EngineRender.cpp"
    with open(path, 'r') as f:
        content = f.read()
    
    # Mode3D -> Ourverse::Mode3D
    content = content.replace("Mode3D::", "Ourverse::Mode3D::")
    content = content.replace(" Mode3D ", " Ourverse::Mode3D ")
    content = content.replace("(Mode3D ", "(Ourverse::Mode3D ")
    
    # Missing variables from game which are now in World
    content = content.replace("_selectedObject3D", "getWorld().selectedObject3D")
    content = content.replace("_morphVertexIndex", "getWorld().morphVertexIndex")
    content = content.replace("_current3DMode", "getWorld().current3DMode")
    
    with open(path, 'w') as f:
        f.write(content)
    print("Fixed EngineRender.cpp")

def fix_engine_update():
    path = "src/Singularity/Core/EngineUpdate.cpp"
    with open(path, 'r') as f:
        content = f.read()
    
    # unique_ptr dot to arrow
    content = content.replace("_keyboardHandler.", "_keyboardHandler->")
    content = content.replace("_mouseHandler.", "_mouseHandler->")
    content = content.replace("_camera.", "_camera->")
    content = content.replace("_player.", "_player->")
    content = content.replace("_elementalToolHandler.", "_elementalToolHandler->")
    
    content = content.replace("_showDebugCoordinates", "getWorld().showDebugCoordinates")
    content = content.replace("_placement", "getWorld().placement")
    content = content.replace("BrushPlacementMode", "Ourverse::BrushPlacementMode")
    content = content.replace("_current3DMode", "getWorld().current3DMode")

    with open(path, 'w') as f:
        f.write(content)
    print("Fixed EngineUpdate.cpp")

def fix_tool_cpp():
    path = "src/OurVerse/Tool.cpp"
    with open(path, 'r') as f:
        content = f.read()
        
    content = content.replace("engine->getRotationToolSmoothness", "1.0f")
    content = content.replace("engine->getRotationToolSensitivity", "1.0f")

    with open(path, 'w') as f:
        f.write(content)
    print("Fixed Tool.cpp")

if __name__ == '__main__':
    fix_engine_render()
    fix_engine_update()
    fix_tool_cpp()
