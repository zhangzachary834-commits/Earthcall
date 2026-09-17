import re
with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "r") as f:
    content = f.read()
content = content.replace(
    'for (Singular* being : Universe::instance().beings()) {',
    'auto all_beings = Universe::instance().beings();\n        std::cout << "Universe beings count: " << all_beings.size() << "\\n";\n        for (Singular* being : all_beings) {\n            std::cout << "Yielding being: " << being->getIdentifier() << "\\n";'
)
with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "w") as f:
    f.write(content)
