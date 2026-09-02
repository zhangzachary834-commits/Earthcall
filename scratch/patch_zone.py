import re
with open("src/ZonesOfEarth/Zone/Zone.cpp", "r") as f:
    content = f.read()

# I want to add Universe::instance().bumpStructuralRevision() to addObject
content = re.sub(
    r'(void Zone::addObject\(std::shared_ptr<Object> obj\) \{)',
    r'\1\n    Earthcall::Universe::instance().bumpStructuralRevision();',
    content
)

with open("src/ZonesOfEarth/Zone/Zone.cpp", "w") as f:
    f.write(content)

