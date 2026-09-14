with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "r") as f:
    content = f.read()

# 1. In elevateSurfaceRegionProperty
elevate_target = "    const auto selected = selectedTexels(ft, selector, *this);"
elevate_replace = """    const auto selected = selectedTexels(ft, selector, *this);
    _regionCache[propertyName] = selected;"""
content = content.replace(elevate_target, elevate_replace, 1)

# 2. In readAuthoredPropertyProjection
read_target = "    const auto selected = selectedTexels(ft, selector, *this);"
read_replace = """    std::vector<glm::ivec2> selected;
    auto it = _regionCache.find(name);
    if (it != _regionCache.end()) {
        selected = it->second;
    } else {
        selected = selectedTexels(ft, selector, *this);
        _regionCache[name] = selected;
    }"""
content = content.replace(read_target, read_replace, 1)

# 3. In writeAuthoredPropertyProjection
write_target = """    } else {
        selected = selectedTexels(ft, selector, *this);
    }"""
write_replace = """    } else {
        auto it = _regionCache.find(name);
        if (it != _regionCache.end()) {
            selected = it->second;
        } else {
            selected = selectedTexels(ft, selector, *this);
            _regionCache[name] = selected;
        }
    }"""
content = content.replace(write_target, write_replace, 1)

with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "w") as f:
    f.write(content)

