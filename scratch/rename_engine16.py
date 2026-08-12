import os

def fix_engine_hpp():
    path = "src/Singularity/Core/Engine.hpp"
    with open(path, 'r') as f:
        content = f.read()
        
    enums = """namespace Core {
    enum class PotteryTool { Pinch, Pull, Push, Smooth, Flatten, Sharpen };
    enum class RotationAxisMode { Free, X, Y, Z };
"""
    if "enum class PotteryTool" not in content:
        content = content.replace("namespace Core {", enums)
        
    members = """
    // Missing rotation state
    bool getRotateDragging() const { return false; }
    double getRotateLastCursorX() const { return 0.0; }
    double getRotateLastCursorY() const { return 0.0; }
    void setRotateLastCursor(double x, double y) {}
    float getRotationToolSensitivity() const { return 1.0f; }
    float getRotationToolSmoothness() const { return 1.0f; }
    RotationAxisMode getRotationAxisMode() const { return RotationAxisMode::Free; }
    
    // Missing Pottery state
    PotteryTool getCurrentPotteryTool() const { return PotteryTool::Pinch; }
    float getPotteryStrength() const { return 1.0f; }
    
    // Missing 2D state
    const std::vector<glm::vec2>& get2DToolDragPoints() const { static std::vector<glm::vec2> v; return v; }
    
    Ourverse _world;"""
    if "bool getRotateDragging() const" not in content:
        content = content.replace("Ourverse _world;", members)

    with open(path, 'w') as f:
        f.write(content)
    print("Fixed Engine.hpp")
    
def fix_tool_cpp():
    path = "src/OurVerse/Tool.cpp"
    with open(path, 'r') as f:
        content = f.read()

    # fix some pointers
    content = content.replace("engine->getRotateDragging", "engine.getRotateDragging")
    content = content.replace("engine->getRotateLastCursorX", "engine.getRotateLastCursorX")
    content = content.replace("engine->getRotateLastCursorY", "engine.getRotateLastCursorY")
    content = content.replace("engine->getRotationToolSensitivity", "engine.getRotationToolSensitivity")
    content = content.replace("engine->getRotationToolSmoothness", "engine.getRotationToolSmoothness")
    content = content.replace("engine->getRotationAxisMode", "engine.getRotationAxisMode")
    
    content = content.replace("game.getRotateDragging", "engine.getRotateDragging")
    content = content.replace("game.getRotateLastCursorX", "engine.getRotateLastCursorX")
    content = content.replace("game.getRotateLastCursorY", "engine.getRotateLastCursorY")
    
    with open(path, 'w') as f:
        f.write(content)
    print("Fixed Tool.cpp")

if __name__ == '__main__':
    fix_engine_hpp()
    fix_tool_cpp()
