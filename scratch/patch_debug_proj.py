import re

with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "r") as f:
    or_cpp = f.read()

or_cpp = or_cpp.replace(
    '    if (colors.size() != selected.size()) return false;',
    '    if (colors.size() != selected.size()) { std::cout << "writeAuthoredPropertyProjection: size mismatch. colors=" << colors.size() << " selected=" << selected.size() << "\\n"; return false; }'
)
or_cpp = or_cpp.replace(
    '    if (!mine) return false;',
    '    if (!mine) { std::cout << "writeAuthoredPropertyProjection: no material!\\n"; return false; }'
)
or_cpp = or_cpp.replace(
    '    if (face < 0 || face >= faces) return false;',
    '    if (face < 0 || face >= faces) { std::cout << "writeAuthoredPropertyProjection: bad face!\\n"; return false; }'
)

with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "w") as f:
    f.write(or_cpp)
