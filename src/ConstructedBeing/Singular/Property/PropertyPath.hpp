#pragma once

#include "PropertyValue.hpp"
#include "Singularity/Core/StringId.hpp"

#include <string>
#include <vector>

class Property;
class Singular;

// ============================================================================
// PropertyPath — The address of a variable on the substrate
//
// Examples: "position.y", "shape.majorR", "body.head.position"
//
// Resolution order at each level:
//   1. longest dotted-name match in the Singular's registry (supports flat
//      registration like "shape.r"),
//   2. descend into nested Singulars via Property::asSingular(),
//   3. one final unmatched x|y|z segment on a glm::vec3 property resolves as a
//      component (read the vec3, mutate the component, write it back whole).
//
// **PHASE 4 OPTIMIZATION (Zero-Allocation Resolve):**
// parse() pre-calculates and interns ALL joined sub-path combinations as
// StringIds, so resolve() performs zero allocations and zero string comparisons.
//
// For path "@shape.color.r":
// - segments = ["shape", "color", "r"]  (kept for toString/debug)
// - _joinedIds = [id("shape"), id("shape.color"), id("shape.color.r")]
//
// resolve() now scans _joinedIds with pure integer lookups. A Law firing on
// 500 targets performs 500 zero-allocation property lookups.
// ============================================================================
struct PropertyPath {
    std::vector<std::string> segments;  // Original segments (for toString/debug)

    // Parse a dotted path string and pre-calculate all joined combinations as
    // StringIds. This is the COLD PATH (happens once at Law author time).
    static PropertyPath parse(const std::string& dotted);

    std::string toString() const;
    bool empty() const { return segments.empty(); }

private:
    // -------------------------------------------------------------------------
    // Pre-calculated joined sub-path combinations, interned as StringIds
    //
    // For path "shape.color.r" with segments ["shape", "color", "r"]:
    //   _joinedIds[0] = [id("shape")]
    //   _joinedIds[1] = [id("shape"), id("shape.color")]
    //   _joinedIds[2] = [id("shape"), id("shape.color"), id("shape.color.r")]
    //
    // Indexed as: _joinedIds[segmentIndex][runLength - 1]
    //
    // Populated once by parse(). Used by resolve() for zero-allocation lookup.
    // -------------------------------------------------------------------------
    std::vector<std::vector<Earthcall::StringId>> _joinedIds;

public:

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
