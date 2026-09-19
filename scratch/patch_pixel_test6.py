import re

with open("tests/law/basic_pixel_changer_test.cpp", "r") as f:
    content = f.read()

new_check = """
    Property* pixel = canvas->findProperty("surface.pixel.0.3.4");
    if (!pixel) std::cerr << "PIXEL IS NULL!!!" << std::endl;
    else std::cerr << "PIXEL IS NOT NULL!!!" << std::endl;
    bool writeResult = pixel && pixel->setValue(PropertyValue(glm::vec3(0.9f, 0.1f, 0.2f)));
    std::cerr << "writeResult=" << writeResult << std::endl;
    glm::vec3 actTexel = texel(texture, 3, 4);
    std::cerr << "texel=(" << actTexel.r << ", " << actTexel.g << ", " << actTexel.b << ")" << std::endl;
    check(writeResult && near(actTexel, glm::vec3(0.9f, 0.1f, 0.2f)),
          "Property write reaches the same texture sample");
"""

content = re.sub(
    r'    Property\* pixel = canvas->findProperty\("surface\.pixel\.0\.3\.4"\);\n\s*check\(pixel && pixel->setValue\(PropertyValue\(glm::vec3\(0\.9f, 0\.1f, 0\.2f\)\)\) &&\n\s*near\(texel\(texture, 3, 4\), glm::vec3\(0\.9f, 0\.1f, 0\.2f\)\),\n\s*"Property write reaches the same texture sample"\);',
    new_check,
    content,
    flags=re.DOTALL
)

with open("tests/law/basic_pixel_changer_test.cpp", "w") as f:
    f.write(content)
