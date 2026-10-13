#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Person/Body/Body.hpp"
#include "Identity/IdentityLedger.hpp"
#include "Identity/KeyStore.hpp"
#include "Identity/KeyPair.hpp"
#include <iostream>
#include <filesystem>
#include <cstdlib>

namespace {
int g_checks = 0;
int g_failures = 0;

void check(bool condition, const std::string& description) {
    ++g_checks;
    if (!condition) {
        ++g_failures;
        std::cerr << "  FAILED: " << description << std::endl;
        return;
    }
    std::cout << "  ok: " << description << std::endl;
}

Person createDummyPerson(const std::string& name) {
    Soul soul(name);
    Body body("Humanoid", "Voxel");
    Person person(soul, std::move(body), "");
    person.setDisplayName(name);
    return person;
}
} // namespace

int main() {
    std::cout << "============================================================\n";
    std::cout << "Running zone_manager_legacy_owner_test...\n";
    std::cout << "============================================================\n";

    // Isolate save system to a sandbox
    std::filesystem::path sandbox = std::filesystem::temp_directory_path() / "zone_manager_legacy_owner_test_sandbox";
    std::filesystem::create_directories(sandbox);

    // Set EARTHCALL_HOME so IdentityLedger works in the sandbox
    setenv("EARTHCALL_HOME", sandbox.string().c_str(), 1);

    {
        std::cout << "\n[1] Testing legacyOwnerNamesPerson...\n";

        Zone zone("test_zone", "strict");
        Person person = createDummyPerson("test_person");

        // Setup initial zone state
        zone.setOwner("Alice");

        // Case 1: Person has no identity, owner matches display name
        person.setDisplayName("Alice");
        check(detail::legacyOwnerNamesPerson(zone, person), "Matches when person has no identity and display name matches zone owner");

        // Case 2: Person has no identity, owner does NOT match display name
        person.setDisplayName("Bob");
        check(!detail::legacyOwnerNamesPerson(zone, person), "Fails when person has no identity and display name does not match");

        // Case 3: Zone owner kind restriction
        zone.setQuality("ownerKind", "not-a-person");
        check(!detail::legacyOwnerNamesPerson(zone, person), "Fails when zone ownerKind is not 'Person'");

        // Reset owner kind restriction
        zone.setQuality("ownerKind", "");

        // Case 4: Person HAS identity, display name matches, but NOT in ledger
        Identity::PrivateKey key = Identity::PrivateKey::generate();
        person.setPersonId(key.id());
        person.setDisplayName("Alice");
        check(!detail::legacyOwnerNamesPerson(zone, person), "Fails when person has identity but ledger does not link legacy owner to identity, even if display name matches");

        // Case 5: Exact identifier match
        zone.setOwner(person.getIdentifier());
        check(detail::legacyOwnerNamesPerson(zone, person), "Matches when zone owner exactly matches person's identifier");

        // Reset zone owner to legacy name
        zone.setOwner("Alice");

        // Case 6: Ledger maps legacy name to this identity
        Identity::IdentityLedger ledger;
        ledger.record("Alice", key.id());
        ledger.save(); // Save to sandbox EARTHCALL_HOME/identity/migration-ledger.json

        check(detail::legacyOwnerNamesPerson(zone, person), "Matches when person has identity and ledger maps legacy owner to this identity");

        // Case 7: Ledger maps legacy name to a DIFFERENT identity
        Identity::PrivateKey key2 = Identity::PrivateKey::generate();
        ledger.record("Alice", key2.id());
        ledger.save();

        check(!detail::legacyOwnerNamesPerson(zone, person), "Fails when ledger maps legacy owner to a different identity");
    }

    std::filesystem::remove_all(sandbox);

    std::cout << "------------------------------------------------------------\n";
    std::cout << g_checks - g_failures << "/" << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cerr << "zone_manager_legacy_owner_test: FAILED\n";
        return 1;
    }
    std::cout << "zone_manager_legacy_owner_test: ALL OK\n";
    return 0;
}
