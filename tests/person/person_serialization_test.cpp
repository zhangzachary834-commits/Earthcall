// Phase 3: the Person root codec preserves the profile schema while making
// Person a first-class session serialization root.

#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "Singularity/Storage/Serialization/Person/PersonSerialization.hpp"
#include "../support/test_save_helper.hpp"

#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>

namespace {


Person makePerson(const char* name) {
    Person person(Soul(name), Body("Humanoid", "Voxel"), "default");
    person.setDisplayName(name);
    return person;
}

bool near(float a, float b) {
    return std::fabs(a - b) < 0.0001f;
}

} // namespace

int main() {
    TempSaveRoot tempSaveRoot;

    Person original = makePerson("Phase 3 Person");
    original.position() = {1.25f, -2.5f, 3.75f};
    original.velocity() = {-4.0f, 5.5f, 6.25f};

    const nlohmann::json saved = personToJson(original);
    assert(saved.contains("displayName"));
    assert(saved.contains("soulName"));
    assert(saved.contains("position"));
    assert(saved.contains("velocity"));
    assert(saved.contains("body"));
    assert(original.serialize() == saved);

    Person restored = makePerson("Temporary");
    personFromJson(saved, restored);
    assert(restored.getDisplayName() == "Phase 3 Person");
    assert(near(restored.position().x, 1.25f));
    assert(near(restored.position().y, -2.5f));
    assert(near(restored.position().z, 3.75f));
    assert(near(restored.velocity().x, -4.0f));
    assert(near(restored.velocity().y, 5.5f));
    assert(near(restored.velocity().z, 6.25f));

    // Test updatePriorPersonSerializations exception handling for malformed JSON
    std::filesystem::path testDir = tempSaveRoot.path / "test_malformed";
    std::filesystem::create_directories(testDir);
    std::string malformedPath = (testDir / "corrupted.json").string();
    std::string validPath = (testDir / "valid.json").string();
    {
        std::ofstream badFile(malformedPath);
        badFile << "{ malformed json: true, ";
        badFile.close();
        std::ofstream goodFile(validPath);
        goodFile << R"({"person": {"displayName": "OldName", "soulName": "OldName"}})";
        goodFile.close();
    }

    Person updatedPerson = makePerson("NewName");
    updatePriorPersonSerializations(updatedPerson, "OldName");

    // Verify valid file was updated and corrupted file did not crash execution
    {
        std::ifstream checkGood(validPath);
        nlohmann::json jGood;
        checkGood >> jGood;
        assert(jGood["person"]["displayName"] == "NewName");
    }

    std::filesystem::remove_all(testDir);

    // Test that updatePriorPersonSerializations does NOT overwrite distinct Person records sharing identical display names
    std::filesystem::path testIdentDir = tempSaveRoot.path / "test_identities";
    std::filesystem::create_directories(testIdentDir);

    std::array<uint8_t, 32> key1Bytes; key1Bytes.fill(0x11);
    std::array<uint8_t, 32> key2Bytes; key2Bytes.fill(0x22);
    Identity::SingularId id1 = Identity::SingularId::fromPublicKey(key1Bytes);
    Identity::SingularId id2 = Identity::SingularId::fromPublicKey(key2Bytes);

    std::string pathPerson1 = (testIdentDir / "person1_world.json").string();
    std::string pathPerson2 = (testIdentDir / "person2_world.json").string();
    {
        nlohmann::json j1 = {
            {"person", {
                {"displayName", "Alice"},
                {"soulName", "Alice"},
                {"personId", id1.toString()}
            }}
        };
        nlohmann::json j2 = {
            {"person", {
                {"displayName", "Alice"},
                {"soulName", "Alice"},
                {"personId", id2.toString()}
            }}
        };
        std::ofstream f1(pathPerson1); f1 << j1.dump(2);
        std::ofstream f2(pathPerson2); f2 << j2.dump(2);
    }

    Person alice1 = makePerson("AliceRenamed");
    alice1.setPersonId(id1);

    // Update prior serializations for alice1, renaming from "Alice" -> "AliceRenamed"
    updatePriorPersonSerializations(alice1, "Alice");

    {
        std::ifstream check1(pathPerson1);
        nlohmann::json j1Check;
        check1 >> j1Check;
        assert(j1Check["person"]["displayName"] == "AliceRenamed");

        std::ifstream check2(pathPerson2);
        nlohmann::json j2Check;
        check2 >> j2Check;
        // Person 2 has a distinct personId (id2), so despite matching display name "Alice",
        // it MUST NOT be overwritten or collapsed!
        assert(j2Check["person"]["displayName"] == "Alice");
        assert(j2Check["person"]["personId"] == id2.toString());
    }

    std::filesystem::remove_all(testIdentDir);

    // Test that updatePriorPersonSerializations cleans up stale .ecform profile files upon rename
    {
        std::string personFolder = SaveSystem::ensureSaveTypeFolder(SaveSystem::SaveType::PERSON);
        std::string oldProfileEcform = personFolder + "/StalePerson.ecform";
        {
            std::ofstream f(oldProfileEcform);
            f << R"({"displayName": "StalePerson"})";
        }
        assert(std::filesystem::exists(oldProfileEcform));

        Person renamedPerson = makePerson("FreshPerson");
        updatePriorPersonSerializations(renamedPerson, "StalePerson");

        assert(!std::filesystem::exists(oldProfileEcform));
    }

    // Test that updatePriorPersonSerializations with empty oldName does NOT overwrite unkeyed legacy save records of other Persons
    std::filesystem::path testLegacyDir = tempSaveRoot.path / "test_legacy_empty_oldname";
    std::filesystem::create_directories(testLegacyDir);

    std::string pathBobLegacy = (testLegacyDir / "bob_world.json").string();
    {
        nlohmann::json jBob = {
            {"person", {
                {"displayName", "Bob"},
                {"soulName", "Bob"}
            }}
        };
        std::ofstream fBob(pathBobLegacy); fBob << jBob.dump(2);
    }

    Person dave = makePerson("Dave");
    // Call updatePriorPersonSerializations for dave with oldName = ""
    updatePriorPersonSerializations(dave, "");

    {
        std::ifstream checkBob(pathBobLegacy);
        nlohmann::json jBobCheck;
        checkBob >> jBobCheck;
        // Unkeyed legacy record for "Bob" MUST NOT be overwritten when oldName is empty!
        assert(jBobCheck["person"]["displayName"] == "Bob");
        assert(jBobCheck["person"]["soulName"] == "Bob");
    }

    std::filesystem::remove_all(testLegacyDir);

    std::puts("person_serialization_test: ALL OK");
    return 0;
}
