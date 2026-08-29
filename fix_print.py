import sys

file_path = "src/ConstructedBeing/Singular/Object/ObjectRender.cpp"
with open(file_path, "r") as f:
    content = f.read()

content = content.replace('r.drawMesh(_fieldMesh, mat);', 'r.drawMesh(_fieldMesh, mat);\n    static int printed = 0; if (printed++ % 60 == 0) printf("Triangles: %zu\\n", _fieldMesh.tris.size() / 3);')

with open(file_path, "w") as f:
    f.write(content)
