import re
with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "r") as f:
    content = f.read()

content = content.replace(
    '        if (!_rete.markFactDirty(owner->getIdentifier(), name)) {',
    '        bool dirtied = _rete.markFactDirty(owner->getIdentifier(), name);\n        if (name == "regionB") std::cout << "regionB dirtied? " << dirtied << "\\n";\n        if (!dirtied) { '
)

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "w") as f:
    f.write(content)
