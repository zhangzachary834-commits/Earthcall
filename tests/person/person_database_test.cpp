#include "Person/PersonDatabase.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Person/Body/Body.hpp"
#include "Identity/PersonPresence.hpp"
#include "Identity/IdentityLedger.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "Singularity/Storage/Serialization/Person/PersonSerialization.hpp"

#include <cassert>
#include <cstdlib>
#include <optional>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>

namespace {

struct TestEnvironment {
    std::filesystem::path tempDir;
    std::string prevSaveRoot;
    std::optional<std::string> prevEarthcallHome;
    std::optional<std::string> prevChosenPerson;

    TestEnvironment() {
        tempDir = std::filesystem::temp_directory_path() / "earthcall_person_db_test";
        std::filesystem::remove_all(tempDir);
        std::filesystem::create_directories(tempDir);

        prevSaveRoot = SaveSystem::saveRoot();
        SaveSystem::setSaveRoot(tempDir.string());

        // The identity ledger lives outside SaveSystem's save root.
        // Isolate it too, never writing to the developer's real identity.
        if (const char* previous = std::getenv("EARTHCALL_HOME")) {
            prevEarthcallHome = previous;
        }
        if (const char* previous = std::getenv("EARTHCALL_PERSON_ID")) {
            prevChosenPerson = previous;
        }
        const std::string sandboxHome = (tempDir / "home").string();
#if defined(_WIN32)
        _putenv_s("EARTHCALL_HOME", sandboxHome.c_str());
        _putenv_s("EARTHCALL_PERSON_ID", "");
#else
        setenv("EARTHCALL_HOME", sandboxHome.c_str(), 1);
        unsetenv("EARTHCALL_PERSON_ID");
#endif
    }

    ~TestEnvironment() {
        SaveSystem::setSaveRoot(prevSaveRoot);
#if defined(_WIN32)
        _putenv_s("EARTHCALL_HOME", prevEarthcallHome ? prevEarthcallHome->c_str() : "");
        _putenv_s("EARTHCALL_PERSON_ID", prevChosenPerson ? prevChosenPerson->c_str() : "");
#else
        if (prevEarthcallHome) setenv("EARTHCALL_HOME", prevEarthcallHome->c_str(), 1);
        else unsetenv("EARTHCALL_HOME");
        if (prevChosenPerson) setenv("EARTHCALL_PERSON_ID", prevChosenPerson->c_str(), 1);
        else unsetenv("EARTHCALL_PERSON_ID");
#endif
        std::filesystem::remove_all(tempDir);
    }
};

Person createDummyPerson(const std::string& name) {
    Soul soul(name);
    Body body("Humanoid", "Voxel");
    Person person(soul, std::move(body), "");
    person.setDisplayName(name);
    return person;
}

} // namespace

static void testGetInstanceSingleton() {
    PersonDatabase& db1 = PersonDatabase::getInstance();
    PersonDatabase& db2 = PersonDatabase::getInstance();
    assert(&db1 == &db2);
    std::cout << "  getInstance singleton behavior OK\n";
}

static void testSaveAndLoadPerson() {
    TestEnvironment env;
    PersonDatabase& db = PersonDatabase::getInstance();

    Person original = createDummyPerson("Alice");
    original.cameraPos = glm::vec3(1.0f, 2.0f, 3.0f);
    original.cameraForward = glm::vec3(0.0f, 1.0f, 0.0f);

    db.savePerson(original);


    Person loaded = createDummyPerson("Temp");
    bool success = db.loadPerson("Alice", loaded);
    assert(success);
    assert(loaded.getDisplayName() == "Alice");

    std::cout << "  savePerson and loadPerson OK\n";
}

static void testSavePersonEmptyName() {
    TestEnvironment env;
    PersonDatabase& db = PersonDatabase::getInstance();

    // Soul with empty string doesn't set a valid display name, Person::Person forces it to "Person"
    Soul soul("");
    Body body("Humanoid", "Voxel");
    Person person(soul, std::move(body), "");

    // Save person profile
    db.savePerson(person);

    std::vector<std::string> persons = db.getAllRegisteredPersons();
    assert(persons.size() == 1); // Saved default/fallback profile or bin

    std::cout << "  savePerson with displayName OK\n";
}

static void testLoadPersonEmptyName() {
    TestEnvironment env;
    PersonDatabase& db = PersonDatabase::getInstance();

    Person loaded = createDummyPerson("Temp");
    bool success = db.loadPerson("", loaded);
    assert(!success);

    std::cout << "  loadPerson with empty name returns false OK\n";
}

static void testLoadNonExistentPerson() {
    TestEnvironment env;
    PersonDatabase& db = PersonDatabase::getInstance();

    Person loaded = createDummyPerson("Temp");
    bool success = db.loadPerson("NonExistentPerson", loaded);
    assert(!success);

    std::cout << "  loadPerson non-existent person returns false OK\n";
}

static void testGetAllRegisteredPersons() {
    TestEnvironment env;
    PersonDatabase& db = PersonDatabase::getInstance();

    Person p1 = createDummyPerson("Bob");
    Person p2 = createDummyPerson("Charlie");

    db.savePerson(p1);
    db.savePerson(p2);

    std::vector<std::string> registered = db.getAllRegisteredPersons();
    assert(registered.size() == 2);

    bool foundBob = false;
    bool foundCharlie = false;
    for (const auto& path : registered) {
        if (path.find("Bob") != std::string::npos) foundBob = true;
        if (path.find("Charlie") != std::string::npos) foundCharlie = true;
    }
    assert(foundBob && foundCharlie);

    std::cout << "  getAllRegisteredPersons OK\n";
}

static void testLoadPersonPathTraversalSanitization() {
    TestEnvironment env;
    PersonDatabase& db = PersonDatabase::getInstance();

    Person loaded = createDummyPerson("Temp");
    bool success = db.loadPerson("../../etc/passwd", loaded);
    assert(!success);

    std::cout << "  loadPerson path traversal sanitization OK\n";
}

static void testLoadPersonMalformedJson() {
    TestEnvironment env;
    PersonDatabase& db = PersonDatabase::getInstance();

    // Write a malformed JSON file directly to the save folder
    std::string folder = SaveSystem::ensureSaveTypeFolder(SaveSystem::SaveType::PERSON);
    std::ofstream file(folder + "/Malformed.ecform");
    file << "{ this is not valid json }";
    file.close();

    Person loaded = createDummyPerson("Temp");
    bool success = db.loadPerson("Malformed", loaded);
    assert(!success); // Should fail and catch exception

    std::cout << "  loadPerson exception handling (malformed json) OK\n";
}


static void testSavePersonDirectoryCreationFailure() {
    TestEnvironment env;
    PersonDatabase& db = PersonDatabase::getInstance();

    // Block folder creation by placing a regular file where the 'persons' folder should be
    std::filesystem::path personsFolder = env.tempDir / "persons";
    std::ofstream blockingFile(personsFolder);
    blockingFile << "blocking file content";
    blockingFile.close();

    Person person = createDummyPerson("BlockedPerson");
    // Should handle save failure gracefully without crashing
    db.savePerson(person);

    std::cout << "  savePerson directory creation failure handling OK\n";
}

static void testLoadPersonUnreadableFile() {
    TestEnvironment env;
    PersonDatabase& db = PersonDatabase::getInstance();

    std::string folder = SaveSystem::ensureSaveTypeFolder(SaveSystem::SaveType::PERSON);
    std::string filepath = folder + "/Unreadable.ecform";

    // Create a directory at the target file path to force std::ifstream file open failure across all environments (including root)
    std::filesystem::create_directory(filepath);

    Person loaded = createDummyPerson("Temp");
    bool success = db.loadPerson("Unreadable", loaded);
    assert(!success);

    std::cout << "  loadPerson unreadable file error handling OK\n";
}

static void testLoadPersonInvalidJsonStructure() {
    TestEnvironment env;
    PersonDatabase& db = PersonDatabase::getInstance();

    std::string folder = SaveSystem::ensureSaveTypeFolder(SaveSystem::SaveType::PERSON);
    std::ofstream file(folder + "/InvalidStruct.ecform");
    file << "[1, 2, 3]";
    file.close();

    Person loaded = createDummyPerson("Temp");
    bool success = db.loadPerson("InvalidStruct", loaded);
    assert(!success);

    std::cout << "  loadPerson invalid json structure error handling OK\n";
}


static void testLoadPersonDeserializationFailure() {
    TestEnvironment env;
    PersonDatabase& db = PersonDatabase::getInstance();

    // Write JSON with a mismatched type for body height to exercise deserialization exception handling.
    std::string folder = SaveSystem::ensureSaveTypeFolder(SaveSystem::SaveType::PERSON);
    std::ofstream file(folder + "/InvalidSchema.ecform");
    file << R"({"displayName": "Invalid", "body": {"height": "not_a_float"}})";
    file.close();

    Person loaded = createDummyPerson("Temp");
    bool success = db.loadPerson("InvalidSchema", loaded);
    assert(!success);

    std::cout << "  loadPerson deserialization failure handling OK\n";
}

static void testSaveAndLoadDistinctPersonsSameDisplayName() {
    TestEnvironment env;
    PersonDatabase& db = PersonDatabase::getInstance();

    Person p1 = createDummyPerson("Alice");
    Person p2 = createDummyPerson("Alice");

    // Assign distinct cryptographic SingularIds
    std::array<uint8_t, 32> k1{}, k2{};
    k1[0] = 1;
    k2[0] = 2;
    p1.setPersonId(Identity::SingularId::fromPublicKey(k1));
    p2.setPersonId(Identity::SingularId::fromPublicKey(k2));

    assert(p1.getDisplayName() == "Alice");
    assert(p2.getDisplayName() == "Alice");
    assert(p1.getIdentifier() != p2.getIdentifier());

    p1.position() = glm::vec3(10.0f, 0.0f, 0.0f);
    p2.position() = glm::vec3(20.0f, 0.0f, 0.0f);

    db.savePerson(p1);
    db.savePerson(p2);

    Person loaded1 = createDummyPerson("Temp");
    Person loaded2 = createDummyPerson("Temp");

    bool success1 = db.loadPerson(p1.getIdentifier(), loaded1);
    bool success2 = db.loadPerson(p2.getIdentifier(), loaded2);

    assert(success1);
    assert(success2);
    assert(loaded1.getDisplayName() == "Alice");
    assert(loaded2.getDisplayName() == "Alice");
    assert(loaded1.getIdentifier() == p1.getIdentifier());
    assert(loaded2.getIdentifier() == p2.getIdentifier());
    assert(loaded1.position().x == 10.0f);
    assert(loaded2.position().x == 20.0f);

    std::cout << "  savePerson and loadPerson distinct persons with same display name OK\n";
}

static void testLoadPersonLegacyJsonFormat() {
    TestEnvironment env;
    PersonDatabase& db = PersonDatabase::getInstance();

    Person p = createDummyPerson("LegacyPerson");
    p.position() = glm::vec3(5.0f, 6.0f, 7.0f);

    // Write a .json Person profile directly to the persons save directory
    std::string folder = SaveSystem::ensureSaveTypeFolder(SaveSystem::SaveType::PERSON);
    std::string jsonPath = folder + "/LegacyPerson.json";
    std::ofstream file(jsonPath);
    file << p.serialize().dump(-1);
    file.close();

    // Verify .ecform does NOT exist
    assert(!std::filesystem::exists(folder + "/LegacyPerson.ecform"));

    Person loaded = createDummyPerson("Temp");
    bool success = db.loadPerson("LegacyPerson", loaded);
    assert(success);
    assert(loaded.getDisplayName() == "LegacyPerson");
    assert(loaded.position().x == 5.0f);

    std::cout << "  loadPerson legacy .json format resolution OK\n";
}

// WITNESS: exercise the same SaveSystem discovery, legacy supersession,
// candidate-count gate, and Person codec used by Engine::initLogic.
// This does not instantiate Engine or prove key possession / Person presence.
// The old PersonDatabase::loadPerson tests exercise the storage API, not boot.
static void testPersonFromJsonProductionRestoration() {
    TestEnvironment env;

    Person source = createDummyPerson("WitnessPerson");
    std::array<uint8_t, 32> pubKey{};
    pubKey[0] = 42;
    const bool keyed = source.setPersonId(Identity::SingularId::fromPublicKey(pubKey));
    assert(keyed);
    source.position() = glm::vec3(11.0f, 12.0f, 13.0f);

    const auto keyedProfile = personToJson(source);
    const std::string savedPath = SaveSystem::writeSaveData(
        keyedProfile, source.getIdentifier(), SaveSystem::SaveType::PERSON);
    assert(!savedPath.empty());

    assert(!Identity::legacyProfileSuperseded("SupersededUser"));

    // A legacy profile exists alongside its keyed successor, as on
    // installations migrated from a name-based Person to a keyed Person.
    Person legacy = createDummyPerson("SupersededUser");
    const std::string legacyPath = SaveSystem::writeSaveData(
        personToJson(legacy), legacy.getIdentifier(), SaveSystem::SaveType::PERSON);
    assert(!legacyPath.empty());

    Identity::IdentityLedger ledger;
    ledger.record("SupersededUser", source.personId());
    const bool savedLedger = ledger.save();
    assert(savedLedger);
    assert(std::filesystem::exists(Identity::IdentityLedger::defaultPath()));
    assert(Identity::legacyProfileSuperseded("SupersededUser"));
    assert(!Identity::legacyProfileSuperseded("UnseenLegacyUser"));

    // Mirror the exact candidate-selection loop in EngineInit.cpp. A future
    // change to that loop still needs an integration test through Engine.
    auto bootCandidates = [] {
        std::vector<nlohmann::json> candidates;
        for (const auto& info : SaveSystem::listWorlds(SaveSystem::SaveType::PERSON)) {
            nlohmann::json profile = SaveSystem::readSaveData(info.path);
            if (!profile.is_object()) continue;
            if (!profile.contains("personId") &&
                Identity::legacyProfileSuperseded(profile.value("displayName", std::string{}))) {
                continue;
            }
            candidates.push_back(std::move(profile));
        }
        return candidates;
    };

    auto candidates = bootCandidates();
    assert(candidates.size() == 1);
    Person restored = createDummyPerson("Temp");
    personFromJson(candidates.front(), restored);
    assert(restored.getDisplayName() == "WitnessPerson");
    assert(restored.hasIdentity());
    assert(restored.personId() == source.personId());
    assert(restored.position().x == 11.0f);
    assert(restored.position().y == 12.0f);
    assert(restored.position().z == 13.0f);

    // Multiple unsuperseded profiles must remain ambiguous: boot must not
    // guess a Person from display names or silently choose the first file.
    Person other = createDummyPerson("OtherPerson");
    const std::string otherPath = SaveSystem::writeSaveData(
        personToJson(other), other.getIdentifier(), SaveSystem::SaveType::PERSON);
    assert(!otherPath.empty());
    candidates = bootCandidates();
    assert(candidates.size() == 2);

    std::cout << "  testPersonFromJsonProductionRestoration (boot selection/codec mirror) OK\n";
}

int main() {
    std::cout << "person_database_test:\n";
    testGetInstanceSingleton();
    testSaveAndLoadPerson();
    testSavePersonEmptyName();
    testLoadPersonEmptyName();
    testLoadNonExistentPerson();
    testGetAllRegisteredPersons();
    testLoadPersonPathTraversalSanitization();
    testLoadPersonMalformedJson();
    testSavePersonDirectoryCreationFailure();
    testLoadPersonUnreadableFile();
    testLoadPersonInvalidJsonStructure();
    testLoadPersonDeserializationFailure();
    testSaveAndLoadDistinctPersonsSameDisplayName();
    testLoadPersonLegacyJsonFormat();
    testPersonFromJsonProductionRestoration();
    std::cout << "person_database_test: ALL OK\n";
    return 0;
}
