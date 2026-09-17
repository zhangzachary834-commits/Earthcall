import re
with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "r") as f:
    content = f.read()

content = content.replace(
    'obj->writeSurfacePixel(0, glm::vec2(0.25f, 0.25f), green); // Top-left',
    'bool w1 = obj->writeSurfacePixel(0, glm::vec2(0.25f, 0.25f), green); std::cout << "w1: " << w1 << "\\n";'
)

with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "w") as f:
    f.write(content)
