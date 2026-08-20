import os

def fix_engine_cpp():
    path = "src/Singularity/Core/Engine.cpp"
    with open(path, 'r') as f:
        content = f.read()
    
    content = content.replace("ZonesOfEarth/AuthorsOfLaw/LawManager.hpp", "ZonesOfEarth/AuthorsOfLaw/Law.hpp")
    
    with open(path, 'w') as f:
        f.write(content)
    print("Fixed Engine.cpp")

def fix_engine_init_cpp():
    path = "src/Singularity/Core/EngineInit.cpp"
    with open(path, 'r') as f:
        content = f.read()

    # Add missing headers to EngineInit
    includes = """#include "Singularity/Screen/Camera.hpp"
#include "Singularity/Network/Chat.hpp"
#include "Singularity/Input/Keyboard/KeyboardHandler.hpp"
#include "Singularity/Input/Mouse/MouseHandler.hpp"
#include "OurVerse/CursorTools.hpp"
#include "OurVerse/ElementalToolHandler.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
"""
    if '#include "Singularity/Screen/Camera.hpp"' not in content:
        content = content.replace('#include "Person/Person.hpp"\n', '#include "Person/Person.hpp"\n' + includes)

    # Fix onFramebufferSize which is missing from Engine.hpp but defined in EngineInit.cpp
    content = content.replace("void Engine::onFramebufferSize", "void Engine::sFramebufferSizeCallback")
    content = content.replace("_camera.viewport[", "_camera->viewport[")
    content = content.replace("_camera.projection", "_camera->projection")
    
    with open(path, 'w') as f:
        f.write(content)
    print("Fixed EngineInit.cpp")

if __name__ == '__main__':
    fix_engine_cpp()
    fix_engine_init_cpp()
