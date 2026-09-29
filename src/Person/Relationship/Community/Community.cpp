#include "Community.hpp"
#include "Person/Person.hpp"
#include <iostream>
#include <stdexcept>

Community::Community(const std::string& name) : Formation(), _name(name) {
}

void Community::addMember(Singular* s) {
    if (dynamic_cast<Person*>(s) != nullptr) {
        Formation::addMember(s);
    } else {
        std::cerr << "Warning: Attempted to add non-Person to Community '" << getIdentifier() << "'\n";
    }
}

void Community::describe() const {
    std::cout << "Community: " << getIdentifier() << "\n";
}

bool Community::involves(const std::string& entity) const {
    if (entity.empty()) return false;
    for (auto* m : getMembers()) {
        if (!m) continue;
        if (const auto* p = dynamic_cast<const Person*>(m)) {
            if (p->matchesIdentifier(entity)) return true;
        } else if (m->getIdentifier() == entity) {
            return true;
        }
    }
    return false;
}

bool Community::involves(const Singular& entity) const {
    const auto* pEntity = dynamic_cast<const Person*>(&entity);
    for (auto* m : getMembers()) {
        if (!m) continue;
        if (m == &entity) return true;

        const auto* pM = dynamic_cast<const Person*>(m);
        if (pM && pEntity) {
            if (pM->hasIdentity() && pEntity->hasIdentity() && pM->personId() == pEntity->personId()) {
                return true;
            }
        } else if (!pM && !pEntity) {
            if (m->getIdentifier() == entity.getIdentifier()) {
                return true;
            }
        }
    }
    return false;
}

bool Community::isBetween(const std::string& a, const std::string& b) const {
    return involves(a) && involves(b);
}

bool Community::isBetween(const Singular& aEntity, const Singular& bEntity) const {
    return involves(aEntity) && involves(bEntity);
}
