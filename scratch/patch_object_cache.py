with open("src/ConstructedBeing/Singular/Object/Object.hpp", "r") as f:
    content = f.read()

if "void onDynamicPropertyChanged" not in content:
    content = content.replace(
        "    bool writeAuthoredPropertyProjection(Earthcall::StringId id, const PropertyValue& value) override;",
        "    bool writeAuthoredPropertyProjection(Earthcall::StringId id, const PropertyValue& value) override;\\n    void onDynamicPropertyChanged(Earthcall::StringId id) override;")

with open("src/ConstructedBeing/Singular/Object/Object.hpp", "w") as f:
    f.write(content)

with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "a") as f:
    f.write("""
void Object::onDynamicPropertyChanged(Earthcall::StringId id) {
    const std::string name = Earthcall::StringInterner::resolve(id);
    if (name.rfind("selection:", 0) == 0) {
        _regionCache.erase(name.substr(10));
    }
}
""")
