#include "Soul.hpp"
#include "Person/Person.hpp"

#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"

Soul::Soul(std::string constructionName)
    : _constructionName(std::move(constructionName)) {}

Soul::Soul(const Soul& o) : Singular(o), _constructionName(o._constructionName) {}

Soul& Soul::operator=(const Soul& o) {
    if (this != &o) {
        Singular::operator=(o);
        _constructionName = o._constructionName;
        // _person is not assigned: belonging is not copied, it is rebound.
    }
    return *this;
}

Soul::Soul(Soul&& o) noexcept
    : Singular(std::move(o)), _constructionName(std::move(o._constructionName)) {}

Soul& Soul::operator=(Soul&& o) noexcept {
    if (this != &o) {
        Singular::operator=(std::move(o));
        _constructionName = std::move(o._constructionName);
    }
    return *this;
}

void Soul::bindPerson(Person* person) {
    _person = person;
    _constructionName.clear();
}

std::string Soul::getIdentifier() const {
    return _person ? _person->getIdentifier() : std::string{};
}

std::string Soul::propPerson() const {
    return _person ? _person->getIdentifier() : std::string{};
}

void Soul::buildProperties() {
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<Soul, std::string>>(
        "person", this, &Soul::propPerson, nullptr));
}
