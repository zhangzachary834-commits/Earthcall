#include "ConstructedBeing/Singular/Property/PropertyValueJson.hpp"
#include <cassert>
#include <iostream>

void testJsonRoundTripBasic() {
    std::cout << "[Test 1] JSON round trip - basic types\n";

    PropertyValue valInt(42);
    nlohmann::json jInt = propertyValueToJson(valInt);
    PropertyValue outInt = propertyValueFromJson(jInt);
    assert(std::get<int>(outInt) == 42);

    PropertyValue valFloat(42.5f);
    nlohmann::json jFloat = propertyValueToJson(valFloat);
    PropertyValue outFloat = propertyValueFromJson(jFloat);
    assert(std::get<float>(outFloat) == 42.5f);

    PropertyValue valStr(std::string("hello"));
    nlohmann::json jStr = propertyValueToJson(valStr);
    PropertyValue outStr = propertyValueFromJson(jStr);
    assert(std::get<std::string>(outStr) == "hello");
}

void testJsonRoundTripCollections() {
    std::cout << "[Test 2] JSON round trip - collections\n";

    auto list = std::make_shared<PropertyList>();
    list->elements.push_back(PropertyValue(1));
    list->elements.push_back(PropertyValue(std::string("two")));

    nlohmann::json jList = propertyValueToJson(PropertyValue(list));
    PropertyValue outList = propertyValueFromJson(jList);
    auto pList = std::get<std::shared_ptr<PropertyList>>(outList);
    assert(pList->elements.size() == 2);
    assert(std::get<int>(pList->elements[0]) == 1);
    assert(std::get<std::string>(pList->elements[1]) == "two");

    auto dict = std::make_shared<PropertyDict>();
    dict->elements["key1"] = PropertyValue(10);
    nlohmann::json jDict = propertyValueToJson(PropertyValue(dict));
    PropertyValue outDict = propertyValueFromJson(jDict);
    auto pDict = std::get<std::shared_ptr<PropertyDict>>(outDict);
    assert(std::get<int>(pDict->elements["key1"]) == 10);
}

void testUntaggedParsing() {
    std::cout << "[Test 3] JSON untagged parsing\n";

    nlohmann::json jArr = {1.0, 2.0, 3.0};
    PropertyValue outVec = propertyValueFromJson(jArr);
    auto vec = std::get<glm::vec3>(outVec);
    assert(vec.x == 1.0f && vec.y == 2.0f && vec.z == 3.0f);

    nlohmann::json jNum = 42;
    PropertyValue outNum = propertyValueFromJson(jNum);
    assert(std::get<int>(outNum) == 42);
}

int main() {
    std::cout << "\n=== Property Value JSON Test ===\n\n";
    testJsonRoundTripBasic();
    testJsonRoundTripCollections();
    testUntaggedParsing();
    std::cout << "\n✓ All tests passed!\n\n";
    return 0;
}
