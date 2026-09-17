with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "r") as f:
    or_cpp = f.read()

or_cpp = or_cpp.replace(
    'if (!setDynamicProperty(std::string(kSelectionPrefix) + propertyName,',
    'bool d1 = setDynamicProperty(std::string(kSelectionPrefix) + propertyName, PropertyValue(definition.dump()));\n    bool d2 = setDynamicProperty(propertyName, PropertyValue(colors));\n    if (!d1 || !d2) {\n        std::cout << "elevateSurfaceRegionProperty: setDynamicProperty failed! d1=" << d1 << " d2=" << d2 << "\\n";\n    }\n    if (!d1 || !d2) {'
)

with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "w") as f:
    f.write(or_cpp)
