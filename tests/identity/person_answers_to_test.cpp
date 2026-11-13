#include "Identity/PersonPresence.hpp"
#include "Identity/IdentityLedger.hpp"
#include "Identity/KeyStore.hpp"
#include "Person/Person.hpp"
#include "ConstructedBeing/Singular/Object/Creation/ObjectConcept.hpp"
#include <cassert>
#include <iostream>
#include <filesystem>

using namespace Identity;

static void testUnkeyedPerson() {
    // Unkeyed person creation
    ObjectConcept c;
    Person p(Soul("TestUser"), Body::createFromConcept(c), "TestUser");
    assert(personAnswersTo(p, "TestUser"));
    assert(!personAnswersTo(p, "OtherUser"));
    assert(!personAnswersTo(p, ""));
    std::cout << "  unkeyed person answers to display name OK\n";
}

static void testKeyedPersonMatchesIdentifier() {
    ObjectConcept c;
    Person p(Soul("TestUser"), Body::createFromConcept(c), "TestUser");

    // Key the person
    PrivateKey key = PrivateKey::generate();
    assert(p.setPersonId(key.id()));
    assert(p.hasIdentity());

    // The identifier is now did:..., so they should answer to it.
    assert(personAnswersTo(p, p.getIdentifier()));

    // They should not answer to an empty string.
    assert(!personAnswersTo(p, ""));
    std::cout << "  keyed person answers to identifier OK\n";
}

static void testKeyedPersonWithLedger() {
    // If the Person's display name is NOT the queried identifier,
    // it will fall through `person.matchesIdentifier(identifier)`
    // and check the ledger.
    ObjectConcept c;
    Person p(Soul("CurrentName"), Body::createFromConcept(c), "CurrentName");

    PrivateKey key = PrivateKey::generate();
    assert(p.setPersonId(key.id()));
    assert(p.hasIdentity());

    std::string legacyName = "LegacyUser";

    // Before the ledger is populated, they shouldn't answer to the legacy name.
    assert(!personAnswersTo(p, legacyName));

    // Write a ledger mapping "LegacyUser" -> key.id()
    std::filesystem::path ledgerPath = IdentityLedger::defaultPath();
    std::filesystem::create_directories(ledgerPath.parent_path());

    IdentityLedger ledger(ledgerPath);
    ledger.record(legacyName, key.id());
    assert(ledger.save());

    // Now they should answer to it because the ledger maps LegacyUser to their ID.
    assert(personAnswersTo(p, legacyName));

    // But not something else.
    assert(!personAnswersTo(p, "OtherUser"));

    // Cleanup
    std::filesystem::remove(ledgerPath);

    std::cout << "  keyed person answers to legacy name via ledger OK\n";
}

int main() {
    std::cout << "person_answers_to_test:\n";

    // Set a custom HOME directory to isolate KeyStore default path which IdentityLedger uses
    std::filesystem::path scratch = std::filesystem::temp_directory_path() / "test_person_answers_to_home";
    std::filesystem::remove_all(scratch);
    std::filesystem::create_directories(scratch);
    setenv("HOME", scratch.c_str(), 1);

    testUnkeyedPerson();
    testKeyedPersonMatchesIdentifier();
    testKeyedPersonWithLedger();

    std::filesystem::remove_all(scratch);

    std::cout << "person_answers_to_test: ALL OK\n";
    return 0;
}
