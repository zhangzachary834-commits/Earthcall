#pragma once

#include "PropertyValue.hpp"
#include <string>
#include <memory>

struct ConditionNode;

// A bounded data structure owned by a Singular.
// Used to store person-originated properties, arbitrarily flexible lists/dicts,
// and enforce constraints (bounds) on when and by whom they can be modified.
class DataStructure {
public:
    std::string name;
    
    // The actual data (typically a std::shared_ptr<PropertyDict> or PropertyList, 
    // but can be any PropertyValue alternative).
    PropertyValue data;
    
    // Bounds defining who/what can modify this data structure.
    // Represents "Person-authored constraints on the conditions upon which 
    // the list of properties may be changed."
    std::shared_ptr<ConditionNode> writeBounds;

    DataStructure() = default;
    DataStructure(const std::string& n, const PropertyValue& v) 
        : name(n), data(v) {}
};
