import os
import re

def fix_tool_cpp():
    path = "src/OurVerse/Tool.cpp"
    with open(path, 'r') as f:
        content = f.read()

    # Step 1: Parameter name replacements
    content = content.replace("Core::Game* game", "Core::Engine* engine")
    content = content.replace("Core::Game& game", "Core::Engine& engine")
    content = content.replace("Core::Game", "Core::Engine")
    content = content.replace("Game.hpp", "Engine.hpp")

    # Step 2: Accessor changes
    content = content.replace("game->getCameraViewport()", "engine->getCamera()->getViewport()")
    content = content.replace("game->isCursorLocked()", "engine->getMouseHandler()->isCursorLocked()")
    content = content.replace("game->getCameraModelview()", "engine->getCamera()->getModelview()")
    content = content.replace("game->getCameraProjection()", "engine->getCamera()->getProjection()")
    content = content.replace("game->getUseLegacy2DTools()", "engine->getUseLegacy2DTools()")
    content = content.replace("game->end2DToolDrag()", "engine->end2DToolDrag()")
    content = content.replace("game->isMouseLeftPressedLast()", "engine->isMouseLeftPressedLast()")
    content = content.replace("game->getCursorX()", "engine->getMouseHandler()->getCursorX()")
    content = content.replace("game->getCursorY()", "engine->getMouseHandler()->getCursorY()")
    content = content.replace("game->setCurrentColor(", "engine->setCurrentColor(")
    content = content.replace("game->begin2DToolDrag(", "engine->begin2DToolDrag(")
    content = content.replace("game->is2DToolDragging(", "engine->is2DToolDragging(")
    content = content.replace("game->update2DToolDrag(", "engine->update2DToolDrag(")
    content = content.replace("game->get2DToolDragPoints()", "engine->get2DToolDragPoints()")
    content = content.replace("game->get2DToolDragStart()", "engine->get2DToolDragStart()")
    content = content.replace("game->setRotateDragging(", "engine->setRotateDragging(")
    content = content.replace("game->setRotateLastCursor(", "engine->setRotateLastCursor(")
    content = content.replace("game->isAdvancedFacePaintEnabled()", "engine->isAdvancedFacePaintEnabled()")
    content = content.replace("game->getRotationToolSensitivity()", "engine->getRotationToolSensitivity()")
    content = content.replace("game->getRotationAxisMode()", "engine->getRotationAxisMode()")
    content = content.replace("game->getWorld()", "engine->getWorld()")
    
    content = content.replace("game.", "engine.")
    content = content.replace("game->", "engine->")

    with open(path, 'w') as f:
        f.write(content)
    print("Fixed Tool.cpp")

def fix_engine_render():
    path = "src/Singularity/Core/EngineRender.cpp"
    with open(path, 'r') as f:
        content = f.read()

    content = content.replace("Game::", "Engine::")
    content = content.replace("Game.hpp", "Engine.hpp")

    # Remove the straight line drawing block
    content = re.sub(r'if \(_drawingStraightLine && getWorld\(\)\.current3DMode == Ourverse::Mode3D::None && _currentTool\.getType\(\) == Tool::Type::Brush\) \{.*?\n    \}', r'', content, flags=re.DOTALL)
    
    # Remove the FaceBrush drawing block
    content = re.sub(r'if \(getWorld\(\)\.current3DMode == Ourverse::Mode3D::FaceBrush && _brush\.showCursor && _brush\.cursorVisible\) \{.*?\n    \}', r'', content, flags=re.DOTALL)

    content = content.replace("_mainMenu.draw();", "// _mainMenu.draw();")
    content = content.replace("_chat.draw();", "// _chat.draw();")
    content = content.replace("_integrationManager->drawUI();", "// _integrationManager->drawUI();")
    content = content.replace("_polyhedronSettings->drawUI();", "// _polyhedronSettings->drawUI();")
    content = content.replace("if (_toolbar) {", "if (false) {")

    with open(path, 'w') as f:
        f.write(content)
    print("Fixed EngineRender.cpp")

def fix_engine_update():
    path = "src/Singularity/Core/EngineUpdate.cpp"
    with open(path, 'r') as f:
        content = f.read()

    content = content.replace("Game::", "Engine::")
    content = content.replace("Game.hpp", "Engine.hpp")

    # Replace everything from the first if (!_mainMenu.isOpen()...) to _worldTime += ...
    content = re.sub(r'if \(!_mainMenu\.isOpen\(\) && getWorld\(\)\.current3DMode == Ourverse::Mode3D::BrushCreate && !anyTextInputActive\) \{.*_worldTime \+= static_cast<double>\(dt\);', r'_worldTime += static_cast<double>(dt);', content, flags=re.DOTALL)

    with open(path, 'w') as f:
        f.write(content)
    print("Fixed EngineUpdate.cpp")

if __name__ == '__main__':
    fix_tool_cpp()
    fix_engine_render()
    fix_engine_update()
