#include "Form/Singular/Property/PropertyPath.hpp"

#include "Form/Singular/Property/Property.hpp"
#include "Form/Singular/Singular.hpp"

#include <glm/glm.hpp>
#include <utility>
#include <variant>

namespace {

// Numeric view of any arithmetic alternative (int/float/double/bool/char/long).
bool numericValue(const PropertyValue& v, double& out) {
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
    if (c == "x") return &v.x;
    if (c == "y") return &v.y;
    if (c == "z") return &v.z;
    return nullptr;
}

} // namespace

PropertyPath PropertyPath::parse(const std::string& dotted) {
    PropertyPath path;
    std::string current;
    for (char ch : dotted) {
        if (ch == '.') {
            if (!current.empty()) path.segments.push_back(current);
            current.clear();
        } else {
            current += ch;
        }
    }
    if (!current.empty()) path.segments.push_back(current);
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

Property* PropertyPath::resolve(Singular& root, std::string* trailingComponent) const {
    if (trailingComponent) trailingComponent->clear();
    if (segments.empty()) return nullptr;

    Singular* current = &root;
    std::size_t i = 0;
    while (current && i < segments.size()) {
        // Longest dotted-name match against this Singular's registry.
        Property* found = nullptr;
        std::size_t consumed = 0;
        std::string joined;
        for (std::size_t j = i; j < segments.size(); ++j) {
            if (j > i) joined += '.';
            joined += segments[j];
            if (Property* candidate = current->findProperty(joined)) {
                found = candidate;
                consumed = j + 1 - i;
            }
        }
        if (!found) return nullptr;
        i += consumed;

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
            if (c == "x" || c == "y" || c == "z") {
                *trailingComponent = c;
                return found;
            }
        }
        return nullptr;
    }
    return nullptr;
}

bool PropertyPath::getValue(Singular& root, PropertyValue& out) const {
    std::string component;
    Property* property = resolve(root, &component);
    if (!property) return false;

    PropertyValue v = property->value();
    if (component.empty()) {
        out = std::move(v);
        return !std::holds_alternative<std::monostate>(out);
    }

    glm::vec3* vec = std::get_if<glm::vec3>(&v);
    if (!vec) return false;
    out = PropertyValue(*componentOf(*vec, component));
    return true;
}

bool PropertyPath::setValue(Singular& root, const PropertyValue& v) const {
    std::string component;
    Property* property = resolve(root, &component);
    if (!property) return false;

    if (component.empty()) {
        if (property->setValue(v)) return true;
        // Arithmetic coercion retry: match the alternative the slot holds.
        double n = 0.0;
        PropertyValue coerced;
        if (numericValue(v, n) && coerceLike(property->value(), n, coerced)) {
            return property->setValue(coerced);
        }
        return false;
    }

    // Component write: read the whole vec3, mutate one lane, write back whole
    // (so setters like Object::setPosition run their full side effects).
    double n = 0.0;
    if (!numericValue(v, n)) return false;
    PropertyValue whole = property->value();
    glm::vec3* vec = std::get_if<glm::vec3>(&whole);
    if (!vec) return false;
    *componentOf(*vec, component) = static_cast<float>(n);
    return property->setValue(PropertyValue(*vec));
}
