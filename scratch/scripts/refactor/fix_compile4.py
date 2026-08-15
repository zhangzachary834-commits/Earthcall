import os

def fix_ourverse_ui():
    ui_content = """#include "Ourverse.hpp"
#include "../../Singularity/Core/Engine.hpp"

void Ourverse::renderCreatorToolbar() {}
void Ourverse::renderCreatorSectionTabs() {}
void Ourverse::renderPaintConsole(Zone& zone) {}
void Ourverse::render3DConsole() {}
void Ourverse::renderCharacterConsole() {}
void Ourverse::renderWorldConsole() {}
void Ourverse::renderAssetsConsole(Zone& zone) {}
void Ourverse::renderRelationsConsole(ZoneManager& zoneMgr) {}
void Ourverse::renderCreatorStatusBar() {}
void Ourverse::renderSectionButton(CreatorSection section, const char* label) {}
void Ourverse::render3DModeButton(Mode3D mode, const char* label) {}
void Ourverse::renderPlacementInspector() {}
void Ourverse::renderSelectionInspector() {}
"""
    with open("src/ZonesOfEarth/Ourverse/OurverseUI.cpp", "w") as f:
        f.write(ui_content)

def fix_ourverse_node_graph():
    node_content = """#include "Ourverse.hpp"
void Ourverse::renderNodeGraph() {}
void Ourverse::renderNodePanel() {}
"""
    with open("src/ZonesOfEarth/Ourverse/OurverseNodeGraph.cpp", "w") as f:
        f.write(node_content)

def fix_ourverse_saveload():
    # empty file since Game::saveLoad is gone
    with open("src/ZonesOfEarth/Ourverse/OurverseSaveLoad.cpp", "w") as f:
        f.write("// Stubs for removed Game SaveLoad\n")

def fix_tool_cpp():
    with open("src/OurVerse/Tool.cpp", "r") as f:
        content = f.read()
    
    # Fix the missing method
    content = content.replace("getMouseLeftPressedLast", "isMouseLeftPressedLast")
    
    with open("src/OurVerse/Tool.cpp", "w") as f:
        f.write(content)

if __name__ == '__main__':
    fix_ourverse_ui()
    fix_ourverse_node_graph()
    fix_ourverse_saveload()
    fix_tool_cpp()
