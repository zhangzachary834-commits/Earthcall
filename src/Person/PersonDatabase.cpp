#include "PersonDatabase.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>

PersonDatabase& PersonDatabase::getInstance() {
    static PersonDatabase instance;
    return instance;
}

void PersonDatabase::savePerson(const Person& person) {
    if (person.getDisplayName().empty()) {
        std::cerr << "Cannot save Person with empty displayName." << std::endl;
        return;
    }
    
    nlohmann::json j = person.serialize();
    
    // Save to the PERSON save type in SaveSystem
    SaveSystem::writeSaveData(j, person.getDisplayName(), SaveSystem::SaveType::PERSON);
    
    std::cout << "Successfully saved Person profile for: " << person.getDisplayName() << std::endl;
}

bool PersonDatabase::loadPerson(const std::string& displayName, Person& outPerson) {
    // displayName reaches here from deserialized save data, so it is untrusted
    // and cannot be concatenated into a path raw. ensureSaveTypeFolder (not
    // getSaveTypeFolderName) is what savePerson writes into.
    std::string safeName = SaveSystem::sanitizeLabel(displayName);
    if (safeName.empty()) return false;

    std::string folder = SaveSystem::ensureSaveTypeFolder(SaveSystem::SaveType::PERSON);
    if (folder.empty()) return false;

    std::string filepath = folder + "/" + safeName + ".json";

    if (!std::filesystem::exists(filepath)) {
        return false;
    }
    
    std::ifstream file(filepath);
    if (!file.is_open()) return false;
    
    nlohmann::json j;
    try {
        file >> j;
        outPerson.deserialize(j);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse Person json: " << e.what() << std::endl;
        return false;
    }
}

std::vector<std::string> PersonDatabase::getAllRegisteredPersons() const {
    return SaveSystem::listFiles(SaveSystem::SaveType::PERSON);
}
