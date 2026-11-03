import re

with open('src/Singularity/OntoMath/ScalarForm.cpp', 'r') as f:
    content = f.read()

target = """    if (j.contains("arg")) {
        node->stringArg = j["arg"].get<std::string>();
    }"""

replacement = """    if (j.contains("arg")) {
        node->stringArg = j["arg"].get<std::string>();
    } else if (j.contains("stringArg")) {
        node->stringArg = j["stringArg"].get<std::string>();
    }"""

content = content.replace(target, replacement)

with open('src/Singularity/OntoMath/ScalarForm.cpp', 'w') as f:
    f.write(content)

print("Patched MathNode::fromJson")
