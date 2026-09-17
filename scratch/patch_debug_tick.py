import re
with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "r") as f:
    content = f.read()

content = content.replace(
    'if (hasTerminals && (law->activation() == Law::Activation::WhileTrue',
    'std::cout << "lawId=" << lawId << " hasTerminals=" << hasTerminals << " activation=" << (int)law->activation() << "\\n";\n        if (hasTerminals && (law->activation() == Law::Activation::WhileTrue'
)
with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "w") as f:
    f.write(content)
