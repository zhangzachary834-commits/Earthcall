// Headless P0 witness for Make the Earth Inhabitable.
//
// Proves that Home ownership follows stable Person identity across persistence
// and fresh hydration, and that duplicate primaries refuse instead of silently
// choosing a winner or minting another Home. No GLFW/OpenGL context required.

#include "HomesOfEarth/Home.hpp"
#include "Identity/SingularId.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Relation/Relation.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"

#include <array>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

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

bool hasOwnedBy(const Zone& home, const Person& person) {
    for (const auto& relation : home.getFormation().relations().getAll()) {
        if (!relation || relation->type != "owned-by" || !relation->directed) continue;
        const bool fromHome =
            relation->a() == &home || relation->aId() == home.getIdentifier();
        const bool toPerson =
            relation->b() == &person || relation->bId() == person.getIdentifier();
        if (fromHome && toPerson) return true;
    }
    return false;
}

} // namespace

int main() {
    namespace fs = std::filesystem;

    const fs::path sandbox =
        fs::temp_directory_path() / "earthcall_home_identity_continuity_headless";
    std::error_code ec;
    fs::remove_all(sandbox, ec);
    ec.clear();
    fs::create_directories(sandbox, ec);
    if (ec) {
        std::cerr << "FAILED: cannot create sandbox: " << ec.message() << "\n";
        return 1;
    }
    SaveSystem::setSaveRoot(sandbox.string());

    // Deterministic dummy key material. This is NOT Zach's real Person identity.
    std::array<uint8_t, 32> keyBytes{};
    for (std::size_t i = 0; i < keyBytes.size(); ++i) {
        keyBytes[i] = static_cast<uint8_t>(i + 1);
    }
    const Identity::SingularId identity =
        Identity::SingularId::fromPublicKey(keyBytes);

    {
        Soul soul("First Spelling");
        Body body("humanoid", "default");
        Person first(std::move(soul), std::move(body), "default");
        first.setPersonId(identity);

        Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
            beings.push_back(&first);
        });

        ZoneManager firstProcess;
        check(firstProcess.primaryHomeCount(first) == 0,
              "fresh Person begins with zero primary Homes before kernel repair");
        check(firstProcess.enforcePrimaryHomeInvariant(first),
              "kernel admission repairs zero primary Homes to at least one");
        check(firstProcess.primaryHomeCount(first) == 1,
              "kernel repair establishes exactly one primary Home from an empty store");
        Zone* home = firstProcess.findPrimaryHome(first);
        check(home != nullptr, "primary Home resolves through the Person");
        check(home && home->getIdentifier() == "Home",
              "first primary Home keeps canonical Home identity");
        check(home && hasOwnedBy(*home, first),
              "Home carries Zone -> owned-by -> Person");

        firstProcess.persistZones();
        const nlohmann::json saved = SaveSystem::readHomeIdentity("Home");
        check(saved.is_object(), "Home identity persisted");
        check(saved.value("owner", std::string{}) == identity.toString(),
              "persisted owner cache is the stable Person identity");

        bool persistedEdge = false;
        for (const auto& relation :
             saved.value("formationRelations", nlohmann::json::array())) {
            if (relation.value("type", std::string{}) == "owned-by"
                && relation.value("entityA", std::string{}) == "Home"
                && relation.value("entityB", std::string{}) == identity.toString()
                && relation.value("directed", false)) {
                persistedEdge = true;
            }
        }
        check(persistedEdge,
              "owned-by persists with the stable Person identity");
        Universe::instance().setProvider({});
    }

    {
        // Fresh Person object, different display spelling, same stable identity.
        Soul soul("Renamed Display");
        Body body("humanoid", "default");
        Person returned(std::move(soul), std::move(body), "default");
        returned.setPersonId(identity);

        Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
            beings.push_back(&returned);
        });

        ZoneManager returnedProcess;
        returnedProcess.hydrateFromZoneStore();
        Zone* home = returnedProcess.findPrimaryHome(returned);
        check(home != nullptr,
              "fresh hydration resolves Home through stable Person identity");
        check(home && home->getIdentifier() == "Home",
              "fresh hydration returns the same Home identity");
        check(home && hasOwnedBy(*home, returned),
              "hydrated owned-by edge binds to the returned Person");

        const std::size_t before = returnedProcess.zones().size();
        check(returnedProcess.enforcePrimaryHomeInvariant(returned),
              "fresh return satisfies the primary Home admission invariant");
        check(returnedProcess.primaryHomeCount(returned) == 1,
              "fresh return still has one primary Home");
        check(returnedProcess.zones().size() == before,
              "fresh return mints no duplicate Home");
        Universe::instance().setProvider({});
    }

    {
        Soul soul("Ambiguous");
        Body body("humanoid", "default");
        Person ambiguous(std::move(soul), std::move(body), "default");
        ambiguous.setPersonId(identity);

        ZoneManager manager;
        auto a = std::make_shared<Home>("PrimaryA", "strict");
        a->markPrimaryHome();
        a->setOwner(identity.toString(), Zone::kOwnerKindPerson);
        auto b = std::make_shared<Home>("PrimaryB", "strict");
        b->markPrimaryHome();
        b->setOwner(identity.toString(), Zone::kOwnerKindPerson);
        manager.addZone(a);
        manager.addZone(b);

        const std::size_t before = manager.zones().size();
        check(manager.primaryHomeCount(ambiguous) == 2,
              "two primaries both count toward the existential Home invariant");
        check(manager.findPrimaryHome(ambiguous) == nullptr,
              "two primaries refuse unique resolution instead of choosing by order");
        check(manager.enforcePrimaryHomeInvariant(ambiguous),
              "duplicate primaries still satisfy at-least-one-Home admission");
        check(manager.zones().size() == before,
              "duplicate-primary ambiguity never mints a third Home");
    }

    {
        // An authoritative owned-by edge outranks the compatibility owner cache.
        // A stale owner string must never trick the admission invariant.
        std::array<uint8_t, 32> otherBytes{};
        for (std::size_t i = 0; i < otherBytes.size(); ++i) {
            otherBytes[i] = static_cast<uint8_t>(0x80 + i);
        }
        const Identity::SingularId otherIdentity =
            Identity::SingularId::fromPublicKey(otherBytes);

        Soul subjectSoul("Subject");
        Body subjectBody("humanoid", "default");
        Person subject(std::move(subjectSoul), std::move(subjectBody), "default");
        subject.setPersonId(identity);

        Soul otherSoul("Other");
        Body otherBody("humanoid", "default");
        Person other(std::move(otherSoul), std::move(otherBody), "default");
        other.setPersonId(otherIdentity);

        ZoneManager manager;
        auto conflicted = std::make_shared<Home>("ConflictedPrimary", "strict");
        conflicted->markPrimaryHome();
        // Stale cache falsely names subject...
        conflicted->setOwner(identity.toString(), Zone::kOwnerKindPerson);
        // ...but relational truth explicitly names somebody else.
        auto ownedByOther =
            std::make_shared<Relation>("owned-by", *conflicted, other, true, 1.0f);
        check(conflicted->getFormation().addRelation(ownedByOther),
              "conflict fixture installs authoritative owned-by relation");
        manager.addZone(conflicted);

        check(manager.primaryHomeCount(subject) == 0,
              "stale owner cache cannot overrule contradictory owned-by relation");
        check(manager.enforcePrimaryHomeInvariant(subject),
              "kernel repairs a Person whose stale cache previously faked Home ownership");
        check(manager.primaryHomeCount(subject) == 1,
              "repair leaves the Person with at least one relationally true primary Home");
    }

    {
        // Hard-failure witness: the canonical unowned Home already carries an
        // authoritative owned-by edge to somebody else. Repair is not allowed
        // to steal it or mint around the conflict. The Person therefore still
        // has zero primary Homes and ordinary admission must be refused.
        std::array<uint8_t, 32> otherBytes{};
        for (std::size_t i = 0; i < otherBytes.size(); ++i) {
            otherBytes[i] = static_cast<uint8_t>(0x40 + i);
        }
        const Identity::SingularId otherIdentity =
            Identity::SingularId::fromPublicKey(otherBytes);

        Soul subjectSoul("Admission Subject");
        Body subjectBody("humanoid", "default");
        Person subject(std::move(subjectSoul), std::move(subjectBody), "default");
        subject.setPersonId(identity);

        Soul otherSoul("Canonical Home Owner");
        Body otherBody("humanoid", "default");
        Person other(std::move(otherSoul), std::move(otherBody), "default");
        other.setPersonId(otherIdentity);

        ZoneManager manager;
        auto canonical = std::make_shared<Home>("Home", "strict");
        canonical->markPrimaryHome();
        auto ownedByOther =
            std::make_shared<Relation>("owned-by", *canonical, other, true, 1.0f);
        check(canonical->getFormation().addRelation(ownedByOther),
              "hard-failure fixture installs another Person's owned-by edge");
        manager.addZone(canonical);

        const std::size_t before = manager.zones().size();
        check(manager.primaryHomeCount(subject) == 0,
              "another Person's canonical Home does not satisfy this Person's invariant");
        check(!manager.enforcePrimaryHomeInvariant(subject),
              "kernel refuses ordinary admission when repair still leaves zero primary Homes");
        check(manager.primaryHomeCount(subject) == 0,
              "failed repair leaves zero rather than fabricating ownership");
        check(manager.zones().size() == before,
              "failed repair does not mint around an ownership conflict");
    }

    Universe::instance().setProvider({});
    SaveSystem::setSaveRoot("");
    fs::remove_all(sandbox, ec);

    if (failures) {
        std::cerr << "home_identity_continuity_test: "
                  << failures << " failure(s)\n";
        return 1;
    }
    std::cout << "home_identity_continuity_test: ALL OK\n";
    return 0;
}
