#include "ConstructedBeing/Singular/Property/PropertyValueJson.hpp"
#include <iostream>
#include <cassert>
#include <cmath>





int main() {
    // 1. Basic typed round trips
    {
        PropertyValue v_int(42);
        nlohmann::json j_int = propertyValueToJson(v_int);
        assert(j_int["t"] == "int");
        assert(j_int["v"] == 42);
        PropertyValue decoded = propertyValueFromJson(j_int);
        assert(std::holds_alternative<int>(decoded));
        assert(std::get<int>(decoded) == 42);
    }

    // 2. glm types
    {
        PropertyValue v_vec(glm::vec3(1.0f, 2.0f, 3.0f));
        nlohmann::json j_vec = propertyValueToJson(v_vec);
        assert(j_vec["t"] == "vec3");
        assert(j_vec["x"] == 1.0f);
        PropertyValue decoded = propertyValueFromJson(j_vec);
        assert(std::holds_alternative<glm::vec3>(decoded));
        assert(std::get<glm::vec3>(decoded).y == 2.0f);
    }

    // 3. Structural mapping
    {
        PropertyValue v_list = std::make_shared<PropertyList>();
        std::get<std::shared_ptr<PropertyList>>(v_list)->elements.push_back(PropertyValue(10));
        std::get<std::shared_ptr<PropertyList>>(v_list)->elements.push_back(PropertyValue(std::string("hello")));

        nlohmann::json j_struct = propertyStructureToJson(v_list);
        assert(j_struct.is_array());
        assert(j_struct.size() == 2);
        assert(j_struct[0] == 10);
        assert(j_struct[1] == "hello");

        PropertyValue decoded = propertyStructureFromJson(j_struct);
        assert(std::holds_alternative<std::shared_ptr<PropertyList>>(decoded));
        auto list = std::get<std::shared_ptr<PropertyList>>(decoded);
        assert(list->elements.size() == 2);
        assert(std::holds_alternative<int>(list->elements[0]));
        assert(std::holds_alternative<std::string>(list->elements[1]));
    }

    std::cout << "property_value_json_test: PASS\n";
    return 0;
}
