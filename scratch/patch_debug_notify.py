import re
with open("src/ConstructedBeing/Singular/Singular.cpp", "r") as f:
    content = f.read()
content = content.replace(
    'if (s_propertyChangeCallback) {',
    'std::cout << "notifyPropertyChanged: " << name << "\\n";\n    if (s_propertyChangeCallback) {'
)
with open("src/ConstructedBeing/Singular/Singular.cpp", "w") as f:
    f.write(content)
