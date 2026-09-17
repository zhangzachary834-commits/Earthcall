import re
with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "r") as f:
    content = f.read()
content = content.replace(
    'harness.lawManager.tick();',
    'std::cout << "--- Ticking LawManager ---\\n";\nharness.lawManager.tick();\nstd::cout << "--- Ticked ---\\n";'
)
with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "w") as f:
    f.write(content)
