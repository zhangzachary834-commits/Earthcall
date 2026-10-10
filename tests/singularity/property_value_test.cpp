#include "ConstructedBeing/Singular/Property/PropertyValue.hpp"
#include <cassert>
#include <iostream>
#include <cmath>

void testComparable() {
    std::cout << "[Test 1] isValueComparable\n";
    assert(isValueComparable(PropertyValue(42)));
    assert(isValueComparable(PropertyValue(42.0f)));
    assert(isValueComparable(PropertyValue(42.0)));
    assert(isValueComparable(PropertyValue(true)));
    assert(isValueComparable(PropertyValue('c')));
    assert(isValueComparable(PropertyValue(42L)));
    assert(isValueComparable(PropertyValue(std::string("hello"))));
    assert(isValueComparable(PropertyValue(glm::vec3(1, 2, 3))));
    assert(isValueComparable(PropertyValue(glm::mat4(1))));

    auto dict = std::make_shared<PropertyDict>();
    assert(isValueComparable(PropertyValue(dict)));

    auto list = std::make_shared<PropertyList>();
    assert(isValueComparable(PropertyValue(list)));

    // Test a pointer
    Singular* s = nullptr;
    assert(!isValueComparable(PropertyValue(s)));
}

void testToNumber() {
    std::cout << "[Test 2] propertyValueToNumber\n";
    double out = 0.0;

    assert(propertyValueToNumber(PropertyValue(42), out));
    assert(out == 42.0);

    assert(propertyValueToNumber(PropertyValue(42.5f), out));
    assert(std::fabs(out - 42.5) < 1e-6);

    assert(propertyValueToNumber(PropertyValue(true), out));
    assert(out == 1.0);

    assert(!propertyValueToNumber(PropertyValue(std::string("42")), out));
}

void testUnchanged() {
    std::cout << "[Test 3] propertyValueUnchanged\n";

    assert(propertyValueUnchanged(PropertyValue(42), PropertyValue(42)));
    assert(!propertyValueUnchanged(PropertyValue(42), PropertyValue(43)));
    assert(!propertyValueUnchanged(PropertyValue(42), PropertyValue(42.0f))); // different types

    auto list1 = std::make_shared<PropertyList>();
    list1->elements.push_back(PropertyValue(1));

    auto list2 = std::make_shared<PropertyList>();
    list2->elements.push_back(PropertyValue(1));

    assert(propertyValueUnchanged(PropertyValue(list1), PropertyValue(list2)));

    list2->elements[0] = PropertyValue(2);
    assert(!propertyValueUnchanged(PropertyValue(list1), PropertyValue(list2)));
}

void testEquivalent() {
    std::cout << "[Test 4] propertyValuesEquivalent\n";

    assert(propertyValuesEquivalent(PropertyValue(42), PropertyValue(42.0)));
    assert(propertyValuesEquivalent(PropertyValue(1), PropertyValue(true)));
    assert(!propertyValuesEquivalent(PropertyValue(42), PropertyValue(43.0)));
    assert(propertyValuesEquivalent(PropertyValue(std::string("foo")), PropertyValue(std::string("foo"))));
    assert(!propertyValuesEquivalent(PropertyValue(std::string("foo")), PropertyValue(std::string("bar"))));
}

int main() {
    std::cout << "\n=== Property Value Test ===\n\n";
    testComparable();
    testToNumber();
    testUnchanged();
    testEquivalent();
    std::cout << "\n✓ All tests passed!\n\n";
    return 0;
}
