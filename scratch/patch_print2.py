import re
with open("src/ZonesOfEarth/AuthorsOfLaw/ConditionModel.cpp", "r") as f:
    content = f.read()

content = content.replace(
    'double a = 0.0, b = 0.0;',
    'resolveProjectionToken(t, rhs, rhs);\n                double a = 0.0, b = 0.0;'
)

content = content.replace(
    'target.readAuthoredPropertyProjectionColors(Earthcall::StringInterner::intern(targetName), out);',
    'target.readAuthoredPropertyProjectionColors(Earthcall::StringInterner::intern(targetName), out);\n    std::cout << "resolveProjectionToken called for " << targetName << "\\n";'
)

with open("src/ZonesOfEarth/AuthorsOfLaw/ConditionModel.cpp", "w") as f:
    f.write(content)
