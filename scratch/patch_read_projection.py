with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "r") as f:
    content = f.read()

import re

target = re.compile(r"auto list = std::make_shared<PropertyList>\(\);\s*list->elements\.reserve\(selected\.size\(\)\);\s*for \(const glm::ivec2& xy : selected\) \{\s*list->elements\.emplace_back\(readTexel\(ft, xy\.x, xy\.y\)\);\s*\}\s*out = PropertyValue\(std::move\(list\)\);")

replace = """auto dict = std::make_shared<PropertyDict>();
    dict->elements["type"] = PropertyValue(std::string("Region"));
    dict->elements["face"] = PropertyValue(face);
    dict->elements["selector"] = PropertyValue(selector.toJson().dump());
    out = PropertyValue(std::move(dict));"""

if target.search(content):
    content = target.sub(replace, content)
else:
    print("Could not find target in ObjectRender.cpp")

with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "w") as f:
    f.write(content)
