#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValue.hpp"
#include "support/test_harness.hpp"
#include <iostream>

void test_property_graph_traversal() {
    Object macro("image.test");
    
    // Set up a micro region Singular
    auto micro = std::make_shared<Object>("region.sky");
    micro->setDynamicProperty("tint", PropertyValue(glm::vec3(0.1f, 0.2f, 0.3f)));
    micro->setDynamicProperty("active", PropertyValue(true));

    // Put it in a PropertyDict
    auto regionsDict = std::make_shared<PropertyDict>();
    regionsDict->elements["sky"] = PropertyValue(static_cast<Singular*>(micro.get()));
    
    // Bind dict to macro Object as dynamic property
    macro.setDynamicProperty("regions", PropertyValue(regionsDict));

    // Test 1: Address micro singular boolean
    {
        PropertyPath path = PropertyPath::parse("regions.sky.active");
        PropertyValue out;
        auto result = path.getValue(macro, out);
        if (result != PropertyPath::PathResult::Ok) {
            std::cout << "Test 1 Failed: result=" << (int)result << "\n";
        }
        assert(result == PropertyPath::PathResult::Ok);
        assert(std::holds_alternative<bool>(out));
        assert(std::get<bool>(out) == true);
    }

    // Test 2: Address micro singular vec3 trailing component
    {
        PropertyPath path = PropertyPath::parse("regions.sky.tint.r");
        PropertyValue out;
        auto result = path.getValue(macro, out);
        assert(result == PropertyPath::PathResult::Ok);
        double n;
        assert(propertyValueToNumber(out, n));
        assert(std::abs(n - 0.1) < 1e-5);
    }

    // Test 3: Mutate micro singular vec3 trailing component through macro path
    {
        PropertyPath path = PropertyPath::parse("regions.sky.tint.g");
        auto result = path.setValue(macro, PropertyValue(0.9f));
        assert(result == PropertyPath::PathResult::Ok);

        // Verify it was mutated
        PropertyValue out;
        path.getValue(macro, out);
        double n;
        propertyValueToNumber(out, n);
        assert(std::abs(n - 0.9) < 1e-5);
        
        // Verify micro object actually changed
        PropertyValue tintVal;
        micro->getDynamicProperty("tint", tintVal);
        glm::vec3 vec = std::get<glm::vec3>(tintVal);
        assert(std::abs(vec.y - 0.9f) < 1e-5);
    }

    // Test 4: Nested dict traversal
    auto deepDict = std::make_shared<PropertyDict>();
    deepDict->elements["nestedVal"] = PropertyValue(42);
    regionsDict->elements["deep"] = PropertyValue(deepDict);

    {
        PropertyPath path = PropertyPath::parse("regions.deep.nestedVal");
        PropertyValue out;
        auto result = path.getValue(macro, out);
        assert(result == PropertyPath::PathResult::Ok);
        assert(std::holds_alternative<int>(out));
        assert(std::get<int>(out) == 42);

        // Mutate deep value
        auto setRes = path.setValue(macro, PropertyValue(100));
        assert(setRes == PropertyPath::PathResult::Ok);

        path.getValue(macro, out);
        assert(std::get<int>(out) == 100);
    }
}

int main() {
    test_property_graph_traversal();
    std::cout << "All property graph tests passed.\n";
    return 0;
}
