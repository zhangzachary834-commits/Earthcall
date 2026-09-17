import re
with open("src/ConstructedBeing/Singular/Singular.cpp", "r") as f:
    content = f.read()
content = content.replace(
    'std::cout << "notifyPropertyChanged: " << name << "\\n";\n',
    'printf("notifyPropertyChanged: %s\\n", name.c_str());\n'
)
with open("src/ConstructedBeing/Singular/Singular.cpp", "w") as f:
    f.write(content)
