#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"

#include "ConstructedBeing/Singular/Property/Property.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValue.hpp"
#include "ConstructedBeing/Singular/Singular.hpp"
#include "Singularity/Core/StringId.hpp"

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <utility>
#include <variant>

namespace {

// Re-express n in the same alternative `like` currently holds, so a float
// arriving for a double slot (or an int for a float slot) still lands.
bool coerceLike(const PropertyValue& like, double n, PropertyValue& out) {
    return std::visit([&](auto&& t) {
        using X = std::decay_t<decltype(t)>;
        if constexpr (std::is_arithmetic_v<X>) {
            out = PropertyValue(static_cast<X>(n));
            return true;
        } else {
            return false;
        }
    }, like);
}

float* componentOf(glm::vec3& v, const std::string& c) {
    // r/g/b alias x/y/z so color-shaped vectors read naturally ("color.g").
    if (c == "x" || c == "r") return &v.x;
    if (c == "y" || c == "g") return &v.y;
    if (c == "z" || c == "b") return &v.z;
    return nullptr;
}

bool isVec3Component(const std::string& c) {
    return c == "x" || c == "y" || c == "z" || c == "r" || c == "g" || c == "b";
}

} // namespace

// ============================================================================
// COLD PATH: Parse and pre-calculate all joined combinations
//
// This happens ONCE at Law author time (when the Law text is compiled).
// We intern every possible joined combination as StringIds, so resolve()
// never allocates strings.
//
// Example: "shape.color.r" → segments ["shape", "color", "r"]
//
// Pre-calculate:
//   From index 0: "shape", "shape.color", "shape.color.r"
//   From index 1:          "color",       "color.r"
//   From index 2:                         "r"
//
// Store as _joinedIds[segmentIndex][runLength - 1]
// ============================================================================
PropertyPath PropertyPath::parse(const std::string& dotted) {
    PropertyPath path;
    std::string current;

    // Parse segments (unchanged)
    for (char ch : dotted) {
        if (ch == '.') {
            if (!current.empty()) path.segments.push_back(current);
            current.clear();
        } else {
            current += ch;
        }
    }
    if (!current.empty()) path.segments.push_back(current);

    // Pre-calculate all joined combinations and intern as StringIds
    path._joinedIds.resize(path.segments.size());
    for (std::size_t i = 0; i < path.segments.size(); ++i) {
        std::string joined;
        for (std::size_t j = i; j < path.segments.size(); ++j) {
            if (j > i) joined += '.';
            joined += path.segments[j];

            // Intern this combination
            Earthcall::StringId id = Earthcall::StringInterner::intern(joined);
            path._joinedIds[i].push_back(id);
        }
    }

    return path;
}

std::string PropertyPath::toString() const {
    std::string joined;
    for (std::size_t i = 0; i < segments.size(); ++i) {
        if (i) joined += '.';
        joined += segments[i];
    }
    return joined;
}

// ============================================================================
// HOT PATH: Resolve with zero allocations
//
// Uses pre-calculated _joinedIds for pure integer lookups. No string
// allocations, no string comparisons. When a Law fires on 500 targets,
// this runs 500 times with ZERO heap allocations.
// ============================================================================
Property* PropertyPath::resolve(Singular& root, std::string* trailingComponent,
                                Singular** owner) const {
    if (trailingComponent) trailingComponent->clear();
    if (owner) *owner = nullptr;
    if (segments.empty()) return nullptr;

    Singular* current = &root;
    std::size_t i = 0;
    while (current && i < segments.size()) {
        // Longest dotted-name match against this Singular's registry.
        // Use pre-calculated StringIds for zero-allocation lookup.
        Property* found = nullptr;
        std::size_t consumed = 0;

        // Scan the pre-calculated IDs for this starting index
        const auto& idsFromHere = _joinedIds[i];
        for (std::size_t runLength = 1; runLength <= idsFromHere.size(); ++runLength) {
            Earthcall::StringId id = idsFromHere[runLength - 1];
            if (Property* candidate = current->findProperty(id)) {
                found = candidate;
                consumed = runLength;
            }
        }

        if (!found) return nullptr;
        i += consumed;

        if (owner) *owner = current;
        if (i == segments.size()) return found;

        // Descend into a nested Singular.
        if (Singular* next = found->asSingular()) {
            current = next;
            continue;
        }

        // Trailing vec3 component ("position.y").
        if (i == segments.size() - 1 && trailingComponent &&
            std::holds_alternative<glm::vec3>(found->value())) {
            const std::string& c = segments[i];
            if (isVec3Component(c)) {
                *trailingComponent = c;
                return found;
            }
        }
        return nullptr;
    }
    return nullptr;
}

PropertyPath::PathResult PropertyPath::getValue(Singular& root, PropertyValue& out) const {
    std::string component;
    Property* property = resolve(root, &component);
    if (!property) {
        if (segments.size() == 1) {
            if (root.getDynamicProperty(segments[0], out)) {
                return PathResult::Ok;
            }
        }
        return PathResult::NoSuchProperty;
    }

    PropertyValue v = property->value();
    if (component.empty()) {
        out = std::move(v);
        return !std::holds_alternative<std::monostate>(out) ? PathResult::Ok : PathResult::NoSuchProperty;
    }

    glm::vec3* vec = std::get_if<glm::vec3>(&v);
    if (!vec) return PathResult::BadComponent;
    out = PropertyValue(*componentOf(*vec, component));
    return PathResult::Ok;
}

PropertyPath::PathResult PropertyPath::setValue(Singular& root, const PropertyValue& v) const {
    std::string component;
    Singular* owner = nullptr;
    Property* property = resolve(root, &component, &owner);

    // ------------------------------------------------------------------
    // EVERY successful write announces itself, from here.
    //
    // PropertyRef::set was the only place in the engine that called
    // notifyPropertyChanged — so a property backed by anything ELSE was
    // invisible to the change feed the Rete's dirty tracking is built on.
    // That is not a corner: Object's `position` and `rotation` live in the
    // transform matrix and are ComputedProperty; shape parameters, face
    // colours, patch controls and every Relation property go through
    // hand-written Property bridges; authored properties live in the dynamic
    // map. None of them ever marked a fact dirty. A WhileTrue law watching
    // `position.y` therefore matched only beings that ALREADY satisfied it
    // when the network first met them, and went permanently deaf to anything
    // that moved afterwards — silently, because the law was still registered,
    // still enabled, still compiled, and its alpha memory simply stayed empty.
    //
    // The fix belongs HERE rather than in each Property subclass: this is the
    // one seam every path-addressed write passes through, whatever backs the
    // slot, so a new bridge cannot forget to announce itself. PropertyRef
    // keeps its own notify for typed set() calls that never touch a path; a
    // duplicate notification is harmless (markFactDirty is idempotent).
    //
    // What this still does NOT catch, stated plainly: a direct C++ setter
    // (`obj.setPosition(...)`) writes the transform without going through the
    // property vocabulary at all. That was always outside the property layer's
    // reach — it is the boundary, not an oversight — and the per-frame world
    // seeding is what keeps such writes from being lost entirely.
    // ------------------------------------------------------------------
    const auto announce = [&](PathResult result, Property* prop, Singular* on) {
        if (result == PathResult::Ok && prop && on) {
            Singular::notifyPropertyChanged(on, prop->name());
        }
        return result;
    };

    if (!property) {
        if (segments.size() == 1) {
            PropertyValue cur;
            if (root.getDynamicProperty(segments[0], cur) &&
                propertyValuesEquivalent(cur, v)) {
                return PathResult::Unchanged;
            }
            root.setDynamicProperty(segments[0], v);   // announces from there
            return PathResult::Ok;
        }
        return PathResult::NoSuchProperty;
    }

    if (component.empty()) {
        if (propertyValuesEquivalent(property->value(), v)) return PathResult::Unchanged;
        if (property->setValue(v)) return announce(PathResult::Ok, property, owner);
        // Arithmetic coercion retry: match the alternative the slot holds.
        double n = 0.0;
        PropertyValue coerced;
        if (propertyValueToNumber(v, n) && coerceLike(property->value(), n, coerced)) {
            if (propertyValuesEquivalent(property->value(), coerced)) {
                return PathResult::Unchanged;
            }
            if (property->setValue(coerced)) return announce(PathResult::Ok, property, owner);
        }
        // If setValue fails, we'll assume it's because the property rejected it, likely read-only or type mismatch.
        // For now, if types could coerce, it's ReadOnly. If not, it's TypeMismatch.
        if (property->value().index() != v.index()) return PathResult::TypeMismatch;
        return PathResult::ReadOnly;
    }

    // Component write: read the whole vec3, mutate one lane, write back whole
    // (so setters like Object::setPosition run their full side effects).
    double n = 0.0;
    if (!propertyValueToNumber(v, n)) return PathResult::TypeMismatch;
    PropertyValue whole = property->value();
    glm::vec3* vec = std::get_if<glm::vec3>(&whole);
    if (!vec) return PathResult::BadComponent;
    float& lane = *componentOf(*vec, component);
    if (std::fabs(static_cast<double>(lane) - n) <= 1e-6 * std::max(1.0, std::fabs(n))) {
        return PathResult::Unchanged;
    }
    lane = static_cast<float>(n);
    if (property->setValue(PropertyValue(*vec))) return announce(PathResult::Ok, property, owner);
    return PathResult::ReadOnly;
}
