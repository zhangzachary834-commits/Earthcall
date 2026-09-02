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

// Is this value one whose EQUALITY is the whole story?
//
// The scalars, the string, and the two glm types hold their content directly,
// so comparing two of them answers "did anything change". The pointer and
// shared_ptr alternatives do not: two equal pointers can address contents that
// were mutated in place, and answering "unchanged" for those would lose a real
// change. So the test is deliberately conservative — it says yes only where a
// yes is provable.
inline bool isValueComparable(const PropertyValue& v) {
    return std::holds_alternative<int>(v) || std::holds_alternative<float>(v) ||
           std::holds_alternative<double>(v) || std::holds_alternative<bool>(v) ||
           std::holds_alternative<char>(v) || std::holds_alternative<long>(v) ||
           std::holds_alternative<std::string>(v) || std::holds_alternative<glm::vec3>(v) ||
           std::holds_alternative<glm::mat4>(v);
}

// "This write changed nothing" — the question every change feed should ask
// before it wakes the world.
//
// A WhileTrue law re-writes its result every tick by design: the ambient theme
// sets the same colour, the draw indicator sets the same label, the crystal's
// pulse sets a value that is usually within a hair of the last one. Each of
// those used to notify, and each notification runs markFactDirty, which SCANS
// THE WHOLE FACT LIST. With a fact per property per being and a world that
// grows as a Person draws, that is a per-frame cost rising with the size of
// the world for writes that changed nothing at all. It is the reason the
// Synthesis Studio's controls went dead after a few seconds of drawing.
inline bool propertyValueUnchanged(const PropertyValue& a, const PropertyValue& b) {
    if (a.index() != b.index()) return false;
    if (!isValueComparable(a)) return false;
    return a == b;
}

struct PropertyList {
    std::vector<PropertyValue> elements;
};

struct PropertyDict {
    std::map<std::string, PropertyValue> elements;
};

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

// A law Map that would write the value already held is not a write.
// Numeric alternatives compare as numbers so int 1 and double 1.0 agree.
inline bool propertyValuesEquivalent(const PropertyValue& a, const PropertyValue& b) {
    double na = 0.0, nb = 0.0;
    if (propertyValueToNumber(a, na) && propertyValueToNumber(b, nb)) {
        const double scale = std::max(1.0, std::max(std::fabs(na), std::fabs(nb)));
        return std::fabs(na - nb) <= 1e-6 * scale;
    }
    return a.index() == b.index() && a == b;
}
