import re
with open("src/ConstructedBeing/Singular/Singular.cpp", "r") as f:
    content = f.read()

content = content.replace(
    '    if (projected && !writeAuthoredPropertyProjection(id, v)) return false;',
    '    if (projected && !writeAuthoredPropertyProjection(id, v)) { std::cout << "writeAuthoredPropertyProjection returned false for " << Earthcall::StringInterner::resolve(id) << "\\n"; return false; }'
)

content = content.replace(
    'bool Singular::setDynamicProperty(Earthcall::StringId id, const PropertyValue& v) {\n    const bool projected = recognizesAuthoredPropertyProjection(id);',
    'bool Singular::setDynamicProperty(Earthcall::StringId id, const PropertyValue& v) {\n    const bool projected = recognizesAuthoredPropertyProjection(id);\n    std::cout << "setDynamicProperty: id=" << Earthcall::StringInterner::resolve(id) << " projected=" << projected << "\\n";'
)

with open("src/ConstructedBeing/Singular/Singular.cpp", "w") as f:
    f.write(content)
