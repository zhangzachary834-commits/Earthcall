#pragma once

#include "PropertyValue.hpp"

#include <string>
#include <typeinfo>

/*

Property = abstract interface for readable/writable/typed/governed state
PropertyRef = wrapper around actual member variables
ComputedProperty = wrapper around getter/setter-derived properties
PropertyPath = address of nested properties
Condition = predicate over PropertyPath
Action = mutation over PropertyPath
PropertyMapping = source-to-target transformation for object generation

*/
class Singular;
class Law;

enum class PropertyGovernance {
    Open,       // Accessible to any synthesis
    Gated,      // Requires specific Law or Person authorization
    Universal   // Hard-override by higher-level law making it always accessible
};

class Property {
public:
    virtual ~Property() = default;

    virtual std::string name() const = 0;
    virtual std::string typeName() const = 0;

    // Governance level of this property (default Open)
    virtual PropertyGovernance governance() const { return PropertyGovernance::Open; }
    
    // Check if the property is accessible for set-to-set synthesis
    virtual bool isAccessibleForSynthesis(const Law* activeLaw = nullptr, const Singular* author = nullptr) const {
        (void)activeLaw;
        (void)author;
        if (governance() == PropertyGovernance::Open || governance() == PropertyGovernance::Universal) {
            return true;
        }
        // If Gated, it requires explicit authorization (e.g., from an active Law or an authorized Person).
        if (governance() == PropertyGovernance::Gated) {
            if (activeLaw != nullptr) return true; // Simplify for now: any active Law can bypass
            // Further logic can be added later as the Kernel strict bounds are defined.
        }
        return false;
    }

    // Runtime-generic access — the door the Law system walks through.
    // value() returns monostate when the underlying type is not legible
    // (not a PropertyValue alternative); setValue returns false on type
    // mismatch or on read-only properties.
    virtual PropertyValue value() const = 0;
    virtual bool setValue(const PropertyValue& v) = 0;

    // Non-null when this property's value is itself a Singular — the
    // recursion point PropertyPath descends through for nested addresses.
    virtual Singular* asSingular() const { return nullptr; }
};
