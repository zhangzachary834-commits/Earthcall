import sys

file_path = "src/Singularity/Storage/Serialization.cpp"
with open(file_path, "r") as f:
    content = f.read()

content = content.replace('j["rotationResponsiveness"] = obj.getRotationResponsiveness();', 'j["rotationResponsiveness"] = obj.getRotationResponsiveness();\n    j["renderMode"] = static_cast<int>(obj.getRenderModeProp());')

content = content.replace('obj.setRotationResponsiveness(j["rotationResponsiveness"].get<float>());\n    }', 'obj.setRotationResponsiveness(j["rotationResponsiveness"].get<float>());\n    }\n    if (j.contains("renderMode")) {\n        obj.setRenderModeProp(j["renderMode"].get<int>());\n    }')

with open(file_path, "w") as f:
    f.write(content)
print("Serialization updated")
