import os

path = "src/Singularity/Storage/Serialization/ZonesOfEarth/ZoneSerialization.cpp"
with open(path, "r") as f:
    content = f.read()

bad = """        const auto& authored = lexeme->getAuthoredProperties();
        if (!authored.empty()) {
            nlohmann::json dyn = nlohmann::json::object();
            for (const auto& kv : authored) dyn[kv.first] = kv.second.toJson();
            item["authoredProperties"] = std::move(dyn);
        }"""

good = """        nlohmann::json dyn = nlohmann::json::object();
        for (const auto& entry : lexeme->dynamicProperties()) {
            dyn[Earthcall::StringInterner::resolve(entry.first)] = propertyValueToJson(entry.second);
        }
        if (!dyn.empty()) {
            item["authoredProperties"] = std::move(dyn);
        }"""

content = content.replace(bad, good)
with open(path, "w") as f:
    f.write(content)
