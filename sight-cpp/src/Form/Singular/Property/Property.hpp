#pragma once

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
};