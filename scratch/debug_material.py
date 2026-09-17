import re
with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "r") as f:
    content = f.read()

content = content.replace(
    '        ok = ft.writePixelWithRadius(uv, color, brushRadius);\n    }\n    if (ok) {\n        std::cout << "writeSurfacePixel wrote to ft! Checking ft.pixels at 0,0: " << (int)ft.pixels[0] << "," << (int)ft.pixels[1] << "," << (int)ft.pixels[2] << "\\n";\n    }',
    '        ok = ft.writePixelWithRadius(uv, color, brushRadius);\n    }\n    if (ok) {\n        std::cout << "writeSurfacePixel mat: " << _materialId << " ft ptr: " << &ft << " ft.pixels[0]: " << (int)ft.pixels[0] << "\\n";\n    }'
)

content = content.replace(
    '        list->elements.emplace_back(readTexel(ft, xy.x, xy.y));',
    '        std::cout << "readAuthoredPropertyProjection mat: " << _materialId << " ft ptr: " << &ft << " readTexel(" << xy.x << "," << xy.y << ") = " << (int)ft.pixels[(xy.y * ft.width + xy.x)*4] << "\\n";\n        list->elements.emplace_back(readTexel(ft, xy.x, xy.y));'
)

with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "w") as f:
    f.write(content)
