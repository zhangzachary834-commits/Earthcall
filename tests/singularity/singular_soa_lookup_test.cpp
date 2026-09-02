#include "ConstructedBeing/Singular/Singular.hpp"
#include "ConstructedBeing/Singular/Property/PropertyRef.hpp"
#include "Singularity/Core/StringId.hpp"
#include <cassert>
#include <iostream>

using namespace Earthcall;

// ============================================================================
// Singular Structure of Arrays (SoA) Lookup Test
//
// Verifies that Singular's property lookup correctly uses the SoA layout:
// 1. findProperty(StringId) scans integer array, not pointer array
// 2. findProperty(string) delegates to StringId version
// 3. Both methods return the same property
// 4. Dynamic properties work with StringId keys
// 5. Parallel arrays stay synchronized
// ============================================================================

// Concrete Singular subclass for testing
class TestSingular : public Singular {
public:
    int value1 = 10;
    double value2 = 20.0;
    std::string value3 = "test";

    std::string getIdentifier() const override { return "test-singular"; }

protected:
    void buildProperties() override {
        // Register 3 properties manually
        _propertyNames.push_back(StringInterner::intern("value1"));
        _propertyRegistry.push_back(std::make_unique<PropertyRef<TestSingular, int>>(
            "value1", this, &TestSingular::value1, this));

        _propertyNames.push_back(StringInterner::intern("value2"));
        _propertyRegistry.push_back(std::make_unique<PropertyRef<TestSingular, double>>(
            "value2", this, &TestSingular::value2, this));

        _propertyNames.push_back(StringInterner::intern("value3"));
        _propertyRegistry.push_back(std::make_unique<PropertyRef<TestSingular, std::string>>(
            "value3", this, &TestSingular::value3, this));
    }
};

void testStringIdLookup() {
    StringInterner::clear();

    std::cout << "[Test 1] findProperty(StringId) scans integer array\n";

    TestSingular obj;

    StringId id1 = StringInterner::intern("value1");
    StringId id2 = StringInterner::intern("value2");
    StringId id3 = StringInterner::intern("value3");

    Property* prop1 = obj.findProperty(id1);
    Property* prop2 = obj.findProperty(id2);
    Property* prop3 = obj.findProperty(id3);

    assert(prop1 != nullptr);
    assert(prop2 != nullptr);
    assert(prop3 != nullptr);

    assert(prop1->name() == "value1");
    assert(prop2->name() == "value2");
    assert(prop3->name() == "value3");

    std::cout << "  ✓ findProperty(StringId) finds all properties\n";
}

void testStringDelegatesToStringId() {
    StringInterner::clear();

    std::cout << "[Test 2] findProperty(string) delegates to StringId version\n";

    TestSingular obj;

    Property* prop1_byString = obj.findProperty("value1");
    Property* prop1_byId = obj.findProperty(StringInterner::intern("value1"));

    // Both methods return the SAME property pointer
    assert(prop1_byString == prop1_byId);

    std::cout << "  ✓ String and StringId lookups return same property\n";
}

void testParallelArraysSynchronized() {
    StringInterner::clear();

    std::cout << "[Test 3] Parallel arrays stay synchronized\n";

    TestSingular obj;

    // Force property build
    obj.findProperty("value1");

    // Check that parallel arrays have same size
    // (We can't access _propertyNames/_propertyRegistry directly, but we can
    // verify behavior: each property found by ID has a corresponding name)

    StringId ids[] = {
        StringInterner::intern("value1"),
        StringInterner::intern("value2"),
        StringInterner::intern("value3"),
        StringInterner::intern("telos")  // Registered by registerTelosProperty()
    };

    for (auto id : ids) {
        Property* prop = obj.findProperty(id);
        assert(prop != nullptr);
        // The property's nameId() should match the ID we looked it up with
        assert(prop->nameId() == id);
    }

    std::cout << "  ✓ All properties found by ID have matching nameId()\n";
}

void testDynamicPropertiesStringId() {
    StringInterner::clear();

    std::cout << "[Test 4] Dynamic properties use StringId keys\n";

    TestSingular obj;

    // Set dynamic property (via string)
    obj.setDynamicProperty("customProp", PropertyValue(42));

    // Get via string
    PropertyValue val1;
    assert(obj.getDynamicProperty("customProp", val1));
    assert(std::get<int>(val1) == 42);

    // Get via StringId
    PropertyValue val2;
    StringId customId = StringInterner::intern("customProp");
    assert(obj.getDynamicProperty(customId, val2));
    assert(std::get<int>(val2) == 42);

    // Both methods return same value
    assert(std::get<int>(val1) == std::get<int>(val2));

    std::cout << "  ✓ Dynamic properties accessible via both string and StringId\n";
}

void testFindPropertyReturnsNullForMissing() {
    StringInterner::clear();

    std::cout << "[Test 5] findProperty returns null for non-existent properties\n";

    TestSingular obj;

    StringId nonExistentId = StringInterner::intern("doesNotExist");
    Property* prop = obj.findProperty(nonExistentId);

    assert(prop == nullptr);

    prop = obj.findProperty("alsoDoesNotExist");
    assert(prop == nullptr);

    std::cout << "  ✓ Missing properties return nullptr\n";
}

void testListPropertiesStillWorks() {
    StringInterner::clear();

    std::cout << "[Test 6] listProperties() enumerates all properties\n";

    TestSingular obj;

    std::vector<Property*> props = obj.listProperties();

    // Should have: value1, value2, value3, telos
    assert(props.size() == 4);

    // All should be non-null
    for (auto* prop : props) {
        assert(prop != nullptr);
        assert(prop->nameId().isValid());
    }

    std::cout << "  ✓ listProperties() returns all registered properties\n";
}

void testDynamicPropertyBridge() {
    StringInterner::clear();

    std::cout << "[Test 7] Dynamic property bridge lazy-creates correctly\n";

    TestSingular obj;

    // Add dynamic property
    obj.setDynamicProperty("dynamic1", PropertyValue(100.0));

    // First findProperty should create bridge
    Property* bridge = obj.findProperty("dynamic1");
    assert(bridge != nullptr);
    assert(bridge->name() == "dynamic1");

    PropertyValue val = bridge->value();
    assert(std::get<double>(val) == 100.0);

    // Second findProperty should return same bridge
    Property* bridge2 = obj.findProperty("dynamic1");
    assert(bridge == bridge2);

    std::cout << "  ✓ Dynamic property bridge created and reused correctly\n";
}

int main() {
    std::cout << "\n=== Singular SoA Lookup Test ===\n\n";

    testStringIdLookup();
    testStringDelegatesToStringId();
    testParallelArraysSynchronized();
    testDynamicPropertiesStringId();
    testFindPropertyReturnsNullForMissing();
    testListPropertiesStillWorks();
    testDynamicPropertyBridge();

    std::cout << "\n✓ All tests passed!\n\n";

    return 0;
}
