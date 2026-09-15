with open("src/ConstructedBeing/Singular/Singular.cpp", "r") as f:
    content = f.read()

target = """bool Singular::setDynamicProperty(Earthcall::StringId id, const PropertyValue& v) {
    const bool projected = recognizesAuthoredPropertyProjection(id);
    if (projected) {
        return writeAuthoredPropertyProjection(id, v);
    }
    PropertyValue stored = v;"""

replace = """bool Singular::setDynamicProperty(Earthcall::StringId id, const PropertyValue& v) {
    const bool projected = recognizesAuthoredPropertyProjection(id);
    if (projected && !writeAuthoredPropertyProjection(id, v)) return false;
    PropertyValue stored = v;"""

content = content.replace(target, replace)

with open("src/ConstructedBeing/Singular/Singular.cpp", "w") as f:
    f.write(content)
