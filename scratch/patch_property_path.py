import sys

file_path = "src/ConstructedBeing/Singular/Property/PropertyPath.hpp"
with open(file_path, "r") as f:
    content = f.read()

find_str = """public:

    // The deepest Property"""

replace_str = """public:
    const std::vector<std::vector<Earthcall::StringId>>& joinedIds() const { return _joinedIds; }

    // The deepest Property"""

if find_str in content:
    content = content.replace(find_str, replace_str)
    with open(file_path, "w") as f:
        f.write(content)
    print("Patched PropertyPath.hpp")
else:
    print("Not found")
