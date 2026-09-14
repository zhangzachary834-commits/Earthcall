import re

with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "r") as f:
    content = f.read()

def replace_first(text, search, replacement):
    return text.replace(search, replacement, 1)

# In elevateSurfaceRegionProperty
# We want to add _regionCache[propertyName] = selected;
# Right after: const auto selected = selectedTexels(ft, selector, *this);
content = replace_first(content, 
    "const auto selected = selectedTexels(ft, selector, *this);", 
    "const auto selected = selectedTexels(ft, selector, *this);\n    _regionCache[propertyName] = selected;")

# In readAuthoredPropertyProjection
# We want to replace: const auto selected = selectedTexels(ft, selector, *this);
# with:
read_replace = """std::vector<glm::ivec2> selected;
    auto it = _regionCache.find(name);
    if (it != _regionCache.end()) {
        selected = it->second;
    } else {
        selected = selectedTexels(ft, selector, *this);
        _regionCache[name] = selected;
    }"""
content = replace_first(content, "const auto selected = selectedTexels(ft, selector, *this);", read_replace)

# In writeAuthoredPropertyProjection
# We want to replace: selected = selectedTexels(ft, selector, *this);
# with:
write_replace = """auto it = _regionCache.find(name);
        if (it != _regionCache.end()) {
            selected = it->second;
        } else {
            selected = selectedTexels(ft, selector, *this);
            _regionCache[name] = selected;
        }"""
content = replace_first(content, "selected = selectedTexels(ft, selector, *this);", write_replace)

with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "w") as f:
    f.write(content)
