import re
with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "r") as f:
    content = f.read()

content = content.replace(
    'bool Object::writeAuthoredPropertyProjection(Earthcall::StringId id,',
    'bool Object::writeAuthoredPropertyProjection(Earthcall::StringId id,\n                                             const PropertyValue& value) {\n    std::cout << "writeAuthoredPropertyProjection: " << Earthcall::StringInterner::resolve(id) << "\\n";\n'
).replace(
    '    if (!mine) return false;',
    '    if (!mine) { std::cout << "no material!\\n"; return false; }'
).replace(
    '    if (face < 0 || face >= faces) return false;',
    '    if (face < 0 || face >= faces) { std::cout << "bad face!\\n"; return false; }'
).replace(
    '    if (colors.size() != selected.size()) return false;',
    '    if (colors.size() != selected.size()) { std::cout << "size mismatch! colors=" << colors.size() << " selected=" << selected.size() << "\\n"; return false; }'
)

# wait I don't want to replace twice. Let me just insert prints.

content = content.replace(
    '    } else if (const auto* list = std::get_if<std::shared_ptr<PropertyList>>(&value);',
    '    } else if (const auto* list = std::get_if<std::shared_ptr<PropertyList>>(&value);\n               list && *list) {\n        std::cout << "is PropertyList, size=" << (*list)->elements.size() << "\\n";\n'
).replace(
    '        for (const PropertyValue& item : (*list)->elements) {',
    '        for (const PropertyValue& item : (*list)->elements) {\n            const auto* color = std::get_if<glm::vec3>(&item);\n            if (!color) { std::cout << "list item not vec3! index=" << item.index() << "\\n"; return false; }\n'
)

with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "w") as f:
    f.write(content)
