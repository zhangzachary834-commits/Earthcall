#include "ConstructedBeing/Singular/Property/PropertyValueJson.hpp"

// Full definitions needed: the pointer alternatives upcast to Singular* for
// identifier extraction, which forward declarations cannot prove.
#include "ConstructedBeing/Singular/Singular.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Relation/Formation/Formation.hpp"
#include "Relation/Relation.hpp"
#include "Singularity/OntoMath/Field.hpp"

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
        } else if constexpr (std::is_same_v<X, glm::mat4>) {
            nlohmann::json m = nlohmann::json::array();
            for (int c = 0; c < 4; ++c) {
                for (int r = 0; r < 4; ++r) {
                    m.push_back(x[c][r]);
                }
            }
            return nlohmann::json{{"t", "mat4"}, {"m", m}};
        } else if constexpr (std::is_same_v<X, std::shared_ptr<PropertyList>>) {
            nlohmann::json arr = nlohmann::json::array();
            if (x) {
                for (const auto& el : x->elements) {
                    arr.push_back(propertyValueToJson(el));
                }
            }
            return nlohmann::json{{"t", "list"}, {"v", arr}};
        } else if constexpr (std::is_same_v<X, std::shared_ptr<PropertyDict>>) {
            nlohmann::json obj = nlohmann::json::object();
            if (x) {
                for (const auto& [k, val] : x->elements) {
                    obj[k] = propertyValueToJson(val);
                }
            }
            return nlohmann::json{{"t", "dict"}, {"v", obj}};
        } else if constexpr (std::is_same_v<X, std::shared_ptr<OntoMath::ScalarField>>) {
            return nlohmann::json{{"t", "scalar_field"}};
        } else if constexpr (std::is_same_v<X, std::shared_ptr<OntoMath::VectorField>>) {
            return nlohmann::json{{"t", "vector_field"}};
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
    if (t == "mat4") {
        glm::mat4 m(1.0f);
        if (j.contains("m") && j["m"].is_array() && j["m"].size() == 16) {
            int i = 0;
            for (int c = 0; c < 4; ++c) {
                for (int r = 0; r < 4; ++r) {
                    m[c][r] = j["m"][i++].get<float>();
                }
            }
        }
        return PropertyValue(m);
    }
    if (t == "list") {
        auto list = std::make_shared<PropertyList>();
        if (j.contains("v") && j["v"].is_array()) {
            for (const auto& el : j["v"]) {
                list->elements.push_back(propertyValueFromJson(el));
            }
        }
        return PropertyValue(list);
    }
    if (t == "dict") {
        auto dict = std::make_shared<PropertyDict>();
        if (j.contains("v") && j["v"].is_object()) {
            for (auto it = j["v"].begin(); it != j["v"].end(); ++it) {
                dict->elements[it.key()] = propertyValueFromJson(it.value());
            }
        }
        return PropertyValue(dict);
    }
    // "none" and "ref" (world references resolve through the loader).
    return PropertyValue{};
}
