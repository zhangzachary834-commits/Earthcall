import os

def fix_engine_hpp():
    path = "src/Singularity/Core/Engine.hpp"
    with open(path, 'r') as f:
        lines = f.readlines()
        
    out = []
    for line in lines:
        if "Chat* getChat();" in line:
            continue
        out.append(line)
        
    content = "".join(out)
    
    # Add dummy 2D tool methods
    if "is2DToolDragging" not in content:
        dummy_methods = """
    template<typename T> void begin2DToolDrag(T type, glm::vec2 pos) {}
    template<typename T> bool is2DToolDragging(T type) const { return false; }
    void update2DToolDrag(glm::vec2 pos) {}
    glm::vec2 get2DToolDragStart() const { return glm::vec2(0); }
    void end2DToolDrag() {}
"""
        content = content.replace("bool getAdvanced2DBrush() const { return true; }", "bool getAdvanced2DBrush() const { return true; }" + dummy_methods)

    with open(path, 'w') as f:
        f.write(content)
    print("Fixed Engine.hpp")

def fix_tool_cpp():
    path = "src/OurVerse/Tool.cpp"
    with open(path, 'r') as f:
        lines = f.readlines()
        
    # from line 595 to 655, engine-> becomes engine.
    for i in range(595, 660):
        if i < len(lines):
            lines[i] = lines[i].replace("engine->", "engine.")
            
    with open(path, 'w') as f:
        f.writelines(lines)
    print("Fixed Tool.cpp")
    
if __name__ == '__main__':
    fix_engine_hpp()
    fix_tool_cpp()
