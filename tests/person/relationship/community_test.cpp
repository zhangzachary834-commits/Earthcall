#include "Person/Relationship/Community/Community.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Person/Body/Body.hpp"
#include "ConstructedBeing/Singular/Singular.hpp"

#include <cassert>
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

    comm.addMember(&bob);
    check(comm.involves(bob), "Community involves Bob after adding him");

    // Add a non-Person member, it should be rejected and print a warning
    DummySingular dummy("NotAPerson");
    comm.addMember(&dummy);
    check(!comm.involves(dummy), "Community should not involve a non-Person after attempting to add it");
    check(!comm.involves("NotAPerson"), "Community should not involve 'NotAPerson' string after attempting to add it");
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
    testIsBetween();

    if (g_failures == 0) {
        std::cout << "community_test: ALL OK (" << g_checks << " checks passed)\n";
        return 0;
    } else {
        std::cout << "community_test: FAILED (" << g_failures << " out of " << g_checks << " checks failed)\n";
        return 1;
    }
}
