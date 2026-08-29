import sys

file_path = "src/ConstructedBeing/Singular/Object/ObjectCollision.cpp"
with open(file_path, "r") as f:
    content = f.read()

bad = """    if (_hasField) {
        rebuildFieldMesh(); // lazy build
    }
    
    if ((_hasSmooth || _hasComplex || _hasField || _hasPatch) && !_supportCloud.empty()) {
        static int cnt = 0; if (cnt++ % 6000 == 0) printf("supportCloud hit! size: %zu\\n", _supportCloud.size());
        float best = -std::numeric_limits<float>::max();
        glm::vec3 bestV = _supportCloud[0];"""

good = """    if (_hasField) {
        return glm::vec3(dir.x >= 0.0f ? _fieldExtent.x : -_fieldExtent.x,
                         dir.y >= 0.0f ? _fieldExtent.y : -_fieldExtent.y,
                         dir.z >= 0.0f ? _fieldExtent.z : -_fieldExtent.z);
    }
    
    if ((_hasSmooth || _hasComplex || _hasPatch) && !_supportCloud.empty()) {
        float best = -std::numeric_limits<float>::max();
        glm::vec3 bestV = _supportCloud[0];"""

if bad in content:
    content = content.replace(bad, good)
    with open(file_path, "w") as f:
        f.write(content)
    print("Fixed getSupportPoint")
else:
    print("Could not find bad block")
