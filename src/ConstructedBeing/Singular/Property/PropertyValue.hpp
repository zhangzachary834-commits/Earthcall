#pragma once

#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <string>
#include <type_traits>
#include <variant>

#include <memory>
#include <vector>
#include <map>
#include <unordered_map>

class Singular;
class Object;
class Relation;
class Formation;

namespace OntoMath {
    class ScalarField;
    class VectorField;
}

struct PropertyList;
struct PropertyDict;

// The typed currency of the property bridge: every legible property value fits
// in this variant. glm::vec3 (not a private Vec3) because the whole codebase
// speaks glm — a local vector type would mean converting at every boundary.
// glm dependency update when moving to more advanced rendering
//
// std::monostate is deliberately the FIRST alternative: a default-constructed
// PropertyValue means "illegible / no value" and never masquerades as int 0.
using PropertyValue = std::variant<
    std::monostate,
    int,
    float,
    double,
    bool,
    char,
    long,
    std::string,
    glm::vec3,
    glm::mat4,
    Singular*,
    Object*,
    Relation*,
    Formation*,
    std::shared_ptr<PropertyList>,
    std::shared_ptr<PropertyDict>,
    std::shared_ptr<OntoMath::ScalarField>,
    std::shared_ptr<OntoMath::VectorField>
>;

// Alternatives admitted to content comparison. Scalars and glm values carry
// their contents directly; list/dictionary graphs use the checked traversal
// below. Raw Singular pointers are not proof that their referent is unchanged.
inline bool isValueComparable(const PropertyValue& v) {
    return std::holds_alternative<int>(v) || std::holds_alternative<float>(v) ||
           std::holds_alternative<double>(v) || std::holds_alternative<bool>(v) ||
           std::holds_alternative<char>(v) || std::holds_alternative<long>(v) ||
           std::holds_alternative<std::string>(v) || std::holds_alternative<glm::vec3>(v) ||
           std::holds_alternative<glm::mat4>(v) ||
           std::holds_alternative<std::shared_ptr<PropertyDict>>(v) ||
           std::holds_alternative<std::shared_ptr<PropertyList>>(v);
}

// "This write changed nothing" — the question every change feed should ask
// before it wakes the world. Content comparison and storage-binding comparison
// are distinct below; the latter also detects equal-valued rebinding.
//
// A WhileTrue law re-writes its result every tick by design: the ambient theme
// sets the same colour, the draw indicator sets the same label, the crystal's
// pulse sets a value that is usually within a hair of the last one. Each of
// those used to notify, and each notification runs markFactDirty, which SCANS
// THE WHOLE FACT LIST. With a fact per property per being and a world that
// grows as a Person draws, that is a per-frame cost rising with the size of
// the world for writes that changed nothing at all. It is the reason the
// Synthesis Studio's controls went dead after a few seconds of drawing.


struct PropertyList {
    std::vector<PropertyValue> elements;
};

struct PropertyDict {
    std::map<std::string, PropertyValue> elements;
};

inline bool propertyValueUnchanged(const PropertyValue& a, const PropertyValue& b) {
    if (a.index() != b.index()) return false;
    if (auto list = std::get_if<std::shared_ptr<PropertyList>>(&a)) {
        if (*list == std::get<std::shared_ptr<PropertyList>>(b)) return true;
    } else if (auto dict = std::get_if<std::shared_ptr<PropertyDict>>(&a)) {
        if (*dict == std::get<std::shared_ptr<PropertyDict>>(b)) return true;
    } else {
        return isValueComparable(a) && a == b;
    }
    // Scratch traversal state beneath the Kernel, not authored cell identity.
    // Avoid the C++ call stack even for deep containers. Distinct cyclic
    // graphs are not proof of an unchanged value: return false conservatively
    // rather than invent cyclic equality or overflow the stack.
    struct Comparison {
        const PropertyValue* a;
        const PropertyValue* b;
        bool finish = false;
    };
    std::vector<Comparison> pending{{&a, &b}};
    std::unordered_map<const void*, std::unordered_map<const void*, bool>> compared;
    while (!pending.empty()) {
        const auto next = pending.back();
        pending.pop_back();
        const auto& left = *next.a;
        const auto& right = *next.b;
        if (left.index() != right.index()) return false;
        if (auto listA = std::get_if<std::shared_ptr<PropertyList>>(&left)) {
            const auto& listB = std::get<std::shared_ptr<PropertyList>>(right);
            if (*listA == listB) continue;
            if (!*listA || !listB || (*listA)->elements.size() != listB->elements.size()) return false;
            auto& pairs = compared[listA->get()];
            if (next.finish) { pairs[listB.get()] = true; continue; }
            const auto found = pairs.find(listB.get());
            if (found != pairs.end()) {
                if (!found->second) return false;
                continue;
            }
            pairs[listB.get()] = false;
            pending.push_back({next.a, next.b, true});
            for (std::size_t i = 0; i < (*listA)->elements.size(); ++i) {
                pending.push_back({&(*listA)->elements[i], &listB->elements[i]});
            }
        } else if (auto dictA = std::get_if<std::shared_ptr<PropertyDict>>(&left)) {
            const auto& dictB = std::get<std::shared_ptr<PropertyDict>>(right);
            if (*dictA == dictB) continue;
            if (!*dictA || !dictB || (*dictA)->elements.size() != dictB->elements.size()) return false;
            auto& pairs = compared[dictA->get()];
            if (next.finish) { pairs[dictB.get()] = true; continue; }
            const auto found = pairs.find(dictB.get());
            if (found != pairs.end()) {
                if (!found->second) return false;
                continue;
            }
            pairs[dictB.get()] = false;
            pending.push_back({next.a, next.b, true});
            for (const auto& [key, value] : (*dictA)->elements) {
                const auto foundValue = dictB->elements.find(key);
                if (foundValue == dictB->elements.end()) return false;
                pending.push_back({&value, &foundValue->second});
            }
        } else if (!isValueComparable(left) || left != right) {
            return false;
        }
    }
    return true;
}

// True when T is one of PropertyValue's alternatives. PropertyRef and
// ComputedProperty use this to decide legibility at compile time.
template <typename T, typename V>
struct is_variant_alternative : std::false_type {};
template <typename T, typename... Ts>
struct is_variant_alternative<T, std::variant<Ts...>>
    : std::bool_constant<(std::is_same_v<T, Ts> || ...)> {};

template <typename T>
inline constexpr bool is_property_value_alternative =
    is_variant_alternative<T, PropertyValue>::value;

// Numeric view of any arithmetic alternative (int/float/double/bool/char/long).
// The shared currency for comparisons, coercion, and Drive-curve inputs.
inline bool propertyValueToNumber(const PropertyValue& v, double& out) {
    return std::visit([&](auto&& x) {
        using X = std::decay_t<decltype(x)>;
        if constexpr (std::is_arithmetic_v<X>) {
            out = static_cast<double>(x);
            return true;
        } else {
            return false;
        }
    }, v);
}

// A container assignment can change its binding even when the contents agree.
// Zach's shared-write/independent-rebinding rule requires preserving that
// distinction. Comparisons of contents still use propertyValueUnchanged.
inline bool propertyStorageUnchanged(const PropertyValue& a, const PropertyValue& b) {
    if (a.index() != b.index()) return false;
    if (auto list = std::get_if<std::shared_ptr<PropertyList>>(&a))
        return *list == std::get<std::shared_ptr<PropertyList>>(b);
    if (auto dict = std::get_if<std::shared_ptr<PropertyDict>>(&a))
        return *dict == std::get<std::shared_ptr<PropertyDict>>(b);
    return propertyValueUnchanged(a, b);
}

// A law Map that would write the value/binding already held is not a write.
// Numeric alternatives compare as numbers so int 1 and double 1.0 agree.
inline bool propertyValuesEquivalent(const PropertyValue& a, const PropertyValue& b) {
    double na = 0.0, nb = 0.0;
    if (propertyValueToNumber(a, na) && propertyValueToNumber(b, nb)) {
        const double scale = std::max(1.0, std::max(std::fabs(na), std::fabs(nb)));
        return std::fabs(na - nb) <= 1e-6 * scale;
    }
    return propertyStorageUnchanged(a, b);
}
