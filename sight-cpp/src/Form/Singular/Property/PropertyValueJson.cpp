#include "Form/Singular/Property/PropertyValueJson.hpp"

// Full definitions needed: the pointer alternatives upcast to Singular* for
// identifier extraction, which forward declarations cannot prove.
#include "Form/Singular/Singular.hpp"
#include "Form/Object/Object.hpp"
#include "Form/Object/Formation/Formation.hpp"
#include "Relation/Relation.hpp"

namespace {

nlohmann::json refJson(const Singular* s) {
    return nlohmann::json{{"t", "ref"}, {"id", s ? s->getIdentifier() : ""}};
}

} // namespace

nlohmann::json propertyValueToJson(const PropertyValue& v) {
    return std::visit([](auto&& x) -> nlohmann::json {
        using X = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<X, std::monostate>) {
            return nlohmann::json{{"t", "none"}};
        } else if constexpr (std::is_same_v<X, int>) {
            return nlohmann::json{{"t", "int"}, {"v", x}};
        } else if constexpr (std::is_same_v<X, float>) {
            return nlohmann::json{{"t", "float"}, {"v", x}};
        } else if constexpr (std::is_same_v<X, double>) {
            return nlohmann::json{{"t", "double"}, {"v", x}};
        } else if constexpr (std::is_same_v<X, bool>) {
            return nlohmann::json{{"t", "bool"}, {"v", x}};
        } else if constexpr (std::is_same_v<X, char>) {
            return nlohmann::json{{"t", "char"}, {"v", static_cast<int>(x)}};
        } else if constexpr (std::is_same_v<X, long>) {
            return nlohmann::json{{"t", "long"}, {"v", static_cast<long long>(x)}};
        } else if constexpr (std::is_same_v<X, std::string>) {
            return nlohmann::json{{"t", "string"}, {"v", x}};
        } else if constexpr (std::is_same_v<X, glm::vec3>) {
            return nlohmann::json{{"t", "vec3"}, {"x", x.x}, {"y", x.y}, {"z", x.z}};
        } else {
            // Singular*/Object*/Relation*/Formation* — identity, not value.
            return refJson(static_cast<const Singular*>(x));
        }
    }, v);
}

PropertyValue propertyValueFromJson(const nlohmann::json& j) {
    const std::string t = j.value("t", "none");
    if (t == "int") return PropertyValue(j.value("v", 0));
    if (t == "float") return PropertyValue(j.value("v", 0.0f));
    if (t == "double") return PropertyValue(j.value("v", 0.0));
    if (t == "bool") return PropertyValue(j.value("v", false));
    if (t == "char") return PropertyValue(static_cast<char>(j.value("v", 0)));
    if (t == "long") return PropertyValue(static_cast<long>(j.value("v", 0LL)));
    if (t == "string") return PropertyValue(j.value("v", std::string()));
    if (t == "vec3") {
        return PropertyValue(glm::vec3(j.value("x", 0.0f), j.value("y", 0.0f), j.value("z", 0.0f)));
    }
    // "none" and "ref" (world references resolve through the loader).
    return PropertyValue{};
}
