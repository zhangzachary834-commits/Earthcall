#include "PersonDatabase.hpp"
#include "Util/SaveSystem.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>

PersonDatabase& PersonDatabase::getInstance() {
    static PersonDatabase instance;
    return instance;
}

void PersonDatabase::savePerson(const Person& person) {
    if (person.soulName.empty()) {
        std::cerr << "Cannot save Person with empty soulName." << std::endl;
        return;
    }
    
    nlohmann::json j = person.serialize();
    
    // Save to the PERSON save type in SaveSystem
    SaveSystem::writeJson(j, person.soulName, SaveSystem::SaveType::PERSON);
    
    std::cout << "Successfully saved Person profile for: " << person.soulName << std::endl;
}

bool PersonDatabase::loadPerson(const std::string& soulName, Person& outPerson) {
    std::string folder = SaveSystem::getSaveTypeFolderName(SaveSystem::SaveType::PERSON);
    std::string filepath = folder + "/" + soulName + ".json";
    
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
