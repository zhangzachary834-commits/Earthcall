import os

path = "src/Singularity/Storage/Serialization/ZonesOfEarth/ZoneSerialization.cpp"
with open(path, "r") as f:
    content = f.read()

# Add to zoneToJson lexemes loop
to_json_patch = """
        auto* lexeme = dynamic_cast<Singularity::Language::Lexeme*>(member);
        if (!lexeme) continue;
        nlohmann::json item = {
            {"id", lexeme->getIdentifier()},
            {"symbol", lexeme->getSymbol()}
        };
        const auto& authored = lexeme->getAuthoredProperties();
        if (!authored.empty()) {
            nlohmann::json dyn = nlohmann::json::object();
            for (const auto& kv : authored) dyn[kv.first] = kv.second.toJson();
            item["authoredProperties"] = std::move(dyn);
        }
        lexemes.push_back(item);
"""
content = content.replace("""        auto* lexeme = dynamic_cast<Singularity::Language::Lexeme*>(member);
        if (!lexeme) continue;
        lexemes.push_back({
            {"id", lexeme->getIdentifier()},
            {"symbol", lexeme->getSymbol()}
        });""", to_json_patch)

# Add to internZoneLexemes
from_json_patch = """
        auto lexeme = language.intern(symbol, id);
        if (item.contains("authoredProperties") && item["authoredProperties"].is_object()) {
            for (auto it = item["authoredProperties"].begin(); it != item["authoredProperties"].end(); ++it) {
                PropertyValue val = propertyValueFromJson(it.value());
                if (Property* prop = lexeme->findProperty(it.key())) prop->setValue(val);
                lexeme->setDynamicProperty(it.key(), val);
            }
        }
        zone.addToFormation(lexeme.get());
"""
content = content.replace("""        auto lexeme = language.intern(symbol, id);
        zone.addToFormation(lexeme.get());""", from_json_patch)

with open(path, "w") as f:
    f.write(content)

print("Patched ZoneSerialization.cpp")
