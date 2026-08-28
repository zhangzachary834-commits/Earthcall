import sys

file_path = "src/ConstructedBeing/Singular/Object/ObjectCollision.cpp"
with open(file_path, "r") as f:
    lines = f.readlines()

new_lines = []
for i, line in enumerate(lines):
    new_lines.append(line)
    if "void Object::rebuildGeometryCaches() {" in line:
        new_lines.append("    _fieldRevision++;\n")

with open(file_path, "w") as f:
    f.writelines(new_lines)
