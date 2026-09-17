import re
with open("src/ZonesOfEarth/AuthorsOfLaw/ConditionModel.cpp", "r") as f:
    content = f.read()

content = content.replace(
    'target.readAuthoredPropertyProjectionColors(Earthcall::StringInterner::intern(targetName), out);\n    std::cout << "resolveProjectionToken called for " << targetName << "\\n";',
    'target.readAuthoredPropertyProjectionColors(Earthcall::StringInterner::intern(targetName), out);'
)

content = content.replace(
    'case Op::Eq: res = (numeric ? a == b : propertyValueUnchanged(lhs, rhs)); std::cout << "Op::Eq evaluated to " << res << "\\n"; break;',
    'case Op::Eq: res = (numeric ? a == b : propertyValueUnchanged(lhs, rhs)); break;'
)

with open("src/ZonesOfEarth/AuthorsOfLaw/ConditionModel.cpp", "w") as f:
    f.write(content)

with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "r") as f:
    content2 = f.read()

content2 = content2.replace(
    'assert(PropertyPath::parse("witness_passed").getValue(*obj, val) == PropertyPath::PathResult::Ok);\n    std::cout << "witness_passed value: " << std::get<bool>(val) << "\\n";',
    'assert(PropertyPath::parse("witness_passed").getValue(*obj, val) == PropertyPath::PathResult::Ok);'
)
content2 = content2.replace(
    'std::cout << "ret2=" << ret2 << "\\n";',
    ''
)

with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "w") as f:
    f.write(content2)
