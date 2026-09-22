#include "ConstructedBeing/Singular/Property/PropertyRef.hpp"
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"
#include "Singularity/Core/StringId.hpp"
#include <cassert>
#include <iostream>

using namespace Earthcall;

// ============================================================================
// Property String ID Integration Test
//
// Verifies that PropertyRef and ComputedProperty correctly cache StringIds:
// 1. Properties with same name return same nameId()
// 2. nameId() matches StringInterner::intern(name())
// 3. Both PropertyRef and ComputedProperty implement nameId()
// 4. Cached ID is stable across multiple calls
// ============================================================================

// Test struct with various property types
struct TestObject {
    int intValue = 42;
    double doubleValue = 3.14;
    std::string stringValue = "test";

    int getComputedInt() const { return intValue * 2; }
    void setComputedInt(const int& v) { intValue = v / 2; }

    double getComputedDouble() const { return doubleValue + 1.0; }
};

void testPropertyRefStringId() {
    StringInterner::clear();

    std::cout << "[Test 1] PropertyRef caches StringId\n";

    TestObject obj;
    PropertyRef<TestObject, int> prop1("value", &obj, &TestObject::intValue);
    PropertyRef<TestObject, double> prop2("score", &obj, &TestObject::doubleValue);
    PropertyRef<TestObject, int> prop3("value", &obj, &TestObject::intValue);  // Same name as prop1

    // nameId() returns a valid ID
    assert(prop1.nameId().isValid());
    assert(prop2.nameId().isValid());
    assert(prop3.nameId().isValid());

    // Same name returns same ID
    assert(prop1.nameId() == prop3.nameId());
    assert(prop1.nameId() != prop2.nameId());

    // nameId() matches direct interning of the name
    StringId directId1 = StringInterner::intern("value");
    StringId directId2 = StringInterner::intern("score");
    assert(prop1.nameId() == directId1);
    assert(prop2.nameId() == directId2);

    // name() still returns the original string
    assert(prop1.name() == "value");
    assert(prop2.name() == "score");

    std::cout << "  ✓ PropertyRef::nameId() returns cached StringId\n";
    std::cout << "  ✓ Same name returns same ID\n";
    std::cout << "  ✓ name() still works for backward compatibility\n";
}

void testComputedPropertyStringId() {
    StringInterner::clear();

    std::cout << "[Test 2] ComputedProperty caches StringId\n";

    TestObject obj;
    ComputedProperty<TestObject, int> prop1(
        "computed", &obj,
        &TestObject::getComputedInt,
        &TestObject::setComputedInt
    );

    ComputedProperty<TestObject, double> prop2(
        "derived", &obj,
        &TestObject::getComputedDouble
    );

    ComputedProperty<TestObject, int> prop3(
        "computed", &obj,
        &TestObject::getComputedInt
    );  // Same name as prop1

    // nameId() returns valid IDs
    assert(prop1.nameId().isValid());
    assert(prop2.nameId().isValid());
    assert(prop3.nameId().isValid());

    // Same name returns same ID
    assert(prop1.nameId() == prop3.nameId());
    assert(prop1.nameId() != prop2.nameId());

    // Matches direct interning
    StringId directId = StringInterner::intern("computed");
    assert(prop1.nameId() == directId);

    // name() still works
    assert(prop1.name() == "computed");
    assert(prop2.name() == "derived");

    std::cout << "  ✓ ComputedProperty::nameId() returns cached StringId\n";
    std::cout << "  ✓ Same name returns same ID\n";
    std::cout << "  ✓ name() still works\n";
}

void testMultipleCallsStable() {
    StringInterner::clear();

    std::cout << "[Test 3] nameId() is stable across calls\n";

    TestObject obj;
    PropertyRef<TestObject, int> prop("stable", &obj, &TestObject::intValue);

    StringId id1 = prop.nameId();
    StringId id2 = prop.nameId();
    StringId id3 = prop.nameId();

    // Multiple calls return identical ID (not just equal, but literally the same value)
    assert(id1 == id2);
    assert(id2 == id3);
    assert(id1.value == id2.value);
    assert(id2.value == id3.value);

    std::cout << "  ✓ nameId() returns stable cached value\n";
}

void testNestedPropertyNames() {
    StringInterner::clear();

    std::cout << "[Test 4] Nested property names intern correctly\n";

    TestObject obj;
    PropertyRef<TestObject, int> prop1("shape", &obj, &TestObject::intValue);
    PropertyRef<TestObject, double> prop2("shape.color", &obj, &TestObject::doubleValue);
    PropertyRef<TestObject, std::string> prop3("shape.color.r", &obj, &TestObject::stringValue);

    // All intern to different IDs
    assert(prop1.nameId() != prop2.nameId());
    assert(prop2.nameId() != prop3.nameId());
    assert(prop1.nameId() != prop3.nameId());

    // Match direct interning
    assert(prop1.nameId() == StringInterner::intern("shape"));
    assert(prop2.nameId() == StringInterner::intern("shape.color"));
    assert(prop3.nameId() == StringInterner::intern("shape.color.r"));

    std::cout << "  ✓ Nested property paths intern to different IDs\n";
}

void testPropertyInterfacePolymorphism() {
    StringInterner::clear();

    std::cout << "[Test 5] nameId() works through Property* base pointer\n";

    TestObject obj;

    // Store as base Property pointers
    std::unique_ptr<Property> prop1 = std::make_unique<PropertyRef<TestObject, int>>(
        "poly1", &obj, &TestObject::intValue
    );

    std::unique_ptr<Property> prop2 = std::make_unique<ComputedProperty<TestObject, int>>(
        "poly2", &obj, &TestObject::getComputedInt
    );

    // Can call nameId() through base pointer (virtual dispatch)
    StringId id1 = prop1->nameId();
    StringId id2 = prop2->nameId();

    assert(id1.isValid());
    assert(id2.isValid());
    assert(id1 != id2);

    // Matches direct interning
    assert(id1 == StringInterner::intern("poly1"));
    assert(id2 == StringInterner::intern("poly2"));

    std::cout << "  ✓ nameId() works polymorphically through Property*\n";
}

void testInternCountGrows() {
    StringInterner::clear();

    std::cout << "[Test 6] Intern count grows with unique property names\n";

    TestObject obj;

    assert(StringInterner::internedCount() == 0);

    PropertyRef<TestObject, int> prop1("first", &obj, &TestObject::intValue);
    assert(StringInterner::internedCount() == 1);

    PropertyRef<TestObject, double> prop2("second", &obj, &TestObject::doubleValue);
    assert(StringInterner::internedCount() == 2);

    PropertyRef<TestObject, int> prop3("first", &obj, &TestObject::intValue);  // Duplicate
    assert(StringInterner::internedCount() == 2);  // Count unchanged

    ComputedProperty<TestObject, int> prop4("third", &obj, &TestObject::getComputedInt);
    assert(StringInterner::internedCount() == 3);

    std::cout << "  ✓ Property construction interns names exactly once\n";
}

int main() {
    std::cout << "\n=== Property String ID Integration Test ===\n\n";

    testPropertyRefStringId();
    testComputedPropertyStringId();
    testMultipleCallsStable();
    testNestedPropertyNames();
    testPropertyInterfacePolymorphism();
    testInternCountGrows();

    std::cout << "\n✓ All tests passed!\n\n";

    return 0;
}
