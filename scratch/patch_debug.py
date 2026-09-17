import re

with open("src/ConstructedBeing/Singular/Object/Object/FaceTexture.cpp", "r") as f:
    ft = f.read()

ft = ft.replace(
    "bool FaceTexture::writeSamples(const std::vector<glm::ivec2>& coordinates,",
    "#include <iostream>\nbool FaceTexture::writeSamples(const std::vector<glm::ivec2>& coordinates,"
)
ft = ft.replace(
    "    if (width <= 0 || height <= 0 || coordinates.size() != colors.size()) return false;",
    '    if (width <= 0 || height <= 0 || coordinates.size() != colors.size()) { std::cout << "writeSamples: early out w/h/size\\n"; return false; }'
)
ft = ft.replace(
    "    if (minX > maxX || minY > maxY) return false;",
    '    if (minX > maxX || minY > maxY) { std::cout << "writeSamples: min>max. minX=" << minX << " maxX=" << maxX << "\\n"; return false; }'
)
with open("src/ConstructedBeing/Singular/Object/Object/FaceTexture.cpp", "w") as f:
    f.write(ft)

with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "r") as f:
    or_cpp = f.read()

or_cpp = or_cpp.replace(
    "    if (colors.size() != selected.size()) return false;\n    return ft.writeSamples(selected, colors);",
    '    if (colors.size() != selected.size()) return false;\n    bool res = ft.writeSamples(selected, colors);\n    if (!res) std::cout << "writeAuthoredPropertyProjection: ft.writeSamples failed\\n";\n    return res;'
)
with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "w") as f:
    f.write(or_cpp)
