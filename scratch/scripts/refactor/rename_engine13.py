import os

def fix_tool_cpp():
    path = "src/OurVerse/Tool.cpp"
    with open(path, 'r') as f:
        content = f.read()

    # 626: engine->getMouseHandler() -> engine.getMouseHandler()
    content = content.replace("engine->getMouseHandler()->", "engine.getMouseHandler()->")
    # 652: engine->getMouseHandler() -> engine.getMouseHandler()
    # Let's just do a global replace for all places where `engine` is passed as a reference.
    # Actually wait, in `execute` methods, `engine` is often passed as a pointer or a reference?
    # Let's fix line 942: `engine.getKeyboardHandler()` -> `engine->getKeyboardHandler()`
    
    # 740: engine.get2DToolDragPoints(type)
    content = content.replace("engine.get2DToolDragPoints(type)", "engine.get2DToolDragPoints()")

    # 942, 954, 955: engine is a pointer in buildMouseRay? No, in FacePaint!
    # "member reference type 'Core::Engine *' is a pointer"
    # I'll just change all `engine.` to `engine->` inside FacePaint and Rotate3D.
    
    lines = content.split('\n')
    for i in range(len(lines)):
        if "engine->getMouseHandler()->" in lines[i] and "Engine& engine" in "".join(lines[max(0, i-50):i]):
            lines[i] = lines[i].replace("engine->", "engine.")
        if "engine.getKeyboardHandler()" in lines[i]:
            lines[i] = lines[i].replace("engine.", "engine->")
        if "engine.getWorld()" in lines[i]:
            lines[i] = lines[i].replace("engine.", "engine->")
        if "engine.getMouseLeftPressedLast()" in lines[i]:
            lines[i] = lines[i].replace("engine.", "engine->")
        if "engine.getCurrentPotteryTool()" in lines[i]:
            lines[i] = lines[i].replace("engine.", "engine->")
        if "engine.getPotteryStrength()" in lines[i]:
            lines[i] = lines[i].replace("engine.", "engine->")
        if "game.getMouseHandler()" in lines[i]:
            lines[i] = lines[i].replace("game.", "engine->")
            
        if "engine->isMouseLeftPressedLast()" in lines[i]:
            # check if it needs to be -> or .
            pass
            
        # 1016: engine.getRotateDragging() -> false() error
        if "false()" in lines[i]:
            lines[i] = lines[i].replace("false()", "engine->getRotateDragging()")
        if "0.0()" in lines[i]:
            lines[i] = lines[i].replace("0.0()", "engine->getRotateLastCursorX()")
            lines[i] = lines[i].replace("0.0f()", "engine->getRotateLastCursorY()")
            
        if "engine.getRotateDragging" in lines[i]:
            lines[i] = lines[i].replace("engine.getRotateDragging", "engine->getRotateDragging")
        if "engine.getRotateLastCursorX" in lines[i]:
            lines[i] = lines[i].replace("engine.getRotateLastCursorX", "engine->getRotateLastCursorX")
        if "engine.getRotateLastCursorY" in lines[i]:
            lines[i] = lines[i].replace("engine.getRotateLastCursorY", "engine->getRotateLastCursorY")
            
        if "engine.getSelectedObject3D" in lines[i]:
            lines[i] = lines[i].replace("engine.getSelectedObject3D", "engine->getWorld().selectedObject3D")
        if "engine.getRotationToolSmoothness" in lines[i]:
            lines[i] = lines[i].replace("engine.getRotationToolSmoothness", "engine->getRotationToolSmoothness")
        if "engine.setRotateLastCursor" in lines[i]:
            lines[i] = lines[i].replace("engine.setRotateLastCursor", "engine->setRotateLastCursor")

    content = '\n'.join(lines)
    
    # Missing PotteryTool
    content = content.replace("PotteryTool::Pinch", "Core::PotteryTool::Pinch")

    with open(path, 'w') as f:
        f.write(content)
    print("Fixed Tool.cpp")

if __name__ == '__main__':
    fix_tool_cpp()
