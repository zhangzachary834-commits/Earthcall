import os

def fix_tool_cpp():
    with open("src/OurVerse/Tool.cpp", "r") as f:
        content = f.read()

    content = content.replace("Core::Engine::PotteryTool::Expand", "1")
    content = content.replace("Core::Engine::RotationAxisMode", "Ourverse::RotationAxisMode")
    content = content.replace("Core::Engine::PublicBrushType::Normal", "0")
    content = content.replace("Core::Engine::PublicBrushType::Smudge", "1")
    content = content.replace("Core::Engine::PublicBrushType::Clone", "2")
    content = content.replace("Core::Engine::PublicBrushType::Erase", "3")
    content = content.replace("Core::Engine::PublicBrushType", "int")
    
    content = content.replace("gradientSettings, smudgeSettings", "nullptr, nullptr")
    content = content.replace("engine->setBrushCursorPos", "// engine->setBrushCursorPos")
    content = content.replace("engine->getBrushFlow()", "0.0f")
    content = content.replace("glm::vec3(0)", "glm::vec2(0)")
    
    with open("src/OurVerse/Tool.cpp", "w") as f:
        f.write(content)

if __name__ == '__main__':
    fix_tool_cpp()
