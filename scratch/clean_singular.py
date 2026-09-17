import re
with open("src/ConstructedBeing/Singular/Singular.cpp", "r") as f:
    content = f.read()

content = content.replace(
    '    std::cout << "setDynamicProperty: id=" << Earthcall::StringInterner::resolve(id) << " projected=" << projected << "\\n";\n    if (projected && !writeAuthoredPropertyProjection(id, v)) { std::cout << "writeAuthoredPropertyProjection returned false for " << Earthcall::StringInterner::resolve(id) << "\\n"; return false; }',
    '    if (projected && !writeAuthoredPropertyProjection(id, v)) return false;'
)

with open("src/ConstructedBeing/Singular/Singular.cpp", "w") as f:
    f.write(content)
