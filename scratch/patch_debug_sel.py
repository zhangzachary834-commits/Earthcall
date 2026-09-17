import re
with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "r") as f:
    content = f.read()

content = content.replace(
    '#include <iostream>\n', ''
).replace(
    'glm::vec3 readTexel(const FaceTexture& ft, int x, int y) {\n    if (x==0 && y==1) std::cout << "readTexel ft=" << &ft << " pixels[8]=" << (int)ft.pixels[8] << "\\n";',
    '#include <iostream>\nglm::vec3 readTexel(const FaceTexture& ft, int x, int y) {'
).replace(
    '        _regionCache[name] = selected;\n    }',
    '        _regionCache[name] = selected;\n        if (name == "regionB") { std::cout << "regionB selected:\\n"; for (auto c : selected) std::cout << "  " << c.x << "," << c.y << "\\n"; }\n    }'
)

with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "w") as f:
    f.write(content)
