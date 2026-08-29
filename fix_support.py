import sys

file_path = "src/ConstructedBeing/Singular/Object/ObjectCollision.cpp"
with open(file_path, "r") as f:
    content = f.read()

bad = """glm::vec3 Object::getSupportPoint(const glm::vec3& dir) const {
    if ((_hasSmooth || _hasComplex || _hasField || _hasPatch) && !_supportCloud.empty()) {"""

good = """glm::vec3 Object::getSupportPoint(const glm::vec3& dir) const {
    if (_hasField) {
        return glm::vec3(dir.x >= 0.0f ? _fieldExtent.x : -_fieldExtent.x,
                         dir.y >= 0.0f ? _fieldExtent.y : -_fieldExtent.y,
                         dir.z >= 0.0f ? _fieldExtent.z : -_fieldExtent.z);
    }
    if ((_hasSmooth || _hasComplex || _hasPatch) && !_supportCloud.empty()) {"""

if bad in content:
    content = content.replace(bad, good)
    with open(file_path, "w") as f:
        f.write(content)
    print("Fixed getSupportPoint")
else:
    print("Could not find bad block")
