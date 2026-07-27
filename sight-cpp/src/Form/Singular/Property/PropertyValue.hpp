#pragma once

#include <glm/glm.hpp>
#include <string>
#include <type_traits>
#include <variant>

class Singular;
class Object;
class Relation;
class Formation;

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
    Formation*
>;

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
