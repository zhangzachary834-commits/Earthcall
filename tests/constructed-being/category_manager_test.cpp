#include "ConstructedBeing/CategoryManager.hpp"
#include <cassert>
#include <iostream>
#include <string>

static void testDefaultCategories() {
    CategoryManager categories;
    auto def = categories.get("category.default");
    assert(def != nullptr);
    assert(def->getIdentifier() == "category.default");
    assert(def->getPhysicalObject() == 0);

    auto brush = categories.get("category.tool.brush");
    assert(brush != nullptr);
    assert(brush->getIdentifier() == "category.tool.brush");
    assert(brush->getPhysicalObject() == 0);
    assert(brush->hasDynamicProperty("size"));
    assert(brush->hasDynamicProperty("scale"));

    std::cout << "  default categories exist OK\n";
}

static void testCreateNewCategory() {
    CategoryManager categories;
    auto cat = categories.create("category.weapon.sword");
    assert(cat != nullptr);
    assert(cat->getIdentifier() == "category.weapon.sword");

    assert(cat->getPhysicalObject() == 0);

    auto same = categories.get("category.weapon.sword");
    assert(same == cat);

    auto createdSame = categories.create("category.weapon.sword");
    assert(createdSame == cat);

    std::cout << "  create new category OK\n";
}

static void testAddExistingCategory() {
    CategoryManager categories;
    auto cat = std::make_shared<Object>("category.magic.wand");
    cat->setPhysicalObject(1); // Set to physical to ensure add doesn't override it initially
    categories.add(cat);

    auto retrieved = categories.get("category.magic.wand");
    assert(retrieved == cat);

    std::cout << "  add existing category OK\n";
}

static void testRemoveCategory() {
    CategoryManager categories;
    categories.create("category.item.potion");
    assert(categories.get("category.item.potion") != nullptr);

    assert(categories.remove("category.item.potion") == true);
    assert(categories.get("category.item.potion") == nullptr);

    assert(categories.remove("category.not.exist") == false);

    std::cout << "  remove category OK\n";
}

static void testResolveOrDefault() {
    CategoryManager categories;
    categories.create("category.known");

    auto known = categories.resolveOrDefault("category.known");
    assert(known != nullptr);
    assert(known->getIdentifier() == "category.known");

    auto unknown = categories.resolveOrDefault("category.unknown");
    assert(unknown != nullptr);
    assert(unknown->getIdentifier() == "category.default");

    std::cout << "  resolve or default OK\n";
}

static void testSerializationRoundtrip() {
    CategoryManager categories1;
    auto myCat = categories1.create("category.custom.test");
    myCat->setDynamicProperty("customProp", PropertyValue(42.0));

    nlohmann::json j = categories1.toJson();

    CategoryManager categories2;
    categories2.loadFromJson(j);

    auto retrieved = categories2.get("category.custom.test");
    assert(retrieved != nullptr);
    assert(retrieved->getIdentifier() == "category.custom.test");
    assert(retrieved->getPhysicalObject() == 0);
    assert(retrieved->hasDynamicProperty("customProp"));

    PropertyValue val;
    assert(retrieved->getDynamicProperty("customProp", val));
    double numVal;
    assert(propertyValueToNumber(val, numVal));
    assert(numVal == 42.0);

    std::cout << "  serialization roundtrip OK\n";
}

int main() {
    std::cout << "category_manager_test:\n";
    testDefaultCategories();
    testCreateNewCategory();
    testAddExistingCategory();
    testRemoveCategory();
    testResolveOrDefault();
    testSerializationRoundtrip();
    std::cout << "category_manager_test: ALL OK\n";
    return 0;
}
