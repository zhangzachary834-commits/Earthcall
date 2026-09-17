with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "r") as f:
    or_cpp = f.read()
or_cpp = "#include <iostream>\n" + or_cpp
with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "w") as f:
    f.write(or_cpp)
