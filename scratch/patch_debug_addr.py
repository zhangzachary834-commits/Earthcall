import re
with open("src/ConstructedBeing/Singular/Object/Object/FaceTexture.cpp", "r") as f:
    content = f.read()
content = content.replace(
    'std::cout << "writeSamples success: coords=" << coordinates.size() << " useLayers=" << useLayers << " pixels[8]=" << (int)pixels[8] << "," << (int)pixels[9] << "," << (int)pixels[10] << "\\n";',
    'std::cout << "writeSamples this=" << this << " coords=" << coordinates.size() << " pixels[8]=" << (int)pixels[8] << "\\n";'
)
with open("src/ConstructedBeing/Singular/Object/Object/FaceTexture.cpp", "w") as f:
    f.write(content)

with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "r") as f:
    content = f.read()
content = content.replace(
    'glm::vec3 readTexel(const FaceTexture& ft, int x, int y) {',
    'glm::vec3 readTexel(const FaceTexture& ft, int x, int y) {\n    if (x==0 && y==1) std::cout << "readTexel ft=" << &ft << " pixels[8]=" << (int)ft.pixels[8] << "\\n";'
)
with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "w") as f:
    f.write(content)
