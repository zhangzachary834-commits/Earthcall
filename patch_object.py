import sys

file_path = "src/ConstructedBeing/Singular/Object/Object.hpp"
with open(file_path, "r") as f:
    lines = f.readlines()

new_lines = []
for i, line in enumerate(lines):
    new_lines.append(line)
    if "glm::vec3               _fieldExtent{1.0f, 1.0f, 1.0f};" in line:
        new_lines.append("    mutable uint64_t        _memoIdBase = 0;\n")
        new_lines.append("    uint32_t                _fieldRevision = 0;\n")
    if "void setRelationships(int r)" in line:
        pass # Just an anchor
    if "int getRelationships() const" in line:
        new_lines.insert(-1, """
    uint64_t getMemoId(int suffix = 0) const {
        if (_memoIdBase == 0) {
            static std::atomic<uint64_t> counter{1000};
            _memoIdBase = counter.fetch_add(100);
        }
        return _memoIdBase + suffix;
    }
    uint32_t getFieldRevision() const { return _fieldRevision; }
""")

with open(file_path, "w") as f:
    f.writelines(new_lines)
