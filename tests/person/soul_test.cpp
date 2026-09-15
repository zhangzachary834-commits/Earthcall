#include "Person/Soul/Soul.hpp"
#include "Person/Person.hpp"
#include "Person/Body/Body.hpp"
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"

#include <cassert>
#include <iostream>

void test_construction_and_bind() {
    // Test initial construction
    Soul s1("InitialName");
    assert(s1.constructionName() == "InitialName");
    assert(s1.person() == nullptr);
    assert(s1.getIdentifier() == "");

    // Test binding clears constructionName and sets person correctly
    Body body;
    Person p(Soul("Temp"), body, "");
    p.setDisplayName("TestPerson");

    Soul s2("ShouldBeCleared");
    s2.bindPerson(&p);

    assert(s2.constructionName() == "");
    assert(s2.person() == &p);
    assert(s2.getIdentifier() == p.getIdentifier());
}

void test_copy_semantics() {
    Soul s1("CopyName");

    Soul s2(s1);
    assert(s2.constructionName() == "CopyName");
    assert(s2.person() == nullptr); // Person ptr is not copied

    Body body;
    Person p(Soul("Temp"), body, "");
    s1.bindPerson(&p);

    Soul s3(s1);
    assert(s3.constructionName() == "");
    assert(s3.person() == nullptr); // Even if s1 has a person, s3's person ptr is not copied

    Soul s4("TargetName");
    s4 = s1;
    assert(s4.constructionName() == "");
    assert(s4.person() == nullptr); // Assignment doesn't copy person ptr
}

void test_move_semantics() {
    Soul s1("MoveName");

    Soul s2(std::move(s1));
    assert(s2.constructionName() == "MoveName");
    assert(s2.person() == nullptr);

    // s1 is in moved-from state, behavior of strings in moved-from is "valid but unspecified"
    // but practically it should be empty for std::string
    assert(s1.constructionName() == "");

    Body body;
    Person p(Soul("Temp"), body, "");
    s2.bindPerson(&p);

    Soul s3("AnotherMoveName");
    s3 = std::move(s2);
    assert(s3.constructionName() == "");
    assert(s3.person() == nullptr); // Move assignment doesn't copy person ptr based on implementation
}

void test_properties() {
    Body body;
    Person p(Soul("Temp"), body, "");
    Soul s("PropSoul");
    s.bindPerson(&p);

    // We expect the property "person" to be accessible and match getIdentifier()
    auto prop = s.findProperty("person");
    assert(prop != nullptr);
    assert(std::get<std::string>(prop->value()) == p.getIdentifier());
}

int main() {
    test_construction_and_bind();
    test_copy_semantics();
    test_move_semantics();
    test_properties();

    std::cout << "Soul tests passed successfully.\n";
    return 0;
}
