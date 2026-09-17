import re
with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "r") as f:
    content = f.read()
content = content.replace(
    'if (!_rete.markFactDirty(owner->getIdentifier(), name)) {',
    'if (!_rete.markFactDirty(owner->getIdentifier(), name)) {\n    std::cout << "markFactDirty failed for " << name << "\\n";'
).replace(
    'if (Property* prop = owner->findProperty(name)) {',
    'if (Property* prop = owner->findProperty(name)) {\n    std::cout << "findProperty succeeded for " << name << "\\n";'
).replace(
    '_rete.addFact(stateFact);',
    '_rete.addFact(stateFact);\n    std::cout << "added stateFact for " << name << "\\n";'
)
with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "w") as f:
    f.write(content)
