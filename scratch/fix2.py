with open("src/Singularity/Storage/Serialization/ConstructedBeing/ObjectSerialization.cpp", "r") as f:
    lines = f.readlines()

new_lines = []
for line in lines:
    if line.strip() == "auto alreadyPainted = ownsItsSurface ? materials.get(obj.materialId()) : nullptr;":
        new_lines.append("    auto alreadyPainted = materials.get(obj.materialId());\n")
    else:
        new_lines.append(line)

with open("src/Singularity/Storage/Serialization/ConstructedBeing/ObjectSerialization.cpp", "w") as f:
    f.writelines(new_lines)
