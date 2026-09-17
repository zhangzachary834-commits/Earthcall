import re
with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "r") as f:
    content = f.read()
content = content.replace(
    'if (_seededSubjects.count(subjectId)) {',
    'std::cout << "Checking _seededSubjects for: " << subjectId << " count: " << _seededSubjects.count(subjectId) << "\\n";\n            if (_seededSubjects.count(subjectId)) {'
)
with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "w") as f:
    f.write(content)
