import re
with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "r") as f:
    content = f.read()
content = content.replace(
    'bool LawManager::propheticHears(const std::string& propertyName) const {',
    'bool LawManager::propheticHears(const std::string& propertyName) const {\n    if (propertyName == "regionB") std::cout << "propheticHears regionB asked!\\n";'
).replace(
    'if (_prophetic.anyConditionReads(propertyName)) return true;',
    'if (_prophetic.anyConditionReads(propertyName)) { if (propertyName == "regionB") std::cout << "regionB returns true!\\n"; return true; }\n    if (propertyName == "regionB") std::cout << "regionB returns false!\\n";'
)
with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "w") as f:
    f.write(content)
