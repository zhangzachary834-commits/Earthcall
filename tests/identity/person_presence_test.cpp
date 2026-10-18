#include "Identity/PersonPresence.hpp"
#include "Identity/KeyStore.hpp"
#include "Identity/FirstMoverRegister.hpp"
#include "Identity/IdentityLedger.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Person/Body/Body.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include <iostream>
#include <cassert>
#include <filesystem>

using namespace Identity;

struct TempSaveRoot {
    std::filesystem::path dir;
    TempSaveRoot() {
        dir = std::filesystem::temp_directory_path() / "earthcall_presence_test_saves";
        std::filesystem::remove_all(dir);
        std::filesystem::create_directories(dir);
        SaveSystem::setSaveRoot(dir.string());
    }
    ~TempSaveRoot() {
        SaveSystem::setSaveRoot("");
        std::filesystem::remove_all(dir);
    }
};

static void testUnkeyedPerson() {
    Soul soul;
    Body body;
    Person person(std::move(soul), std::move(body), "person_presence_test");
    person.setDisplayName("Test Person");
    person.soul().bindPerson(&person);

    assert(!person.hasIdentity());
    assert(personAnswersTo(person, "Test Person"));
    assert(!personAnswersTo(person, "Other Person"));

    std::cout << "  unkeyed person answers to display name OK\n";
}

static void testKeyingAndPresence() {
    Soul soul;
    Body body;
    Person person(std::move(soul), std::move(body), "person_presence_test_2");
    person.setDisplayName("Alice");
    person.soul().bindPerson(&person);

    assert(!person.hasIdentity());
    assert(personAnswersTo(person, "Alice"));

    // Key with too short passphrase
    PresenceResult failResult = keyPresentPerson(person, "short");
    assert(!failResult.ok);
    assert(!person.hasIdentity());

    // Key with valid passphrase
    std::string pass = "correct horse battery staple";
    PresenceResult keyResult = keyPresentPerson(person, pass);
    assert(keyResult.ok);
    assert(person.hasIdentity());

    // Once keyed, the person has a SingularId.
    std::string idStr = person.personId().toString();

    // Now that they are keyed, they are the trusted root
    assert(FirstMoverRegister::instance().isAuthenticatedPerson(person.personId()));

    // Answers to old display name (migrated via ledger)
    assert(personAnswersTo(person, "Alice"));
    assert(personAnswersTo(person, idStr));
    assert(!personAnswersTo(person, "Bob"));

    // Try keying again (should fail)
    PresenceResult rekeyResult = keyPresentPerson(person, "another passphrase entirely");
    assert(!rekeyResult.ok);

    std::cout << "  keying person works and establishes presence OK\n";
}

static void testUnlockPresence() {
    Soul soul;
    Body body;
    Person person(std::move(soul), std::move(body), "person_presence_test_3");
    person.setDisplayName("Bob");
    person.soul().bindPerson(&person);

    std::string pass = "this is bob's passphrase";
    PresenceResult keyResult = keyPresentPerson(person, pass);
    assert(keyResult.ok);
    assert(person.hasIdentity());
    SingularId bobId = person.personId();

    // Reset FirstMoverRegister to simulate fresh boot/re-login
    FirstMoverRegister::instance().clearAuthenticatedPersons();
    assert(!FirstMoverRegister::instance().isAuthenticatedPerson(bobId));

    // Try unlock with empty passphrase
    PresenceResult emptyUnlock = unlockPresentPerson(person, "");
    assert(!emptyUnlock.ok);

    // Try unlock with wrong passphrase
    PresenceResult wrongUnlock = unlockPresentPerson(person, "wrong pass");
    assert(!wrongUnlock.ok);

    // Unlock with right passphrase
    PresenceResult unlock = unlockPresentPerson(person, pass);
    assert(unlock.ok);
    assert(FirstMoverRegister::instance().isAuthenticatedPerson(bobId));

    std::cout << "  unlocking person with passphrase works OK\n";
}

int main() {
    std::cout << "person_presence_test:\n";
    TempSaveRoot saves;
    testUnkeyedPerson();
    testKeyingAndPresence();
    testUnlockPresence();
    std::cout << "person_presence_test: ALL OK\n";
    return 0;
}
