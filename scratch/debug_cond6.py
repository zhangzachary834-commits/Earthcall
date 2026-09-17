import re
with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "r") as f:
    content = f.read()

content = content.replace(
    '    if (_prophetic.anyConditionReads(propertyName)) return true;\n    ++_propheticCounters.filtered;\n    return false;',
    '    bool reads = _prophetic.anyConditionReads(propertyName);\n    if (propertyName == "regionB") { std::cout << "propheticHears regionB: " << reads << " complete: " << _prophetic.complete() << "\\n"; }\n    if (reads) return true;\n    ++_propheticCounters.filtered;\n    return false;'
)

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "w") as f:
    f.write(content)
