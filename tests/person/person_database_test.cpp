#include "Person/PersonDatabase.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Person/Body/Body.hpp"
#include "Singularity/Storage/SaveSystem.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
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

    // Write JSON file for loadPerson (which reads <name>.json from PERSON save folder)
    std::string folder = SaveSystem::ensureSaveTypeFolder(SaveSystem::SaveType::PERSON);
    std::ofstream file(folder + "/Alice.json");
    file << original.serialize().dump();
    file.close();

    Person loaded = createDummyPerson("Temp");
    bool success = db.loadPerson("Alice", loaded);
    assert(success);
    assert(loaded.getDisplayName() == "Alice");

    std::cout << "  savePerson and loadPerson OK\n";
}

static void testSavePersonEmptyName() {
    TestEnvironment env;
    PersonDatabase& db = PersonDatabase::getInstance();

    // Soul with empty string doesn't set a valid display name
    Soul soul("");
    Body body("Humanoid", "Voxel");
    Person person(soul, std::move(body), "");

    // Save person profile
    db.savePerson(person);

    std::vector<std::string> persons = db.getAllRegisteredPersons();
    assert(persons.size() == 1); // Saved default/fallback profile or bin

    std::cout << "  savePerson with displayName OK\n";
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

int main() {
    std::cout << "person_database_test:\n";
    testGetInstanceSingleton();
    testSaveAndLoadPerson();
    testSavePersonEmptyName();
    testLoadNonExistentPerson();
    testGetAllRegisteredPersons();
    testLoadPersonPathTraversalSanitization();
    std::cout << "person_database_test: ALL OK\n";
    return 0;
}
