#include "ConstructedBeing/Singular/Singular.hpp"
#include "ConstructedBeing/Singular/Property/Property.hpp"
#include "ConstructedBeing/Singular/Property/PropertyRef.hpp"
#include "ConstructedBeing/Singular/Property/DataStructure.hpp"
#include "Singularity/Core/StringId.hpp"
#include "Singularity/TransferPolicy.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"

#include <atomic>

static std::atomic<int> s_singularAliveCount{0};

int Singular::getAliveCount() {
    return s_singularAliveCount.load(std::memory_order_relaxed);
}

Singular::Singular() {
    s_singularAliveCount.fetch_add(1, std::memory_order_relaxed);
}

Singular::~Singular() {
    // Announce BEFORE the count drops, and take nothing but the pointer:
    // every derived destructor has already run, so this object is a Singular
    // and nothing more. See the header for why this exists.
    notifyBeingReleased(this);
    detachAllPropertyCells();
    s_singularAliveCount.fetch_sub(1, std::memory_order_relaxed);
}

Singular::Singular(const Singular& o)
    : designatedZones(o.designatedZones),
      _stakeholders(o._stakeholders),
      _dynamicProperties(o._dynamicProperties),
      _dataStructures(o._dataStructures),
      name(o.name),
      _telosId(o._telosId) {
    s_singularAliveCount.fetch_add(1, std::memory_order_relaxed);
}

Singular& Singular::operator=(const Singular& o) {
    if (this != &o) {
        detachAllPropertyCells();
        designatedZones = o.designatedZones;
        _stakeholders = o._stakeholders;
        _dynamicProperties = o._dynamicProperties;
        _dataStructures = o._dataStructures;
        name = o.name;
        _telosId = o._telosId;
    }
    _propertyNames.clear();        // Clear SoA array #1
    _propertyRegistry.clear();      // Clear SoA array #2
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
      _telosId(std::move(o._telosId)),
      _propertyCells(std::move(o._propertyCells)),
      _propertyCellModes(std::move(o._propertyCellModes)) {
    PropertyCellTable::instance().retargetBearer(&o, this);
    o._propertyCells.clear();
    o._propertyCellModes.clear();
    s_singularAliveCount.fetch_add(1, std::memory_order_relaxed);
}

Singular& Singular::operator=(Singular&& o) noexcept {
    if (this != &o) {
        detachAllPropertyCells();
        designatedZones = std::move(o.designatedZones);
        _stakeholders = std::move(o._stakeholders);
        _dynamicProperties = std::move(o._dynamicProperties);
        _dataStructures = std::move(o._dataStructures);
        name = std::move(o.name);
        _telosId = std::move(o._telosId);
        _propertyCells = std::move(o._propertyCells);
        _propertyCellModes = std::move(o._propertyCellModes);
        PropertyCellTable::instance().retargetBearer(&o, this);
        o._propertyCells.clear();
        o._propertyCellModes.clear();
    }
    _propertyNames.clear();
    _propertyRegistry.clear();
    _property_formation = nullptr;
    _propertiesBuilt = false;
    return *this;
}

static Singular::PropertyChangeCallback s_propertyChangeCallback = nullptr;
static Singular::BeingReleasedCallback s_beingReleasedCallback = nullptr;

void Singular::setPropertyChangeCallback(PropertyChangeCallback cb) {
    s_propertyChangeCallback = std::move(cb);
}

void Singular::setBeingReleasedCallback(BeingReleasedCallback cb) {
    s_beingReleasedCallback = std::move(cb);
}

void Singular::notifyBeingReleased(const Singular* being) {
    if (s_beingReleasedCallback) {
        s_beingReleasedCallback(being);
    }
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
    Earthcall::StringId _nameId;
    Singular* _owner;
public:
    DynamicPropertyBridge(const std::string& name, Singular* owner)
        : _name(name), _nameId(Earthcall::StringInterner::intern(name)), _owner(owner) {}

    Earthcall::StringId nameId() const override { return _nameId; }
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
                    return _owner->setDynamicProperty(_name, coerced);
                }
            }
        }
        return _owner->setDynamicProperty(_name, v);
    }
};

void Singular::registerTelosProperty() {
    Earthcall::StringId telosId = Earthcall::StringInterner::intern("telos");

    // Check if already registered (scan the ID array)
    for (size_t i = 0; i < _propertyNames.size(); ++i) {
        if (_propertyNames[i] == telosId) return;
    }

    // registerProperty registers in BOTH parallel arrays — pushing the id here
    // too appended a name with no property behind it, and since this runs for
    // EVERY being at buildProperties time, `_propertyNames` was one entry
    // longer than `_propertyRegistry` from the first lookup onward. The scan
    // pairs them by index, so every property registered after `telos` resolved
    // to the property registered one slot BEFORE it — a being answering a
    // read with a different property's value, engine-wide.
    auto prop = std::make_unique<PropertyRef<Singular, std::string>>(
        "telos", this, &Singular::_telosId);
    registerProperty(std::move(prop));
}

// ============================================================================
// HOT PATH: Find property by StringId (cache-optimal integer scan)
//
// Scans _propertyNames (contiguous array of 4-byte integers) with zero
// allocations and perfect L1 cache locality. A 64-byte cache line holds 16
// StringIds, so most property registries (under ~20 properties) complete the
// entire scan without leaving the CPU cache.
//
// Only dereferences _propertyRegistry[index] when a match is found (once per
// lookup, not once per iteration).
// ============================================================================
Property* Singular::findProperty(Earthcall::StringId id) {
    if (!_propertiesBuilt) {
        _propertiesBuilt = true;   // set first: buildProperties may itself query
        buildProperties();
        registerTelosProperty();
    }

    // Scan the integer array (cache-friendly)
    for (size_t i = 0; i < _propertyNames.size(); ++i) {
        if (_propertyNames[i] == id) {
            return _propertyRegistry[i].get();
        }
    }

    // Dynamic property fallback (check by ID).
    //
    // registerProperty pushes to BOTH arrays — that is the whole of its job,
    // and the two are indexed in parallel by the scan above. Pushing the id
    // here as well appended a SECOND copy of the name with no property behind
    // it, so from the first lazy bridge onward `_propertyNames` ran one entry
    // longer than `_propertyRegistry` and every later index was off by one:
    // the next dynamic property looked up on the same being read
    // `_propertyRegistry[i]` past the end.
    //
    // The symptom was not a crash. It was an authored property that read
    // correctly ONCE and then stopped — the Synthesis Studio's chord pad
    // answered `isChordPad == true` on the first click and false on every
    // click after it, so a Person could work each control exactly one time
    // per session. Every authored property in every save was exposed to this.
    auto it = _dynamicProperties.find(id);
    if (it != _dynamicProperties.end() || _propertyCells.find(id) != _propertyCells.end()) {
        auto bridge = std::make_unique<DynamicPropertyBridge>(
            Earthcall::StringInterner::resolve(id), this);
        Property* p = bridge.get();
        registerProperty(std::move(bridge));
        return p;
    }

    return nullptr;
}

// ============================================================================
// COLD PATH: Find property by string name (backward compatible)
//
// Interns the string once to get its ID, then delegates to the hot path.
// Kept for existing code that doesn't know about StringId yet.
// ============================================================================
Property* Singular::findProperty(const std::string& name) {
    return findProperty(Earthcall::StringInterner::intern(name));
}

std::vector<Property*> Singular::listProperties() {
    if (!_propertiesBuilt) {
        _propertiesBuilt = true;
        buildProperties();
        registerTelosProperty();
    }

    // AUTHORED PROPERTIES ARE AS REAL AS FIRST-MOVER ONES, and this is where
    // that stopped being true. `_propertyRegistry` only gains a
    // DynamicPropertyBridge when findProperty is asked for that exact name, so
    // an authored property nobody had looked up yet was absent from every
    // caller of this function — and the callers are the ones that matter:
    //
    //   * LawManager::seedStateFacts walks listProperties() to assert the
    //     property-state facts the Rete matches on. No fact, no match, so a
    //     WhileTrue law conditioned on an authored property never fired for
    //     any being — which is every continuous law the Synthesis Studio has.
    //   * The inspection surfaces read it too, so an authored property was
    //     invisible until something happened to touch it. Refusal #6 says a
    //     field law can read is a field law can SEE.
    //
    // Materialising the bridges here makes the registry complete on demand,
    // which is what every caller already assumed it was.
    for (const auto& entry : _dynamicProperties) {
        findProperty(entry.first);   // creates the bridge if it is not there yet
    }
    for (const auto& entry : _propertyCells) {
        findProperty(entry.first);
    }

    std::vector<Property*> out;
    out.reserve(_propertyRegistry.size());
    for (auto& property : _propertyRegistry) {
        if (property) out.push_back(property.get());
    }
    return out;
}

bool Singular::hasPropertyStartingWith(const std::string& prefix) {
    if (!_propertiesBuilt) {
        _propertiesBuilt = true;
        buildProperties();
        registerTelosProperty();
    }
    for (const auto& entry : _dynamicProperties) {
        findProperty(entry.first);
    }
    for (auto& property : _propertyRegistry) {
        if (!property) continue;
        const std::string& propName = property->name();
        if (propName.size() >= prefix.size() && propName.compare(0, prefix.size(), prefix) == 0) return true;
    }
    return false;
}


// ============================================================================
// Dynamic properties (Person-authored via AddProperty)
//
// Hot path uses StringId keys for O(1) hash map lookup with zero allocations.
// String overloads kept for backward compatibility.
// ============================================================================

bool Singular::getDynamicProperty(const std::string& name, PropertyValue& out) const {
    return getDynamicProperty(Earthcall::StringInterner::intern(name), out);
}

bool Singular::getDynamicProperty(Earthcall::StringId id, PropertyValue& out) const {
    if (recognizesAuthoredPropertyProjection(id)) {
        return readAuthoredPropertyProjection(id, out);
    }
    auto itCell = _propertyCells.find(id);
    if (itCell != _propertyCells.end()) {
        auto* cell = PropertyCellTable::instance().getCell(itCell->second);
        if (!cell) {
            return false;
        }
        out = cell->value;
        return true;
    }
    auto it = _dynamicProperties.find(id);
    if (it != _dynamicProperties.end()) {
        out = it->second;
        return true;
    }
    return false;
}

bool Singular::setDynamicProperty(const std::string& name, const PropertyValue& v) {
    return setDynamicProperty(Earthcall::StringInterner::intern(name), v);
}

bool Singular::setDynamicProperty(Earthcall::StringId id, const PropertyValue& v) {
    const bool projected = recognizesAuthoredPropertyProjection(id);
    if (projected && !writeAuthoredPropertyProjection(id, v)) return false;

    auto itCell = _propertyCells.find(id);
    if (itCell != _propertyCells.end()) {
        auto itMode = _propertyCellModes.find(id);
        if (itMode != _propertyCellModes.end() && itMode->second == CellOwnershipMode::Weak) {
            return false;
        }
        auto* cell = PropertyCellTable::instance().getCell(itCell->second);
        if (!cell) return false;
        if (propertyValuesEquivalent(cell->value, v)) return true;
        cell->value = v;
        _dynamicProperties[id] = v;
        PropertyCellTable::instance().notifyMutation(itCell->second);
        return true;
    }

    PropertyValue stored = v;
    auto existing = _dynamicProperties.find(id);
    if (existing == _dynamicProperties.end()) {
        Universe::instance().bumpStructuralRevision();
    } else if (propertyStorageUnchanged(existing->second, stored)) {
        existing->second = stored;
        return true;
    }
    _dynamicProperties[id] = std::move(stored);
    notifyPropertyChanged(this, Earthcall::StringInterner::resolve(id));
    return true;
}

bool Singular::hasAuthoredPropertyAccessor(const std::string& name) const {
    // Accessor origin is derived from the existing bridge, not cached state or
    // a second authority office. Its value may be absent after RemoveProperty.
    // A registered engine accessor wins even if malformed authored storage
    // happens to contain a duplicate name; that must never open shadowing.
    for (const auto& property : _propertyRegistry)
        if (property && property->name() == name)
            return dynamic_cast<const DynamicPropertyBridge*>(property.get()) != nullptr;
    return hasDynamicProperty(name);
}

bool Singular::hasDynamicProperty(const std::string& name) const {
    return hasDynamicProperty(Earthcall::StringInterner::intern(name));
}

bool Singular::removeDynamicProperty(const std::string& name) {
    return removeDynamicProperty(Earthcall::StringInterner::intern(name));
}

bool Singular::removeDynamicProperty(Earthcall::StringId id) {
    unbindPropertyCell(id);
    if (_dynamicProperties.erase(id)) {
        Universe::instance().bumpStructuralRevision();
        return true;
    }
    return false;
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

PropertyValue* Singular::getDynamicPropertyPtr(Earthcall::StringId id) {
    auto itCell = _propertyCells.find(id);
    if (itCell != _propertyCells.end()) {
        auto itMode = _propertyCellModes.find(id);
        if (itMode != _propertyCellModes.end() && itMode->second == CellOwnershipMode::Weak) {
            return nullptr;
        }
        auto* cell = PropertyCellTable::instance().getCell(itCell->second);
        if (cell) {
            _dynamicProperties[id] = cell->value;
            return &cell->value;
        }
        return nullptr;
    }
    if (recognizesAuthoredPropertyProjection(id)) {
        PropertyValue proj;
        if (readAuthoredPropertyProjection(id, proj)) {
            _dynamicProperties[id] = std::move(proj);
        }
    }
    auto it = _dynamicProperties.find(id);
    return (it != _dynamicProperties.end()) ? &it->second : nullptr;
}

const PropertyValue* Singular::getDynamicPropertyPtr(Earthcall::StringId id) const {
    auto itCell = _propertyCells.find(id);
    if (itCell != _propertyCells.end()) {
        auto* cell = PropertyCellTable::instance().getCell(itCell->second);
        if (cell) {
            return &cell->value;
        }
        return nullptr;
    }
    if (recognizesAuthoredPropertyProjection(id)) {
        PropertyValue proj;
        if (readAuthoredPropertyProjection(id, proj)) {
            const_cast<Singular*>(this)->_dynamicProperties[id] = std::move(proj);
        }
    }
    auto it = _dynamicProperties.find(id);
    return (it != _dynamicProperties.end()) ? &it->second : nullptr;
}

bool Singular::bindPropertyCell(Earthcall::StringId id, CellId cellId, CellOwnershipMode mode) {
    auto it = _propertyCells.find(id);
    if (it != _propertyCells.end()) {
        PropertyCellTable::instance().detachObserver(it->second, this, id);
        _propertyCells.erase(it);
        _propertyCellModes.erase(id);
    }
    if (mode == CellOwnershipMode::InlineValue || !cellId.valid()) {
        return true;
    }
    if (!PropertyCellTable::instance().attachObserver(cellId, this, id, mode)) {
        return false;
    }
    _propertyCells[id] = cellId;
    _propertyCellModes[id] = mode;
    auto* cell = PropertyCellTable::instance().getCell(cellId);
    if (cell && mode != CellOwnershipMode::Weak) {
        Property* p = findProperty(id);
        if (p && p->typeName() != "dynamic") {
            p->setValue(cell->value);
        } else {
            _dynamicProperties[id] = cell->value;
        }
    }
    return true;
}

bool Singular::bindPropertyCell(const std::string& name, CellId cellId, CellOwnershipMode mode) {
    return bindPropertyCell(Earthcall::StringInterner::intern(name), cellId, mode);
}

bool Singular::unbindPropertyCell(Earthcall::StringId id) {
    auto it = _propertyCells.find(id);
    if (it == _propertyCells.end()) return false;
    PropertyCellTable::instance().detachObserver(it->second, this, id);
    _propertyCells.erase(it);
    _propertyCellModes.erase(id);
    return true;
}

bool Singular::unbindPropertyCell(const std::string& name) {
    return unbindPropertyCell(Earthcall::StringInterner::intern(name));
}

bool Singular::hasPropertyCell(const std::string& name) const {
    return hasPropertyCell(Earthcall::StringInterner::intern(name));
}

CellId Singular::getPropertyCell(const std::string& name) const {
    return getPropertyCell(Earthcall::StringInterner::intern(name));
}

CellOwnershipMode Singular::getPropertyCellMode(const std::string& name) const {
    return getPropertyCellMode(Earthcall::StringInterner::intern(name));
}

bool Singular::convertPropertyCellToShared(Earthcall::StringId id) {
    auto it = _propertyCells.find(id);
    if (it == _propertyCells.end()) return false;
    bool ok = PropertyCellTable::instance().convertUniqueToShared(it->second, this, id);
    if (ok) {
        _propertyCellModes[id] = CellOwnershipMode::Shared;
    }
    return ok;
}

bool Singular::convertPropertyCellToShared(const std::string& name) {
    return convertPropertyCellToShared(Earthcall::StringInterner::intern(name));
}

void Singular::detachAllPropertyCells() {
    for (const auto& [propId, cellId] : _propertyCells) {
        PropertyCellTable::instance().detachObserver(cellId, this, propId);
    }
    _propertyCells.clear();
    _propertyCellModes.clear();
    PropertyCellTable::instance().detachAllForBearer(this);
    TransferPolicy::notifyBearerDestroyed(this);
}
