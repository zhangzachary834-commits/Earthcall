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

void test_string_relationship_instantiation() {
    Relationship r("spouse", "person-id-alice", "person-id-bob", false, 1.0f);

    assert(r.aId() == "person-id-alice");
    assert(r.bId() == "person-id-bob");
    assert(r.involves("person-id-alice"));
    assert(r.involves("person-id-bob"));
    assert(!r.involves("person-id-charlie"));

    assert(r.isBetween("person-id-alice", "person-id-bob"));
    assert(r.isBetween("person-id-bob", "person-id-alice"));
    assert(!r.isBetween("person-id-alice", "person-id-charlie"));

    assert(r.getIdentifier() == "person-id-alice-spouse-person-id-bob");

    std::cout << "test_string_relationship_instantiation passed" << std::endl;
}

#include "Identity/SingularId.hpp"
#include <array>

Identity::SingularId makeTestKey(uint8_t fillByte) {
    std::array<uint8_t, 32> keyBytes;
    keyBytes.fill(fillByte);
    return Identity::SingularId::fromPublicKey(keyBytes);
}

void test_identity_matching_and_display_name_distinctness() {
    Soul soulA1, soulA2, soulB1, soulB2;
    Body bodyA1, bodyA2, bodyB1, bodyB2;

    // Two distinct Person instances in memory representing the SAME authenticated Person (key 0x01)
    Person personA1(soulA1, bodyA1, "default");
    personA1.setDisplayName("Alice");
    personA1.setPersonId(makeTestKey(0x01));

    Person personA2(soulA2, bodyA2, "default");
    personA2.setDisplayName("Alice");
    personA2.setPersonId(makeTestKey(0x01));

    // Person B with authenticated key 0x02
    Person personB1(soulB1, bodyB1, "default");
    personB1.setDisplayName("Bob");
    personB1.setPersonId(makeTestKey(0x02));

    Relationship r("friend", personA1, personB1, false, 0.5f);

    // Distinct in-memory Person instance with matching authenticated personId is involved
    assert(r.involves(personA2));
    assert(r.isBetween(personA2, personB1));

    // Two unkeyed Persons sharing display name "Alice"
    Person unkeyedAlice1(soulA1, bodyA1, "default");
    unkeyedAlice1.setDisplayName("Alice");

    Person unkeyedAlice2(soulA2, bodyA2, "default");
    unkeyedAlice2.setDisplayName("Alice");

    Relationship rUnkeyed("friend", unkeyedAlice1, personB1, false, 0.5f);

    // Unkeyed Alice1 is involved
    assert(rUnkeyed.involves(unkeyedAlice1));
    // Unkeyed Alice2 shares spelling "Alice" but is a distinct entity and MUST NOT be conflated
    assert(!rUnkeyed.involves(unkeyedAlice2));
    assert(!rUnkeyed.isBetween(unkeyedAlice2, personB1));

    std::cout << "test_identity_matching_and_display_name_distinctness passed" << std::endl;
}

int main() {
    test_relationship_instantiation();
    test_directed_relationship();
    test_string_relationship_instantiation();
    test_identity_matching_and_display_name_distinctness();
    return 0;
}
