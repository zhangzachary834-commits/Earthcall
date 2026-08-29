import sys

file_path = "src/ConstructedBeing/Singular/Object/ObjectCollision.cpp"
with open(file_path, "r") as f:
    content = f.read()

content = content.replace('void Object::rebuildFieldMesh() const {\n    if (!_fieldMeshDirty || !_hasField) return;', 'void Object::rebuildFieldMesh() const {\n    if (!_fieldMeshDirty || !_hasField) return;\n    printf("REBUILDING FIELD MESH\\n");')

with open(file_path, "w") as f:
    f.write(content)
