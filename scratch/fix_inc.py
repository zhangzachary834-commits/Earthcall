with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "r") as f:
    code = f.read()

code = code.replace('#include <nlohmann/json.hpp>\\n', '')

with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "w") as f:
    f.write(code)
