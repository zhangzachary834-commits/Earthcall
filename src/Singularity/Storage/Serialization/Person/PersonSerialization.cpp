#include "Singularity/Storage/Serialization/Person/PersonSerialization.hpp"

#include "Person/Person.hpp"
#include "Singularity/Storage/Serialization/Person/BodySerialization.hpp"

nlohmann::json personToJson(const Person& person) {
    nlohmann::json j;
    j["displayName"] = person.getDisplayName();
    if (person.called()) j["displayLexemeId"] = person.called()->getIdentifier();
    // Keep this legacy alias: older profile readers still use it as a label.
    j["soulName"] = person.getDisplayName();
    if (person.personId().canAuthenticate()) j["personId"] = person.personId().toString();
    const auto& position = person.position();
    const auto& velocity = person.velocity();
    j["position"] = {position.x, position.y, position.z};
    j["velocity"] = {velocity.x, velocity.y, velocity.z};
    j["body"] = bodyToJson(person.getBody());
    return j;
}

void personFromJson(const nlohmann::json& j, Person& person) {
    // Type-checked: profile/session files are untrusted by construction.
    if (j.contains("displayName") && j["displayName"].is_string()) {
        person.setDisplayName(j["displayName"].get<std::string>());
    } else if (j.contains("soulName") && j["soulName"].is_string()) {
        person.setDisplayName(j["soulName"].get<std::string>());
    }

    // A personId read from a file is only a claim. Signature/authority
    // verification remains outside this storage codec, as it did before.
    if (j.contains("personId") && j["personId"].is_string()) {
        Identity::SingularId claimed =
            Identity::SingularId::parse(j["personId"].get<std::string>());
        if (claimed.canAuthenticate()) person.setPersonId(claimed);
    }
    if (j.contains("position") && j["position"].is_array() && j["position"].size() >= 3) {
        person.position() = glm::vec3(j["position"][0], j["position"][1], j["position"][2]);
    }
    if (j.contains("velocity") && j["velocity"].is_array() && j["velocity"].size() >= 3) {
        person.velocity() = glm::vec3(j["velocity"][0], j["velocity"][1], j["velocity"][2]);
    }
    if (j.contains("body")) {
        bodyFromJson(j["body"], person.getBody());
    }
}
