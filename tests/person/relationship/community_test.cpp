#include "Person/Relationship/Community/Community.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Identity/KeyPair.hpp"
#include "Identity/IdentityLedger.hpp"
#include "Person/Body/Body.hpp"
#include "ConstructedBeing/Singular/Singular.hpp"

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

namespace {
    int g_checks = 0;
    int g_failures = 0;

    void check(bool condition, const std::string& description) {
        ++g_checks;
        if (!condition) {
            ++g_failures;
            std::cout << "  FAILED: " << description << "\n";
        }
    }

    Person createDummyPerson(const std::string& name) {
        Soul soul(name);
        Body body("Humanoid", "Voxel");
        Person person(soul, std::move(body), "");
        person.setDisplayName(name);
        return person;
    }

    class DummySingular : public Singular {
    public:
        DummySingular(const std::string& name) : _name(name) {}
        std::string getIdentifier() const override { return _name; }
    protected:
        void buildProperties() override {}
    private:
        std::string _name;
    };
}

static void testBasicInstantiation() {
    Community comm("Earthcall Devs");
    check(comm.getIdentifier() == "Earthcall Devs", "Community getIdentifier returns the name");
}

static void testDescribe() {
    Community comm("Earthcall Devs");
    // Just ensure it doesn't crash
    comm.describe();
    check(true, "Describe executed without crashing");
}

static void testAddMemberAndInvolves() {
    Community comm("Players");

    Person alice = createDummyPerson("Alice");
    Person bob = createDummyPerson("Bob");

    check(!comm.involves(alice), "Community should not involve Alice initially");
    check(!comm.involves("Bob"), "Community should not involve Bob initially");

    comm.addMember(&alice);
    check(comm.involves(alice), "Community involves Alice after adding her");
    check(comm.involves("Alice"), "Community involves 'Alice' string after adding her");

    // Regression test: distinct unkeyed Person with same display name ("Alice") must NOT be conflated.
    Person alice2 = createDummyPerson("Alice");
    check(!comm.involves(alice2), "Community must NOT involve distinct Person 'alice2' sharing the same display name 'Alice'");

    // Regression test: distinct keyed Persons sharing display name "Alice" must NEVER be conflated.
    Person keyedAlice1 = createDummyPerson("Alice");
    Identity::PrivateKey priv1 = Identity::PrivateKey::generate();
    keyedAlice1.setPersonId(priv1.publicKey().id());

    Person keyedAlice2 = createDummyPerson("Alice");
    Identity::PrivateKey priv2 = Identity::PrivateKey::generate();
    keyedAlice2.setPersonId(priv2.publicKey().id());

    Community commKeyed("Keyed Group");
    commKeyed.addMember(&keyedAlice1);

    check(commKeyed.involves(keyedAlice1), "commKeyed involves keyedAlice1 by key identity");
    check(!commKeyed.involves(keyedAlice2), "commKeyed MUST NOT involve distinct keyedAlice2 sharing display name 'Alice'");
    check(!commKeyed.involves("Alice"), "commKeyed MUST NOT match unverified display name string 'Alice' for keyed person");
    check(commKeyed.involves(keyedAlice1.getIdentifier()), "keyed Person is addressable by key string");

    Person sameIdentity = createDummyPerson("Another Display Name");
    check(sameIdentity.setPersonId(keyedAlice1.personId()), "test alias accepts same cryptographic identity");
    check(commKeyed.involves(sameIdentity), "same keyed identity across Person instances is involved");

    Person unkeyedAlice = createDummyPerson("Alice");
    check(!commKeyed.involves(unkeyedAlice), "unkeyed Person sharing a keyed member's name is distinct");

    // A legacy Person's display name can spell a key identifier. Such a name
    // must not impersonate the keyed Person in either object-query direction.
    Person nameSpoof = createDummyPerson(keyedAlice1.getIdentifier());
    check(!commKeyed.involves(nameSpoof), "unkeyed Person named after key cannot impersonate keyed member");
    Community legacyCommunity("Legacy Group");
    legacyCommunity.addMember(&nameSpoof);
    check(!legacyCommunity.involves(keyedAlice1), "keyed Person cannot impersonate unkeyed member via its name");

    comm.addMember(&bob);
    check(comm.involves(bob), "Community involves Bob after adding him");

    // Once a Person has cryptographic identity, display spelling is not identity.
    Person keyedAlice = createDummyPerson("AliceKeyed");
    Identity::PrivateKey priv = Identity::PrivateKey::generate();
    keyedAlice.setPersonId(priv.publicKey().id());
    comm.addMember(&keyedAlice);
    check(comm.involves(keyedAlice), "Community involves keyedAlice by object reference");
    check(comm.involves(keyedAlice.getIdentifier()), "Community involves keyedAlice by SingularId identifier");
    check(!comm.involves(keyedAlice.getDisplayName()), "Community does not involve keyedAlice by display name string");

    // Add a non-Person member, it should be rejected and print a warning
    DummySingular dummy("NotAPerson");
    comm.addMember(&dummy);
    check(!comm.involves(dummy), "Community should not involve a non-Person after attempting to add it");
    check(!comm.involves("NotAPerson"), "Community should not involve 'NotAPerson' string after attempting to add it");
}

static void testMigratedNameBridgeDoesNotMergePersonObjects() {
    // Isolate the ledger completely: never read or write the real Person's
    // EARTHCALL_HOME, even when the test is launched from a developer machine.
    const char* previous = std::getenv("EARTHCALL_HOME");
    const bool hadPrevious = previous != nullptr;
    const std::string oldHome = hadPrevious ? previous : "";
    const auto root = std::filesystem::temp_directory_path() /
        ("earthcall-community-bridge-" +
         std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root);
#if defined(_WIN32)
    const int setResult = _putenv_s("EARTHCALL_HOME", root.string().c_str());
#else
    const int setResult = setenv("EARTHCALL_HOME", root.string().c_str(), 1);
#endif
    check(setResult == 0, "test uses an isolated EARTHCALL_HOME");

    if (setResult == 0) {
        Person keyed = createDummyPerson("New Name");
        const auto id = Identity::PrivateKey::generate().id();
        check(keyed.setPersonId(id), "migrated Person accepts identity");
        Identity::IdentityLedger ledger;
        ledger.record("Old Name", id);
        check(ledger.save(), "isolated legacy-name ledger persists");

        Community community("Migrated Group");
        community.addMember(&keyed);
        check(community.involves("Old Name"), "legacy string resolves to keyed Person through ledger");
        Person distinctLegacy = createDummyPerson("Old Name");
        check(!community.involves(distinctLegacy),
              "ledger bridge must not equate distinct unkeyed Person with keyed Person");
    }

#if defined(_WIN32)
    if (hadPrevious) _putenv_s("EARTHCALL_HOME", oldHome.c_str());
    else _putenv_s("EARTHCALL_HOME", "");
#else
    if (hadPrevious) setenv("EARTHCALL_HOME", oldHome.c_str(), 1);
    else unsetenv("EARTHCALL_HOME");
#endif
    std::filesystem::remove_all(root);
}

static void testIsBetween() {
    Community comm("Gamers");

    Person alice = createDummyPerson("Alice");
    Person bob = createDummyPerson("Bob");
    Person charlie = createDummyPerson("Charlie");

    comm.addMember(&alice);
    comm.addMember(&bob);

    check(comm.isBetween(alice, bob), "Community is between Alice and Bob");
    check(comm.isBetween("Alice", "Bob"), "Community is between 'Alice' and 'Bob' strings");

    check(!comm.isBetween(alice, charlie), "Community is not between Alice and Charlie");
    check(!comm.isBetween("Bob", "Charlie"), "Community is not between 'Bob' and 'Charlie' strings");
    check(!comm.isBetween(charlie, bob), "Community is not between Charlie and Bob");
}

int main() {
    std::cout << "community_test:\n";
    testBasicInstantiation();
    testDescribe();
    testAddMemberAndInvolves();
    testMigratedNameBridgeDoesNotMergePersonObjects();
    testIsBetween();

    if (g_failures == 0) {
        std::cout << "community_test: ALL OK (" << g_checks << " checks passed)\n";
        return 0;
    } else {
        std::cout << "community_test: FAILED (" << g_failures << " out of " << g_checks << " checks failed)\n";
        return 1;
    }
}
