#include "CodecChannel.hpp"
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"
#include "json.hpp"
#include <vector>
#include <iomanip>
#include <sstream>

namespace Singularity {
namespace Core {

static const char kBase64Chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

std::string CodecChannel::base64Encode(const std::string& input) {
    std::string out;
    int val = 0;
    int valb = -6;
    for (unsigned char c : input) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(kBase64Chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) {
        out.push_back(kBase64Chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    while (out.size() % 4) {
        out.push_back('=');
    }
    return out;
}

std::string CodecChannel::base64Decode(const std::string& input) {
    std::string out;
    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; i++) {
        T[static_cast<unsigned char>(kBase64Chars[i])] = i;
    }
    int val = 0;
    int valb = -8;
    for (unsigned char c : input) {
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            out.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

std::string CodecChannel::hexEncode(const std::string& input) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned char c : input) {
        oss << std::setw(2) << static_cast<int>(c);
    }
    return oss.str();
}

std::string CodecChannel::hexDecode(const std::string& input) {
    std::string out;
    if (input.length() % 2 != 0) return out;
    out.reserve(input.length() / 2);
    for (size_t i = 0; i < input.length(); i += 2) {
        std::string byteString = input.substr(i, 2);
        char byte = static_cast<char>(std::strtol(byteString.c_str(), nullptr, 16));
        out.push_back(byte);
    }
    return out;
}

std::string CodecChannel::propBase64() const {
    return base64Encode(_input);
}

void CodecChannel::propSetBase64(const std::string& v) {
    _input = base64Decode(v);
}

std::string CodecChannel::propHex() const {
    return hexEncode(_input);
}

void CodecChannel::propSetHex(const std::string& v) {
    _input = hexDecode(v);
}

bool CodecChannel::propJsonValid() const {
    if (_input.empty()) return false;
    return nlohmann::json::accept(_input);
}

std::string CodecChannel::propJsonCompact() const {
    if (_input.empty()) return "";
    try {
        auto j = nlohmann::json::parse(_input);
        return j.dump();
    } catch (...) {
        return "";
    }
}

std::string CodecChannel::propJsonPretty() const {
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
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                val.erase(0, val.find_first_not_of(" \t"));
                val.erase(val.find_last_not_of(" \t") + 1);
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


void CodecChannel::propSetArrayShift(const bool& v) {
    if (!v || _jsonArray.empty()) return;
    try {
        auto j = nlohmann::json::parse(_jsonArray);
        if (j.is_array() && !j.empty()) {
            _shiftItem = j[0].dump();
            j.erase(0);
            _jsonArray = j.dump();
        } else {
            _shiftItem = "{}";
            _jsonArray = "[]";
        }
    } catch (...) {
        _shiftItem = "{}";
        _jsonArray = "[]";
    }
}

std::string CodecChannel::propQueryValue() const {
    if (_shiftItem.empty() || _queryKey.empty()) return "";
    try {
        auto j = nlohmann::json::parse(_shiftItem);
        if (j.contains(_queryKey)) {
            if (j[_queryKey].is_string()) return j[_queryKey].get<std::string>();
            return j[_queryKey].dump();
        }
    } catch (...) {}
    return "";
}

bool CodecChannel::propArrayEmpty() const {
    if (_jsonArray.empty()) return true;
    try {
        auto j = nlohmann::json::parse(_jsonArray);
        return j.empty();
    } catch (...) {
        return true;
    }
}

std::string CodecChannel::propJsonExtract() const {
    if (_input.empty() || _queryKey.empty()) return "";
    try {
        auto j = nlohmann::json::parse(_input);
        if (j.contains(_queryKey)) {
            if (j[_queryKey].is_string()) return j[_queryKey].get<std::string>();
            return j[_queryKey].dump();
        }
    } catch (...) {}
    return "";
}

void CodecChannel::buildProperties() {
    registerProperty(std::make_unique<ComputedProperty<CodecChannel, std::string>>(
        "codec.input", this, &CodecChannel::propInput, &CodecChannel::propSetInput));
    registerProperty(std::make_unique<ComputedProperty<CodecChannel, std::string>>(
        "codec.base64", this, &CodecChannel::propBase64, &CodecChannel::propSetBase64));
    registerProperty(std::make_unique<ComputedProperty<CodecChannel, std::string>>(
        "codec.hex", this, &CodecChannel::propHex, &CodecChannel::propSetHex));
    registerProperty(std::make_unique<ComputedProperty<CodecChannel, bool>>(
        "codec.jsonValid", this, &CodecChannel::propJsonValid, nullptr));
    registerProperty(std::make_unique<ComputedProperty<CodecChannel, std::string>>(
        "codec.jsonCompact", this, &CodecChannel::propJsonCompact, nullptr));
    registerProperty(std::make_unique<ComputedProperty<CodecChannel, std::string>>(
        "codec.jsonPretty", this, &CodecChannel::propJsonPretty, nullptr));
    registerProperty(std::make_unique<ComputedProperty<CodecChannel, std::string>>(
        "codec.ecgraphToJson", this, &CodecChannel::propEcgraphToJson, nullptr));
    registerProperty(std::make_unique<ComputedProperty<CodecChannel, std::string>>(
        "codec.jsonToMsgpack", this, &CodecChannel::propJsonToMsgpack, nullptr));

    registerProperty(std::make_unique<ComputedProperty<CodecChannel, std::string>>(
        "codec.jsonArray", this, &CodecChannel::propJsonArray, &CodecChannel::propSetJsonArray));
    registerProperty(std::make_unique<ComputedProperty<CodecChannel, std::string>>(
        "codec.shiftItem", this, &CodecChannel::propShiftItem, &CodecChannel::propSetShiftItem));
    registerProperty(std::make_unique<ComputedProperty<CodecChannel, bool>>(
        "codec.arrayShift", this, &CodecChannel::propArrayShift, &CodecChannel::propSetArrayShift));
    registerProperty(std::make_unique<ComputedProperty<CodecChannel, std::string>>(
        "codec.queryKey", this, &CodecChannel::propQueryKey, &CodecChannel::propSetQueryKey));
    registerProperty(std::make_unique<ComputedProperty<CodecChannel, std::string>>(
        "codec.queryValue", this, &CodecChannel::propQueryValue, nullptr));
    registerProperty(std::make_unique<ComputedProperty<CodecChannel, bool>>(
        "codec.arrayEmpty", this, &CodecChannel::propArrayEmpty, nullptr));
    registerProperty(std::make_unique<ComputedProperty<CodecChannel, std::string>>(
        "codec.jsonExtract", this, &CodecChannel::propJsonExtract, nullptr));
}

} // namespace Core
} // namespace Singularity
