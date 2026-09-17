import re
with open("src/ConstructedBeing/Singular/Object/Object/FaceTexture.cpp", "r") as f:
    content = f.read()

content = content.replace(
    '        if (xy.x < 0 || xy.y < 0 || xy.x >= width || xy.y >= height) return false;',
    '        if (xy.x < 0 || xy.y < 0 || xy.x >= width || xy.y >= height) { std::cout << "writeSamples OUT OF BOUNDS: " << xy.x << "," << xy.y << " limit " << width << "," << height << "\\n"; return false; }'
)

with open("src/ConstructedBeing/Singular/Object/Object/FaceTexture.cpp", "w") as f:
    f.write(content)
