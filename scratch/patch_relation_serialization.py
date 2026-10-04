import os

path = "src/Singularity/Storage/Serialization/Relation/RelationSerialization.cpp"
with open(path, "r") as f:
    content = f.read()

# Add to relationToJson
to_json_patch = """
    if (relation.hasGroundedType()) {
        out["typeId"] = relation.type;
    }

    const auto& authored = relation.getAuthoredProperties();
    if (!authored.empty()) {
        nlohmann::json dyn = nlohmann::json::object();
        for (const auto& kv : authored) dyn[kv.first] = kv.second.toJson();
        out["authoredProperties"] = std::move(dyn);
    }

    return out;
"""
content = content.replace("""    if (relation.hasGroundedType()) {
        out["typeId"] = relation.type;
    }
    return out;""", to_json_patch)

# Add to relationFromJson
from_json_patch = """
    if (json.contains("attachment")) {
        relation.attachment = Relation::AttachmentData::fromJson(json["attachment"]);
    }

    if (json.contains("authoredProperties") && json["authoredProperties"].is_object()) {
        for (auto it = json["authoredProperties"].begin(); it != json["authoredProperties"].end(); ++it) {
            PropertyValue val = propertyValueFromJson(it.value());
            if (Property* prop = relation.findProperty(it.key())) prop->setValue(val);
            relation.setDynamicProperty(it.key(), val);
        }
    }

    const std::string savedA = json.value("entityA", std::string{});
"""
content = content.replace("""    if (json.contains("attachment")) {
        relation.attachment = Relation::AttachmentData::fromJson(json["attachment"]);
    }

    const std::string savedA = json.value("entityA", std::string{});""", from_json_patch)

with open(path, "w") as f:
    f.write(content)

print("Patched RelationSerialization.cpp")
