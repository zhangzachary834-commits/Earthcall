#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"

#include "ConstructedBeing/Singular/Property/Property.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValue.hpp"
#include "ConstructedBeing/Singular/Singular.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Relation/Relation.hpp"
#include "Relation/Formation/Formation.hpp"
#include "Singularity/Core/StringId.hpp"

#include <algorithm>
#include <charconv>
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

Earthcall::StringId PropertyPath::fullId() const {
    if (_joinedIds.empty() || _joinedIds[0].empty()) return Earthcall::StringId();
    return _joinedIds[0].back();
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
// HOT PATH: Registered flat lookups allocate no traversal storage
//
// Uses pre-calculated _joinedIds for pure integer lookups. No string
// allocations. Nested containers additionally pin their shared storage for
// the duration of this access; no value graph is deep-copied.
// ============================================================================
PropertyPath::ResolvedSlot PropertyPath::resolve(Singular& root, std::size_t startIndex) const {
    ResolvedSlot slot;
    if (startIndex >= segments.size() || _joinedIds.size() != segments.size()) return slot;

    Singular* currentOwner = &root;
    Property* currentRegistered = nullptr;
    PropertyValue* currentDynamic = nullptr;
    std::size_t i = startIndex;

    while (i < segments.size()) {
        if (currentOwner) {
            Property* foundReg = nullptr;
            PropertyValue* foundDyn = nullptr;
            std::size_t consumed = 0;
            const auto& idsFromHere = _joinedIds[i];
            for (std::size_t run = idsFromHere.size(); run > 0; --run) {
                if (PropertyValue* candidate = currentOwner->getDynamicPropertyPtr(idsFromHere[run - 1])) {
                    foundDyn = candidate;
                    consumed = run;
                    break;
                }
            }
            if (!foundDyn) {
                for (std::size_t run = idsFromHere.size(); run > 0; --run) {
                    if (Property* candidate = currentOwner->findProperty(idsFromHere[run - 1])) {
                        foundReg = candidate;
                        consumed = run;
                        break;
                    }
                }
            }
            if (!foundReg && !foundDyn) return {};
            slot.owner = currentOwner;
            slot.dynamicKey = foundDyn
                ? Earthcall::StringInterner::resolve(idsFromHere[consumed - 1]) : std::string();
            slot.containerProperty = nullptr;
            currentRegistered = foundReg;
            currentDynamic = foundDyn;
            currentOwner = nullptr;
            i += consumed;
        }

        if (i == segments.size()) {
            slot.prop = currentRegistered;
            slot.dynamicSlot = currentDynamic;
            return slot;
        }

        if (currentRegistered) {
            if (Singular* next = currentRegistered->asSingular()) {
                currentOwner = next;
                currentRegistered = nullptr;
                currentDynamic = nullptr;
                continue;
            }
        }
        // Materialize only the typed view, never a deep copy. Container pins
        // keep elements from a temporary getter alive until this access ends.
        PropertyValue value = currentRegistered ? currentRegistered->value() : *currentDynamic;
        if (!currentOwner) {
            if (auto next = std::get_if<Singular*>(&value)) currentOwner = *next;
            else if (auto next = std::get_if<Object*>(&value)) currentOwner = static_cast<Singular*>(*next);
            else if (auto next = std::get_if<Relation*>(&value)) currentOwner = static_cast<Singular*>(*next);
            else if (auto next = std::get_if<Formation*>(&value)) currentOwner = static_cast<Singular*>(*next);
        }
        if (currentOwner) {
            currentRegistered = nullptr;
            currentDynamic = nullptr;
            continue;
        }

        if (i == segments.size() - 1 && std::holds_alternative<glm::vec3>(value) &&
            isVec3Component(segments[i])) {
            slot.prop = currentRegistered;
            slot.dynamicSlot = currentDynamic;
            slot.trailingComponent = segments[i];
            return slot;
        }

        PropertyValue* element = nullptr;
        if (auto dict = std::get_if<std::shared_ptr<PropertyDict>>(&value); dict && *dict) {
            const auto found = (*dict)->elements.find(segments[i]);
            if (found == (*dict)->elements.end()) return {};
            element = &found->second;
        } else if (auto list = std::get_if<std::shared_ptr<PropertyList>>(&value); list && *list) {
            std::size_t index = 0;
            const std::string& text = segments[i];
            const auto parsed = std::from_chars(text.data(), text.data() + text.size(), index);
            if (text.empty() || parsed.ec != std::errc() || parsed.ptr != text.data() + text.size() ||
                index >= (*list)->elements.size()) return {};
            element = &(*list)->elements[index];
        } else {
            return {};
        }
        if (currentRegistered) slot.containerProperty = currentRegistered;
        slot.containerPins.push_back(std::move(value));
        currentRegistered = nullptr;
        currentDynamic = element;
        ++i;
        if (i == segments.size()) {
            slot.dynamicSlot = currentDynamic;
            return slot;
        }
    }
    return {};
}

PropertyPath::PathResult PropertyPath::getValue(Singular& root, PropertyValue& out, std::size_t startIndex) const {
    if (startIndex >= segments.size()) {
        out = PropertyValue{};
        return PathResult::NoSuchProperty;
    }
    ResolvedSlot slot = resolve(root, startIndex);

    if (!slot.prop && !slot.dynamicSlot) {
        if (segments.size() - startIndex == 1) {
            if (root.getDynamicProperty(segments[startIndex], out)) return PathResult::Ok;
        }
        out = PropertyValue{};
        return PathResult::NoSuchProperty;
    }

    PropertyValue v;
    if (slot.prop) v = slot.prop->value();
    else if (slot.dynamicSlot) v = *slot.dynamicSlot;

    if (slot.trailingComponent.empty()) {
        out = std::move(v);
        return !std::holds_alternative<std::monostate>(out) ? PathResult::Ok : PathResult::NoSuchProperty;
    }

    glm::vec3* vec = std::get_if<glm::vec3>(&v);
    if (!vec) {
        out = PropertyValue{};
        return PathResult::BadComponent;
    }
    out = PropertyValue(*componentOf(*vec, slot.trailingComponent));
    return PathResult::Ok;
}

PropertyPath::PathResult PropertyPath::setValue(Singular& root, const PropertyValue& v, std::size_t startIndex) const {
    if (startIndex >= segments.size()) return PathResult::NoSuchProperty;
    ResolvedSlot slot = resolve(root, startIndex);

    // A read-only wrapper refuses even when the proposed value is identical.
    // Equality is a value comparison, not authority to attempt a write.
    if (slot.prop && !slot.prop->isStructurallyWritable()) return PathResult::ReadOnly;
    if (slot.containerProperty) {
        if (!slot.containerProperty->isStructurallyWritable()) return PathResult::ReadOnly;
        if (!slot.containerProperty->exposesMutableContainer()) return PathResult::Unsupported;
    }

    const auto announce = [&](PathResult result, Property* prop, Singular* on, const std::string& fallbackName = "") {
        if (result == PathResult::Ok && on) {
            Singular::notifyPropertyChanged(on, prop ? prop->name() : fallbackName);
        }
        return result;
    };

        if (!slot.prop && !slot.dynamicSlot) {
        if (segments.size() - startIndex == 1) {
            PropertyValue cur;
            if (root.getDynamicProperty(segments[startIndex], cur) && propertyValuesEquivalent(cur, v)) {
                return PathResult::Unchanged;
            }
            if (root.setDynamicProperty(segments[startIndex], v)) {
                return PathResult::Ok; // setDynamicProperty already called notifyPropertyChanged
            }
            return PathResult::TypeMismatch;
        }
        return PathResult::NoSuchProperty;
    }

    if (slot.trailingComponent.empty()) {
        PropertyValue currentVal = slot.prop ? slot.prop->value() : *slot.dynamicSlot;
        if (propertyValuesEquivalent(currentVal, v)) return PathResult::Unchanged;
        
        if (slot.prop) {
            if (slot.prop->setValue(v)) return announce(PathResult::Ok, slot.prop, slot.owner);
            double n = 0.0;
            PropertyValue coerced;
            if (propertyValueToNumber(v, n) && coerceLike(currentVal, n, coerced)) {
                if (propertyValuesEquivalent(currentVal, coerced)) return PathResult::Unchanged;
                if (slot.prop->setValue(coerced)) return announce(PathResult::Ok, slot.prop, slot.owner);
            }
            if (currentVal.index() != v.index()) return PathResult::TypeMismatch;
            return PathResult::ReadOnly;
                        } else if (slot.dynamicSlot) {
            double n = 0.0;
            PropertyValue coerced;
            bool coerceSuccess = propertyValueToNumber(v, n) && coerceLike(*slot.dynamicSlot, n, coerced);
            const PropertyValue& valToWrite = coerceSuccess ? coerced : v;

            if (propertyValuesEquivalent(*slot.dynamicSlot, valToWrite)) return PathResult::Unchanged;

            if (slot.dynamicSlot == slot.owner->getDynamicPropertyPtr(Earthcall::StringInterner::intern(slot.dynamicKey))) {
                if (slot.owner->setDynamicProperty(Earthcall::StringInterner::intern(slot.dynamicKey), valToWrite)) {
                    return PathResult::Ok;
                }
            } else {
                *slot.dynamicSlot = valToWrite;
                announce(PathResult::Ok, slot.containerProperty, slot.owner, slot.dynamicKey);
                return PathResult::Ok;
            }
            return PathResult::ReadOnly;
        }
    }

    // Component write
    double n = 0.0;
    if (!propertyValueToNumber(v, n)) return PathResult::TypeMismatch;
    
    PropertyValue whole = slot.prop ? slot.prop->value() : *slot.dynamicSlot;
    glm::vec3* vec = std::get_if<glm::vec3>(&whole);
    if (!vec) return PathResult::BadComponent;
    
    float& lane = *componentOf(*vec, slot.trailingComponent);
    if (std::fabs(static_cast<double>(lane) - n) <= 1e-6 * std::max(1.0, std::fabs(n))) {
        return PathResult::Unchanged;
    }
    lane = static_cast<float>(n);
    
    if (slot.prop) {
        if (slot.prop->setValue(PropertyValue(*vec))) return announce(PathResult::Ok, slot.prop, slot.owner);
        return PathResult::ReadOnly;
            } else if (slot.dynamicSlot) {
        if (slot.dynamicSlot == slot.owner->getDynamicPropertyPtr(Earthcall::StringInterner::intern(slot.dynamicKey))) {
            if (slot.owner->setDynamicProperty(Earthcall::StringInterner::intern(slot.dynamicKey), PropertyValue(*vec))) {
                return PathResult::Ok;
            }
        } else {
            *slot.dynamicSlot = PropertyValue(*vec);
            announce(PathResult::Ok, slot.containerProperty, slot.owner, slot.dynamicKey);
            return PathResult::Ok;
        }
        return PathResult::ReadOnly;
    }
    return PathResult::NoSuchProperty;
}
