import re

with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "r") as f:
    content = f.read()

new_wapp = """bool selectionDefinition(const Object& object, const std::string& propertyName,
                         int& face, OntoMath::Piecewise& selector) {
    const auto id = Earthcall::StringInterner::intern(
        std::string(kSelectionPrefix) + propertyName);
    const auto found = object.dynamicProperties().find(id);
    if (found == object.dynamicProperties().end()) {
        if (propertyName == "authored.full-canvas") {
            std::cerr << "WAPP selectionDefinition: not found in dynamicProperties for " << propertyName << std::endl;
        }
        return false;
    }
    const auto* encoded = std::get_if<std::string>(&found->second);
    if (!encoded) {
        if (propertyName == "authored.full-canvas") {
            std::cerr << "WAPP selectionDefinition: not encoded as string for " << propertyName << " index=" << found->second.index() << std::endl;
        }
        return false;
    }
    try {
        const nlohmann::json definition = nlohmann::json::parse(*encoded);
        face = definition.value("face", -1);
        if (face < 0 || !definition.contains("selector")) {
            if (propertyName == "authored.full-canvas") {
                std::cerr << "WAPP selectionDefinition: bad face or missing selector" << std::endl;
            }
            return false;
        }
        selector = OntoMath::Piecewise::fromJson(definition["selector"]);
        return true;
    } catch (...) {
        if (propertyName == "authored.full-canvas") {
            std::cerr << "WAPP selectionDefinition: json parse failed" << std::endl;
        }
        return false;
    }
}"""

content = re.sub(
    r'bool selectionDefinition\(const Object& object, const std::string& propertyName,\n\s*int& face, OntoMath::Piecewise& selector\) \{.*?\n\}',
    new_wapp,
    content,
    flags=re.DOTALL
)

with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "w") as f:
    f.write(content)
