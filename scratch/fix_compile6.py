import os

def fix_tool_cpp():
    with open("src/OurVerse/Tool.cpp", "r") as f:
        content = f.read()

    content = content.replace("engine->getCurrentPotteryTool() == 1", "true")
    content = content.replace("Ourverse::RotationAxisMode", "Core::RotationAxisMode")
    content = content.replace("case int::Airbrush:", "case 4:")
    content = content.replace("case int::Chalk:", "case 5:")
    content = content.replace("case int::Spray:", "case 6:")
    
    with open("src/OurVerse/Tool.cpp", "w") as f:
        f.write(content)

if __name__ == '__main__':
    fix_tool_cpp()
