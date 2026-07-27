#include "Form/Singular/Singular.hpp"

#include "Form/Singular/Property/Property.hpp"

Formation* Singular::singular_properties() {
    return _property_formation;
}

const Formation* Singular::singular_properties() const {
    return _property_formation;
}

class DynamicPropertyBridge : public Property {
    std::string _name;
    Singular* _owner;
public:
    DynamicPropertyBridge(const std::string& name, Singular* owner) : _name(name), _owner(owner) {}
    
    std::string name() const override { return _name; }
    std::string typeName() const override { return "dynamic"; }
    
    PropertyValue value() const override {
        PropertyValue v;
        _owner->getDynamicProperty(_name, v);
        return v;
    }
    
    bool setValue(const PropertyValue& v) override {
        _owner->setDynamicProperty(_name, v);
        return true;
    }
};

Property* Singular::findProperty(const std::string& name) {
    if (!_propertiesBuilt) {
        _propertiesBuilt = true;   // set first: buildProperties may itself query
        buildProperties();
    }
    for (auto& property : _propertyRegistry) {
        if (property && property->name() == name) return property.get();
    }
    
    // Dynamic property fallback
    auto bridge = std::make_unique<DynamicPropertyBridge>(name, this);
    Property* p = bridge.get();
    _propertyRegistry.push_back(std::move(bridge));
    return p;
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

bool Singular::getDynamicProperty(const std::string& name, PropertyValue& out) const {
    auto it = _dynamicProperties.find(name);
    if (it != _dynamicProperties.end()) {
        out = it->second;
        return true;
    }
    return false;
}

void Singular::setDynamicProperty(const std::string& name, const PropertyValue& v) {
    _dynamicProperties[name] = v;
}
