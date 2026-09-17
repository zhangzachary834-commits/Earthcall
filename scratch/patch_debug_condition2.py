import re
with open("src/ZonesOfEarth/AuthorsOfLaw/ConditionModel.cpp", "r") as f:
    content = f.read()

content = content.replace(
    'case Op::Eq: res = (numeric ? a == b : propertyValueUnchanged(lhs, rhs)); if (path.segmen',
    'case Op::Eq: res = (numeric ? a == b : propertyValueUnchanged(lhs, rhs)); if (lhsPath.segmen'
)
with open("src/ZonesOfEarth/AuthorsOfLaw/ConditionModel.cpp", "w") as f:
    f.write(content)
