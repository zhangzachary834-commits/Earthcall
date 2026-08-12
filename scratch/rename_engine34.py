import os

def fix_engine_init():
    path = "src/Singularity/Core/EngineInit.cpp"
    with open(path, 'r') as f:
        content = f.read()
        
    if "ElementalToolHandler.hpp" not in content:
        content = content.replace('#include "Singularity/Core/Engine.hpp"', '#include "Singularity/Core/Engine.hpp"\n#include "../../OurVerse/ElementalToolHandler.hpp"')
        with open(path, 'w') as f:
            f.write(content)
        print("Fixed EngineInit.cpp")
        
if __name__ == '__main__':
    fix_engine_init()
