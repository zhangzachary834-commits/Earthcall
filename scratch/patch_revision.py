import re
with open("src/ConstructedBeing/Singular/Object/Object/FaceTexture.hpp", "r") as f:
    content = f.read()
if "uint64_t revision = 0;" not in content:
    content = content.replace(
        'mutable TextureHandle id = 0;',
        'mutable TextureHandle id = 0;\n    mutable uint64_t revision = 0;'
    )
with open("src/ConstructedBeing/Singular/Object/Object/FaceTexture.hpp", "w") as f:
    f.write(content)

with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "r") as f:
    content2 = f.read()

# Add to writePixel
content2 = content2.replace(
    'return true;\n}',
    'revision++;\n    return true;\n}'
)
with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "w") as f:
    f.write(content2)

