#include "ConstructedBeing/CategoryManager.hpp"
#include "Identity/SingularId.hpp"
#include "Person/Body/Body.hpp"
#include "Person/Person.hpp"
#include "Person/PersonDatabase.hpp"
#include "Person/Soul/Soul.hpp"
#include "Singularity/Storage/SaveSystem.hpp"

#include <array>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>

namespace {
struct ScratchSaveRoot {
    std::filesystem::path path;
    std::string previous;

    ScratchSaveRoot()
        : path(std::filesystem::temp_directory_path() /
               ("earthcall-person-not-object-" + std::to_string(
                    std::chrono::steady_clock::now().time_since_epoch().count()))),
          previous(SaveSystem::saveRoot()) {
        std::filesystem::remove_all(path);
        std::filesystem::create_directories(path);
        SaveSystem::setSaveRoot(path.string());
    }

    ~ScratchSaveRoot() {
        SaveSystem::setSaveRoot(previous);
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};

Person makePerson(const std::string& displayName) {
    Person person(Soul(displayName), Body("Humanoid", "Voxel"), "");
    person.setDisplayName(displayName);
    return person;
}

Identity::SingularId deterministicPersonId() {
    std::array<uint8_t, 32> key{};
    for (std::size_t i = 0; i < key.size(); ++i) {
        key[i] = static_cast<uint8_t>(i + 1);
    }
    return Identity::SingularId::fromPublicKey(key);
}
} // namespace

int main() {
    ScratchSaveRoot scratch;

    // A lexical/display-name collision is not an identity collision. An
    // Object is allowed to be called "Zach" even when a Person is also called
    // Zach; Personhood is not protected by reserving human-readable strings.
    CategoryManager legacy;
    assert(legacy.create("Zach") != nullptr);
    assert(legacy.create("author.gemini-spark") != nullptr);
    assert(legacy.create("category.control") != nullptr);
    const nlohmann::json legacyBag = legacy.toJson();
    assert(legacyBag.dump().find("\"objectID\":\"Zach\"") != std::string::npos);

    Person zach = makePerson("Zach");
    const Identity::SingularId personId = deterministicPersonId();
    assert(personId.canAuthenticate());
    zach.setPersonId(personId);
    PersonDatabase::getInstance().savePerson(zach);

    CategoryManager categories;
    categories.loadFromJson(legacyBag);

    // Display-name coincidence remains legal.
    assert(categories.get("Zach") != nullptr);
    assert(categories.create("Zach") != nullptr);

    // But the Person's actual unique identity cannot re-enter as an Object.
    const std::string identity = personId.toString();
    assert(categories.create(identity) == nullptr);
    auto counterfeit = std::make_shared<Object>(identity);
    categories.add(counterfeit);
    assert(categories.get(identity) == nullptr);

    // Compatibility stays deliberately narrow: authored category roots remain
    // Objects, and old MODEL author referents are still admitted until their
    // worlds migrate to the First Mover / explicit model-author representation.
    assert(categories.get("category.control") != nullptr);
    assert(categories.get("author.gemini-spark") != nullptr);
    assert(categories.get("category.default") != nullptr);

    const std::string persisted = categories.toJson().dump();
    assert(persisted.find(identity) == std::string::npos);
    assert(persisted.find("\"objectID\":\"Zach\"") != std::string::npos);

    std::cout << "person_not_object_test: Person identity is protected by provenance, not name coincidence\n";
    return 0;
}
