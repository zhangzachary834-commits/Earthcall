import re
with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "r") as f:
    content = f.read()

content = content.replace(
    '            const std::string subjectId = owner->getIdentifier();\n            if (_seededSubjects.count(subjectId)) {',
    '            const std::string subjectId = owner->getIdentifier();\n            if (name == "regionB") std::cout << "regionB seeded? " << _seededSubjects.count(subjectId) << "\\n";\n            if (_seededSubjects.count(subjectId)) {'
)

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "w") as f:
    f.write(content)
