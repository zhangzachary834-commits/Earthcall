import re
with open("tests/zones/prism_cathedral_test.cpp", "r") as f:
    c = f.read()

c = c.replace('check(valSt1 > 0.5, "Foundation 1 scalar radiance evaluates positive at Z=40");',
              'std::cout << "valSt1: " << valSt1 << std::endl; check(valSt1 > 0.5, "Foundation 1 scalar radiance evaluates positive at Z=40");')
c = c.replace('check(valNear > valFar * 1.5,',
              'std::cout << "valNear: " << valNear << " valFar: " << valFar << std::endl; check(valNear > valFar * 1.5,')

with open("tests/zones/prism_cathedral_test.cpp", "w") as f:
    f.write(c)
