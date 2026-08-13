#pragma once
#include <string>
#include <vector>
#include "../Zone/Zone.hpp"
#include "../../Person/Person.hpp"

class Home : public Zone {
public:
    std::vector<std::string> owners;
    std::string members;

    Home(std::vector<std::string> owners);
    void welcome() const;

    std::vector<Person*> getPersons() const;
    void addPerson(Person* person);
    void removePerson(Person* person);

    std::vector<Object*> getObjects() const;
    void addObject(Object* obj);
    void removeObject(Object* obj);

    // Singular interface
    std::string getIdentifier() const override { return owners.empty() ? "Home" : "Home_of_" + owners[0]; }
};
