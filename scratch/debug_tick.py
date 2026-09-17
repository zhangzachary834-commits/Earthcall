import re
with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "r") as f:
    content = f.read()

content = content.replace(
    '    harness.lawManager.tick();',
    '    std::cout << "DEBUG: beings count: " << Universe::instance().beings().size() << "\\n";\n    harness.lawManager.tick();'
)

with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "w") as f:
    f.write(content)
