import re
with open("src/ZonesOfEarth/AuthorsOfLaw/ConditionModel.cpp", "r") as f:
    content = f.read()

content = content.replace(
    '                Singular& t = const_cast<Singular&>(target);\n                if (lhsPath.segments.size() == 1 && lhsPath.segments[0] == "regionB") { std::cout << "EVALUATING regionB!\\n"; }',
    '                Singular& t = const_cast<Singular&>(target);\n                std::cout << "EVALUATING Condition. lhsPath: " << lhsPath.asString() << "\\n";'
)

with open("src/ZonesOfEarth/AuthorsOfLaw/ConditionModel.cpp", "w") as f:
    f.write(content)
