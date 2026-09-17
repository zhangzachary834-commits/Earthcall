import re
with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "r") as f:
    content = f.read()

content = content.replace(
    '    if (colors.size() != selected.size()) { std::cout << "writeAuthoredPropertyProjection: size mismatch. colors=" << colors.size() << " selected=" << selected.size() << "\\n"; return false; }',
    '    if (colors.size() != selected.size()) { std::cout << "writeAuthoredPropertyProjection: size mismatch. colors=" << colors.size() << " selected=" << selected.size() << "\\n"; return false; }\n    for (auto xy : selected) { if (xy.x < 0 || xy.y < 0 || xy.x >= ft.width || xy.y >= ft.height) { std::cout << "BAD COORD: " << xy.x << "," << xy.y << " for " << ft.width << "x" << ft.height << "\\n"; } }'
)

with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "w") as f:
    f.write(content)
