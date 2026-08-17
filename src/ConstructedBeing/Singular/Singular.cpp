#include "ConstructedBeing/Singular/Singular.hpp"
#include "ConstructedBeing/Object/Formation/Formation.hpp"
#include "ConstructedBeing/Singular/Property/Property.hpp"
#include "ConstructedBeing/Singular/Property/DataStructure.hpp"

Singular::Singular() = default;
Singular::~Singular() = default;
Singular::Singular(const Singular&) {}
Singular& Singular::operator=(const Singular&) {
    _propertyRegistry.clear();
    _property_formation = nullptr;
    _propertiesBuilt = false;
    return *this;
}
Singular::Singular(Singular&&) noexcept {}
Singular& Singular::operator=(Singular&&) noexcept {
    _propertyRegistry.clear();
    _property_formation = nullptr;
    _propertiesBuilt = false;
    return *this;
}

static Singular::PropertyChangeCallback s_propertyChangeCallback = nullptr;

void Singular::setPropertyChangeCallback(PropertyChangeCallback cb) {
    s_propertyChangeCallback = cb;
}

void Singular::notifyPropertyChanged(Singular* owner, const std::string& name) {
    if (s_propertyChangeCallback) {
        s_propertyChangeCallback(owner, name);
    }
}

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
    if (_dynamicProperties.find(name) != _dynamicProperties.end()) {
        auto bridge = std::make_unique<DynamicPropertyBridge>(name, this);
        Property* p = bridge.get();
        _propertyRegistry.push_back(std::move(bridge));
        return p;
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

void Singular::addDataStructure(const DataStructure& ds) {
    _dataStructures[ds.name] = ds;
}

DataStructure* Singular::getDataStructure(const std::string& name) {
    auto it = _dataStructures.find(name);
    if (it != _dataStructures.end()) {
        return &(it->second);
    }
    return nullptr;
}

const DataStructure* Singular::getDataStructure(const std::string& name) const {
    auto it = _dataStructures.find(name);
    if (it != _dataStructures.end()) {
        return &(it->second);
    }
    return nullptr;
}

bool Singular::removeDataStructure(const std::string& name) {
    return _dataStructures.erase(name) > 0;
}
