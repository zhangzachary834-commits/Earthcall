import re
with open("src/ConstructedBeing/Singular/Object/Object/FaceTexture.cpp", "r") as f:
    content = f.read()
content = content.replace(
    'std::cout << "writeSamples success: coords=" << coordinates.size() << "\\n";',
    'std::cout << "writeSamples success: coords=" << coordinates.size() << " useLayers=" << useLayers << "\\n";'
)
with open("src/ConstructedBeing/Singular/Object/Object/FaceTexture.cpp", "w") as f:
    f.write(content)
