import re

header_path = "src/ConstructedBeing/Singular/Property/PropertyPath.hpp"
with open(header_path, "r") as f:
    content = f.read()

# Add Earthcall::StringId fullId() const;
content = content.replace(
    "std::string toString() const;\n    bool empty() const { return segments.empty(); }",
    "std::string toString() const;\n    Earthcall::StringId fullId() const;\n    bool empty() const { return segments.empty(); }"
)

with open(header_path, "w") as f:
    f.write(content)

cpp_path = "src/ConstructedBeing/Singular/Property/PropertyPath.cpp"
with open(cpp_path, "r") as f:
    content = f.read()

# Implement fullId()
# Earthcall::StringId fullId() const {
#     if (segments.empty()) return Earthcall::StringId();
#     return _joinedIds[0].back();
# }
content = content.replace(
    "std::string PropertyPath::toString() const {",
    "Earthcall::StringId PropertyPath::fullId() const {\n    if (_joinedIds.empty() || _joinedIds[0].empty()) return Earthcall::StringId();\n    return _joinedIds[0].back();\n}\n\nstd::string PropertyPath::toString() const {"
)

with open(cpp_path, "w") as f:
    f.write(content)
