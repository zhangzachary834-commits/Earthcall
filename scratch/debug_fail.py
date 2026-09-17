import re
with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "r") as f:
    content = f.read()

content = content.replace(
    '    bool successA = obj->elevateSurfaceRegionProperty("regionA", 0, selectorA, reason);\n    assert(successA);',
    '    bool successA = obj->elevateSurfaceRegionProperty("regionA", 0, selectorA, reason);\n    if (!successA) std::cout << "FAIL REASON A: " << reason << "\\n";\n    assert(successA);'
)

with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "w") as f:
    f.write(content)
