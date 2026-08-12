import os
import re

def fix_tool_cpp():
    path = "src/OurVerse/Tool.cpp"
    with open(path, 'r') as f:
        content = f.read()

    # Replace standalone 'game' variables
    # We only want to replace standalone 'game' with 'engine'
    # But wait, earlier I replaced 'Core::Game' with 'Core::Engine', so 'game' as a type is gone.
    # Now replace variable names:
    content = re.sub(r'\bgame\b', 'engine', content)
    
    with open(path, 'w') as f:
        f.write(content)
    print("Fixed Tool.cpp")

def fix_engine_files():
    for path in ["src/Singularity/Core/EngineUpdate.cpp", "src/Singularity/Core/EngineRender.cpp"]:
        with open(path, 'r') as f:
            content = f.read()
            
        content = content.replace("_camera.", "_camera->")
        content = content.replace("_keyboardHandler.", "_keyboardHandler->")
        content = content.replace("_mouseHandler.", "_mouseHandler->")
        content = content.replace("_player.", "_player->")
        content = content.replace("void Engine::update(float dt)", "void Engine::tick(float dt)")
        
        # We need to comment out lines that use _showDebugCoordinates or _placement
        bad_vars = ["_showDebugCoordinates", "_placement", "_toolbar"]
        for bad in bad_vars:
            content = re.sub(r'^.*' + bad + r'.*$', r'// \g<0>', content, flags=re.MULTILINE)

        with open(path, 'w') as f:
            f.write(content)
        print("Fixed " + path)

if __name__ == '__main__':
    fix_tool_cpp()
    fix_engine_files()
