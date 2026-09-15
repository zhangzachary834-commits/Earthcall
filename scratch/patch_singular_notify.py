with open("src/ConstructedBeing/Singular/Singular.hpp", "r") as f:
    content = f.read()

content = content.replace(
"""    bool getDynamicProperty(Earthcall::StringId id, PropertyValue& out) const;
    bool setDynamicProperty(Earthcall::StringId id, const PropertyValue& v);""",
"""    bool getDynamicProperty(Earthcall::StringId id, PropertyValue& out) const;
    bool setDynamicProperty(Earthcall::StringId id, const PropertyValue& v);
    virtual void onDynamicPropertyChanged(Earthcall::StringId id) {}""")

with open("src/ConstructedBeing/Singular/Singular.hpp", "w") as f:
    f.write(content)

with open("src/ConstructedBeing/Singular/Singular.cpp", "r") as f:
    content = f.read()

content = content.replace(
"""    _dynamicProperties[id] = std::move(stored);""",
"""    _dynamicProperties[id] = std::move(stored);
    onDynamicPropertyChanged(id);""")

with open("src/ConstructedBeing/Singular/Singular.cpp", "w") as f:
    f.write(content)
