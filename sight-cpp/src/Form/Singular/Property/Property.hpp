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

class Property {
public:
    virtual ~Property() = default;

    virtual std::string name() const = 0;
    virtual std::string typeName() const = 0;

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
