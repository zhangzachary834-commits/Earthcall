#include "ConstructedBeing/Singular/Property/PropertyValueJson.hpp"

// Full definitions needed: pointer alternatives upcast to Singular* for
// identifier extraction and typed-reference restoration.
#include "ConstructedBeing/Singular/Singular.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Relation/Formation/Formation.hpp"
#include "Relation/Relation.hpp"
#include "Singularity/OntoMath/Field.hpp"

namespace {

nlohmann::json refJson(const Singular* s, const char* kind) {
    return nlohmann::json{
        {"t", "ref"},
        {"k", kind},
        {"id", s ? s->getIdentifier() : ""}
    };
}

std::optional<PropertyValue> resolvedReference(
    const nlohmann::json& j,
    const PropertyReferenceResolver& resolve) {
    if (!resolve) return std::nullopt;
    const std::string id = j.value("id", std::string{});
    const std::string kind = j.value("k", std::string{"singular"});
    if (id.empty()) {
        if (kind == "object") return PropertyValue(static_cast<Object*>(nullptr));
        if (kind == "relation") return PropertyValue(static_cast<Relation*>(nullptr));
        if (kind == "formation") return PropertyValue(static_cast<Formation*>(nullptr));
        return PropertyValue(static_cast<Singular*>(nullptr));
    }
    if (!resolve) return std::nullopt;
    Singular* being = resolve(id);
    if (!being) return std::nullopt;

    if (kind == "object") {
        if (auto* object = dynamic_cast<Object*>(being)) return PropertyValue(object);
        return std::nullopt;
    }
    if (kind == "relation") {
        if (auto* relation = dynamic_cast<Relation*>(being)) return PropertyValue(relation);
        return std::nullopt;
    }
    if (kind == "formation") {
        if (auto* formation = dynamic_cast<Formation*>(being)) return PropertyValue(formation);
        return std::nullopt;
    }
    return PropertyValue(being);
}

std::optional<PropertyValue> decode(
    const nlohmann::json& j,
    const PropertyReferenceResolver& resolve) {
    if (j.is_null()) return PropertyValue{};
    if (j.is_boolean()) return PropertyValue(j.get<bool>());
    if (j.is_number_integer()) return PropertyValue(j.get<int>());
    if (j.is_number_float()) return PropertyValue(j.get<double>());
    if (j.is_string()) return PropertyValue(j.get<std::string>());

    if (j.is_array()) {
        if (j.size() == 3 && j[0].is_number() && j[1].is_number() && j[2].is_number()) {
            return PropertyValue(glm::vec3(j[0].get<float>(), j[1].get<float>(), j[2].get<float>()));
        }
        if (j.size() == 16) {
            glm::mat4 m(1.0f);
            int i = 0;
            for (int c = 0; c < 4; ++c) {
                for (int r = 0; r < 4; ++r) {
                    m[c][r] = j[i++].get<float>();
                }
            }
            return PropertyValue(m);
        }
        auto list = std::make_shared<PropertyList>();
        for (const auto& el : j) {
            auto decoded = decode(el, resolve);
            if (!decoded) return std::nullopt;
            list->elements.push_back(std::move(*decoded));
        }
        return PropertyValue(list);
    }

    if (j.is_object() && !j.contains("t")) {
        auto dict = std::make_shared<PropertyDict>();
        for (auto it = j.begin(); it != j.end(); ++it) {
            auto decoded = decode(it.value(), resolve);
            if (!decoded) return std::nullopt;
            dict->elements[it.key()] = std::move(*decoded);
        }
        return PropertyValue(dict);
    }

    const std::string t = j.value("t", "none");
    if (t == "none") return PropertyValue{};
    if (t == "int") return PropertyValue(j.value("v", 0));
    if (t == "float") return PropertyValue(j.value("v", 0.0f));
    if (t == "double") return PropertyValue(j.value("v", 0.0));
    if (t == "bool") return PropertyValue(j.value("v", false));
    if (t == "char") return PropertyValue(static_cast<char>(j.value("v", 0)));
    if (t == "long") return PropertyValue(static_cast<long>(j.value("v", 0LL)));
    if (t == "string") return PropertyValue(j.value("v", std::string()));
    if (t == "vec3") {
        return PropertyValue(glm::vec3(
            j.value("x", 0.0f), j.value("y", 0.0f), j.value("z", 0.0f)));
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
                auto decoded = decode(el, resolve);
                if (!decoded) return std::nullopt;
                list->elements.push_back(std::move(*decoded));
            }
        }
        return PropertyValue(list);
    }
    if (t == "dict") {
        auto dict = std::make_shared<PropertyDict>();
        if (j.contains("v") && j["v"].is_object()) {
            for (auto it = j["v"].begin(); it != j["v"].end(); ++it) {
                auto decoded = decode(it.value(), resolve);
                if (!decoded) return std::nullopt;
                dict->elements[it.key()] = std::move(*decoded);
            }
        }
        return PropertyValue(dict);
    }
    if (t == "ref") return resolvedReference(j, resolve);
    if (t == "scalar_field") {
        if (!j.contains("v")) return std::nullopt;
        if (j["v"].is_null()) {
            return PropertyValue(std::shared_ptr<OntoMath::ScalarField>{});
        }
        if (!j["v"].is_object()) return std::nullopt;
        return PropertyValue(OntoMath::ScalarField::fromJson(j["v"]));
    }
    if (t == "vector_field") {
        if (!j.contains("v")) return std::nullopt;
        if (j["v"].is_null()) {
            return PropertyValue(std::shared_ptr<OntoMath::VectorField>{});
        }
        if (!j["v"].is_object()) return std::nullopt;
        return PropertyValue(OntoMath::VectorField::fromJson(j["v"]));
    }

    return std::nullopt;
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
                for (int r = 0; r < 4; ++r) m.push_back(x[c][r]);
            }
            return nlohmann::json{{"t", "mat4"}, {"m", m}};
        } else if constexpr (std::is_same_v<X, std::shared_ptr<PropertyList>>) {
            nlohmann::json arr = nlohmann::json::array();
            if (x) {
                for (const auto& el : x->elements) arr.push_back(propertyValueToJson(el));
            }
            return nlohmann::json{{"t", "list"}, {"v", arr}};
        } else if constexpr (std::is_same_v<X, std::shared_ptr<PropertyDict>>) {
            nlohmann::json obj = nlohmann::json::object();
            if (x) {
                for (const auto& [k, val] : x->elements) obj[k] = propertyValueToJson(val);
            }
            return nlohmann::json{{"t", "dict"}, {"v", obj}};
        } else if constexpr (std::is_same_v<X, std::shared_ptr<OntoMath::ScalarField>>) {
            return nlohmann::json{{"t", "scalar_field"},
                                  {"v", x ? x->toJson() : nlohmann::json(nullptr)}};
        } else if constexpr (std::is_same_v<X, std::shared_ptr<OntoMath::VectorField>>) {
            return nlohmann::json{{"t", "vector_field"},
                                  {"v", x ? x->toJson() : nlohmann::json(nullptr)}};
        } else if constexpr (std::is_same_v<X, Object*>) {
            return refJson(x, "object");
        } else if constexpr (std::is_same_v<X, Relation*>) {
            return refJson(x, "relation");
        } else if constexpr (std::is_same_v<X, Formation*>) {
            return refJson(x, "formation");
        } else {
            return refJson(static_cast<const Singular*>(x), "singular");
        }
    }, v);
}

std::optional<PropertyValue> tryPropertyValueFromJson(
    const nlohmann::json& j,
    const PropertyReferenceResolver& resolve) {
    return decode(j, resolve);
}

PropertyValue propertyValueFromJson(const nlohmann::json& j) {
    auto decoded = decode(j, {});
    return decoded ? std::move(*decoded) : PropertyValue{};
}
