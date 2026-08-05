#pragma once
#include <string>
#include <vector>
#include "../Zone/Zone.hpp"
#include "../../Person/Person.hpp"

class Home : public Zone {
public:
    std::vector<std::string> owners;
    std::string members;

    //Home();
    Home(std::vector<std::string> owners);
    //Home(std::string members);
    //Home(std::string owners, std::string members);
    void welcome() const;

    std::vector<Person*> getPersons() const { return _persons; }
    void addPerson(Person* person) { _persons.push_back(person); }
    void removePerson(Person* person) { _persons.erase(std::remove(_persons.begin(), _persons.end(), person), _persons.end()); }

    std::vector<Object*> getObjects() const { return _objects; }
    void addObject(Object* obj) { _objects.push_back(obj); }
    void removeObject(Object* obj) { _objects.erase(std::remove(_objects.begin(), _objects.end(), obj), _objects.end()); }

    // Singular interface
    std::string getIdentifier() const { return owners.empty()? "Home" : "Home_of_"+owners[0]; }

private: 
    std::vector<Object*> _objects;
    std::vector<Person*> _persons;

};
