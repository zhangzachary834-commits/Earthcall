#include "Singularity/Core/StringId.hpp"
#include <cassert>
#include <iostream>

using namespace Earthcall;

// ============================================================================
// String ID Interning Test
//
// Verifies the StringInterner's core contract:
// 1. Same string always returns same ID
// 2. Different strings get different IDs
// 3. resolve() recovers the original string
// 4. Default-constructed StringId is invalid
// 5. Comparison operators work correctly
// ============================================================================

void testBasicInterning() {
    StringInterner::clear();  // Start fresh

    std::cout << "[Test 1] Basic interning\n";

    StringId id1 = StringInterner::intern("shape");
    StringId id2 = StringInterner::intern("shape");
    StringId id3 = StringInterner::intern("color");

    // Same string returns same ID
    assert(id1 == id2);
    assert(id1.value == id2.value);

    // Different strings get different IDs
    assert(id1 != id3);
    assert(id1.value != id3.value);

    std::cout << "  ✓ Same string returns same ID\n";
    std::cout << "  ✓ Different strings get different IDs\n";
}

void testResolve() {
    StringInterner::clear();

    std::cout << "[Test 2] Resolve recovers original string\n";

    StringId shapeId = StringInterner::intern("shape");
    StringId colorId = StringInterner::intern("color");
    StringId positionId = StringInterner::intern("position");

    assert(StringInterner::resolve(shapeId) == "shape");
    assert(StringInterner::resolve(colorId) == "color");
    assert(StringInterner::resolve(positionId) == "position");

    std::cout << "  ✓ resolve() returns correct strings\n";
}

void testInvalidIds() {
    StringInterner::clear();

    std::cout << "[Test 3] Invalid IDs\n";

    StringId defaultId;  // Default constructor
    assert(!defaultId.isValid());
    assert(defaultId.value == 0);
    assert(StringInterner::resolve(defaultId) == "");

    StringId outOfBounds(999999);
    assert(StringInterner::resolve(outOfBounds) == "");

    std::cout << "  ✓ Default-constructed ID is invalid\n";
    std::cout << "  ✓ Out-of-bounds ID resolves to empty string\n";
}

void testComparisonOperators() {
    StringInterner::clear();

    std::cout << "[Test 4] Comparison operators\n";

    StringId id1 = StringInterner::intern("alpha");
    StringId id2 = StringInterner::intern("alpha");
    StringId id3 = StringInterner::intern("beta");

    assert(id1 == id2);
    assert(!(id1 != id2));
    assert(id1 != id3);
    assert(!(id1 == id3));

    // Ordering (for std::map, if needed)
    assert((id1 < id3) || (id3 < id1));  // One must be less than the other

    std::cout << "  ✓ Equality operators work\n";
    std::cout << "  ✓ Ordering operator works\n";
}

void testPropertyPathScenario() {
    StringInterner::clear();

    std::cout << "[Test 5] Property path scenario\n";

    // Simulate PropertyPath interning all segments and combinations
    StringId shape = StringInterner::intern("shape");
    StringId shapeColor = StringInterner::intern("shape.color");
    StringId shapeColorR = StringInterner::intern("shape.color.r");

    assert(StringInterner::resolve(shape) == "shape");
    assert(StringInterner::resolve(shapeColor) == "shape.color");
    assert(StringInterner::resolve(shapeColorR) == "shape.color.r");

    // All three are different IDs
    assert(shape != shapeColor);
    assert(shapeColor != shapeColorR);
    assert(shape != shapeColorR);

    std::cout << "  ✓ Nested property paths intern correctly\n";
}

void testInternedCount() {
    StringInterner::clear();

    std::cout << "[Test 6] Interned count tracking\n";

    assert(StringInterner::internedCount() == 0);

    StringInterner::intern("one");
    assert(StringInterner::internedCount() == 1);

    StringInterner::intern("two");
    assert(StringInterner::internedCount() == 2);

    StringInterner::intern("one");  // Already interned
    assert(StringInterner::internedCount() == 2);  // Count unchanged

    StringInterner::intern("three");
    assert(StringInterner::internedCount() == 3);

    std::cout << "  ✓ internedCount() tracks unique strings\n";
}

void testHashFunction() {
    StringInterner::clear();

    std::cout << "[Test 7] Hash function for std::unordered_map\n";

    // Create an unordered_map keyed by StringId
    std::unordered_map<StringId, int> testMap;

    StringId id1 = StringInterner::intern("key1");
    StringId id2 = StringInterner::intern("key2");

    testMap[id1] = 100;
    testMap[id2] = 200;

    assert(testMap[id1] == 100);
    assert(testMap[id2] == 200);
    assert(testMap.size() == 2);

    std::cout << "  ✓ StringId works as unordered_map key\n";
}

int main() {
    std::cout << "\n=== String ID Interning Test Suite ===\n\n";

    testBasicInterning();
    testResolve();
    testInvalidIds();
    testComparisonOperators();
    testPropertyPathScenario();
    testInternedCount();
    testHashFunction();

    std::cout << "\n✓ All tests passed!\n\n";

    return 0;
}
