#include "Util/Serialization.hpp"
#include <cstring>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <vector>
#include <algorithm>

// ------------------------------------------------------------------
// Simple Base64 encode/decode for binary pixel buffers (RGBA8)
// ------------------------------------------------------------------
namespace {
    static const char kBase64Table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string base64Encode(const std::vector<uint8_t>& data) {
        std::string out;
        out.reserve(((data.size() + 2) / 3) * 4);
        size_t i = 0;
        while (i + 3 <= data.size()) {
            uint32_t n = (static_cast<uint32_t>(data[i]) << 16) |
                         (static_cast<uint32_t>(data[i + 1]) << 8) |
                         (static_cast<uint32_t>(data[i + 2]));
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
            if (i + 1 < data.size()) {
                out.push_back(kBase64Table[(n >> 6) & 63]);
            } else {
                out.push_back('=');
            }
            out.push_back('=');
        }
        return out;
    }

    inline uint8_t b64Val(char c) {
        if (c >= 'A' && c <= 'Z') return static_cast<uint8_t>(c - 'A');
        if (c >= 'a' && c <= 'z') return static_cast<uint8_t>(c - 'a' + 26);
        if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0' + 52);
        if (c == '+') return 62;
        if (c == '/') return 63;
        return 255; // invalid
    }

    std::vector<uint8_t> base64Decode(const std::string& input) {
        // Remove whitespace
        std::string s; s.reserve(input.size());
        for (char c : input) {
            if (c == '\n' || c == '\r' || c == '\t' || c == ' ') continue;
            s.push_back(c);
        }

        size_t len = s.size();
        if (len % 4 != 0) return {};
        size_t pad = 0;
        if (len >= 2) {
            if (s[len - 1] == '=') pad++;
            if (s[len - 2] == '=') pad++;
        }
        size_t outLen = (len / 4) * 3 - pad;
        std::vector<uint8_t> out; out.reserve(outLen);
        for (size_t i = 0; i < len; i += 4) {
            uint8_t a = b64Val(s[i]);
            uint8_t b = b64Val(s[i + 1]);
            uint8_t c = s[i + 2] == '=' ? 0 : b64Val(s[i + 2]);
            uint8_t d = s[i + 3] == '=' ? 0 : b64Val(s[i + 3]);
            if (a == 255 || b == 255 || (s[i + 2] != '=' && c == 255) || (s[i + 3] != '=' && d == 255)) {
                return {}; // invalid char
            }
            uint32_t n = (static_cast<uint32_t>(a) << 18) |
                         (static_cast<uint32_t>(b) << 12) |
                         (static_cast<uint32_t>(c) << 6) |
                         (static_cast<uint32_t>(d));
            out.push_back(static_cast<uint8_t>((n >> 16) & 0xFF));
            if (s[i + 2] != '=') out.push_back(static_cast<uint8_t>((n >> 8) & 0xFF));
            if (s[i + 3] != '=') out.push_back(static_cast<uint8_t>(n & 0xFF));
        }
        return out;
    }
} // namespace

// Helper: serialise glm::mat4 to vector<float>
static std::vector<float> mat4ToVector(const glm::mat4& m){
    std::vector<float> v(16);
    const float* p = glm::value_ptr(m);
    for(int i=0;i<16;++i) v[i]=p[i];
    return v;
}
static glm::mat4 vectorToMat4(const std::vector<float>& v){
    glm::mat4 m(1.0f);
    if(v.size()==16){ std::memcpy(glm::value_ptr(m), v.data(), sizeof(float)*16); }
    return m;
}

// ------------------------------------------------------------------
// Object
// ------------------------------------------------------------------
void to_json(nlohmann::json& j, const Object& obj){
    j = nlohmann::json{};
    j["geometryType"] = static_cast<int>(obj.getGeometryType());
    j["transform"] = mat4ToVector(obj.getTransform());
    // Persist baseline marker so baseline demo objects remain identifiable after load
    if (obj.hasAttribute("baseline")) {
        j["baseline"] = obj.getAttribute("baseline");
    }
    // Face colours (legacy: 6 faces)
    nlohmann::json faces = nlohmann::json::array();
    for(int f=0;f<6;++f){ faces.push_back({obj.faceColors[f][0], obj.faceColors[f][1], obj.faceColors[f][2]}); }
    j["faceColors"] = faces;

    // If polyhedron, persist vertices and faces so geometry reconstructs on load
    if (obj.getGeometryType() == Object::GeometryType::Polyhedron) {
        const auto& pd = obj.getPolyhedronData();
        nlohmann::json pj;
        nlohmann::json verts = nlohmann::json::array();
        for (const auto& v : pd.vertices) verts.push_back({v.x, v.y, v.z});
        pj["vertices"] = std::move(verts);
        nlohmann::json fcs = nlohmann::json::array();
        for (const auto& f : pd.faces) {
            nlohmann::json fj = nlohmann::json::array();
            for (int idx : f) fj.push_back(idx);
            fcs.push_back(std::move(fj));
        }
        pj["faces"] = std::move(fcs);
        j["polyhedron"] = std::move(pj);
    }

    // Persist mass attribute if present
    if (obj.hasAttribute("mass")) {
        j["mass"] = obj.getAttribute("mass");
    }

    // Per-face textures (composited RGBA8, Base64-encoded)
    if (!obj.faceTextures.empty()) {
        nlohmann::json texArr = nlohmann::json::array();
        for (const auto& ft : obj.faceTextures) {
            // If layers are used, composite into pixels before saving
            if (ft.useLayers) {
                ft.compositeLayers();
            }
            nlohmann::json ftj;
            ftj["size"] = ft.size;
            ftj["pixelsB64"] = base64Encode(ft.pixels);
            texArr.push_back(std::move(ftj));
        }
        j["textureVersion"] = 1;
        j["faceTextures"] = std::move(texArr);
    }
}

void from_json(const nlohmann::json& j, Object& obj){
    int gt = j.value("geometryType", 0);
    obj.setGeometryType(static_cast<Object::GeometryType>(gt));
    std::vector<float> tvals = j.value("transform", std::vector<float>{});
    if(tvals.size()==16){ obj.setTransform(vectorToMat4(tvals)); }
    if (j.contains("baseline") && j["baseline"].is_string()) {
        obj.setAttribute("baseline", j["baseline"].get<std::string>());
    }
    // Load mass attribute (store as attribute string)
    if (j.contains("mass")) {
        try {
            if (j["mass"].is_number()) {
                obj.setAttribute("mass", std::to_string(j["mass"].get<double>()));
            } else if (j["mass"].is_string()) {
                obj.setAttribute("mass", j["mass"].get<std::string>());
            }
        } catch (...) {}
    }
    if(j.contains("faceColors")){
        const auto& faces = j["faceColors"];
        for(size_t f=0; f<faces.size() && f<6; ++f){ obj.setFaceColor(static_cast<int>(f), faces[f][0], faces[f][1], faces[f][2]); }
    }

    // For polyhedron, restore geometry first so textures can size correctly
    if (obj.getGeometryType() == Object::GeometryType::Polyhedron && j.contains("polyhedron")) {
        const auto& pj = j["polyhedron"];
        std::vector<glm::vec3> verts;
        std::vector<std::vector<int>> faces;
        if (pj.contains("vertices")) {
            const auto& vs = pj["vertices"];
            verts.reserve(vs.size());
            for (const auto& vj : vs) {
                if (vj.size() >= 3) verts.emplace_back(vj[0].get<float>(), vj[1].get<float>(), vj[2].get<float>());
            }
        }
        if (pj.contains("faces")) {
            const auto& fs = pj["faces"];
            faces.reserve(fs.size());
            for (const auto& fj : fs) {
                std::vector<int> face;
                face.reserve(fj.size());
                for (const auto& idx : fj) face.push_back(idx.get<int>());
                faces.push_back(std::move(face));
            }
        }
        if (!verts.empty() && !faces.empty()) {
            // Use setPolyhedronData so textures are resized appropriately
            obj.setPolyhedronData(Object::PolyhedronData::createCustomPolyhedron(verts, faces));
        }
    }

    // Load per-face textures if present (after geometry restoration for correct sizing)
    if (j.contains("faceTextures")) {
        const auto& arr = j["faceTextures"];
        int limit = std::min<int>(static_cast<int>(arr.size()), static_cast<int>(obj.faceTextures.size()));
        for (int i = 0; i < limit; ++i) {
            const auto& ftj = arr[i];
            int size = ftj.value("size", (i < static_cast<int>(obj.faceTextures.size()) ? obj.faceTextures[i].size : 64));
            std::string b64 = ftj.value("pixelsB64", std::string());
            if (!b64.empty()) {
                std::vector<uint8_t> data = base64Decode(b64);
                if (size > 0 && static_cast<int>(data.size()) == size * size * 4 && i < static_cast<int>(obj.faceTextures.size())) {
                    auto& ft = obj.faceTextures[i];
                    ft.size = size;
                    ft.pixels = std::move(data);
                    ft.updateWholeGPU();
                }
            }
        }
    }
}

// ------------------------------------------------------------------
// World
// ------------------------------------------------------------------
void to_json(nlohmann::json& j, const World& world){
    j = nlohmann::json{};
    nlohmann::json arr = nlohmann::json::array();
    for(const auto& ptr : world.objects()){
        if(ptr) arr.push_back(*ptr); // relies on Object to_json
    }
    j["objects"] = arr;
}

void from_json(const nlohmann::json& j, World& world){
    if(!j.contains("objects")) return;
    const auto& arr = j["objects"];
    for(const auto& oj : arr){
        std::unique_ptr<Object> obj(new Object());
        from_json(oj, *obj);
        world.addObject(std::move(obj));
    }
} 