import re
with open("src/ZonesOfEarth/AuthorsOfLaw/ConditionModel.cpp", "r") as f:
    content = f.read()

content = content.replace(
    '                Singular& t = const_cast<Singular&>(target);\n                if (lhsPath.segments.size() > 0 && lhsPath.segments[0] == "regionB") {',
    '                Singular& t = const_cast<Singular&>(target);\n                PropertyValue lhs;\n                PropertyValue rhs = rhsLiteral;\n                if (lhsPath.segments.size() > 0 && lhsPath.segments[0] == "regionB") {'
)

with open("src/ZonesOfEarth/AuthorsOfLaw/ConditionModel.cpp", "w") as f:
    f.write(content)
