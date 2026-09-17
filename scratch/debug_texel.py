import re
with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "r") as f:
    content = f.read()

content = content.replace(
    '        ok = ft.writePixelWithRadius(uv, color, brushRadius);\n    }',
    '        ok = ft.writePixelWithRadius(uv, color, brushRadius);\n    }\n    if (ok) {\n        std::cout << "writeSurfacePixel wrote to ft! Checking ft.pixels at 0,0: " << (int)ft.pixels[0] << "," << (int)ft.pixels[1] << "," << (int)ft.pixels[2] << "\\n";\n    }'
)

with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "w") as f:
    f.write(content)
