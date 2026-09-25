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
// HOT PATH: Resolve with zero allocations
//
// Uses pre-calculated _joinedIds for pure integer lookups. No string
// allocations, no string comparisons. When a Law fires on 500 targets,
// this runs 500 times with ZERO heap allocations.
// ============================================================================
PropertyPath::ResolvedSlot PropertyPath::resolve(Singular& root, std::size_t startIndex) const {
    ResolvedSlot slot;
    if (segments.empty() || startIndex >= segments.size()) return slot;

    Singular* currentOwner = &root;
    Property* currentRegistered = nullptr;
    PropertyValue* currentDynamic = nullptr;
    std::size_t i = startIndex;

    while (i < segments.size()) {
        if (currentOwner) {
            Property* foundReg = nullptr;
            std::size_t consumed = 0;
            const auto& idsFromHere = _joinedIds[i];
            
            PropertyValue* foundDyn = nullptr;
            for (std::size_t runLength = idsFromHere.size(); runLength > 0; --runLength) {
                Earthcall::StringId id = idsFromHere[runLength - 1];
                if (PropertyValue* candidate = currentOwner->getDynamicPropertyPtr(id)) {
                    foundDyn = candidate;
                    consumed = runLength;
                    break;
                }
            }

            if (!foundDyn) {
                for (std::size_t runLength = idsFromHere.size(); runLength > 0; --runLength) {
                    Earthcall::StringId id = idsFromHere[runLength - 1];
                    if (Property* candidate = currentOwner->findProperty(id)) {
                        foundReg = candidate;
                        consumed = runLength;
                        break;
                    }
                }
            }

            if (!foundReg && !foundDyn) return slot;

            i += consumed;
            slot.owner = currentOwner;
            if (foundDyn) {
                slot.dynamicKey = Earthcall::StringInterner::resolve(_joinedIds[i - consumed][consumed - 1]);
            }
            currentRegistered = foundReg;
            currentDynamic = foundDyn;
            currentOwner = nullptr;

        } else if (currentDynamic) {
            std::shared_ptr<PropertyDict>* pDict = std::get_if<std::shared_ptr<PropertyDict>>(currentDynamic);
            if (pDict && *pDict) {
                auto it = (*pDict)->elements.find(segments[i]);
                if (it != (*pDict)->elements.end()) {
                    currentDynamic = &it->second;
                    currentRegistered = nullptr;
                    i++;
                } else {
                    return slot;
                }
            } else {
                std::shared_ptr<PropertyList>* pList = std::get_if<std::shared_ptr<PropertyList>>(currentDynamic);
                if (pList && *pList) {
                    try {
                        std::size_t idx = std::stoull(segments[i]);
                        if (idx < (*pList)->elements.size()) {
                            currentDynamic = &(*pList)->elements[idx];
                            currentRegistered = nullptr;
                            i++;
                        } else {
                            return slot;
                        }
                    } catch (...) {
                        return slot;
                    }
                } else {
                    break;
                }
            }
        } else {
            return slot;
        }

        if (i == segments.size()) {
            slot.prop = currentRegistered;
            slot.dynamicSlot = currentDynamic;
            return slot;
        }

        PropertyValue val;
        if (currentRegistered) {
            if (Singular* next = currentRegistered->asSingular()) {
                currentOwner = next;
                currentRegistered = nullptr;
                currentDynamic = nullptr;
                continue;
            }
            val = currentRegistered->value();
        } else if (currentDynamic) {
            val = *currentDynamic;
        }

        if (Singular** nextSingular = std::get_if<Singular*>(&val)) {
            if (*nextSingular) { currentOwner = *nextSingular; currentRegistered = nullptr; currentDynamic = nullptr; }
        } else if (Object** nextObj = std::get_if<Object*>(&val)) {
            if (*nextObj) { currentOwner = reinterpret_cast<Singular*>(*nextObj); currentRegistered = nullptr; currentDynamic = nullptr; }
        } else if (Relation** nextRel = std::get_if<Relation*>(&val)) {
            if (*nextRel) { currentOwner = reinterpret_cast<Singular*>(*nextRel); currentRegistered = nullptr; currentDynamic = nullptr; }
        } else if (Formation** nextForm = std::get_if<Formation*>(&val)) {
            if (*nextForm) { currentOwner = reinterpret_cast<Singular*>(*nextForm); currentRegistered = nullptr; currentDynamic = nullptr; }
        }

        if (i == segments.size() - 1 && std::holds_alternative<glm::vec3>(val)) {
            const std::string& c = segments[i];
            if (isVec3Component(c)) {
                slot.prop = currentRegistered;
                slot.dynamicSlot = currentDynamic;
                slot.trailingComponent = c;
                return slot;
            }
        }
    }
    return slot;
}

PropertyPath::PathResult PropertyPath::getValue(Singular& root, PropertyValue& out, std::size_t startIndex) const {
    ResolvedSlot slot = resolve(root, startIndex);
    
    if (!slot.prop && !slot.dynamicSlot) {
        if (segments.size() - startIndex == 1) {
            if (root.getDynamicProperty(segments[startIndex], out)) return PathResult::Ok;
        }
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
    if (!vec) return PathResult::BadComponent;
    out = PropertyValue(*componentOf(*vec, slot.trailingComponent));
    return PathResult::Ok;
}

PropertyPath::PathResult PropertyPath::setValue(Singular& root, const PropertyValue& v, std::size_t startIndex) const {
    ResolvedSlot slot = resolve(root, startIndex);

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
                if (!slot.dynamicKey.empty()) {
                    slot.owner->notifyPropertyChanged(slot.owner, slot.dynamicKey);
                }
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
            if (!slot.dynamicKey.empty()) {
                slot.owner->notifyPropertyChanged(slot.owner, slot.dynamicKey);
            }
            return PathResult::Ok;
        }
        return PathResult::ReadOnly;
    }
    return PathResult::NoSuchProperty;
}
