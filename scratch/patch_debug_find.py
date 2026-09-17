import re
with open("src/ConstructedBeing/Singular/Singular.cpp", "r") as f:
    content = f.read()
content = content.replace(
    'auto it = _dynamicProperties.find(id);',
    'if (Earthcall::StringInterner::resolve(id) == "regionB") { std::cout << "findProperty regionB! in dynamicProperties? " << (_dynamicProperties.find(id) != _dynamicProperties.end()) << "\\n"; }\n    auto it = _dynamicProperties.find(id);'
)
with open("src/ConstructedBeing/Singular/Singular.cpp", "w") as f:
    f.write(content)
