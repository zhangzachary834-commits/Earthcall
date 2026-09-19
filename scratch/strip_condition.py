import re

with open("src/ZonesOfEarth/AuthorsOfLaw/ConditionModel.cpp", "r") as f:
    content = f.read()

content = re.sub(r'static void resolveProjectionToken.*?\}\n\n', '', content, flags=re.DOTALL)
content = content.replace('                resolveProjectionToken(t, lhs, lhs);\n', '')

with open("src/ZonesOfEarth/AuthorsOfLaw/ConditionModel.cpp", "w") as f:
    f.write(content)
