import re
with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "r") as f:
    content = f.read()

content = content.replace(
    'assert(PropertyPath::parse("witness_passed").getValue(*obj, val) == PropertyPath::PathResult::Ok);',
    'assert(PropertyPath::parse("witness_passed").getValue(*obj, val) == PropertyPath::PathResult::Ok);\n    std::cout << "witness_passed value: " << std::get<bool>(val) << "\\n";'
)

with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "w") as f:
    f.write(content)
