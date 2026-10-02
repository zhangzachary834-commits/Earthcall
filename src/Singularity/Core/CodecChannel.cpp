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
}

} // namespace Core
} // namespace Singularity
