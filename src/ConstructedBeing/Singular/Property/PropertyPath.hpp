#pragma once

#include "PropertyValue.hpp"
#include "Singularity/Core/StringId.hpp"

#include <functional>
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
//      or list/dictionary elements while pinning their storage for this access,
//   3. one final unmatched x|y|z segment on a glm::vec3 property resolves as a
//      component (read the vec3, mutate the component, write it back whole).
//
// **PHASE 4 OPTIMIZATION (Interned Flat Lookup):**
// parse() pre-calculates and interns ALL joined sub-path combinations as
// StringIds, so registered flat lookup needs no traversal allocation.
// Container traversal additionally pins shared storage for the access.
//
// For path "@shape.color.r":
// - segments = ["shape", "color", "r"]  (kept for toString/debug)
// - _joinedIds[0] = [id("shape"), id("shape.color"), id("shape.color.r")]
//
// resolve() now scans _joinedIds with pure integer lookups. A Law firing on
// 500 targets avoids reparsing or deep-copying the addressed values.
// ============================================================================
struct PropertyPath {
    std::vector<std::string> segments;  // Original segments (for toString/debug)

    // Parse a dotted path string and pre-calculate all joined combinations as
    // StringIds. This is the COLD PATH (happens once at Law author time).
    static PropertyPath parse(const std::string& dotted);

    std::string toString() const;
    Earthcall::StringId fullId() const;
    bool empty() const { return segments.empty(); }

private:
    // -------------------------------------------------------------------------
    // Pre-calculated joined sub-path combinations, interned as StringIds
    //
    // For path "shape.color.r" with segments ["shape", "color", "r"]:
    //   _joinedIds[0] = [id("shape"), id("shape.color"), id("shape.color.r")]
    //   _joinedIds[1] = [id("color"), id("color.r")]
    //   _joinedIds[2] = [id("r")]
    //
    // Indexed as: _joinedIds[segmentIndex][runLength - 1]
    //
    // Populated once by parse(). Used by resolve() for zero-allocation lookup.
    // -------------------------------------------------------------------------
    std::vector<std::vector<Earthcall::StringId>> _joinedIds;

public:
    const std::vector<std::vector<Earthcall::StringId>>& joinedIds() const { return _joinedIds; }

    // The deepest Property the path reaches, or nullptr. When the last segment
    // is a vec3 component it is reported through trailingComponent ("x"/"y"/"z")
    // and the returned Property is the vec3 itself.
    //
    // `owner` reports the Singular the returned Property is registered ON,
    // which is NOT `root` once the path descends through a nested Singular
    // ("body.head.position" resolves on the head). Change notification needs
    // that being, not the one the walk started from.
    struct ResolvedSlot {
        Property* prop = nullptr;
        PropertyValue* dynamicSlot = nullptr;
        Singular* owner = nullptr;
        std::string trailingComponent;
        std::string dynamicKey;
        Property* containerProperty = nullptr;
        // Operation-local lifetime pins, beneath the Kernel: a getter may
        // return the only strong reference to a list/dict. These keep the
        // resolved element alive for this access; the PropertyPath holds none.
        std::vector<PropertyValue> containerPins;
        // Operation-local structural view of a typed mathematical value.
        // Writes validate and commit through its original setter/storage;
        // this is a codec window, never a second canonical definition.
        Property* structuredProperty = nullptr;
        PropertyValue* structuredSource = nullptr;
        PropertyValue structuredOriginal;
        PropertyValue structuredView;
    };

    ResolvedSlot resolve(Singular& root, std::size_t startIndex = 0) const;

    enum class PathResult {
        Ok,
        NoSuchProperty,
        TypeMismatch,
        ReadOnly,
        BadComponent,
        Unchanged, // the slot already held this value; not a write
        Unsupported // readable storage whose bridge cannot safely mutate an element
    };

    PathResult getValue(Singular& root, PropertyValue& out, std::size_t startIndex = 0) const;
    PathResult setValue(Singular& root, const PropertyValue& v, std::size_t startIndex = 0) const;
};

// ============================================================================
// DERIVED READINGS (2026-10-19). A read-only observation that an authored
// route carries WITHOUT storing it: `<prefix>.<name>` (and any path into its
// result, `<prefix>.<name>.<key>…`) is answered by the registered reading,
// from the being's own authored properties under `<prefix>`.
//
// Codex placed the authored Integral's evaluation report on the expression's
// existing bearer as a read-only derived PropertyDict, not as a new kind or a
// fixed field on every Singular (2026-10-19 21:30). This is the seam that lets
// an authored route carry one. The rules:
//  · It is consulted only where resolution finds NO stored property, so it
//    never shadows authored or registered state and costs nothing on a hit.
//  · Writes to the route refuse with ReadOnly: an observation is not a dial.
//  · Each read computes afresh, so a refusal replaces a success and no stale
//    report survives a changed input. The result is a fresh value, never
//    shared canonical storage.
//  · It is registered by a modality channel (OntoMath registers "evaluation"),
//    like world readings. Nothing is registered by default.
// Known gap, stated: a derived reading publishes no change facts, so a Rete
// alpha condition does not wake when it changes. A sweep-path law reads it
// fresh.
// ============================================================================
using DerivedReading = std::function<bool(Singular& root, const std::string& prefix, PropertyValue& out)>;
void registerDerivedReading(const std::string& name, DerivedReading reading);
bool hasDerivedReading(const std::string& name);
