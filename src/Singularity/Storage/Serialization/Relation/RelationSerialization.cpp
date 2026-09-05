#include "Singularity/Storage/Serialization/Relation/RelationSerialization.hpp"

#include <cstdio>
#include <cstring>

namespace {

std::vector<float> mat4ToVector(const glm::mat4& matrix) {
    std::vector<float> values(16);
    const float* raw = glm::value_ptr(matrix);
    for (int i = 0; i < 16; ++i) values[i] = raw[i];
    return values;
}

glm::mat4 vectorToMat4(const std::vector<float>& values) {
    glm::mat4 matrix(1.0f);
    if (values.size() == 16) {
        std::memcpy(glm::value_ptr(matrix), values.data(), sizeof(float) * 16);
    }
    return matrix;
}

} // namespace

nlohmann::json Relation::AttachmentData::toJson() const {
    return nlohmann::json{
        {"enabled", enabled},
        {"localOffset", mat4ToVector(localOffset)},
        {"parentAnchor", {parentAnchor.x, parentAnchor.y, parentAnchor.z}},
        {"childAnchor", {childAnchor.x, childAnchor.y, childAnchor.z}},
        {"inheritTranslation", inheritTranslation},
        {"inheritRotation", inheritRotation},
        {"inheritScale", inheritScale}
    };
}

Relation::AttachmentData Relation::AttachmentData::fromJson(const nlohmann::json& json) {
    AttachmentData data;
    data.enabled = json.value("enabled", false);
    data.localOffset = vectorToMat4(json.value("localOffset", std::vector<float>{}));
    if (json.contains("parentAnchor") && json["parentAnchor"].is_array() &&
        json["parentAnchor"].size() >= 3) {
        data.parentAnchor = glm::vec3(json["parentAnchor"][0].get<float>(),
                                      json["parentAnchor"][1].get<float>(),
                                      json["parentAnchor"][2].get<float>());
    }
    if (json.contains("childAnchor") && json["childAnchor"].is_array() &&
        json["childAnchor"].size() >= 3) {
        data.childAnchor = glm::vec3(json["childAnchor"][0].get<float>(),
                                     json["childAnchor"][1].get<float>(),
                                     json["childAnchor"][2].get<float>());
    }
    data.inheritTranslation = json.value("inheritTranslation", true);
    data.inheritRotation = json.value("inheritRotation", true);
    data.inheritScale = json.value("inheritScale", true);
    return data;
}

nlohmann::json relationToJson(const Relation& relation) {
    nlohmann::json events = nlohmann::json::array();
    for (const auto& event : relation.events) events.push_back(event.toJson());

    return nlohmann::json{{"type", relation.type},
                          {"entityA", relation.aId()},
                          {"entityB", relation.bId()},
                          {"directed", relation.directed},
                          {"weight", relation.getWeight()},
                          {"events", events},
                          {"attachment", relation.attachment.toJson()}};
}

Relation relationFromJson(const nlohmann::json& json,
                          const RelationEndpointResolver& resolve) {
    Relation relation;
    relation.type = json.at("type").get<std::string>();
    relation.directed = json.value("directed", false);
    relation.setWeight(json.value("weight", 1.0f));

    if (json.contains("events") && json["events"].is_array()) {
        for (const auto& item : json["events"]) {
            relation.events.push_back(RelationEvent::fromJson(item));
        }
    }
    if (json.contains("attachment")) {
        relation.attachment = Relation::AttachmentData::fromJson(json["attachment"]);
    }

    const std::string savedA = json.value("entityA", std::string{});
    const std::string savedB = json.value("entityB", std::string{});
    relation._endpointA.savedId = savedA;
    relation._endpointB.savedId = savedB;

    if (resolve) {
        Singular* a = savedA.empty() ? nullptr : resolve(savedA);
        Singular* b = savedB.empty() ? nullptr : resolve(savedB);
        relation.bind(a, b);
        if ((!savedA.empty() && !relation.a()) || (!savedB.empty() && !relation.b())) {
            std::fprintf(stderr,
                "Relation load: unbound endpoint(s) type='%s' a='%s' b='%s'. "
                "Identifier properties are kept for a later bind.\n",
                relation.type.c_str(), savedA.c_str(), savedB.c_str());
        }
    }
    return relation;
}
