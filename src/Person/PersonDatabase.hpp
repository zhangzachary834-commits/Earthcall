#pragma once
#include <string>
#include <vector>
#include "Person/Person.hpp"

class PersonDatabase {
public:
    static PersonDatabase& getInstance();
    
    // Save a person's complete profile
    void savePerson(const Person& person);
    
    // Load a person's profile by soul name (identifier)
    // Returns true if successful, with populated person object
    bool loadPerson(const std::string& displayName, Person& outPerson);
    
    // Get a list of all registered person identifiers
    std::vector<std::string> getAllRegisteredPersons() const;

private:
    PersonDatabase() = default;
    ~PersonDatabase() = default;
    
    PersonDatabase(const PersonDatabase&) = delete;
    PersonDatabase& operator=(const PersonDatabase&) = delete;
};
