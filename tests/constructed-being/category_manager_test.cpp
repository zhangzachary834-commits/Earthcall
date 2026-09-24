#include "ConstructedBeing/CategoryManager.hpp"
#include "Person/Person.hpp"
#include "Person/PersonDatabase.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "test_save_helper.hpp"
#include <cassert>
#include <iostream>

void test_basic_crud() {
    CategoryManager mgr;
    auto cat1 = mgr.create("category.test.1");
    assert(cat1 != nullptr);
    assert(cat1->getIdentifier() == "category.test.1");
    assert(mgr.get("category.test.1") == cat1);

    auto cat2 = mgr.create("category.test.1");
    assert(cat1 == cat2);

    assert(mgr.remove("category.test.1"));
    assert(!mgr.remove("category.test.1"));
    assert(mgr.get("category.test.1") == nullptr);
}

void test_defaults() {
    CategoryManager mgr;
    assert(mgr.get("category.default") != nullptr);
    assert(mgr.get("category.tool.brush") != nullptr);
}

void test_serialization() {
    CategoryManager mgr1;
    mgr1.create("category.custom.A");
    auto j = mgr1.toJson();

    CategoryManager mgr2;
    mgr2.loadFromJson(j);
    assert(mgr2.get("category.custom.A") != nullptr);
    assert(mgr2.get("category.default") != nullptr); // Should ensure defaults
}

int main() {
    test_basic_crud();
    test_defaults();
    test_serialization();
    std::cout << "CategoryManager tests passed!" << std::endl;
    return 0;
}
