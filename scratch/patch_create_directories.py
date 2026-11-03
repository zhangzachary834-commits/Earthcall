import re

with open('src/Singularity/Storage/SaveSystem.cpp', 'r') as f:
    content = f.read()

target = """    std::filesystem::create_directories(dir, ec);
    if (ec) {"""

replacement = """    std::filesystem::create_directories(dir, ec);
    if (ec && !std::filesystem::exists(dir, ec)) {"""

content = content.replace(target, replacement)

with open('src/Singularity/Storage/SaveSystem.cpp', 'w') as f:
    f.write(content)

print("Patched create_directories in SaveSystem.cpp")
