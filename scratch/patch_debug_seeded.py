import re
with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "r") as f:
    content = f.read()
content = content.replace(
    'if (_seededSubjects.count(subjectId)) {',
    'if (_seededSubjects.count(subjectId)) {\n    std::cout << "seededSubjects contains " << subjectId << "\\n";'
).replace(
    'if (Property* prop = owner->findProperty(name)) {',
    'std::cout << "calling findProperty for " << name << "\\n";\n                if (Property* prop = owner->findProperty(name)) {'
)
with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "w") as f:
    f.write(content)
