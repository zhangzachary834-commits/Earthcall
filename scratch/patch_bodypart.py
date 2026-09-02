import re

HPP_PATH = "src/Person/Body/BodyPart/BodyPart.hpp"
with open(HPP_PATH, 'r') as f:
    hpp = f.read()

automations_methods = """
    bool hasAutomations() const { return _primaryObject ? _primaryObject->hasAutomations() : false; }
    glm::mat4 sampleAutomations(const glm::mat4& restTransform) { 
        return _primaryObject ? _primaryObject->sampleAutomations(restTransform) : restTransform; 
    }
"""
hpp = hpp.replace("void setFaceColor(int faceIndex, float r, float g, float b);", "void setFaceColor(int faceIndex, float r, float g, float b);\n" + automations_methods)

with open(HPP_PATH, 'w') as f:
    f.write(hpp)
