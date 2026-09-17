import re
with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "r") as f:
    content = f.read()

content = content.replace(
    '    for (auto* prop : being->listProperties()) {',
    '    for (auto* prop : being->listProperties()) {\n        if (subjectId == "witness_img") { std::cout << "Seeding property: " << prop->name() << "\\n"; }'
)

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "w") as f:
    f.write(content)
