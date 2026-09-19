import re

with open("src/ZonesOfEarth/Physics/Physics.cpp", "r") as f:
    content = f.read()

content = content.replace("return; // STUBBED FOR PROFILING", "")

with open("src/ZonesOfEarth/Physics/Physics.cpp", "w") as f:
    f.write(content)
