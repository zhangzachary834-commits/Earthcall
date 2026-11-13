#include "ConstructedBeing/Singular/Property/PropertyValueJson.hpp"

#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <variant>

int main() {
    // Tagged values preserve the exact PropertyValue alternative, even where
    // an untagged JSON number could otherwise collapse int/long/float/double.
    {
        const PropertyValue integer(42);
        const auto encoded = propertyValueToJson(integer);
        assert(encoded["t"] == "int" && encoded["v"] == 42);
        const auto decoded = propertyValueFromJson(encoded);
        assert(std::holds_alternative<int>(decoded));
        assert(std::get<int>(decoded) == 42);
    }
    {
        const PropertyValue largeInteger(123456789L);
        const auto encoded = propertyValueToJson(largeInteger);
        assert(encoded["t"] == "long");
        const auto decoded = propertyValueFromJson(encoded);
        assert(std::holds_alternative<long>(decoded));
        assert(std::get<long>(decoded) == 123456789L);
    }
    {
        const auto decoded = propertyValueFromJson(propertyValueToJson(PropertyValue(1.25f)));
        assert(std::holds_alternative<float>(decoded));
        assert(std::get<float>(decoded) == 1.25f);
    }
    {
        const auto decoded = propertyValueFromJson(propertyValueToJson(PropertyValue(-2.5)));
        assert(std::holds_alternative<double>(decoded));
        assert(std::get<double>(decoded) == -2.5);
    }
    {
        const auto decoded = propertyValueFromJson(propertyValueToJson(PropertyValue(true)));
        assert(std::holds_alternative<bool>(decoded));
        assert(std::get<bool>(decoded));
    }
    {
        const auto decoded = propertyValueFromJson(propertyValueToJson(PropertyValue('Q')));
        assert(std::holds_alternative<char>(decoded));
        assert(std::get<char>(decoded) == 'Q');
    }
    {
        const auto decoded = propertyValueFromJson(
            propertyValueToJson(PropertyValue(std::string("law.enabled"))));
        assert(std::holds_alternative<std::string>(decoded));
        assert(std::get<std::string>(decoded) == "law.enabled");
    }
    {
        const auto decoded = propertyValueFromJson(propertyValueToJson(PropertyValue{}));
        assert(std::holds_alternative<std::monostate>(decoded));
    }

    // Tagged geometry is not inferred from an arbitrary structural array.
    {
        const PropertyValue vec(glm::vec3(1.0f, 2.0f, 3.0f));
        const auto encoded = propertyValueToJson(vec);
        assert(encoded["t"] == "vec3" && encoded["x"] == 1.0f);
        const auto decoded = propertyValueFromJson(encoded);
        assert(std::holds_alternative<glm::vec3>(decoded));
        const auto& value = std::get<glm::vec3>(decoded);
        assert(value.x == 1.0f && value.y == 2.0f && value.z == 3.0f);
    }
    {
        glm::mat4 matrix(1.0f);
        matrix[2][1] = 7.5f;
        const auto encoded = propertyValueToJson(PropertyValue(matrix));
        assert(encoded["t"] == "mat4" && encoded["m"].size() == 16);
        const auto decoded = propertyValueFromJson(encoded);
        assert(std::holds_alternative<glm::mat4>(decoded));
        assert(std::get<glm::mat4>(decoded)[2][1] == 7.5f);
    }

    // Tagged nested containers preserve their child types and identifiers.
    {
        auto list = std::make_shared<PropertyList>();
        list->elements.push_back(PropertyValue(10L));
        list->elements.push_back(PropertyValue(1.25f));
        auto dict = std::make_shared<PropertyDict>();
        dict->elements["values"] = PropertyValue(list);
        dict->elements["name"] = PropertyValue(std::string("Earthcall"));
        const auto encoded = propertyValueToJson(PropertyValue(dict));
        assert(encoded["t"] == "dict");
        const auto decoded = propertyValueFromJson(encoded);
        assert(std::holds_alternative<std::shared_ptr<PropertyDict>>(decoded));
        const auto& entries = std::get<std::shared_ptr<PropertyDict>>(decoded)->elements;
        assert(std::get<std::string>(entries.at("name")) == "Earthcall");
        const auto& values = std::get<std::shared_ptr<PropertyList>>(entries.at("values"))->elements;
        assert(values.size() == 2);
        assert(std::holds_alternative<long>(values[0]));
        assert(std::holds_alternative<float>(values[1]));
    }

    // The structural codec keeps arrays as lists, even a three-number array
    // that the legacy untagged value decoder might otherwise treat as vec3.
    {
        const nlohmann::json input = nlohmann::json::object({
            {"coordinates", nlohmann::json::array({1, 2, 3})},
            {"label", "a structural list"}
        });
        const auto decoded = propertyStructureFromJson(input);
        assert(std::holds_alternative<std::shared_ptr<PropertyDict>>(decoded));
        const auto& dict = std::get<std::shared_ptr<PropertyDict>>(decoded)->elements;
        assert(std::holds_alternative<std::shared_ptr<PropertyList>>(dict.at("coordinates")));
        const auto& list = std::get<std::shared_ptr<PropertyList>>(dict.at("coordinates"))->elements;
        assert(list.size() == 3);
        assert(std::holds_alternative<int>(list[0]));
        assert(propertyStructureToJson(decoded) == input);
    }

    // World references are deliberately unresolved, never fabricated by the
    // value codec; malformed matrix payloads also cannot invent a value.
    {
        const auto ref = propertyValueFromJson(
            nlohmann::json{{"t", "ref"}, {"id", "person-123"}});
        assert(std::holds_alternative<std::monostate>(ref));
        const auto badMatrix = propertyValueFromJson(
            nlohmann::json{{"t", "matrix"}, {"rows", 2}, {"cols", 2}});
        assert(std::holds_alternative<std::monostate>(badMatrix));
    }

    std::cout << "property_value_json_test: PASS\n";
    return 0;
}
