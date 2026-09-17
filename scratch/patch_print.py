import re
with open("src/ZonesOfEarth/AuthorsOfLaw/ConditionModel.cpp", "r") as f:
    content = f.read()

content = content.replace(
    'static void resolveProjectionToken(Singular& target, const PropertyValue& in, PropertyValue& out) {',
    'static void resolveProjectionToken(Singular& target, const PropertyValue& in, PropertyValue& out) {\n    std::cout << "resolveProjectionToken called\\n";'
)
content = content.replace(
    'std::string targetName = std::get<std::string>(itTarget->second);',
    'std::string targetName = std::get<std::string>(itTarget->second);\n    std::cout << "resolveProjectionToken: targetName=" << targetName << "\\n";'
)
with open("src/ZonesOfEarth/AuthorsOfLaw/ConditionModel.cpp", "w") as f:
    f.write(content)
