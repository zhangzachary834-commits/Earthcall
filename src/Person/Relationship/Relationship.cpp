#include "Relationship.hpp"
#include <iostream>

Relationship::Relationship(const std::string& type, const std::string& a, const std::string& b, bool directed, float initialWeight)
    : Relation() {
    this->type = type;
    this->directed = directed;
    this->setWeight(initialWeight);
}

Relationship::Relationship(const std::string& type, const Singular& aEntity, const Singular& bEntity, bool directed, float initialWeight)
    : Relation(type, aEntity, bEntity, directed, initialWeight) {
}

Relationship::~Relationship() {
}

void Relationship::describe() const {
    std::cout << "Relationship: " << type << " between " << aId() << " and " << bId() << std::endl;
}

bool Relationship::involves(const std::string& entity) const {
    return aId() == entity || bId() == entity;
}

bool Relationship::involves(const Singular& entity) const {
    return Relation::involves(entity);
}

bool Relationship::isBetween(const std::string& a, const std::string& b) const {
    return (aId() == a && bId() == b) || (!directed && aId() == b && bId() == a);
}

bool Relationship::isBetween(const Singular& aEntity, const Singular& bEntity) const {
    return Relation::isBetween(aEntity, bEntity);
}
