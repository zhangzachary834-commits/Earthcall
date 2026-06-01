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
    j["objectID"] = obj.getIdentifier();
    j["transform"] = mat4ToVector(obj.getTransform());
    j["center"] = {obj.getCenter().x, obj.getCenter().y, obj.getCenter().z};
    j["authoritativeAxis"] = {obj.getAuthoritativeAxis().x,
                               obj.getAuthoritativeAxis().y,
                               obj.getAuthoritativeAxis().z};
    j["targetRotation"] = {obj.getTargetRotationEulerDegrees().x,
                            obj.getTargetRotationEulerDegrees().y,
                            obj.getTargetRotationEulerDegrees().z};
    j["rotationResponsiveness"] = obj.getRotationResponsiveness();
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
    if (j.contains("objectID") && j["objectID"].is_string()) {
        obj.setObjectID(j["objectID"].get<std::string>());
    }
    std::vector<float> tvals = j.value("transform", std::vector<float>{});
    if(tvals.size()==16){ obj.setTransform(vectorToMat4(tvals)); }
    if (j.contains("center") && j["center"].is_array() && j["center"].size() >= 3) {
        obj.setCenter(glm::vec3(j["center"][0].get<float>(),
                                j["center"][1].get<float>(),
                                j["center"][2].get<float>()));
    }
    if (j.contains("authoritativeAxis") && j["authoritativeAxis"].is_array() && j["authoritativeAxis"].size() >= 3) {
        obj.setAuthoritativeAxis(glm::vec3(j["authoritativeAxis"][0].get<float>(),
                                           j["authoritativeAxis"][1].get<float>(),
                                           j["authoritativeAxis"][2].get<float>()));
    }
    if (j.contains("rotationResponsiveness")) {
        obj.setRotationResponsiveness(j["rotationResponsiveness"].get<float>());
    }
    if (j.contains("targetRotation") && j["targetRotation"].is_array() && j["targetRotation"].size() >= 3) {
        obj.setTargetRotationEulerDegrees(glm::vec3(j["targetRotation"][0].get<float>(),
                                                    j["targetRotation"][1].get<float>(),
                                                    j["targetRotation"][2].get<float>()));
    }
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
            obj.setPolyhedronData(PolyhedronData::createCustomPolyhedron(verts, faces));
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
// BodyPart (reuses Object faceTexture serialization)
// ------------------------------------------------------------------
nlohmann::json bodyPartToJson(const BodyPart& part) {
    nlohmann::json j;
    j["name"] = part.getName();
    j["type"] = static_cast<int>(part.getType());

    // Primary 3D shape
    j["geometryType"] = static_cast<int>(part.getPrimaryShape());

    // Geometry dimensions
    auto dims = part.getGeometry().getDimensions();
    j["dimensions"] = {dims.x, dims.y, dims.z};
    j["geometryShape"] = static_cast<int>(part.getGeometry().getShape());

    // Base color
    const float* col = part.getColor();
    j["color"] = {col[0], col[1], col[2]};

    // Local transform (relative to body root)
    j["localTransform"] = mat4ToVector(part.localTransform());
    j["center"] = {part.getCenter().x, part.getCenter().y, part.getCenter().z};
    j["authoritativeAxis"] = {part.getAuthoritativeAxis().x,
                               part.getAuthoritativeAxis().y,
                               part.getAuthoritativeAxis().z};
    j["targetRotation"] = {part.getTargetRotationEulerDegrees().x,
                            part.getTargetRotationEulerDegrees().y,
                            part.getTargetRotationEulerDegrees().z};
    j["rotationResponsiveness"] = part.getRotationResponsiveness();

    // Face textures — same format as Object serialization
    if (!part.faceTextures.empty()) {
        nlohmann::json texArr = nlohmann::json::array();
        for (const auto& ft : part.faceTextures) {
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

    // Legacy faceColors
    nlohmann::json faces = nlohmann::json::array();
    for (int f = 0; f < 6; ++f) {
        faces.push_back({part.faceColors[f][0], part.faceColors[f][1], part.faceColors[f][2]});
    }
    j["faceColors"] = faces;

    // Composite sub-objects — save local offsets, not world transforms
    if (part.getSubObjectCount() > 0) {
        nlohmann::json subArr = nlohmann::json::array();
        for (size_t si = 0; si < part.getSubObjectCount(); ++si) {
            const Object* sub = part.getSubObject(si);
            if (!sub) continue;
            nlohmann::json sj;
            to_json(sj, *sub);
            // Override transform with the local offset (to_json wrote world transform)
            sj["transform"] = mat4ToVector(part.getSubObjectLocalOffset(si));
            subArr.push_back(std::move(sj));
        }
        j["subObjects"] = std::move(subArr);
    }

    return j;
}

void bodyPartFromJson(const nlohmann::json& j, BodyPart& part) {
    // Primary 3D shape (must come before texture load so face count is correct)
    if (j.contains("geometryType")) {
        part.setPrimaryShape(static_cast<Object::GeometryType>(j["geometryType"].get<int>()));
    }

    // Geometry dimensions
    if (j.contains("dimensions")) {
        auto dims = j["dimensions"];
        if (dims.size() >= 3) {
            part.getGeometry().setDimensions(glm::vec3(dims[0], dims[1], dims[2]));
        }
    }

    // Base color
    if (j.contains("color")) {
        auto col = j["color"];
        if (col.size() >= 3) {
            part.setColor(col[0], col[1], col[2]);
        }
    }

    // Local transform
    if (j.contains("localTransform")) {
        auto tvals = j["localTransform"].get<std::vector<float>>();
        if (tvals.size() == 16) {
            part.setLocalTransform(vectorToMat4(tvals));
        }
    }
    if (j.contains("center") && j["center"].is_array() && j["center"].size() >= 3) {
        part.setCenter(glm::vec3(j["center"][0].get<float>(),
                                 j["center"][1].get<float>(),
                                 j["center"][2].get<float>()));
    }
    if (j.contains("authoritativeAxis") && j["authoritativeAxis"].is_array() && j["authoritativeAxis"].size() >= 3) {
        part.setAuthoritativeAxis(glm::vec3(j["authoritativeAxis"][0].get<float>(),
                                            j["authoritativeAxis"][1].get<float>(),
                                            j["authoritativeAxis"][2].get<float>()));
    }
    if (j.contains("rotationResponsiveness")) {
        part.setRotationResponsiveness(j["rotationResponsiveness"].get<float>());
    }
    if (j.contains("targetRotation") && j["targetRotation"].is_array() && j["targetRotation"].size() >= 3) {
        part.setTargetRotationEulerDegrees(glm::vec3(j["targetRotation"][0].get<float>(),
                                                     j["targetRotation"][1].get<float>(),
                                                     j["targetRotation"][2].get<float>()));
    }

    // Legacy faceColors
    if (j.contains("faceColors")) {
        const auto& faces = j["faceColors"];
        for (size_t f = 0; f < faces.size() && f < 6; ++f) {
            part.setFaceColor(static_cast<int>(f), faces[f][0], faces[f][1], faces[f][2]);
        }
    }

    // Face textures (same as Object deserialization)
    if (j.contains("faceTextures")) {
        const auto& arr = j["faceTextures"];
        int limit = std::min<int>(static_cast<int>(arr.size()), static_cast<int>(part.faceTextures.size()));
        for (int i = 0; i < limit; ++i) {
            const auto& ftj = arr[i];
            int size = ftj.value("size", (i < static_cast<int>(part.faceTextures.size()) ? part.faceTextures[i].size : 64));
            std::string b64 = ftj.value("pixelsB64", std::string());
            if (!b64.empty()) {
                std::vector<uint8_t> data = base64Decode(b64);
                if (size > 0 && static_cast<int>(data.size()) == size * size * 4 &&
                    i < static_cast<int>(part.faceTextures.size())) {
                    auto& ft = part.faceTextures[i];
                    ft.size = size;
                    ft.pixels = std::move(data);
                    ft.updateWholeGPU();
                }
            }
        }
    }

    // Composite sub-objects — saved transforms are local offsets
    if (j.contains("subObjects")) {
        while (part.getSubObjectCount() > 0) {
            part.removeSubObject(part.getSubObjectCount() - 1);
        }
        for (const auto& sj : j["subObjects"]) {
            auto gt = static_cast<Object::GeometryType>(sj.value("geometryType", 0));

            // Extract the local offset from the saved transform
            glm::mat4 localOffset(1.0f);
            std::vector<float> tvals = sj.value("transform", std::vector<float>{});
            if (tvals.size() == 16) {
                localOffset = vectorToMat4(tvals);
            }

            Object* sub = part.addSubObject(gt, localOffset);
            if (sub) {
                // Load everything except transform (already set by addSubObject)
                glm::mat4 savedWorld = sub->getTransform();
                from_json(sj, *sub);
                sub->setTransform(savedWorld);
            }
        }
    }
}

nlohmann::json bodyToJson(const Body& body) {
    nlohmann::json j;
    j["shape"] = body.shape;
    j["artStyle"] = body.artStyle;
    j["bodyType"] = static_cast<int>(body.bodyType);
    j["proportions"] = static_cast<int>(body.proportions);
    j["height"] = body.height;
    j["weight"] = body.weight;
    j["muscleMass"] = body.muscleMass;
    j["bodyFat"] = body.bodyFat;

    nlohmann::json partsArr = nlohmann::json::array();
    for (const auto* part : body.parts) {
        if (part) {
            partsArr.push_back(bodyPartToJson(*part));
        }
    }
    j["bodyParts"] = partsArr;

    return j;
}

void bodyFromJson(const nlohmann::json& j, Body& body) {
    if (j.contains("bodyType")) {
        body.setBodyType(static_cast<Body::BodyType>(j["bodyType"].get<int>()));
    }
    if (j.contains("proportions")) {
        body.setProportions(static_cast<Body::Proportions>(j["proportions"].get<int>()));
    }
    if (j.contains("height")) body.height = j["height"].get<float>();
    if (j.contains("weight")) body.weight = j["weight"].get<float>();
    if (j.contains("muscleMass")) body.muscleMass = j["muscleMass"].get<float>();
    if (j.contains("bodyFat")) body.bodyFat = j["bodyFat"].get<float>();

    // Match saved parts to existing parts by name
    if (j.contains("bodyParts")) {
        const auto& partsArr = j["bodyParts"];
        for (const auto& pj : partsArr) {
            std::string name = pj.value("name", std::string());
            if (name.empty()) continue;

            // Find matching part in the body
            for (auto* part : body.parts) {
                if (part && part->getName() == name) {
                    bodyPartFromJson(pj, *part);
                    break;
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
