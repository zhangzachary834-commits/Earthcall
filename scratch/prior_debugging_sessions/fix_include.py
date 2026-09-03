import sys

file_path = "src/Singularity/FirstMoverOntology/FirstMoverWindowTools/CreatorConsole/Create3DConsole.cpp"
with open(file_path, "r") as f:
    content = f.read()

old_include = """#include "Singularity/Core/Engine.hpp"
#include "Singularity/Screen/Renderer.hpp"
#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/ConsoleState.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Object/Contour.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include <GLFW/glfw3.h>"""

new_include = """#include "Singularity/Core/Engine.hpp"
#include "Singularity/Screen/Renderer.hpp"
#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/ConsoleState.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Object/Contour.hpp"
#include "ConstructedBeing/Material/Material.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include <GLFW/glfw3.h>"""

if old_include in content:
    content = content.replace(old_include, new_include)
    print("Replaced includes successfully")
else:
    print("Could not find includes")
    
with open(file_path, "w") as f:
    f.write(content)
