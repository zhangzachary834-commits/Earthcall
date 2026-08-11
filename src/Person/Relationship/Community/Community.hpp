#pragma once

#include "ConstructedBeing/Object/Formation/Formation.hpp"

class Community : public Formation {
private:
    std::string _name;
public:
    Community(const std::string& name);
    ~Community() override = default;

    std::string getIdentifier() const override { return _name; }
    void addMember(Singular* s) override;

    void describe() const;
    bool involves(const std::string& entity) const;
    bool involves(const Singular& entity) const;
    bool isBetween(const std::string& a, const std::string& b) const;
    bool isBetween(const Singular& aEntity, const Singular& bEntity) const;
};