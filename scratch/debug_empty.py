import re
with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "r") as f:
    content = f.read()

content = content.replace(
    '    std::vector<glm::ivec2> selected = selectedTexels(ft, selector, *this);',
    '    std::vector<glm::ivec2> selected = selectedTexels(ft, selector, *this);\n    std::cout << "ELEVATE " << propertyName << " selected " << selected.size() << " texels\\n";'
)

with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "w") as f:
    f.write(content)
