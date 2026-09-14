import os

with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "r") as f:
    content = f.read()

def replace_selected_texels():
    global content
    
    # 1. In elevateSurfaceRegionProperty
    # It currently has: const auto selected = selectedTexels(ft, selector, *this);
    # Replace it with:
    # const auto selected = selectedTexels(ft, selector, *this);
    # _regionCache[propertyName] = selected;
    content = content.replace(
        "const auto selected = selectedTexels(ft, selector, *this);",
        "const auto selected = selectedTexels(ft, selector, *this);\n    _regionCache[propertyName] = selected;"
    )

    # 2. In readAuthoredPropertyProjection and writeAuthoredPropertyProjection
    # Replace: selected = selectedTexels(ft, selector, *this);
    # With cache lookup
    cache_lookup = """auto it = _regionCache.find(name);
        if (it != _regionCache.end()) {
            selected = it->second;
        } else {
            selected = selectedTexels(ft, selector, *this);
            _regionCache[name] = selected;
        }"""
    content = content.replace("selected = selectedTexels(ft, selector, *this);", cache_lookup)
    
    # Wait, readAuthoredPropertyProjection has:
    # const auto selected = selectedTexels(ft, selector, *this);
    cache_lookup_const = """std::vector<glm::ivec2> selected;
    auto it = _regionCache.find(name);
    if (it != _regionCache.end()) {
        selected = it->second;
    } else {
        selected = selectedTexels(ft, selector, *this);
        _regionCache[name] = selected;
    }"""
    content = content.replace("const auto selected = selectedTexels(ft, selector, *this);\n    _regionCache[propertyName] = selected;", "const auto selected = selectedTexels(ft, selector, *this);\n    _regionCache[propertyName] = selected; // MARK1")
    content = content.replace("const auto selected = selectedTexels(ft, selector, *this);", cache_lookup_const)
    content = content.replace("// MARK1", "")

replace_selected_texels()

with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "w") as f:
    f.write(content)

