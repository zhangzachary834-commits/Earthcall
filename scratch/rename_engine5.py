import os

def fix_tool_cpp():
    path = "src/OurVerse/Tool.cpp"
    with open(path, 'r') as f:
        content = f.read()

    if '#include "Singularity/Screen/Camera.hpp"' not in content:
        content = '#include "Singularity/Screen/Camera.hpp"\n' + content

    content = content.replace('engine.getMouseHandler()->isLeftPressedLast()', 'engine->isMouseLeftPressedLast()')
    content = content.replace('engine->getMouseHandler()->isLeftPressedLast()', 'engine->isMouseLeftPressedLast()')
    content = content.replace('engine.is2DToolDragging(', 'engine->is2DToolDragging(')
    content = content.replace('engine.begin2DToolDrag(', 'engine->begin2DToolDrag(')
    content = content.replace('engine.end2DToolDrag(', 'engine->end2DToolDrag(')
    content = content.replace('engine.getAdvanced2DBrush()', 'engine->getAdvanced2DBrush()')

    with open(path, 'w') as f:
        f.write(content)
    print("Fixed Tool.cpp")

def fix_cursor_tools_cpp():
    path = "src/OurVerse/CursorTools.cpp"
    with open(path, 'r') as f:
        content = f.read()

    if '#include "Singularity/Input/MouseHandler.hpp"' not in content:
        content = '#include "Singularity/Input/MouseHandler.hpp"\n' + content

    with open(path, 'w') as f:
        f.write(content)
    print("Fixed CursorTools.cpp")

def fix_engine_hpp():
    path = "src/Singularity/Core/Engine.hpp"
    with open(path, 'r') as f:
        content = f.read()

    methods = """
    bool isMouseLeftPressedLast() const { return _mouseLeftPressedLast; }
    void setMouseLeftPressedLast(bool v) { _mouseLeftPressedLast = v; }
    bool is2DToolDragging(int type) const { return _is2DToolDragging; }
    void begin2DToolDrag(int type) { _is2DToolDragging = true; }
    void end2DToolDrag(int type) { _is2DToolDragging = false; }
    """
    
    if "isMouseLeftPressedLast" not in content:
        content = content.replace('bool getAdvanced2DBrush() const { return true; }', 
                                  'bool getAdvanced2DBrush() const { return true; }\n' + methods)

    members = """
    bool _mouseLeftPressedLast = false;
    bool _is2DToolDragging = false;
    """
    if "_mouseLeftPressedLast = false" not in content:
        content = content.replace('double _worldTime = 0.0;', 'double _worldTime = 0.0;\n' + members)

    with open(path, 'w') as f:
        f.write(content)
    print("Fixed Engine.hpp")

if __name__ == '__main__':
    fix_tool_cpp()
    fix_cursor_tools_cpp()
    fix_engine_hpp()
