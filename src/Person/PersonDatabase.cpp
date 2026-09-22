#include "PersonDatabase.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "Singularity/Core/Logger.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>

PersonDatabase& PersonDatabase::getInstance() {
    static PersonDatabase instance;
    return instance;
}

void PersonDatabase::savePerson(const Person& person) {
    std::string identifier = person.getIdentifier();
    if (identifier.empty()) {
        std::cerr << "Cannot save Person with empty identifier." << std::endl;
        return;
    }
    
    nlohmann::json j = person.serialize();
    
    // Save to the PERSON save type in SaveSystem keyed on unique identifier
    SaveSystem::writeSaveData(j, identifier, SaveSystem::SaveType::PERSON);
    
    ECA::Logger::instance().log(ECA::LogCategory::Person, "PROFILE_SAVE", "Successfully saved Person profile for: " + person.getDisplayName() + " [" + identifier + "]", nlohmann::json{{"person", person.getDisplayName()}, {"identifier", identifier}});
    std::cout << "Successfully saved Person profile for: " << person.getDisplayName() << " [" << identifier << "]" << std::endl;
}

bool PersonDatabase::loadPerson(const std::string& identifier, Person& outPerson) {
    // identifier reaches here from untrusted sources, so sanitize before path construction
    std::string safeName = SaveSystem::sanitizeLabel(identifier);
    if (safeName.empty()) return false;

    std::string folder = SaveSystem::ensureSaveTypeFolder(SaveSystem::SaveType::PERSON);
    if (folder.empty()) return false;

    std::string filepath = folder + "/" + safeName + ".ecform";

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
