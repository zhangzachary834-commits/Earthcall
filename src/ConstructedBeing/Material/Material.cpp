#include "ConstructedBeing/Material/Material.hpp"
#include "ConstructedBeing/Singular/Property/PropertyRef.hpp"

#include <cstdint>
#include <string>
#include <vector>

using json = nlohmann::json;

namespace {
// Paint lives on the Material being. Object serialization used to carry
// faceTextures, then paint moved here and the pixels stopped round-tripping
// — a checkerboard authored into a save loaded as a blank face. The encode
// is the same Base64 the Object path used; it stays local so Material does
// not take a dependency on the save codec.
const char kBase64Table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64Encode(const std::vector<uint8_t>& data) {
    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);
    size_t i = 0;
    while (i + 3 <= data.size()) {
        uint32_t n = (static_cast<uint32_t>(data[i]) << 16) |
                     (static_cast<uint32_t>(data[i + 1]) << 8) |
                     static_cast<uint32_t>(data[i + 2]);
        out.push_back(kBase64Table[(n >> 18) & 63]);
        out.push_back(kBase64Table[(n >> 12) & 63]);
        out.push_back(kBase64Table[(n >> 6) & 63]);
        out.push_back(kBase64Table[n & 63]);
        i += 3;
    }
    if (i < data.size()) {
        uint32_t n = static_cast<uint32_t>(data[i]) << 16;
        if (i + 1 < data.size()) n |= static_cast<uint32_t>(data[i + 1]) << 8;
        out.push_back(kBase64Table[(n >> 18) & 63]);
        out.push_back(kBase64Table[(n >> 12) & 63]);
        if (i + 1 < data.size()) out.push_back(kBase64Table[(n >> 6) & 63]);
        else out.push_back('=');
        out.push_back('=');
    }
    return out;
}

uint8_t b64Val(char c) {
    if (c >= 'A' && c <= 'Z') return static_cast<uint8_t>(c - 'A');
    if (c >= 'a' && c <= 'z') return static_cast<uint8_t>(c - 'a' + 26);
    if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0' + 52);
    if (c == '+') return 62;
    if (c == '/') return 63;
    return 255;
}

std::vector<uint8_t> base64Decode(const std::string& input) {
    std::string s;
    s.reserve(input.size());
    for (char c : input) {
        if (c == '\n' || c == '\r' || c == '\t' || c == ' ') continue;
        s.push_back(c);
    }
    const size_t len = s.size();
    if (len == 0 || len % 4 != 0) return {};
    size_t pad = 0;
    if (s[len - 1] == '=') pad++;
    if (len >= 2 && s[len - 2] == '=') pad++;
    std::vector<uint8_t> out;
    out.reserve((len / 4) * 3 - pad);
    for (size_t i = 0; i < len; i += 4) {
        const uint8_t a = b64Val(s[i]);
        const uint8_t b = b64Val(s[i + 1]);
        const uint8_t c = s[i + 2] == '=' ? 0 : b64Val(s[i + 2]);
        const uint8_t d = s[i + 3] == '=' ? 0 : b64Val(s[i + 3]);
        if (a == 255 || b == 255 || (s[i + 2] != '=' && c == 255) ||
            (s[i + 3] != '=' && d == 255)) {
            return {};
        }
        out.push_back(static_cast<uint8_t>((a << 2) | (b >> 4)));
        if (s[i + 2] != '=') out.push_back(static_cast<uint8_t>(((b & 15) << 4) | (c >> 2)));
        if (s[i + 3] != '=') out.push_back(static_cast<uint8_t>(((c & 3) << 6) | d));
    }
    return out;
}

json faceTexturesToJson(const std::vector<FaceTexture>& textures) {
    json arr = json::array();
    for (const auto& ft : textures) {
        json ftj;
        ftj["size"] = ft.size;
        if (ft.useLayers) ft.compositeLayers();
        ftj["pixelsB64"] = base64Encode(ft.pixels);
        arr.push_back(std::move(ftj));
    }
    return arr;
}

void faceTexturesFromJson(Material& m, const json& arr) {
    if (!arr.is_array()) return;
    m.faceTextures.clear();
    m.faceTextures.reserve(arr.size());
    for (const auto& ftj : arr) {
        FaceTexture ft;
        const int size = ftj.value("size", 64);
        if (size <= 0 || size > 4096) continue;
        ft.size = size;
        const std::string b64 = ftj.value("pixelsB64", std::string());
        std::vector<uint8_t> data = base64Decode(b64);
        const size_t expected = static_cast<size_t>(size) * static_cast<size_t>(size) * 4;
        if (data.size() != expected) {
            ft.create(0xFFFFFFFFu);
        } else {
            ft.pixels = std::move(data);
            ft.updateWholeGPU();
        }
        m.faceTextures.push_back(std::move(ft));
    }
}
} // namespace

void Material::buildProperties() {
    registerProperty(std::make_unique<PropertyRef<Material, glm::vec3>>(
        "baseColor", this, &Material::baseColor));
    registerProperty(std::make_unique<PropertyRef<Material, float>>(
        "opacity", this, &Material::opacity));
    registerProperty(std::make_unique<PropertyRef<Material, float>>(
        "shininess", this, &Material::shininess));
    registerProperty(std::make_unique<PropertyRef<Material, float>>(
        "specular", this, &Material::specular));
    registerProperty(std::make_unique<PropertyRef<Material, float>>(
        "ambient", this, &Material::ambient));
    registerProperty(std::make_unique<PropertyRef<Material, float>>(
        "diffuse", this, &Material::diffuse));
}

json Material::toJson() const {
    json j{
        {"name", _name},
        {"baseColor", {baseColor.r, baseColor.g, baseColor.b}},
        {"opacity", opacity},
        {"shininess", shininess},
        {"specular", specular},
        {"ambient", ambient},
        {"diffuse", diffuse},
    };
    if (!faceTextures.empty()) {
        j["faceTextures"] = faceTexturesToJson(faceTextures);
    }
    return j;
}

Material Material::fromJson(const json& j) {
    Material m(j.value("name", std::string("default")));
    if (j.contains("baseColor") && j["baseColor"].is_array() && j["baseColor"].size() == 3) {
        m.baseColor = glm::vec3(j["baseColor"][0].get<float>(),
                                j["baseColor"][1].get<float>(),
                                j["baseColor"][2].get<float>());
    }
    m.opacity   = j.value("opacity", 1.0f);
    m.shininess = j.value("shininess", 32.0f);
    m.specular  = j.value("specular", 1.0f);
    m.ambient   = j.value("ambient", 0.2f);
    m.diffuse   = j.value("diffuse", 0.8f);
    if (j.contains("faceTextures")) {
        faceTexturesFromJson(m, j["faceTextures"]);
    }
    return m;
}

void Material::initFaceTextures(int numFaces) {
    if (faceTextures.size() == static_cast<size_t>(numFaces)) {
        return; // Already initialised correctly
    }
    faceTextures.clear();
    for (int i = 0; i < numFaces; ++i) {
        FaceTexture tex;
        tex.create(); // Default 64x64 white texture
        faceTextures.push_back(std::move(tex));
    }
}
