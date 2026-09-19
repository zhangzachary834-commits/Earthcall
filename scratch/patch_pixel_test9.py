import re

with open("tests/law/basic_pixel_changer_test.cpp", "r") as f:
    content = f.read()

new_fill = """
    // Click FILL INK at (78, 161)
    std::cerr << "TEXTURE PIXELS ADDR: " << (void*)texture.pixels.data() << std::endl;
    frame(78.0f, 161.0f, false);
    frame(78.0f, 161.0f, true);
    frame(78.0f, 161.0f, false);
    std::cout << "DEBUG fill color expected: (" << std::get<glm::vec3>(selectedColorProperty->value()).r << ", "
"""

content = re.sub(
    r'\s*// Click FILL INK at \(78, 161\)\n\s*frame\(78\.0f, 161\.0f, false\);\n\s*frame\(78\.0f, 161\.0f, true\);\n\s*frame\(78\.0f, 161\.0f, false\);\n\s*std::cout << "DEBUG fill color expected: \(" << std::get<glm::vec3>\(selectedColorProperty->value\(\)\)\.r << ", "',
    new_fill,
    content,
    flags=re.DOTALL
)

with open("tests/law/basic_pixel_changer_test.cpp", "w") as f:
    f.write(content)

with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "r") as f:
    content = f.read()

new_wapp = """    if (colors.size() != selected.size()) return false;
    std::cerr << "WAPP writes to ft at " << (void*)ft.pixels.data() << " size=" << selected.size() << std::endl;
    return ft.writeSamples(selected, colors);
}"""

content = re.sub(
    r'    if \(colors\.size\(\) != selected\.size\(\)\) return false;\n\s*return ft\.writeSamples\(selected, colors\);\n\}',
    new_wapp,
    content,
    flags=re.DOTALL
)

with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "w") as f:
    f.write(content)
