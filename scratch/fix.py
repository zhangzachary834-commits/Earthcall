with open("src/Singularity/Storage/Serialization/ConstructedBeing/ObjectSerialization.cpp", "r") as f:
    lines = f.readlines()

new_lines = []
for line in lines:
    if line.strip() == "// Face colours":
        new_lines.append(line)
        new_lines.append("    const bool ownsItsSurface = obj.materialId() == \"material.\" + obj.getIdentifier();\n")
        new_lines.append("    auto alreadyPainted = ownsItsSurface ? materials.get(obj.materialId()) : nullptr;\n")
        new_lines.append("    const bool texturesAlreadyHere = alreadyPainted && !alreadyPainted->faceTextures.empty();\n")
    elif line.strip() == "if (j.contains(\"faceColors\")) {":
        new_lines.append(line)
    elif "const bool ownsItsSurface =" in line and not line.startswith("    const bool ownsItsSurface"):
        continue
    elif "auto alreadyPainted =" in line and not line.startswith("    auto alreadyPainted"):
        continue
    elif "const bool texturesAlreadyHere =" in line and not line.startswith("    const bool texturesAlreadyHere"):
        continue
    elif "alreadyPainted && !alreadyPainted->faceTextures.empty();" in line and not line.startswith("    const bool texturesAlreadyHere"):
        continue
    elif line.strip() == "":
        new_lines.append(line)
    else:
        new_lines.append(line)

with open("src/Singularity/Storage/Serialization/ConstructedBeing/ObjectSerialization.cpp", "w") as f:
    f.writelines(new_lines)
