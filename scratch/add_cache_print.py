with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "r") as f:
    content = f.read()

content = content.replace("auto it = _regionCache.find(name);", "auto it = _regionCache.find(name);\n    if (it != _regionCache.end()) { std::printf(\"CACHE HIT %s\\n\", name.c_str()); } else { std::printf(\"CACHE MISS %s\\n\", name.c_str()); }")

with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "w") as f:
    f.write(content)
