import re

with open("tests/law/basic_pixel_changer_test.cpp", "r") as f:
    content = f.read()

new_check = """
    check(lawSetValue(*canvas, pixelPath, PropertyValue(glm::vec3(0.9f, 0.1f, 0.2f))) == PropertyPath::PathResult::Ok &&
              near(texel(texture, 3, 4), glm::vec3(0.9f, 0.1f, 0.2f)),
          "Property write reaches the same texture sample");
"""

content = re.sub(
    r'    check\(pixel && pixel->setValue\(PropertyValue\(glm::vec3\(0\.9f, 0\.1f, 0\.2f\)\)\) &&\n\s*near\(texel\(texture, 3, 4\), glm::vec3\(0\.9f, 0\.1f, 0\.2f\)\),\n\s*"Property write reaches the same texture sample"\);',
    new_check,
    content,
    flags=re.DOTALL
)

with open("tests/law/basic_pixel_changer_test.cpp", "w") as f:
    f.write(content)
