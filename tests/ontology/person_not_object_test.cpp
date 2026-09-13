#include "ConstructedBeing/CategoryManager.hpp"
#include "Person/Body/Body.hpp"
#include "Person/Person.hpp"
#include "Person/PersonDatabase.hpp"
#include "Person/Soul/Soul.hpp"
#include "Singularity/Storage/SaveSystem.hpp"

#include <cassert>
#include <chrono>
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
} // namespace

int main() {
    ScratchSaveRoot scratch;

    // Reproduce the historical generator shape before a Person profile exists:
    // a human author was smuggled through the categories bag as an extra-spatial
    // Object solely so Law author lookup could find the string "Zach".
    CategoryManager legacy;
    assert(legacy.create("Zach") != nullptr);
    assert(legacy.create("author.gemini-spark") != nullptr);
    assert(legacy.create("category.control") != nullptr);
    const nlohmann::json legacyBag = legacy.toJson();
    assert(legacyBag.dump().find("\"objectID\":\"Zach\"") != std::string::npos);

    // Once Zach is a registered Person, that identifier belongs to Personhood.
    // CategoryManager must not mint a second being of a lesser ontology with
    // the same identity just because an old save asks it to.
    Person zach = makePerson("Zach");
    PersonDatabase::getInstance().savePerson(zach);

    CategoryManager categories;
    categories.loadFromJson(legacyBag);

    assert(categories.get("Zach") == nullptr);
    assert(categories.create("Zach") == nullptr);

    auto counterfeit = std::make_shared<Object>("Zach");
    categories.add(counterfeit);
    assert(categories.get("Zach") == nullptr);

    // Compatibility stays deliberately narrow: authored category roots remain
    // Objects, and old MODEL author referents are still admitted until their
    // worlds migrate to the First Mover / explicit model-author representation.
    assert(categories.get("category.control") != nullptr);
    assert(categories.get("author.gemini-spark") != nullptr);
    assert(categories.get("category.default") != nullptr);

    const std::string persisted = categories.toJson().dump();
    assert(persisted.find("\"objectID\":\"Zach\"") == std::string::npos);

    std::cout << "person_not_object_test: Person identity cannot re-enter through CategoryManager\n";
    return 0;
}
