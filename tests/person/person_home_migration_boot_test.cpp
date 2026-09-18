// P0 Make the Earth Inhabitable: migrate the actual tracked legacy Person
// against a disposable copy of the real saves tree, then boot Home continuity.
// The test's entire point is that introducing a Person key must not mint a
// third Home. Never point SaveSystem or EARTHCALL_HOME at the real tree here.

#include "Identity/IdentityLedger.hpp"
#include "Identity/KeyStore.hpp"
#include "Identity/PersonMigration.hpp"
#include "Person/Person.hpp"
#include "Person/PersonDatabase.hpp"
#include "Person/Soul/Soul.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "Singularity/Storage/Serialization/Person/PersonSerialization.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <set>
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

void setEnv(const char* key, const std::string& value) {
#if defined(_WIN32)
    _putenv_s(key, value.c_str());
#else
    setenv(key, value.c_str(), 1);
#endif
}

void unsetEnv(const char* key) {
#if defined(_WIN32)
    _putenv_s(key, "");
#else
    unsetenv(key);
#endif
}

struct EnvRestore {
    std::string key;
    bool had = false;
    std::string value;

    explicit EnvRestore(const char* name) : key(name) {
        if (const char* old = std::getenv(name)) {
            had = true;
            value = old;
        }
    }

    ~EnvRestore() {
        if (had) setEnv(key.c_str(), value);
        else unsetEnv(key.c_str());
    }
};

std::set<std::string> directoryNames(const std::filesystem::path& root) {
    std::set<std::string> out;
    std::error_code ec;
    if (!std::filesystem::exists(root, ec)) return out;
    for (const auto& entry : std::filesystem::directory_iterator(root, ec)) {
        if (ec) break;
        if (entry.is_directory()) out.insert(entry.path().filename().string());
    }
    return out;
}

nlohmann::json findZachProfile() {
    for (const auto& entry : SaveSystem::listWorlds(SaveSystem::SaveType::PERSON)) {
        nlohmann::json profile = SaveSystem::readSaveData(entry.path);
        if (!profile.is_object()) continue;
        const std::string name = profile.value(
            "displayName", profile.value("soulName", std::string{}));
        if (name == "Zach") return profile;
    }
    return {};
}

} // namespace

int main() {
    namespace fs = std::filesystem;

    const fs::path sourceSaves = fs::current_path() / "saves";
    check(fs::exists(sourceSaves / "persons" / "Zach.ecform"),
          "the checked-in legacy Zach profile is present");
    check(fs::exists(sourceSaves / "homes" / "Home" / "home.json"),
          "the checked-in primary Home is present");
    check(fs::exists(sourceSaves / "homes" / "Home_of_Zach" / "home.json"),
          "the historical duplicate Home is present");
    if (failures) return 1;

    const fs::path sandbox =
        fs::temp_directory_path() / "earthcall_person_home_migration_boot";
    const fs::path copiedSaves = sandbox / "saves";
    std::error_code ec;
    fs::remove_all(sandbox, ec);
    ec.clear();
    fs::create_directories(sandbox, ec);
    if (ec) {
        std::cerr << "FAILED: cannot create sandbox: " << ec.message() << "\n";
        return 1;
    }
    fs::copy(sourceSaves, copiedSaves,
             fs::copy_options::recursive | fs::copy_options::copy_symlinks, ec);
    if (ec) {
        std::cerr << "FAILED: cannot copy real saves tree: " << ec.message() << "\n";
        fs::remove_all(sandbox);
        return 1;
    }

    EnvRestore restoreHome("EARTHCALL_HOME");
    setEnv("EARTHCALL_HOME", sandbox.string());
    SaveSystem::setSaveRoot(copiedSaves.string());

    const auto homesBefore = directoryNames(copiedSaves / "homes");
    check(homesBefore.count("Home") == 1 && homesBefore.count("Home_of_Zach") == 1,
          "sandbox begins with the same two historical Home directories");

    Soul soul("Person");
    Body body("humanoid", "default");
    Person zach(std::move(soul), std::move(body), "default");
    const nlohmann::json profile = findZachProfile();
    check(profile.is_object(), "Zach profile is discovered from the copied save root");
    if (profile.is_object()) personFromJson(profile, zach);
    check(zach.getDisplayName() == "Zach", "copied profile restores Zach");
    check(!zach.hasIdentity(), "tracked Zach profile is still legacy before migration");

    Identity::IdentityLedger ledger;
    (void)ledger.load();
    Identity::KeyStore keys;
    const auto migrated = Identity::migratePersonIdentity(
        zach, ledger, keys, "sandbox-only-passphrase");
    check(migrated.has_value() && zach.hasIdentity(),
          "explicit live migration gives the copied Person a stable identity");
    check(ledger.find("Zach").has_value()
              && ledger.find("Zach")->toString() == zach.personId().toString(),
          "migration ledger proves Zach spelling -> this exact Person identity");

    // Persist only into the copied Person store, exactly as explicit app boot does.
    PersonDatabase::getInstance().savePerson(zach);

    Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
        beings.push_back(&zach);
    });

    ZoneManager zones;
    zones.hydrateFromZoneStore();
    const std::size_t liveCountBeforeEnsure = zones.zones().size();

    // The copied tree currently has two primary Homes. Policy is deliberately
    // unresolved, so the correct behavior is refusal, NOT choosing one and NOT
    // inventing a third. Both legacy owner spellings resolve through the ledger.
    check(!zones.ensureHomeZone(zach),
          "duplicate legacy primaries refuse after identity migration");
    check(zones.zones().size() == liveCountBeforeEnsure,
          "identity migration does not mint a third live Home");

    // Exercise the write boundary too. On the historical buggy path the new
    // Home becomes a directory only when persistence runs.
    zones.persistZones();

    const auto homesAfter = directoryNames(copiedSaves / "homes");
    check(homesAfter == homesBefore,
          "save after migrated boot creates zero new Home directories");

    const std::string forbidden =
        std::string("Home_of_") + zach.getIdentifier();
    check(homesAfter.count(forbidden) == 0,
          "no Home_of_<stable-Person-id> directory is minted");

    Universe::instance().setProvider({});
    SaveSystem::setSaveRoot("");
    fs::remove_all(sandbox, ec);

    if (failures) {
        std::cerr << "person_home_migration_boot_test: "
                  << failures << " failure(s)\n";
        return 1;
    }
    std::cout << "person_home_migration_boot_test: ALL OK\n";
    return 0;
}
