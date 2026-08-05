#pragma once

#include "Form/Singular/Singular.hpp"
#include "json.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>

class Concept : public Singular {
public:
    // A template for instantiating a new Singular. It holds the string identifier
    // for the class type (e.g. "Lexeme", "Zone", "Object") and any initial property values.
    struct SingularTemplate {
        std::string classType; // e.g. "Lexeme"
        std::map<std::string, PropertyValue> initialProperties;

        nlohmann::json toJson() const;
        static SingularTemplate fromJson(const nlohmann::json& j);
    };

    explicit Concept(const std::string& name = "Concept");

    std::string getIdentifier() const override { return _conceptId; }
    const std::string& name() const { return _name; }
    void setName(const std::string& name) { _name = name.empty() ? "Concept" : name; }

    std::vector<SingularTemplate>& members() { return _members; }
    const std::vector<SingularTemplate>& members() const { return _members; }
    void addMember(const SingularTemplate& member) { _members.push_back(member); }

    nlohmann::json toJson() const;
    static std::shared_ptr<Concept> fromJson(const nlohmann::json& j);

private:
    void buildProperties() override;

    std::string _conceptId;
    std::string _name;
    std::vector<SingularTemplate> _members;
};

class UniversalConceptRegistry {
public:
    static UniversalConceptRegistry& instance();

    void registerConcept(std::shared_ptr<Concept> concept);
    std::shared_ptr<Concept> findConcept(const std::string& id) const;

private:
    UniversalConceptRegistry() = default;
    std::map<std::string, std::shared_ptr<Concept>> _concepts;
};
