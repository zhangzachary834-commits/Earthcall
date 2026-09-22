import re

with open("tests/law/basic_pixel_changer_test.cpp", "r") as f:
    content = f.read()

replacement3 = """region ? [&]() { PropertyValue val; canvas->readAuthoredPropertyProjectionColors(Earthcall::StringInterner::intern("authored.left-quarter"), val); return val; }() : PropertyValue{}"""
content = content.replace("region ? region->value() : PropertyValue{}", replacement3)

with open("tests/law/basic_pixel_changer_test.cpp", "w") as f:
    f.write(content)
