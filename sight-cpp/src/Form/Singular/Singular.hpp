#pragma once
#include "Form/Singular/Property/Property.hpp"

#include <memory>
#include <string>
#include <vector>

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
    Singular() = default;
    Singular(const Singular&) {}
    Singular& operator=(const Singular&) {
        _propertyRegistry.clear();
        _property_formation = nullptr;
        _propertiesBuilt = false;
        return *this;
    }
    // Moves do NOT carry the property registry: registered properties hold
    // owner pointers into the source object. The registry is derived state
    // and lazily rebuilds against the new address on next findProperty().
    Singular(Singular&&) noexcept {}
    Singular& operator=(Singular&&) noexcept {
        _propertyRegistry.clear();
        _property_formation = nullptr;
        _propertiesBuilt = false;
        return *this;
    }
    virtual ~Singular() = default;
    virtual std::string getIdentifier() const = 0;

    Formation* singular_properties();
    const Formation* singular_properties() const;

    // Registry lookup by registered name (may be dotted, e.g. "shape.r").
    // Lazily calls buildProperties() on first access.
    Property* findProperty(const std::string& name);

    // A Singular can flexibly own any kind of thing. That ownership IS the
    // property registry below: each owned value is wrapped as a Property
    // (PropertyRef over a member, ComputedProperty over getters) and typed
    // through PropertyValue, so it stays legible to the Law system instead of
    // sitting as a raw untyped member no law can address.

protected:
    std::vector<std::unique_ptr<Property>> _propertyRegistry;
    Formation* _property_formation = nullptr;
    bool _propertiesBuilt = false;
    virtual void buildProperties() = 0;
}; 
