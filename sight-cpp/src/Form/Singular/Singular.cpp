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
