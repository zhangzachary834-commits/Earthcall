import sys

file_path = "src/Singularity/FirstMoverOntology/FirstMoverWindowTools/CreatorConsole/Create3DConsole.cpp"
with open(file_path, "r") as f:
    content = f.read()

content = content.replace('#include "Singularity/Core/SdfBuild.hpp"', '#include "ConstructedBeing/Material/Material.hpp"\n#include "Singularity/Core/SdfBuild.hpp"')

with open(file_path, "w") as f:
    f.write(content)
