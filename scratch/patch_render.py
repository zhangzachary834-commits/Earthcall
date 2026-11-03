with open('src/ConstructedBeing/Singular/Object/ObjectRender.cpp', 'r') as f:
    lines = f.readlines()

new_lines = []
for line in lines:
    if "bool Object::writeAuthoredPropertyProjection(Earthcall::StringId id," in line:
        new_lines.append("#include <iostream>\n")
    if "if (!single && !selectionDefinition(*this, name, face, selector)) return false;" in line:
        new_lines.append("    if (!single && !selectionDefinition(*this, name, face, selector)) { std::cout << \"writeAuthoredPropertyProjection: selectionDefinition failed\\n\"; return false; }\n")
        continue
    if "auto mine = ownMaterial();" in line:
        new_lines.append("    auto mine = ownMaterial();\n")
        new_lines.append("    if (!mine) { std::cout << \"writeAuthoredPropertyProjection: ownMaterial failed\\n\"; return false; }\n")
        continue
    if "if (!mine) return false;" in line:
        continue
    if "if (face < 0 || face >= faces) return false;" in line:
        new_lines.append("    if (face < 0 || face >= faces) { std::cout << \"writeAuthoredPropertyProjection: face out of bounds\\n\"; return false; }\n")
        continue
    if "if (pixel.x >= ft.width || pixel.y >= ft.height) return false;" in line:
        new_lines.append("        if (pixel.x >= ft.width || pixel.y >= ft.height) { std::cout << \"writeAuthoredPropertyProjection: pixel out of bounds\\n\"; return false; }\n")
        continue
    if "return false;" in line and "if (colors.size() != selected.size())" in line:
        new_lines.append("    if (colors.size() != selected.size()) { std::cout << \"writeAuthoredPropertyProjection: sizes mismatch\\n\"; return false; }\n")
        continue
    if "return false;" in line and "} else {" in line:
        new_lines.append("        std::cout << \"writeAuthoredPropertyProjection: parse type failed\\n\";\n")
        new_lines.append("        return false;\n")
        continue
    new_lines.append(line)

with open('src/ConstructedBeing/Singular/Object/ObjectRender.cpp', 'w') as out:
    out.writelines(new_lines)

