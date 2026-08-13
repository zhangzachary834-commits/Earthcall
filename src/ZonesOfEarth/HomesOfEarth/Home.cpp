#include "HomesOfEarth/Home.hpp"
#include <vector>
#include <string>
#include <iostream>

Home::Home(std::vector<std::string> owners)
    : Zone("Home", "default"), owners(owners) {}

void Home::welcome() const {
    if (!owners.empty()) {
        std::cout << "Welcome to " << owners[0] << "'s home." << std::endl;
    } else {
        std::cout << "Welcome home." << std::endl;
    }
}

std::vector<Person*> Home::getPersons() const {
    std::vector<Person*> persons;
    for (auto* member : getFormation().getMembers()) {
        if (auto* p = dynamic_cast<Person*>(member)) {
            persons.push_back(p);
        }
    }
    return persons;
}

void Home::addPerson(Person* person) {
    if (person) {
        // We cast Person to Singular* implicitly or explicitly
        getFormation().addMember(person);
    }
}

void Home::removePerson(Person* person) {
    if (person) {
        getFormation().removeMember(person);
    }
}

std::vector<Object*> Home::getObjects() const {
    std::vector<Object*> objects;
    for (auto* member : getFormation().getMembers()) {
        if (auto* obj = dynamic_cast<Object*>(member)) {
            objects.push_back(obj);
        }
    }
    return objects;
}

void Home::addObject(Object* obj) {
    if (obj) {
        getFormation().addMember(obj);
    }
}

void Home::removeObject(Object* obj) {
    if (obj) {
        getFormation().removeMember(obj);
    }
}
