import sys

file_path = "src/ConstructedBeing/Singular/Object/ObjectRender.cpp"
with open(file_path, "r") as f:
    content = f.read()

content = content.replace("if (r.rendersImplicitExactly()) {", "if (r.rendersImplicitExactly() && _renderMode != RenderMode::Mesh) {")

with open(file_path, "w") as f:
    f.write(content)
print("ObjectRender updated")
