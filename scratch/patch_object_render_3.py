import re

with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "r") as f:
    content = f.read()

# elevateSurfaceRegionProperty
content = re.sub(
    r'(bool Object::elevateSurfaceRegionProperty[\s\S]*?)(const auto selected = selectedTexels\(ft, selector, \*this\);)',
    r'\1\2\n    _regionCache[propertyName] = selected;',
    content,
    count=1
)

# readAuthoredPropertyProjection
read_replace = r"""    std::vector<glm::ivec2> selected;
    auto it = _regionCache.find(name);
    if (it != _regionCache.end()) {
        selected = it->second;
    } else {
        selected = selectedTexels(ft, selector, *this);
        _regionCache[name] = selected;
    }"""
content = re.sub(
    r'(bool Object::readAuthoredPropertyProjection[\s\S]*?)    const auto selected = selectedTexels\(ft, selector, \*this\);',
    r'\1' + read_replace,
    content,
    count=1
)

# writeAuthoredPropertyProjection
write_replace = r"""    } else {
        auto it = _regionCache.find(name);
        if (it != _regionCache.end()) {
            selected = it->second;
        } else {
            selected = selectedTexels(ft, selector, *this);
            _regionCache[name] = selected;
        }
    }"""
content = re.sub(
    r'(bool Object::writeAuthoredPropertyProjection[\s\S]*?)    } else {\n        selected = selectedTexels\(ft, selector, \*this\);\n    }',
    r'\1' + write_replace,
    content,
    count=1
)

with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "w") as f:
    f.write(content)
