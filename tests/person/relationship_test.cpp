#include "Person/Relationship/Relationship.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Person/Body/Body.hpp"
#include <cassert>
#include <iostream>
#include <string>

void test_relationship_instantiation() {
    Soul soulA;
    Body bodyA;
    Person personA(soulA, bodyA, "symbolA");

    Soul soulB;
    Body bodyB;
    Person personB(soulB, bodyB, "symbolB");

    Relationship r("friend", personA, personB, false, 0.5f);

    assert(r.type == "friend");
    assert(r.directed == false);
    assert(r.getWeight() == 0.5f);

    assert(r.involves(personA));
    assert(r.involves(personB));

    assert(r.isBetween(personA, personB));
    assert(r.isBetween(personB, personA));

    std::cout << "test_relationship_instantiation passed" << std::endl;
}

void test_directed_relationship() {
    Soul soulA;
    Body bodyA;
    Person personA(soulA, bodyA, "symbolA");

    Soul soulB;
    Body bodyB;
    Person personB(soulB, bodyB, "symbolB");

    Relationship r("boss", personA, personB, true, 0.8f);

    assert(r.type == "boss");
    assert(r.directed == true);
    assert(r.getWeight() == 0.8f);

    assert(r.isBetween(personA, personB));
    assert(!r.isBetween(personB, personA)); // since it is directed

    std::cout << "test_directed_relationship passed" << std::endl;
}

int main() {
    test_relationship_instantiation();
    test_directed_relationship();
    return 0;
}
