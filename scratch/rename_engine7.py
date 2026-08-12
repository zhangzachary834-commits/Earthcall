import os

def fix_engine_hpp():
    path = "src/Singularity/Core/Engine.hpp"
    with open(path, 'r') as f:
        content = f.read()

    if '#include <glm/glm.hpp>' not in content:
        content = '#include <glm/glm.hpp>\n' + content
        
    old_methods = """    glm::vec2 get2DToolDragStart() const { return _dragStart; }
    void update2DToolDrag(glm::vec2 pos) { _dragCurrent = pos; }
    void begin2DToolDrag(int type, glm::vec2 pos) { _is2DToolDragging = true; _dragStart = pos; _dragCurrent = pos; }
    bool getUseLegacy2DTools() const { return _useLegacy2DTools; }
    void setCurrentColor(int index, float val) {} // dummy for now"""
    
    new_methods = """    glm::vec2 get2DToolDragStart() const { return _dragStart; }
    void update2DToolDrag(glm::vec2 pos) { _dragCurrent = pos; }
    template<typename T> void begin2DToolDrag(T type, glm::vec2 pos) { _is2DToolDragging = true; _dragStart = pos; _dragCurrent = pos; }
    void end2DToolDrag() { _is2DToolDragging = false; }
    template<typename T> bool is2DToolDragging(T type) const { return _is2DToolDragging; }
    bool getUseLegacy2DTools() const { return _useLegacy2DTools; }
    void setCurrentColor(int index, float val) {} // dummy for now"""
    
    content = content.replace(old_methods, new_methods)
    
    content = content.replace('bool is2DToolDragging(int type) const { return _is2DToolDragging; }\n', '')
    content = content.replace('void end2DToolDrag(int type) { _is2DToolDragging = false; }\n', '')
    
    with open(path, 'w') as f:
        f.write(content)
    print("Fixed Engine.hpp")

if __name__ == '__main__':
    fix_engine_hpp()
