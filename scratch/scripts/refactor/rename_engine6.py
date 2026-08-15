import os

def fix_tool_cpp():
    path = "src/OurVerse/Tool.cpp"
    with open(path, 'r') as f:
        content = f.read()

    # We need to change engine-> back to engine. for all the methods we incorrectly changed
    # EXCEPT in buildMouseRay where engine is a pointer.
    
    # Just fix the specific lines
    content = content.replace('engine->isMouseLeftPressedLast()', 'engine.isMouseLeftPressedLast()')
    content = content.replace('engine->is2DToolDragging(', 'engine.is2DToolDragging(')
    content = content.replace('engine->begin2DToolDrag(', 'engine.begin2DToolDrag(')
    content = content.replace('engine->end2DToolDrag(', 'engine.end2DToolDrag(')
    content = content.replace('engine->getAdvanced2DBrush()', 'engine.getAdvanced2DBrush()')
    content = content.replace('engine.getMouseHandler()->getCursorX()', 'engine.getMouseHandler()->getCursorX()')
    content = content.replace('engine.update2DToolDrag(', 'engine.update2DToolDrag(')
    content = content.replace('engine.get2DToolDragStart()', 'engine.get2DToolDragStart()')
    content = content.replace('engine.getUseLegacy2DTools()', 'engine.getUseLegacy2DTools()')
    content = content.replace('engine.setCurrentColor(', 'engine.setCurrentColor(')
    
    # wait, there are some engine. methods missing from Engine.hpp:
    # get2DToolDragStart, update2DToolDrag, getUseLegacy2DTools, setCurrentColor
    with open(path, 'w') as f:
        f.write(content)
    print("Fixed Tool.cpp")

def fix_engine_hpp():
    path = "src/Singularity/Core/Engine.hpp"
    with open(path, 'r') as f:
        content = f.read()

    methods = """
    glm::vec2 get2DToolDragStart() const { return _dragStart; }
    void update2DToolDrag(glm::vec2 pos) { _dragCurrent = pos; }
    void begin2DToolDrag(int type, glm::vec2 pos) { _is2DToolDragging = true; _dragStart = pos; _dragCurrent = pos; }
    bool getUseLegacy2DTools() const { return _useLegacy2DTools; }
    void setCurrentColor(int index, float val) {} // dummy for now
    """
    
    if "get2DToolDragStart" not in content:
        content = content.replace('void begin2DToolDrag(int type) { _is2DToolDragging = true; }', methods)
        
    members = """
    glm::vec2 _dragStart = {0,0};
    glm::vec2 _dragCurrent = {0,0};
    bool _useLegacy2DTools = false;
    """
    if "_dragStart" not in content:
        content = content.replace('bool _is2DToolDragging = false;', 'bool _is2DToolDragging = false;\n' + members)

    with open(path, 'w') as f:
        f.write(content)
    print("Fixed Engine.hpp")

if __name__ == '__main__':
    fix_tool_cpp()
    fix_engine_hpp()
