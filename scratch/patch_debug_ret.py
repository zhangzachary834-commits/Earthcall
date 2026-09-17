import re

with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "r") as f:
    content = f.read()

content = content.replace(
    '    obj->writeSurfacePixel(0, glm::vec2(0.25f, 0.75f), red); // Bottom-left',
    '    bool ret2 = obj->writeSurfacePixel(0, glm::vec2(0.25f, 0.75f), red); // Bottom-left\n    std::cout << "ret2=" << ret2 << "\\n";'
)

with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "w") as f:
    f.write(content)
