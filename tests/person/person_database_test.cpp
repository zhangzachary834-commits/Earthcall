// WITNESS DOCUMENTATION:
// What the old test proved:
//   That PersonDatabase::savePerson wrote a serialized profile to disk and
//   PersonDatabase::loadPerson (a standalone helper in PersonDatabase) could read it back.
//
// What the old test COULD NOT prove:
//   That PersonDatabase::savePerson produced a profile that the actual production
//   app boot sequence (EngineInit.cpp via SaveSystem::listWorlds(SaveSystem::SaveType::PERSON)
//   and personFromJson) discovers, parses, and restores into the active Person.
//   PersonDatabase::loadPerson is never called on any real application runtime path.

#include "Person/PersonDatabase.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Person/Body/Body.hpp"
#include "Person/PersonBootRestoration.hpp"
#include "Singularity/Storage/SaveSystem.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <string>

namespace {

struct TestEnvironment {
    std::filesystem::path tempDir;
    std::string prevSaveRoot;

    TestEnvironment() {
        tempDir = std::filesystem::temp_directory_path() / "earthcall_person_db_test";
        std::filesystem::remove_all(tempDir);
        std::filesystem::create_directories(tempDir);

        prevSaveRoot = SaveSystem::saveRoot();
        SaveSystem::setSaveRoot(tempDir.string());
    }

    ~TestEnvironment() {
        SaveSystem::setSaveRoot(prevSaveRoot);
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

static void testProductionAppBootPersonRestorationPath() {
    TestEnvironment env;
    PersonDatabase& db = PersonDatabase::getInstance();

    Person original = createDummyPerson("Zach");
    original.position() = glm::vec3(12.0f, 3.0f, 4.0f);
    original.velocity() = glm::vec3(1.0f, 0.0f, -0.5f);
    db.savePerson(original);

    // Invoke the very same function called from Engine::initLogic, not a
    // separately copied approximation of production profile selection.
    Person bootedPerson = createDummyPerson("Temp");
    const auto result = restoreUnambiguousPersonProfileForBoot(bootedPerson);
    if (result != PersonBootRestorationResult::Restored ||
        bootedPerson.getDisplayName() != "Zach" ||
        bootedPerson.position().x != 12.0f ||
        bootedPerson.position().y != 3.0f ||
        bootedPerson.position().z != 4.0f ||
        bootedPerson.velocity().x != 1.0f ||
        bootedPerson.velocity().z != -0.5f) {
        throw std::runtime_error("production boot did not restore the saved Person profile");
    }
    std::cout << "  production EngineInit profile restoration call path OK\n";
}

static void testProductionAppBootRefusesAmbiguousProfiles() {
    TestEnvironment env;
    PersonDatabase& db = PersonDatabase::getInstance();
    db.savePerson(createDummyPerson("First"));
    db.savePerson(createDummyPerson("Second"));

    Person bootedPerson = createDummyPerson("Temp");
    bootedPerson.position() = glm::vec3(99.0f, 0.0f, 0.0f);
    const auto result = restoreUnambiguousPersonProfileForBoot(bootedPerson);
    if (result != PersonBootRestorationResult::Ambiguous ||
        bootedPerson.getDisplayName() != "Temp" ||
        bootedPerson.position().x != 99.0f) {
        throw std::runtime_error("production boot guessed between multiple Person profiles");
    }
    std::cout << "  production EngineInit ambiguity refusal OK\n";
}

static void testProductionAppBootWithoutProfile() {
    TestEnvironment env;
    Person bootedPerson = createDummyPerson("Temp");
    const auto result = restoreUnambiguousPersonProfileForBoot(bootedPerson);
    if (result != PersonBootRestorationResult::NoProfile ||
        bootedPerson.getDisplayName() != "Temp") {
        throw std::runtime_error("production boot modified Person without a profile");
    }
    std::cout << "  production EngineInit empty profile set OK\n";
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
    testProductionAppBootPersonRestorationPath();
    testProductionAppBootRefusesAmbiguousProfiles();
    testProductionAppBootWithoutProfile();
    std::cout << "person_database_test: ALL OK\n";
    return 0;
}
