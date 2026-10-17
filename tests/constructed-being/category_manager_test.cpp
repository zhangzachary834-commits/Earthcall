#include "ConstructedBeing/CategoryManager.hpp"
#include <cassert>
#include <iostream>

static void testCreateAndGet() {
    CategoryManager cm;
    auto obj = cm.create("category.test");
    assert(obj != nullptr);
    assert(obj->getIdentifier() == "category.test");
    assert(obj->getPhysicalObject() == 0);

    auto retrieved = cm.get("category.test");
    assert(obj == retrieved);
    std::cout << "  create and get OK\n";
}

static void testRemove() {
    CategoryManager cm;
    cm.create("category.remove_me");
    assert(cm.get("category.remove_me") != nullptr);
    assert(cm.remove("category.remove_me"));
    assert(cm.get("category.remove_me") == nullptr);
    assert(!cm.remove("category.non_existent"));
    std::cout << "  remove OK\n";
}

static void testResolveOrDefault() {
    CategoryManager cm;
    auto obj = cm.create("category.specific");

    assert(cm.resolveOrDefault("category.specific") == obj);
    assert(cm.resolveOrDefault("category.unknown")->getIdentifier() == "category.default");
    std::cout << "  resolve or default OK\n";
}

static void testEnsureDefaults() {
    CategoryManager cm;
    assert(cm.get("category.default") != nullptr);
    auto brush = cm.get("category.tool.brush");
    assert(brush != nullptr);
    assert(brush->getPhysicalObject() == 0);
    assert(brush->hasDynamicProperty("size"));
    assert(brush->hasDynamicProperty("scale"));
    std::cout << "  ensure defaults OK\n";
}

static void testJsonSerialization() {
    CategoryManager cm;
    cm.create("category.custom1");
    auto json = cm.toJson();

    CategoryManager cm2;
    cm2.loadFromJson(json);

    assert(cm2.get("category.custom1") != nullptr);
    assert(cm2.get("category.default") != nullptr); // Should have been added back by ensureDefaults
    std::cout << "  json serialization OK\n";
}

int main() {
    std::cout << "category_manager_test:\n";
    testCreateAndGet();
    testRemove();
    testResolveOrDefault();
    testEnsureDefaults();
    testJsonSerialization();
    std::cout << "category_manager_test: ALL OK\n";
    return 0;
}
