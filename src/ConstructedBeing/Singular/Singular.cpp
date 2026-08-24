#include "ConstructedBeing/Singular/Singular.hpp"
#include "ConstructedBeing/Singular/Object/Formation/Formation.hpp"
#include "ConstructedBeing/Singular/Property/Property.hpp"
#include "ConstructedBeing/Singular/Property/PropertyRef.hpp"
#include "ConstructedBeing/Singular/Property/DataStructure.hpp"

Singular::Singular() = default;
Singular::~Singular() = default;
Singular::Singular(const Singular& o)
    : designatedZones(o.designatedZones),
      _stakeholders(o._stakeholders),
      _dynamicProperties(o._dynamicProperties),
      _dataStructures(o._dataStructures),
      name(o.name),
      _telosId(o._telosId) {}

Singular& Singular::operator=(const Singular& o) {
    if (this != &o) {
        designatedZones = o.designatedZones;
        _stakeholders = o._stakeholders;
        _dynamicProperties = o._dynamicProperties;
        _dataStructures = o._dataStructures;
        name = o.name;
        _telosId = o._telosId;
    }
    _propertyRegistry.clear();
    _property_formation = nullptr;
    _propertiesBuilt = false;
    return *this;
}

Singular::Singular(Singular&& o) noexcept
    : designatedZones(std::move(o.designatedZones)),
      _stakeholders(std::move(o._stakeholders)),
      _dynamicProperties(std::move(o._dynamicProperties)),
      _dataStructures(std::move(o._dataStructures)),
      name(std::move(o.name)),
      _telosId(std::move(o._telosId)) {}

Singular& Singular::operator=(Singular&& o) noexcept {
    if (this != &o) {
        designatedZones = std::move(o.designatedZones);
        _stakeholders = std::move(o._stakeholders);
        _dynamicProperties = std::move(o._dynamicProperties);
        _dataStructures = std::move(o._dataStructures);
        name = std::move(o.name);
        _telosId = std::move(o._telosId);
    }
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

// Re-express `n` in whatever arithmetic alternative `like` currently holds.
// The same rule PropertyPath::setValue applies to registered properties; kept
// local here rather than shared because the version there is a file-local
// helper in PropertyPath.cpp and hoisting it is a wider change than this fix.
namespace {
bool coerceToHeldAlternative(const PropertyValue& like, double n, PropertyValue& out) {
    return std::visit([&](auto&& t) {
        using X = std::decay_t<decltype(t)>;
        if constexpr (std::is_arithmetic_v<X>) {
            out = PropertyValue(static_cast<X>(n));
            return true;
        } else {
            return false;
        }
    }, like);
}
} // namespace

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
        // AN AUTHORED PROPERTY KEEPS ITS TYPE, exactly as a registered one
        // does.
        //
        // This used to store whatever arrived, so a Map action writing the
        // double 1.0 into a property granted as `bool` REPLACED the bool with
        // a double. Every later reader asking std::get_if<bool> then got
        // nothing and read it as false — the property was still there, still
        // numerically right, and answered the wrong question. A toggle
        // authored as law text flipped correctly and looked stuck.
        //
        // The registered path has always coerced (PropertyPath::setValue's
        // coerceLike retry), so the same law text behaved differently
        // depending on whether the property was registered in C++ or granted
        // by an author. That is the split this tree refuses everywhere else:
        // "authored properties are as real as first-mover ones"
        // (Law::couldApplyTo). Arithmetic only — a string or a vector arriving
        // for a numeric slot is a genuine change of what the property is, and
        // guessing a conversion there would be inventing meaning.
        PropertyValue held;
        if (_owner->getDynamicProperty(_name, held) &&
            held.index() != v.index()) {
            double incoming = 0.0;
            double existing = 0.0;
            if (propertyValueToNumber(v, incoming) &&
                propertyValueToNumber(held, existing)) {
                PropertyValue coerced;
                if (coerceToHeldAlternative(held, incoming, coerced)) {
                    _owner->setDynamicProperty(_name, coerced);
                    return true;
                }
            }
        }
        _owner->setDynamicProperty(_name, v);
        return true;
    }
};

void Singular::registerTelosProperty() {
    for (const auto& property : _propertyRegistry) {
        if (property && property->name() == "telos") return;
    }
    _propertyRegistry.push_back(std::make_unique<PropertyRef<Singular, std::string>>(
        "telos", this, &Singular::_telosId));
}

Property* Singular::findProperty(const std::string& name) {
    if (!_propertiesBuilt) {
        _propertiesBuilt = true;   // set first: buildProperties may itself query
        buildProperties();
        registerTelosProperty();
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
        registerTelosProperty();
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
