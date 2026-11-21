#include "Identity/PersonPresence.hpp"

#include "Identity/FirstMoverRegister.hpp"
#include "Identity/IdentityLedger.hpp"
#include "Identity/KeyStore.hpp"
#include "Identity/PersonMigration.hpp"
#include "Person/Person.hpp"
#include "Person/PersonDatabase.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "Singularity/Storage/Serialization/Person/PersonSerialization.hpp"

#include <cstdlib>
#include <utility>
#include <vector>

namespace Identity {

namespace {

// The keyed profiles on disk, filtered by EARTHCALL_PERSON_ID when set. That
// variable only CHOOSES among profiles; the key is what authenticates.
std::vector<std::pair<SingularId, nlohmann::json>> keyedProfiles() {
    const char* chosen = std::getenv("EARTHCALL_PERSON_ID");
    std::vector<std::pair<SingularId, nlohmann::json>> keyed;
    for (const auto& info : SaveSystem::listWorlds(SaveSystem::SaveType::PERSON)) {
        nlohmann::json profile = SaveSystem::readSaveData(info.path);
        if (!profile.is_object() || !profile.contains("personId") ||
            !profile["personId"].is_string()) continue;
        const auto id = SingularId::parse(profile["personId"].get<std::string>());
        if (!id.canAuthenticate()) continue;
        if (chosen && *chosen && id.toString() != chosen) continue;
        keyed.emplace_back(id, std::move(profile));
    }
    return keyed;
}

PresenceResult trust(Person& person, const PrivateKey& key) {
    if (!FirstMoverRegister::instance().trustAuthenticatedPerson(key)) {
        return {false, "refused: the key could not be seated as the trusted Person root"};
    }
    person.login("key-" + person.personId().abbreviated());
    return {true, "'" + person.getDisplayName() + "' is present (" + person.personId().abbreviated() +
                  "); First Movers you granted may act this session"};
}

} // namespace

bool keyedProfileExists() { return !keyedProfiles().empty(); }

bool personAnswersTo(const Person& person, const std::string& identifier) {
    if (identifier.empty()) return false;
    if (person.matchesIdentifier(identifier)) return true;
    if (!person.hasIdentity()) return false;
    IdentityLedger ledger;
    if (!ledger.load()) return false;
    const auto migrated = ledger.find(identifier);
    return migrated.has_value() && *migrated == person.personId();
}

bool legacyProfileSuperseded(const std::string& displayName) {
    if (displayName.empty()) return false;
    IdentityLedger ledger;
    if (!ledger.load()) return false;
    const auto migrated = ledger.find(displayName);
    if (!migrated) return false;
    for (const auto& [id, profile] : keyedProfiles()) {
        if (id == *migrated) return true;
    }
    return false;
}

PresenceResult unlockPresentPerson(Person& person, const std::string& passphrase) {
    if (passphrase.empty()) return {false, "refused: empty passphrase"};
    KeyStore keys;

    if (!person.hasIdentity()) {
        auto keyed = keyedProfiles();
        if (keyed.empty()) {
            return {false, "refused: no keyed Person profile exists yet; key your Person first"};
        }
        if (keyed.size() > 1) {
            return {false, "refused: several keyed Person profiles exist; set EARTHCALL_PERSON_ID "
                           "to say which Person is present (refusing to guess)"};
        }
        auto key = keys.load(keyed.front().first, passphrase);
        if (!key || key->id() != keyed.front().first) {
            return {false, "refused: the key for " + keyed.front().first.abbreviated() +
                           " did not open with that passphrase"};
        }
        personFromJson(keyed.front().second, person);
        if (person.personId() != keyed.front().first) {
            return {false, "refused: the profile did not restore the identity its key proves"};
        }
        return trust(person, *key);
    }

    auto key = keys.load(person.personId(), passphrase);
    if (!key || key->id() != person.personId()) {
        return {false, "refused: the key for '" + person.getDisplayName() +
                       "' did not open with that passphrase"};
    }
    return trust(person, *key);
}

PresenceResult keyPresentPerson(Person& person, const std::string& passphrase) {
    if (person.hasIdentity()) return {false, "refused: this Person already has a key; unlock instead"};
    if (passphrase.size() < 8) return {false, "refused: use a passphrase of at least 8 characters"};
    IdentityLedger ledger;
    (void)ledger.load();   // absence is normal on the first migration
    KeyStore keys;
    auto migrated = migratePersonIdentity(person, ledger, keys, passphrase);
    if (!migrated) return {false, "refused: identity migration failed; you remain the unkeyed Person"};
    PersonDatabase::getInstance().savePerson(person);
    auto key = keys.load(person.personId(), passphrase);
    if (!key || key->id() != person.personId()) {
        return {false, "keyed as " + migrated->abbreviated() + ", but the new key did not reopen"};
    }
    PresenceResult r = trust(person, *key);
    if (r.ok) r.report = "keyed and present: '" + person.getDisplayName() + "' is now " +
                         person.personId().abbreviated() + ". Remember the passphrase; there is no recovery";
    return r;
}

} // namespace Identity
