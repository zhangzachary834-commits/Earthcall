with open("src/Singularity/Core/CodecChannel.cpp", "r") as f:
    text = f.read()

impls = """std::string CodecChannel::propJsonPretty() const {
    if (_input.empty()) return "";
    try {
        auto j = nlohmann::json::parse(_input);
        return j.dump(2);
    } catch (...) {
        return "";
    }
}

std::string CodecChannel::propEcgraphToJson() const {
    if (_input.empty()) return "[]";
    nlohmann::json root = nlohmann::json::array();
    std::istringstream iss(_input);
    std::string line;
    nlohmann::json currentObj;
    
    while (std::getline(iss, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        if (line[0] == '[') {
            if (!currentObj.empty()) {
                root.push_back(currentObj);
                currentObj.clear();
            }
            size_t space = line.find(' ');
            size_t end = line.find(']');
            if (space != std::string::npos && end != std::string::npos) {
                std::string type = line.substr(1, space - 1);
                std::string id = line.substr(space + 2, end - space - 3);
                currentObj["type"] = type;
                currentObj["id"] = id;
            }
        } else {
            size_t eq = line.find('=');
            if (eq != std::string::npos) {
                std::string key = line.substr(0, eq);
                std::string val = line.substr(eq + 1);
                key.erase(0, key.find_first_not_of(" \\t"));
                key.erase(key.find_last_not_of(" \\t") + 1);
                val.erase(0, val.find_first_not_of(" \\t"));
                val.erase(val.find_last_not_of(" \\t") + 1);
                currentObj[key] = val;
            }
        }
    }
    if (!currentObj.empty()) {
        root.push_back(currentObj);
    }
    return root.dump();
}

std::string CodecChannel::propJsonToMsgpack() const {
    if (_input.empty()) return "";
    try {
        auto j = nlohmann::json::parse(_input);
        std::vector<uint8_t> v = nlohmann::json::to_msgpack(j);
        return std::string(v.begin(), v.end());
    } catch (...) {
        return "";
    }
}
"""

text = text.replace("std::string CodecChannel::propJsonPretty() const {\n    if (_input.empty()) return \"\";\n    try {\n        auto j = nlohmann::json::parse(_input);\n        return j.dump(2);\n    } catch (...) {\n        return \"\";\n    }\n}", impls)

props = """    registerProperty(std::make_unique<ComputedProperty<CodecChannel, std::string>>(
        "codec.jsonPretty", this, &CodecChannel::propJsonPretty, nullptr));
    registerProperty(std::make_unique<ComputedProperty<CodecChannel, std::string>>(
        "codec.ecgraphToJson", this, &CodecChannel::propEcgraphToJson, nullptr));
    registerProperty(std::make_unique<ComputedProperty<CodecChannel, std::string>>(
        "codec.jsonToMsgpack", this, &CodecChannel::propJsonToMsgpack, nullptr));
"""

text = text.replace('    registerProperty(std::make_unique<ComputedProperty<CodecChannel, std::string>>(\n        "codec.jsonPretty", this, &CodecChannel::propJsonPretty, nullptr));', props)

with open("src/Singularity/Core/CodecChannel.cpp", "w") as f:
    f.write(text)
