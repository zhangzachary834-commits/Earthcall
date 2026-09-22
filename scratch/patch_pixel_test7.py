import re

with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "r") as f:
    content = f.read()

new_write = """bool Object::writeAuthoredPropertyProjection(Earthcall::StringId id,
                                             const PropertyValue& value) {
    const std::string name = Earthcall::StringInterner::resolve(id);
    PixelAddress pixel;
    int face = -1;
    OntoMath::Piecewise selector;
    const bool single = parsePixelAddress(name, pixel);
    if (single) face = pixel.face;
    else if (!selectionDefinition(*this, name, face, selector)) {
        std::cerr << "WAPP FAIL: selectionDefinition failed for " << name << std::endl;
        return false;
    }
    auto mine = ownMaterial();
    if (!mine) {
        std::cerr << "WAPP FAIL: no ownMaterial" << std::endl;
        return false;
    }
    const int faces = getFaces() > 0 ? getFaces() : 1;
    if (face < 0 || face >= faces) {
        std::cerr << "WAPP FAIL: face out of bounds" << std::endl;
        return false;
    }
    if (static_cast<int>(mine->faceTextures.size()) != faces) mine->initFaceTextures(faces);
    FaceTexture& ft = mine->faceTextures[static_cast<std::size_t>(face)];
    std::vector<glm::ivec2> selected;
    if (single) {
        if (pixel.x >= ft.width || pixel.y >= ft.height) {
            std::cerr << "WAPP FAIL: pixel out of bounds" << std::endl;
            return false;
        }
        selected.emplace_back(pixel.x, pixel.y);
    } else {
        auto it = _regionCache.find(name);
        if (it != _regionCache.end()) {
            selected = it->second;
        } else {
            selected = selectedTexels(ft, selector, *this);
            _regionCache[name] = selected;
        }
    }

    std::vector<glm::vec3> colors;
    if (const auto* one = std::get_if<glm::vec3>(&value)) {
        colors.assign(selected.size(), *one);
    } else if (const auto* list = std::get_if<std::shared_ptr<PropertyList>>(&value);
               list && *list) {
        for (const PropertyValue& item : (*list)->elements) {
            const auto* color = std::get_if<glm::vec3>(&item);
            if (!color) {
                std::cerr << "WAPP FAIL: invalid list item" << std::endl;
                return false;
            }
            colors.push_back(*color);
        }
    } else {
        std::cerr << "WAPP FAIL: value is not vec3, index=" << value.index() << std::endl;
        return false;
    }
"""

content = re.sub(
    r'bool Object::writeAuthoredPropertyProjection\(Earthcall::StringId id,\n\s*const PropertyValue& value\) \{.*?\n\s*std::vector<glm::vec3> colors;\n.*?\n\s*\} else \{\n\s*return false;\n\s*\}',
    new_write,
    content,
    flags=re.DOTALL
)

with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "w") as f:
    f.write(content)
