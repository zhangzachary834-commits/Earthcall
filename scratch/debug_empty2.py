import re
with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "r") as f:
    content = f.read()

content = content.replace(
    '    const auto selected = selectedTexels(ft, selector, *this);',
    '    const auto selected = selectedTexels(ft, selector, *this);\n    std::cout << "ELEVATE " << propertyName << " selected " << selected.size() << " texels\\n";'
)

with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "w") as f:
    f.write(content)
