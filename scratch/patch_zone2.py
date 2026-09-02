import re
with open("src/ZonesOfEarth/Zone/Zone.cpp", "r") as f:
    content = f.read()

content = content.replace("Earthcall::Universe::", "Universe::")

if '#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"' not in content:
    content = '#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"\n' + content

with open("src/ZonesOfEarth/Zone/Zone.cpp", "w") as f:
    f.write(content)

