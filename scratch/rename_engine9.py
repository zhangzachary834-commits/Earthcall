import os
import re

def fix_engine_cpp():
    path = "src/Singularity/Core/Engine.cpp"
    with open(path, 'r') as f:
        content = f.read()

    # Add includes
    includes = """#include "Person/Person.hpp"
#include "Singularity/Screen/Camera.hpp"
#include "ZonesOfEarth/Law/LawManager.hpp"
#include "Singularity/Network/Chat.hpp"
#include "Singularity/Input/KeyboardHandler.hpp"
#include "Singularity/Input/MouseHandler.hpp"
#include "OurVerse/CursorTools.hpp"
#include "OurVerse/ElementalToolHandler.hpp"
"""
    if '#include "Person/Person.hpp"' not in content:
        content = content.replace('#include "Singularity/Core/EventBus.hpp"\n', '#include "Singularity/Core/EventBus.hpp"\n' + includes)

    # Remove Game from LoopContext
    content = content.replace("    Game* game;\n", "")
    content = content.replace("ctx->game->shutdown();\n", "")
    content = content.replace("&game, ", "")
    content = content.replace("*(ctx->game), ", "")

    # Fix run() and tick()
    content = content.replace("void Engine::run(Game& game) {", "void Engine::run() {")
    content = content.replace("tick(game, dt);", "tick(dt);")
    content = content.replace("game.shutdown();", "")
    
    content = content.replace("void Engine::tick(Game& game, float dt) {", "void Engine::tick(float dt) {")
    content = content.replace("        game.update(dt);\n", "")
    content = content.replace("        game.render(); // brackets itself with Renderer begin/endFrame\n", "")

    with open(path, 'w') as f:
        f.write(content)
    print("Fixed Engine.cpp")

def fix_tool_cpp():
    path = "src/OurVerse/Tool.cpp"
    with open(path, 'r') as f:
        content = f.read()

    content = content.replace("game.get2DToolDragPoints(type)", "engine.get2DToolDragStart()")
    content = content.replace("game.getCurrentPotteryTool()", "PotteryTool::Pinch") # dummy
    content = content.replace("game.getPotteryStrength()", "1.0f") # dummy
    content = content.replace("game.getRotateDragging()", "false") # dummy
    content = content.replace("game.getRotateLastCursorX()", "0.0f") # dummy
    content = content.replace("game.getRotateLastCursorY()", "0.0f") # dummy
    content = content.replace("game.setSelectedObject3D", "engine.setSelectedObject3D") # wait
    content = content.replace("engine->", "engine.") # fix line 942, 1029 where engine is a reference
    
    # Actually wait, in buildMouseRay, engine is a pointer. 
    # Let's be careful.
    content = content.replace("engine.getCamera()", "engine->getCamera()") # Fix buildMouseRay (lines 238, 262-265)

    with open(path, 'w') as f:
        f.write(content)
    print("Fixed Tool.cpp")

def fix_engine_init():
    path = "src/Singularity/Core/EngineInit.cpp"
    with open(path, 'r') as f:
        content = f.read()
    
    content = re.sub(r'mgr\.active\(\)\.world\(\)\.getMainMenu\(\)', 'true', content)
    content = re.sub(r'mgr\.active\(\)\.world\(\)\.saveStateWithLog\([^)]+\)', '', content)
    content = re.sub(r'mgr\.active\(\)\.world\(\)\.showChatWindow\([^)]+\)', '', content)
    content = re.sub(r'mgr\.active\(\)\.world\(\)\.showToolbar\([^)]+\)', '', content)
    content = re.sub(r'mgr\.active\(\)\.world\(\)\.showKeymapWindow\([^)]+\)', '', content)
    
    # EngineInit line 211: use of undeclared identifier 'registerCallbacks'
    content = content.replace("registerCallbacks(window);", "")

    with open(path, 'w') as f:
        f.write(content)
    print("Fixed EngineInit.cpp")

if __name__ == '__main__':
    fix_engine_cpp()
    fix_tool_cpp()
    fix_engine_init()
