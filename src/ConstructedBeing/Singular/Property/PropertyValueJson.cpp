#include "ConstructedBeing/Singular/Property/PropertyValueJson.hpp"

// Full definitions needed: the pointer alternatives upcast to Singular* for
// identifier extraction, which forward declarations cannot prove.
#include "ConstructedBeing/Singular/Singular.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Relation/Formation/Formation.hpp"
#include "Relation/Relation.hpp"
#include "Singularity/OntoMath/Field.hpp"
#include <stdexcept>
#include <cmath>
#include <limits>

namespace {

nlohmann::json refJson(const Singular* s) {
    return nlohmann::json{{"t", "ref"}, {"id", s ? s->getIdentifier() : ""}};
}

} // namespace

PropertyValue propertyStructureFromJson(const nlohmann::json& value, unsigned depth) {
    if (depth >= 64) throw std::runtime_error("structural value exceeds 64 levels");
    if (value.is_object()) {
        auto dict = std::make_shared<PropertyDict>();
        for (auto it=value.begin(); it!=value.end(); ++it)
            dict->elements[it.key()] = propertyStructureFromJson(it.value(), depth+1);
        return dict;
    }
    if (value.is_array()) {
        auto list = std::make_shared<PropertyList>();
        for (const auto& item:value) list->elements.push_back(propertyStructureFromJson(item, depth+1));
        return list;
    }
    if (value.is_number_unsigned()) {
        const auto n=value.get<unsigned long long>();
        if (n>static_cast<unsigned long long>(std::numeric_limits<long>::max()))
            throw std::runtime_error("structural integer exceeds PropertyValue long");
        if (n<=static_cast<unsigned long long>(std::numeric_limits<int>::max())) return int(n);
        return long(n);
    }
    if (value.is_number_integer()) {
        const auto n=value.get<long long>();
        if (n<std::numeric_limits<long>::min() || n>std::numeric_limits<long>::max())
            throw std::runtime_error("structural integer exceeds PropertyValue long");
        if (n>=std::numeric_limits<int>::min() && n<=std::numeric_limits<int>::max()) return int(n);
        return long(n);
    }
    return propertyValueFromJson(value);
}

nlohmann::json propertyStructureToJson(const PropertyValue& value, unsigned depth) {
    if (depth >= 64) throw std::runtime_error("structural value exceeds 64 levels");
    if (auto dict=std::get_if<std::shared_ptr<PropertyDict>>(&value)) {
        if (!*dict) return nullptr;
        auto out=nlohmann::json::object();
        for (const auto& [key,item]:(*dict)->elements) out[key]=propertyStructureToJson(item,depth+1);
        return out;
    }
    if (auto list=std::get_if<std::shared_ptr<PropertyList>>(&value)) {
        if (!*list) return nullptr;
        auto out=nlohmann::json::array();
        for (const auto& item:(*list)->elements) out.push_back(propertyStructureToJson(item,depth+1));
        return out;
    }
    if (auto text=std::get_if<std::string>(&value)) return *text;
    if (auto flag=std::get_if<bool>(&value)) return *flag;
    if (auto integer=std::get_if<int>(&value)) return *integer;
    if (auto integer=std::get_if<long>(&value)) return *integer;
    if (auto character=std::get_if<char>(&value)) return int(*character);
    double number=0;
    if (propertyValueToNumber(value,number)) {
        if (!std::isfinite(number)) throw std::runtime_error("nonfinite structural number");
        return number;
    }
    if (std::holds_alternative<std::monostate>(value)) return nullptr;
    throw std::runtime_error("value has no structural representation");
}

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
            return nlohmann::json{{"t", "scalar_field"}, {"v", x ? x->toJson() : nlohmann::json(nullptr)}};
        } else if constexpr (std::is_same_v<X, std::shared_ptr<OntoMath::VectorField>>) {
            return nlohmann::json{{"t", "vector_field"}, {"v", x ? x->toJson() : nlohmann::json(nullptr)}};
        } else {
            // Singular*/Object*/Relation*/Formation* — identity, not value.
            return refJson(static_cast<const Singular*>(x));
        }
    }, v);
}

PropertyValue propertyValueFromJson(const nlohmann::json& j) {
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
            list->elements.push_back(propertyValueFromJson(el));
        }
        return PropertyValue(list);
    }
    if (j.is_object() && !j.contains("t")) {
        auto dict = std::make_shared<PropertyDict>();
        for (auto it = j.begin(); it != j.end(); ++it) {
            dict->elements[it.key()] = propertyValueFromJson(it.value());
        }
        return PropertyValue(dict);
    }

    const std::string t = j.value("t", "none");
    // Old tag-only records contain no recoverable mathematical value. Retain
    // their prior undefined result rather than inventing a zero/default field.
    if (t == "scalar_field" || t == "vector_field") {
        if (!j.contains("v")) return PropertyValue{};
        const auto& payload = j["v"];
        if (payload.is_null()) {
            if (t == "scalar_field") return PropertyValue(std::shared_ptr<OntoMath::ScalarField>{});
            return PropertyValue(std::shared_ptr<OntoMath::VectorField>{});
        }
        if (!payload.is_object() || !payload.contains("mode")) return PropertyValue{};
        const std::string mode = payload.value("mode", "");
        if (mode != "AST" && mode != "Procedural") return PropertyValue{};
        if (mode == "AST" && !payload.contains("astDefinition")) return PropertyValue{};
        if (t == "scalar_field") return PropertyValue(OntoMath::ScalarField::fromJson(payload));
        return PropertyValue(OntoMath::VectorField::fromJson(payload));
    }
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
