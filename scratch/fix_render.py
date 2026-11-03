with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "r") as f:
    code = f.read()

replacement = """bool Object::readAuthoredPropertyProjection(Earthcall::StringId id,
                                            PropertyValue& out) const {
    const std::string name = Earthcall::StringInterner::resolve(id);
    PixelAddress pixel;
    int face = -1;
    OntoMath::Piecewise selector;
    const bool single = parsePixelAddress(name, pixel);
    if (single) face = pixel.face;
    else if (!selectionDefinition(*this, name, face, selector)) {
        if (name == "regionA") std::cout << "regionA failed at selectionDefinition\\n";
        return false;
    }
    auto mat = materials.resolveOrDefault(_materialId);
    if (!mat) {
        if (name == "regionA") std::cout << "regionA failed at mat\\n";
        return false;
    }
    if (face < 0 || face >= static_cast<int>(mat->faceTextures.size())) {
        if (name == "regionA") std::cout << "regionA failed at face bounds\\n";
        return false;
    }
    const FaceTexture& ft = mat->faceTextures[static_cast<std::size_t>(face)];
    const std::size_t expected = static_cast<std::size_t>(ft.width) * ft.height * 4;
    if (ft.width <= 0 || ft.height <= 0 || ft.pixels.size() != expected) {
        if (name == "regionA") std::cout << "regionA failed at ft check\\n";
        return false;
    }
    if (single) {
        if (pixel.x >= ft.width || pixel.y >= ft.height) return false;
        out = PropertyValue(readTexel(ft, pixel.x, pixel.y));
        return true;
    }
    std::vector<glm::ivec2> selected;
    auto it = _regionCache.find(name);
    if (it != _regionCache.end()) {
        selected = it->second;
    } else {
        selected = selectedTexels(ft, selector, *this);
        _regionCache[name] = selected;
    }
    auto list = std::make_shared<PropertyList>();
    list->elements.reserve(selected.size());
    for (const glm::ivec2& xy : selected) {
        list->elements.emplace_back(readTexel(ft, xy.x, xy.y));
    }
    out = PropertyValue(std::move(list));
    if (name == "regionA") std::cout << "regionA SUCCESS\\n";
    return true;
}"""

import re
code = re.sub(r'bool Object::readAuthoredPropertyProjection.*?(?=\nbool Object::writeAuthoredPropertyProjection)', replacement, code, flags=re.DOTALL)

with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "w") as f:
    f.write(code)
