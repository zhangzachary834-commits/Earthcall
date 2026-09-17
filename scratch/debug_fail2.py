import re
with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "r") as f:
    content = f.read()

content = content.replace(
    '''    if (!setDynamicProperty(std::string(kSelectionPrefix) + propertyName,
                            PropertyValue(definition.dump())) ||
        !setDynamicProperty(propertyName, PropertyValue(colors))) {
        reason = "surface selection could not be installed as a Property";
        return false;
    }''',
    '''    if (!setDynamicProperty(std::string(kSelectionPrefix) + propertyName, PropertyValue(definition.dump()))) {
        reason = "definition failed";
        return false;
    }
    if (!setDynamicProperty(propertyName, PropertyValue(colors))) {
        reason = "projection failed";
        return false;
    }'''
)

with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "w") as f:
    f.write(content)
