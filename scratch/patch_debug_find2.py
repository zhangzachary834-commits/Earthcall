import re
with open("src/ConstructedBeing/Singular/Singular.cpp", "r") as f:
    content = f.read()
content = content.replace(
    'if (Earthcall::StringInterner::resolve(id) == "regionB") { std::cout << "findProperty regionB! in dynamicProperties? " << (_dynamicProperties.find(id) != _dynamicProperties.end()) << "\\n"; }',
    'if (Earthcall::StringInterner::resolve(id) == "regionB") { printf("findProperty regionB! in dynamicProperties? %d\\n", (int)(_dynamicProperties.find(id) != _dynamicProperties.end())); }'
)
with open("src/ConstructedBeing/Singular/Singular.cpp", "w") as f:
    f.write(content)
