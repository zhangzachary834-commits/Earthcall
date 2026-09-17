import re

with open("src/ConstructedBeing/Singular/Object/Object/FaceTexture.cpp", "r") as f:
    content = f.read()

content = content.replace(
    'bool FaceTexture::writeSamples(const std::vector<glm::ivec2>& coordinates,',
    '#include <iostream>\nbool FaceTexture::writeSamples(const std::vector<glm::ivec2>& coordinates,'
)

content = content.replace(
    'return true;\n}',
    'std::cout << "writeSamples success: coords=" << coordinates.size() << "\\n";\n    for (auto c : coordinates) std::cout << "  " << c.x << "," << c.y << "\\n";\n    return true;\n}'
)

with open("src/ConstructedBeing/Singular/Object/Object/FaceTexture.cpp", "w") as f:
    f.write(content)
