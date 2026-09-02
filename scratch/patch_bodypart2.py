import re

HPP_PATH = "src/Person/Body/BodyPart/BodyPart.hpp"
with open(HPP_PATH, 'r') as f:
    hpp = f.read()

get_primary_methods = """
    Object* getPrimaryObject() { return _primaryObject.get(); }
    const Object* getPrimaryObject() const { return _primaryObject.get(); }
"""
hpp = hpp.replace("void setFaceColor(int faceIndex, float r, float g, float b);", "void setFaceColor(int faceIndex, float r, float g, float b);\n" + get_primary_methods)

with open(HPP_PATH, 'w') as f:
    f.write(hpp)

CPP_PATH = "src/Singularity/FirstMoverOntology/FirstMoverWindowTools/CreatorConsole/CharacterConsole.cpp"
with open(CPP_PATH, 'r') as f:
    cpp = f.read()

cpp = cpp.replace("state.selectedObject3D = part;", "state.selectedObject3D = part->getPrimaryObject();")

with open(CPP_PATH, 'w') as f:
    f.write(cpp)

