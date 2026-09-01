#pragma once

#include "PropertyValue.hpp"

#include <string>
#include <vector>

class Property;
class Singular;

// The address of a variable on the substrate: "position.y", "shape.majorR".
//
// Resolution order at each level:
//   1. longest dotted-name match in the Singular's registry (supports flat
//      registration like "shape.r"),
//   2. descend into nested Singulars via Property::asSingular(),
//   3. one final unmatched x|y|z segment on a glm::vec3 property resolves as a
//      component (read the vec3, mutate the component, write it back whole).
struct PropertyPath {
    std::vector<std::string> segments;

    static PropertyPath parse(const std::string& dotted);
    std::string toString() const;
    bool empty() const { return segments.empty(); }

    // The deepest Property the path reaches, or nullptr. When the last segment
    // is a vec3 component it is reported through trailingComponent ("x"/"y"/"z")
    // and the returned Property is the vec3 itself.
    //
    // `owner` reports the Singular the returned Property is registered ON,
    // which is NOT `root` once the path descends through a nested Singular
    // ("body.head.position" resolves on the head). Change notification needs
    // that being, not the one the walk started from.
    Property* resolve(Singular& root, std::string* trailingComponent = nullptr,
                      Singular** owner = nullptr) const;

    enum class PathResult {
        Ok,
        NoSuchProperty,
        TypeMismatch,
        ReadOnly,
        BadComponent,
        Unchanged  // the slot already held this value; not a write
    };

    PathResult getValue(Singular& root, PropertyValue& out) const;
    PathResult setValue(Singular& root, const PropertyValue& v) const;
};
