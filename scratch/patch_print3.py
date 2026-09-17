import re
with open("src/ZonesOfEarth/AuthorsOfLaw/ConditionModel.cpp", "r") as f:
    content = f.read()

content = content.replace(
    'case Op::Eq: res = (numeric ? a == b : propertyValueUnchanged(lhs, rhs)); break;',
    'case Op::Eq: res = (numeric ? a == b : propertyValueUnchanged(lhs, rhs)); std::cout << "Op::Eq evaluated to " << res << "\\n"; break;'
)

with open("src/ZonesOfEarth/AuthorsOfLaw/ConditionModel.cpp", "w") as f:
    f.write(content)
