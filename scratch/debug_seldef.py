import re
with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "r") as f:
    content = f.read()

content = content.replace(
    '    if (found == object.dynamicProperties().end()) return false;\n    const auto* encoded = std::get_if<std::string>(&found->second);\n    if (!encoded) return false;',
    '    if (found == object.dynamicProperties().end()) { std::cout << "seldef: not found\\n"; return false; }\n    const auto* encoded = std::get_if<std::string>(&found->second);\n    if (!encoded) { std::cout << "seldef: not string, type_index=" << found->second.index() << "\\n"; return false; }'
)

with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "w") as f:
    f.write(content)
