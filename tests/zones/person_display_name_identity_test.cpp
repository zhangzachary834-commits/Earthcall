#include "HomesOfEarth/Home.hpp"
#include "Identity/SingularId.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Relation/Relation.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"

#include <array>
#include <iostream>
#include <memory>
#include <string>

namespace {

int failures = 0;

void check(bool ok, const std::string& what) {
    if (!ok) {
        ++failures;
        std::cerr << "FAILED: " << what << "\n";
    } else {
        std::cout << "ok: " << what << "\n";
    }
}

Identity::SingularId makeTestKey(uint8_t fillByte) {
    std::array<uint8_t, 32> keyBytes;
    keyBytes.fill(fillByte);
    return Identity::SingularId::fromPublicKey(keyBytes);
}

} // namespace

int main() {
    // Demonstrates distinct Person identities sharing the same display name spelling.
    // A name may point. It must not silently constitute identity or ownership.

    Soul soul1;
    Body body1;
    Person person1(soul1, body1, "default");
    person1.setDisplayName("Alice");
    person1.setPersonId(makeTestKey(0x01));

    Soul soul2;
    Body body2;
    Person person2(soul2, body2, "default");
    person2.setDisplayName("Alice");
    person2.setPersonId(makeTestKey(0x02));

    check(person1.getDisplayName() == person2.getDisplayName(), "Persons share same display name 'Alice'");
    check(person1.getIdentifier() != person2.getIdentifier(), "Persons have distinct cryptographic identifiers");

    ZoneManager manager;
    check(manager.ensureHomeZone(person1), "Home zone ensured for person1");

    const Zone* home1 = manager.findPrimaryHome(person1);
    check(home1 != nullptr, "findPrimaryHome(person1) finds Home for person1");

    const Zone* home2 = manager.findPrimaryHome(person2);
    check(home2 == nullptr, "findPrimaryHome(person2) returns nullptr despite matching display name");

    check(manager.findPrimaryHome(person1.getIdentifier()) == home1, "findPrimaryHome by person1 identifier succeeds");
    check(manager.findPrimaryHome(person2.getIdentifier()) == nullptr, "findPrimaryHome by person2 identifier returns nullptr");
    check(manager.findPrimaryHome(person1.getDisplayName()) == nullptr, "findPrimaryHome by display name 'Alice' returns nullptr when identity exists");

    if (failures > 0) {
        std::cerr << failures << " test failure(s).\n";
        return 1;
    }
    std::cout << "All display name identity tests passed.\n";
    return 0;
}
