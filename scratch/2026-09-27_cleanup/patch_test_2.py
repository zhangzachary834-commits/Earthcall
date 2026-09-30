import re
with open("tests/zones/prism_cathedral_test.cpp", "r") as f:
    c = f.read()

c = c.replace('std::cout << "valSt1: " << valSt1 << std::endl; check(valSt1 > 0.5, "Foundation 1 scalar radiance evaluates positive at Z=40");',
              'std::cout << "AST JSON: " << root->field->astDefinition.toJson().dump(2) << std::endl; std::cout << "valSt1: " << valSt1 << std::endl; check(valSt1 > 0.5, "Foundation 1 scalar radiance evaluates positive at Z=40");')

with open("tests/zones/prism_cathedral_test.cpp", "w") as f:
    f.write(c)
