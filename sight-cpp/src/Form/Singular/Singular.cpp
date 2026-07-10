#include "Form/Singular/Singular.hpp"

#include "Form/Singular/Property/Property.hpp"

Formation* Singular::singular_properties() {
    return _property_formation;
}

const Formation* Singular::singular_properties() const {
    return _property_formation;
}

Property* Singular::findProperty(const std::string& name) {
    if (!_propertiesBuilt) {
        _propertiesBuilt = true;   // set first: buildProperties may itself query
        buildProperties();
    }
    for (auto& property : _propertyRegistry) {
        if (property && property->name() == name) return property.get();
    }
    return nullptr;
}

std::vector<Property*> Singular::listProperties() {
    if (!_propertiesBuilt) {
        _propertiesBuilt = true;
        buildProperties();
    }
    std::vector<Property*> out;
    out.reserve(_propertyRegistry.size());
    for (auto& property : _propertyRegistry) {
        if (property) out.push_back(property.get());
    }
    return out;
}
