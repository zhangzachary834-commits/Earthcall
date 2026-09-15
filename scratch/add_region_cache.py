import os

with open("src/ConstructedBeing/Singular/Object/Object.hpp", "r") as f:
    content = f.read()

# find private section
idx = content.find("std::string _materialId")
content = content[:idx] + "mutable std::unordered_map<std::string, std::vector<glm::ivec2>> _regionCache;\n    " + content[idx:]

with open("src/ConstructedBeing/Singular/Object/Object.hpp", "w") as f:
    f.write(content)
