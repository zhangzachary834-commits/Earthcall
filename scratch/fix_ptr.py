import re
with open("src/ConstructedBeing/Singular/Singular.cpp", "r") as f:
    content = f.read()

content = content.replace(
    'PropertyValue* Singular::getDynamicPropertyPtr(Earthcall::StringId id) {\n    auto it = _dynamicProperties.find(id);\n    return (it != _dynamicProperties.end()) ? &it->second : nullptr;\n}\n\nconst PropertyValue* Singular::getDynamicPropertyPtr(Earthcall::StringId id) const {\n    auto it = _dynamicProperties.find(id);\n    return (it != _dynamicProperties.end()) ? &it->second : nullptr;\n}',
    '''PropertyValue* Singular::getDynamicPropertyPtr(Earthcall::StringId id) {
    if (recognizesAuthoredPropertyProjection(id)) {
        PropertyValue proj;
        if (readAuthoredPropertyProjection(id, proj)) {
            _dynamicProperties[id] = std::move(proj);
        }
    }
    auto it = _dynamicProperties.find(id);
    return (it != _dynamicProperties.end()) ? &it->second : nullptr;
}

const PropertyValue* Singular::getDynamicPropertyPtr(Earthcall::StringId id) const {
    // For const, we can't update the cache safely without making it mutable.
    // However, readAuthoredPropertyProjection modifies _regionCache which IS mutable.
    // We can cast away constness JUST for the cache update since it's logically const.
    if (recognizesAuthoredPropertyProjection(id)) {
        PropertyValue proj;
        if (readAuthoredPropertyProjection(id, proj)) {
            const_cast<Singular*>(this)->_dynamicProperties[id] = std::move(proj);
        }
    }
    auto it = _dynamicProperties.find(id);
    return (it != _dynamicProperties.end()) ? &it->second : nullptr;
}'''
)

with open("src/ConstructedBeing/Singular/Singular.cpp", "w") as f:
    f.write(content)
