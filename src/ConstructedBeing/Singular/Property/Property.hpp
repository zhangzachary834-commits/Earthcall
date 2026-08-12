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

// THERE IS ONE GATE, AND IT IS NOT HERE.
//
// A `PropertyGovernance` enum used to sit at this spot with an
// `isAccessibleForSynthesis` beside it, and it was a second, weaker answer to
// a question `Singularity/TransferPolicy` already answers: which properties
// set-to-set creation may take. Weaker in every particular — its Gated tier
// was bypassed by "any active Law", it had no Kernel floor that laws cannot
// close, it was not itself a legible Singular so no law could govern it, and
// it did not persist. Two permission systems that disagree are not twice the
// governance; they are the absence of it, because the answer depends on which
// one you happen to ask.
//
// Permissions root at Singularity. Ask TransferPolicy.

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
