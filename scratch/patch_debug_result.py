import re

with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "r") as f:
    content = f.read()

content = content.replace(
    '    assert(result == PropertyPath::PathResult::Ok);',
    '    std::cout << "setValue result: " << (int)result << "\\n";\n    assert(result == PropertyPath::PathResult::Ok);'
)

with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "w") as f:
    f.write(content)
