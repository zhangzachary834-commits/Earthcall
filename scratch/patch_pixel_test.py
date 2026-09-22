import re

with open("tests/law/basic_pixel_changer_test.cpp", "r") as f:
    content = f.read()

content = content.replace("[]() {", "[&]() {")

with open("tests/law/basic_pixel_changer_test.cpp", "w") as f:
    f.write(content)
