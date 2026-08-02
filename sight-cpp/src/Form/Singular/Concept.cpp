#include "Form/Singular/Concept.hpp"
#include "Form/Singular/Property/PropertyValueJson.hpp"
#include <uuid/uuid.h>
#include <iostream>

Concept::Concept(const std::string& name) : _name(name) {
    uuid_t id;
    uuid_generate(id);
    char idStr[37];
    uuid_unparse(id, idStr);
    _conceptId = "concept_" + std::string(idStr);
}

void Concept::buildProperties() {
    // Register concept-specific properties if any
}

nlohmann::json Concept::SingularTemplate::toJson() const {
    nlohmann::json j;
    j["classType"] = classType;
    nlohmann::json props;
    for (const auto& [k, v] : initialProperties) {
        props[k] = propertyValueToJson(v);
    }
    j["initialProperties"] = props;
    return j;
}

Concept::SingularTemplate Concept::SingularTemplate::fromJson(const nlohmann::json& j) {
    SingularTemplate st;
    st.classType = j.value("classType", "Singular");
    if (j.contains("initialProperties")) {
        for (auto& item : j["initialProperties"].items()) {
            st.initialProperties[item.key()] = propertyValueFromJson(item.value());
        }
    }
    return st;
}

nlohmann::json Concept::toJson() const {
    nlohmann::json j;
    j["id"] = _conceptId;
    j["name"] = _name;
    nlohmann::json membersJson = nlohmann::json::array();
    for (const auto& member : _members) {
        membersJson.push_back(member.toJson());
    }
    j["members"] = membersJson;
    return j;
}

std::shared_ptr<Concept> Concept::fromJson(const nlohmann::json& j) {
    auto concept = std::make_shared<Concept>(j.value("name", "Concept"));
    concept->_conceptId = j.value("id", concept->_conceptId);
    if (j.contains("members")) {
        for (const auto& m : j["members"]) {
            concept->_members.push_back(SingularTemplate::fromJson(m));
        }
    }
    return concept;
}

UniversalConceptRegistry& UniversalConceptRegistry::instance() {
    static UniversalConceptRegistry inst;
    return inst;
}

void UniversalConceptRegistry::registerConcept(std::shared_ptr<Concept> concept) {
    if (concept) {
        _concepts[concept->getIdentifier()] = concept;
    }
}

std::shared_ptr<Concept> UniversalConceptRegistry::findConcept(const std::string& id) const {
    auto it = _concepts.find(id);
    if (it != _concepts.end()) {
        return it->second;
    }
    return nullptr;
}
