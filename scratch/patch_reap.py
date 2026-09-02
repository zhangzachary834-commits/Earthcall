with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "r") as f:
    content = f.read()

content = content.replace(
    "std::vector<Singular*> victims = Universe::instance().takeUnmakings();",
    "std::vector<Singular*> victims = Universe::instance().takeUnmakings();\n    Universe::instance().bumpStructuralRevision();"
)

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "w") as f:
    f.write(content)

