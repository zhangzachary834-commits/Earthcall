import re
with open("src/ConstructedBeing/Singular/Object/Object/FaceTexture.cpp", "r") as f:
    content = f.read()

content = content.replace(
    'return writeSamples(coordinates, colors);\n}',
    'bool ok = writeSamples(coordinates, colors);\n    if (ok) revision++;\n    return ok;\n}'
)

content = content.replace(
    'currentRenderer().uploadTextureRegion(id, pixels.data(), width, height, minX, minY, regionW, regionH);\n    }\n    return true;\n}',
    'currentRenderer().uploadTextureRegion(id, pixels.data(), width, height, minX, minY, regionW, regionH);\n    }\n    revision++;\n    return true;\n}'
)

with open("src/ConstructedBeing/Singular/Object/Object/FaceTexture.cpp", "w") as f:
    f.write(content)
