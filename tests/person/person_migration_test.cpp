// Migrating a name-addressed world into an identity-bearing one.
//
// The property that matters: after migration, editing the save no longer
// transfers ownership. Before it, editing the save WAS the transfer.
#include "Identity/IdentityLedger.hpp"
#include "Identity/KeyStore.hpp"
#include "Identity/PersonMigration.hpp"

#include <cassert>
#include <filesystem>
#include <iostream>

using namespace Identity;

namespace {

const char* kPass = "test-passphrase";

struct Sandbox {
    std::filesystem::path dir;
    KeyStore keys;
    IdentityLedger ledger;

    explicit Sandbox(const std::string& name)
        : dir(std::filesystem::temp_directory_path() / ("earthcall_mig_" + name)),
          keys((std::filesystem::remove_all(
                    std::filesystem::temp_directory_path() / ("earthcall_mig_" + name)),
                std::filesystem::temp_directory_path() / ("earthcall_mig_" + name) / "keys")),
          ledger(std::filesystem::temp_directory_path() / ("earthcall_mig_" + name) /
                 "ledger.json") {}

    ~Sandbox() { std::filesystem::remove_all(dir); }
};

// A world as it looked before identities existed.
nlohmann::json legacySave() {
    return nlohmann::json{
        {"currentZone", 0},
        {"zones",
         nlohmann::json::array(
             {nlohmann::json{{"name", "Home"},
                             {"owner", "Zach"},
                             {"deletable", {{"Zach", true}, {"Guest", false}}}},
              nlohmann::json{{"name", "Commons"}, {"owner", ""}}})},
        {"authoredLaws",
         {{"laws", nlohmann::json::array({nlohmann::json{
                       {"id", "law-1"},
                       {"authors", nlohmann::json::array({"Zach"})},
                       {"targets", nlohmann::json::array({"pillar-north"})}}})}}},
    };
}

} // namespace

static void testMigrationMintsAndRewrites() {
    Sandbox box("basic");
    nlohmann::json save = legacySave();

    assert(!isMigrated(save));
    MigrationReport r = migrateSave(save, box.ledger, box.keys, kPass, 1000);

    assert(r.ran);
    assert(r.identitiesMinted >= 1);
    assert(r.ownersRewritten == 1);

    // The forgeable field is gone, replaced by an id and a signature.
    assert(!save["zones"][0].contains("owner"));
    assert(save["zones"][0].contains("ownerId"));
    assert(save["zones"][0].contains("ownerClaim"));
    assert(save["zones"][0].contains("zoneId"));

    // And it verifies.
    assert(verifyOwnership(save).empty());

    // The unowned zone stays unowned rather than acquiring anyone.
    assert(!save["zones"][1].contains("ownerId"));

    std::cout << "  legacy world migrates and its ownership verifies OK\n";
    std::cout << "    " << r.summary() << "\n";
}

static void testEditingOwnershipNoLongerWorks() {
    // This is the whole point. Before migration, this edit WAS the transfer.
    Sandbox box("tamper");
    nlohmann::json save = legacySave();
    migrateSave(save, box.ledger, box.keys, kPass, 1000);
    assert(verifyOwnership(save).empty());

    // Mallory mints her own identity and writes herself in as owner.
    PrivateKey mallory = PrivateKey::generate();

    nlohmann::json stolen = save;
    stolen["zones"][0]["ownerId"] = mallory.id().toString();
    assert(verifyOwnership(stolen).size() == 1);

    // Re-pointing the claim at a zone she does own does not help either.
    nlohmann::json retargeted = save;
    retargeted["zones"][0]["zoneId"] = SingularId::mintOpaque().toString();
    assert(verifyOwnership(retargeted).size() == 1);

    // Dropping the signature is not a way out.
    nlohmann::json unsigned_ = save;
    unsigned_["zones"][0]["ownerClaim"]["signature"] = "";
    assert(verifyOwnership(unsigned_).size() == 1);

    // Neither is reviving the old string field alongside it.
    nlohmann::json revived = save;
    revived["zones"][0]["owner"] = "Mallory";
    assert(verifyOwnership(revived).empty());       // the claim still governs ...
    assert(revived["zones"][0]["ownerId"] ==
           save["zones"][0]["ownerId"]);            // ... and it still names Zach

    std::cout << "  post-migration ownership cannot be edited into place OK\n";
}

static void testIdempotent() {
    Sandbox box("idempotent");
    nlohmann::json save = legacySave();

    MigrationReport first = migrateSave(save, box.ledger, box.keys, kPass, 1000);
    assert(first.ran);
    nlohmann::json afterFirst = save;

    MigrationReport second = migrateSave(save, box.ledger, box.keys, kPass, 2000);
    assert(!second.ran);
    assert(save == afterFirst); // untouched, not re-signed with a new nonce

    std::cout << "  migration is idempotent OK\n";
}

static void testSameNameSameIdentityAcrossSaves() {
    // Two legacy worlds both naming "Zach" must yield ONE Zach, or his history
    // splits in half at the moment of migration.
    Sandbox box("continuity");

    nlohmann::json worldA = legacySave();
    nlohmann::json worldB = legacySave();
    worldB["zones"][0]["name"] = "Workshop";

    migrateSave(worldA, box.ledger, box.keys, kPass, 1000);
    MigrationReport second = migrateSave(worldB, box.ledger, box.keys, kPass, 2000);

    assert(worldA["zones"][0]["ownerId"] == worldB["zones"][0]["ownerId"]);
    assert(second.identitiesReused >= 1);
    assert(second.identitiesMinted == 0);

    // Distinct zones, though -- two worlds' Homes are not the same place.
    assert(worldA["zones"][0]["zoneId"] != worldB["zones"][0]["zoneId"]);

    std::cout << "  one name resolves to one identity across worlds OK\n";
}

static void testLedgerSurvivesReload() {
    Sandbox box("ledger");
    nlohmann::json save = legacySave();
    migrateSave(save, box.ledger, box.keys, kPass, 1000);

    IdentityLedger reopened(box.ledger.path());
    assert(reopened.load());
    auto zach = reopened.find("Zach");
    assert(zach.has_value());
    assert(zach->toString() == save["zones"][0]["ownerId"].get<std::string>());

    std::cout << "  ledger survives reload OK\n";
}

static void testLawAuthorsMigrateButNonPersonsDoNot() {
    Sandbox box("authors");
    nlohmann::json save = legacySave();
    migrateSave(save, box.ledger, box.keys, kPass, 1000);

    const auto& law = save["authoredLaws"]["laws"][0];

    // "Zach" is a confirmed Person (he owns a zone), so authorship follows him.
    assert(law["authors"][0].get<std::string>().rfind("did:earthcall:", 0) == 0);

    // "pillar-north" is an Object, not a Person. Rewriting it would silently
    // detach the law from what it acts on.
    assert(law["targets"][0].get<std::string>() == "pillar-north");

    std::cout << "  law authorship migrates, non-Person targets untouched OK\n";
}

static void testDeletabilityRekeyed() {
    Sandbox box("deletability");
    nlohmann::json save = legacySave();
    migrateSave(save, box.ledger, box.keys, kPass, 1000);

    const auto& del = save["zones"][0]["deletable"];
    const std::string zachId = save["zones"][0]["ownerId"];

    // Zach's entry follows his identity.
    assert(del.contains(zachId));
    assert(del[zachId] == true);
    assert(!del.contains("Zach"));

    // "Guest" owns nothing, but the deletability map is keyed per Person by
    // definition, so appearing in it IS what confirms a Person. Guest gets an
    // identity too -- and must, because leaving the entry under a bare name
    // would let anyone claiming that name inherit the permission.
    assert(!del.contains("Guest"));
    auto guest = box.ledger.find("Guest");
    assert(guest.has_value());
    assert(del.contains(guest->toString()));
    assert(del[guest->toString()] == false); // and the value is carried across

    std::cout << "  deletability rekeyed for every Person in the map OK\n";
}

static void testMissingKeyDoesNotRemint() {
    // If the ledger names an identity whose key is gone, reminting would hand
    // back an id that can never sign. Refusing is the safer failure.
    Sandbox box("missingkey");
    nlohmann::json save = legacySave();
    migrateSave(save, box.ledger, box.keys, kPass, 1000);

    SingularId zach = *box.ledger.find("Zach");
    assert(box.keys.remove(zach));

    nlohmann::json other = legacySave();
    MigrationReport r = migrateSave(other, box.ledger, box.keys, kPass, 2000);

    assert(r.ran);
    assert(!r.warnings.empty());
    assert(r.ownersRewritten == 0);   // ownership left unsigned rather than forged
    assert(!other["zones"][0].contains("ownerId"));

    std::cout << "  missing key refuses to remint an identity OK\n";
}

static void testEmptyAndMalformedSaves() {
    Sandbox box("malformed");

    nlohmann::json empty = nlohmann::json::object();
    MigrationReport r1 = migrateSave(empty, box.ledger, box.keys, kPass, 1000);
    assert(r1.ran && r1.ownersRewritten == 0);

    nlohmann::json notAnObject = nlohmann::json::array();
    MigrationReport r2 = migrateSave(notAnObject, box.ledger, box.keys, kPass, 1000);
    assert(!r2.ran && !r2.warnings.empty());

    nlohmann::json wrongTypes = {{"zones", "not an array"}};
    MigrationReport r3 = migrateSave(wrongTypes, box.ledger, box.keys, kPass, 1000);
    assert(r3.ran);

    assert(verifyOwnership(nlohmann::json::object()).empty());

    std::cout << "  empty and malformed saves handled without crashing OK\n";
}

int main() {
    std::cout << "person_migration_test:\n";
    testMigrationMintsAndRewrites();
    testEditingOwnershipNoLongerWorks();
    testIdempotent();
    testSameNameSameIdentityAcrossSaves();
    testLedgerSurvivesReload();
    testLawAuthorsMigrateButNonPersonsDoNot();
    testDeletabilityRekeyed();
    testMissingKeyDoesNotRemint();
    testEmptyAndMalformedSaves();
    std::cout << "person_migration_test: ALL OK\n";
    return 0;
}
