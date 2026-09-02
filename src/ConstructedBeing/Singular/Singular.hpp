#pragma once
#include "ConstructedBeing/Singular/Property/Property.hpp"
#include "ConstructedBeing/Singular/Property/DataStructure.hpp"

#include <ctime>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <functional>
#include <ctime>

class Formation;

// ------------------------------------------------------------------
// A Singular represents any entity or concept with meaning greater than the sum of its parts.
// Marker interface representing a "singular being" in the system.
// Any class that wishes to participate in Relation should inherit
// from this interface and provide a stable textual identifier
// via getIdentifier().
// ------------------------------------------------------------------
class Singular {
public:
    Singular();
    Singular(const Singular&);
    Singular& operator=(const Singular&);
    Singular(Singular&&) noexcept;
    Singular& operator=(Singular&&) noexcept;
    virtual ~Singular();
    virtual std::string getIdentifier() const = 0;
    
    static int getAliveCount();

    // Checks if this Singular satisfies the strict Kernel bounds required for set-to-set synthesis.
    // Placeholder for when we define these bounds later.
    virtual bool satisfiesKernelBounds() const { return true; }

    using PropertyChangeCallback = std::function<void(Singular*, const std::string&)>;
    static void setPropertyChangeCallback(PropertyChangeCallback cb);
    static void notifyPropertyChanged(Singular* owner, const std::string& name);

    // ------------------------------------------------------------------
    // "This being is about to stop existing."
    //
    // ReteFacts hold RAW participant pointers and outlive the round that
    // asserted them, so a fact about a freed being is a dangling read waiting
    // for the next tick. ReteNetwork::retractFactsAbout exists exactly to
    // prevent that, but only the law-driven unmaking path ever called it — a
    // being freed by ordinary C++ scope exit (a stack-local Object, a Law and
    // the provenance Relations it owns) left its facts behind, pointing at
    // reclaimed memory.
    //
    // Called from ~Singular, which runs AFTER every derived destructor, so
    // the listener gets a pointer and NOTHING ELSE: no virtual call is valid
    // here, getIdentifier() least of all. Matching by pointer is all that is
    // needed and all that is safe.
    // ------------------------------------------------------------------
    using BeingReleasedCallback = std::function<void(const Singular*)>;
    static void setBeingReleasedCallback(BeingReleasedCallback cb);
    static void notifyBeingReleased(const Singular* being);

    Formation* singular_properties();
    const Formation* singular_properties() const;

    // -------------------------------------------------------------------------
    // Property lookup — hot path uses StringId (cache-friendly integer scan),
    // cold path uses string (backward compatible, delegates to StringId version)
    // -------------------------------------------------------------------------

    // Hot path: Find property by its interned ID. Scans _propertyNames (a
    // contiguous array of 4-byte integers) with zero allocations and perfect
    // L1 cache utilization. At 21 properties, the entire scan fits in one
    // cache line (64 bytes). Lazily calls buildProperties() on first access.
    Property* findProperty(Earthcall::StringId id);

    // Cold path: Find property by string name. Interns the string once, then
    // delegates to the StringId version. Kept for backward compatibility —
    // existing code that doesn't know about StringId can keep working.
    Property* findProperty(const std::string& name);

    // Enumerate the registered properties (lazy-builds first). This is what
    // makes a Singular's vocabulary DISCOVERABLE — authoring UIs offer these
    // names instead of asking Persons to know the registry by heart.
    std::vector<Property*> listProperties();

    // A Singular can flexibly own any kind of thing. That ownership IS the
    // property registry below: each owned value is wrapped as a Property
    // (PropertyRef over a member, ComputedProperty over getters) and typed
    // through PropertyValue, so it stays legible to the Law system instead of
    // sitting as a raw untyped member no law can address.

    // Dynamic properties to allow Laws to attach state to objects
    // Hot path: StringId keys
    bool getDynamicProperty(Earthcall::StringId id, PropertyValue& out) const;
    void setDynamicProperty(Earthcall::StringId id, const PropertyValue& v);

    // Cold path: String names (backward compatible, delegates to StringId version)
    bool getDynamicProperty(const std::string& name, PropertyValue& out) const;
    void setDynamicProperty(const std::string& name, const PropertyValue& v);

    // A law-added property is a real part of the being, so it has to be
    // ENUMERABLE (the authoring UI offers it beside the registered vocabulary)
    // and PERSISTABLE (a property that vanishes on save was never granted).
    // The registry above is the first-mover vocabulary; this is the authored one.
    bool hasDynamicProperty(const std::string& name) const;
    bool hasDynamicProperty(Earthcall::StringId id) const {
        return _dynamicProperties.find(id) != _dynamicProperties.end();
    }
    const std::unordered_map<Earthcall::StringId, PropertyValue>& dynamicProperties() const {
        return _dynamicProperties;
    }
    bool removeDynamicProperty(const std::string& name);
    bool removeDynamicProperty(Earthcall::StringId id);
    
    // Authored data structure methods
    void addDataStructure(const class DataStructure& ds);
    class DataStructure* getDataStructure(const std::string& name);
    const class DataStructure* getDataStructure(const std::string& name) const;
    bool removeDataStructure(const std::string& name);
    const std::map<std::string, class DataStructure>& dataStructures() const {
        return _dataStructures;
    }

    // Stakeholders: an append-only log of who has changed this being's properties.
    // When a Law mutates a property, its authors are recorded as stakeholders.
    struct StakeholderRecord {
        std::string propertyPath;
        std::string authorId;
        std::string lawId;
        std::time_t timestamp;
    };

    void addStakeholder(const std::string& propertyPath, const std::string& authorId, const std::string& lawId, std::time_t timestamp) {
        _stakeholders.push_back({propertyPath, authorId, lawId, timestamp});
    }

    const std::vector<StakeholderRecord>& stakeholders() const {
        return _stakeholders;
    }


    // Zone Designations
    void addZoneDesignation(const std::string& zoneName) {
        if (std::find(designatedZones.begin(), designatedZones.end(), zoneName) == designatedZones.end()) {
            designatedZones.push_back(zoneName);
        }
    }
    void removeZoneDesignation(const std::string& zoneName) {
        auto it = std::find(designatedZones.begin(), designatedZones.end(), zoneName);
        if (it != designatedZones.end()) {
            designatedZones.erase(it);
        }
    }
    bool belongsToZone(const std::string& zoneName) const {
        return std::find(designatedZones.begin(), designatedZones.end(), zoneName) != designatedZones.end();
    }
    const std::vector<std::string>& getDesignatedZones() const { return designatedZones; }
    std::vector<std::string>& getDesignatedZonesMutable() { return designatedZones; }

    // Telos: the Lexeme this being is ordered toward. Stored as that Lexeme's
    // identifier. Empty means unranked in any joy hierarchy. Not a slogan and
    // not a skinned Object — see HIERARCHY_OF_JOYS.md.
    const std::string& telosId() const { return _telosId; }
    void setTelosId(const std::string& id) { _telosId = id; }
    std::string propTelos() const { return _telosId; }
    void propSetTelos(const std::string& id) { _telosId = id; }

protected:
    // Registered on every Singular after the subclass vocabulary, so a being
    // that forgets to mention telos is not a black box.
    void registerTelosProperty();
    std::vector<std::string> designatedZones;
    std::vector<StakeholderRecord> _stakeholders;

    // -------------------------------------------------------------------------
    // Structure of Arrays (SoA) for cache-optimal property lookup
    //
    // BEFORE: std::vector<std::unique_ptr<Property>> required dereferencing
    // each unique_ptr before checking its name (cache miss per iteration).
    //
    // AFTER: Scan _propertyNames (contiguous integers) with perfect L1 cache
    // locality, only dereferencing _propertyRegistry[index] when found.
    //
    // Both arrays MUST stay synchronized (same size, same order). Populated
    // in parallel by property registration (buildProperties, telos, dynamic).
    // -------------------------------------------------------------------------
    std::vector<Earthcall::StringId> _propertyNames;          // Parallel array #1: names as IDs
    std::vector<std::unique_ptr<Property>> _propertyRegistry; // Parallel array #2: property pointers

    // Dynamic properties (Person-authored via AddProperty). Keys are StringIds
    // for fast lookup. String overloads kept for backward compatibility.
    std::unordered_map<Earthcall::StringId, PropertyValue> _dynamicProperties;
    
    // Authored data structures and bounds (manifesto property framework)
    std::map<std::string, class DataStructure> _dataStructures;
    Formation* _property_formation = nullptr;
    // Subclasses will deserialize their own specific properties here.
    virtual void buildProperties() = 0;

    void registerProperty(std::unique_ptr<Property> prop) {
        if (!prop) return;
        _propertyNames.push_back(prop->nameId());
        _propertyRegistry.push_back(std::move(prop));
    }
    
    // Dynamic property access helper
    template<typename T>
    T getDynamicPropertyOrDefault(const std::string& name, const T& defaultValue) const {
        auto it = _dynamicProperties.find(Earthcall::StringInterner::intern(name));
        if (it != _dynamicProperties.end()) {
            if (const T* val = std::get_if<T>(&it->second)) {
                return *val;
            }
        }
        return defaultValue;
    }

    std::string name;
    bool _propertiesBuilt = false;
    std::string _telosId;

    /*
     * RELATION AND FORMATION OWNERSHIP AND POINTERS
     * Every Singular knows the Relations and Formations its part of */

    // Parent Formation instances that this Singular is a part of
    std::vector<Formation> parentFormationInstances;

    // Child Formation instances that are within this Singular
    std::vector<Formation> childFormationInstances;

};
