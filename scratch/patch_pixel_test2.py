import re

with open("tests/law/basic_pixel_changer_test.cpp", "r") as f:
    content = f.read()

replacement2 = """[]() { PropertyValue val; canvas->readAuthoredPropertyProjectionColors(Earthcall::StringInterner::intern("surface.left-quarter"), val); return val; }()"""
content = content.replace("leftSide->value()", replacement2)

with open("tests/law/basic_pixel_changer_test.cpp", "w") as f:
    f.write(content)
