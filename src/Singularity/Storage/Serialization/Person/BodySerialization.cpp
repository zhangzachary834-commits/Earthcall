#include "Singularity/Storage/Serialization/Person/BodySerialization.hpp"
#include "Singularity/Storage/Serialization/ConstructedBeing/ObjectSerialization.hpp"
#include <cstring>
#include <vector>
#include <glm/gtc/type_ptr.hpp>

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
// BodyPart (reuses Object faceTexture serialization)
// ------------------------------------------------------------------
nlohmann::json bodyPartToJson(const BodyPart& part) {
    nlohmann::json j;
    j["name"] = part.getName();
    j["type"] = static_cast<int>(part.getType());

    // Primary 3D shape
    j["geometryType"] = static_cast<int>(part.getPrimaryShape());

    // Geometry dimensions
    auto dims = part.getDimensions();
    j["dimensions"] = {dims.x, dims.y, dims.z};
    j["geometryShape"] = static_cast<int>(part.getPrimaryShape());

    // Base color
    const float* col = part.getColor();
    j["color"] = {col[0], col[1], col[2]};

    // Local transform (relative to body root)
    j["localTransform"] = mat4ToVector(part.localTransform());
    j["center"] = {part.getPrimaryObject()->getCenter().x, part.getPrimaryObject()->getCenter().y, part.getPrimaryObject()->getCenter().z};
    j["authoritativeAxis"] = {part.getPrimaryObject()->getAuthoritativeAxis().x,
                               part.getPrimaryObject()->getAuthoritativeAxis().y,
                               part.getPrimaryObject()->getAuthoritativeAxis().z};
    j["targetRotation"] = {part.getPrimaryObject()->getTargetRotationEulerDegrees().x,
                            part.getPrimaryObject()->getTargetRotationEulerDegrees().y,
                            part.getPrimaryObject()->getTargetRotationEulerDegrees().z};
    j["rotationResponsiveness"] = part.getPrimaryObject()->getRotationResponsiveness();

    // Face textures — same format as Object serialization
    // if (!part.faceTextures.empty()) {
    //     nlohmann::json texArr = nlohmann::json::array();
    //     for (const auto& ft : part.faceTextures) {
    //         if (ft.useLayers) {
    //             ft.compositeLayers();
    //         }
    //         nlohmann::json ftj;
    //         ftj["size"] = ft.size;
    //         ftj["pixelsB64"] = base64Encode(ft.pixels);
    //         texArr.push_back(std::move(ftj));
    //     }
    //     j["textureVersion"] = 1;
    //     j["faceTextures"] = std::move(texArr);
    // }

    // Legacy faceColors
    nlohmann::json faces = nlohmann::json::array();
    for (int f = 0; f < 6; ++f) {
        faces.push_back({part.getPrimaryObject()->faceColors[f][0], part.getPrimaryObject()->faceColors[f][1], part.getPrimaryObject()->faceColors[f][2]});
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
        part.setPrimaryShape(static_cast<ObjectTypes::ShapeKind>(j["geometryType"].get<int>()));
    }

    // Geometry dimensions
    if (j.contains("dimensions")) {
        auto dims = j["dimensions"];
        if (dims.size() >= 3) {
            part.setDimensions(glm::vec3(dims[0], dims[1], dims[2]));
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
        part.getPrimaryObject()->setCenter(glm::vec3(j["center"][0].get<float>(),
                                 j["center"][1].get<float>(),
                                 j["center"][2].get<float>()));
    }
    if (j.contains("authoritativeAxis") && j["authoritativeAxis"].is_array() && j["authoritativeAxis"].size() >= 3) {
        part.getPrimaryObject()->setAuthoritativeAxis(glm::vec3(j["authoritativeAxis"][0].get<float>(),
                                            j["authoritativeAxis"][1].get<float>(),
                                            j["authoritativeAxis"][2].get<float>()));
    }
    if (j.contains("rotationResponsiveness")) {
        part.getPrimaryObject()->setRotationResponsiveness(j["rotationResponsiveness"].get<float>());
    }
    if (j.contains("targetRotation") && j["targetRotation"].is_array() && j["targetRotation"].size() >= 3) {
        part.getPrimaryObject()->setTargetRotationEulerDegrees(glm::vec3(j["targetRotation"][0].get<float>(),
                                                     j["targetRotation"][1].get<float>(),
                                                     j["targetRotation"][2].get<float>()));
    }

    // Legacy faceColors
    if (j.contains("faceColors")) {
        const auto& faces = j["faceColors"];
        for (size_t f = 0; f < faces.size() && f < 6; ++f) {
            part.getPrimaryObject()->faceColors[f][0] = faces[f][0];
            part.getPrimaryObject()->faceColors[f][1] = faces[f][1];
            part.getPrimaryObject()->faceColors[f][2] = faces[f][2];
        }
    }

    // Face textures (same as Object deserialization)
    // if (j.contains("faceTextures")) {
    //     const auto& arr = j["faceTextures"];
    //     int limit = std::min<int>(static_cast<int>(arr.size()), static_cast<int>(part.faceTextures.size()));
    //     for (int i = 0; i < limit; ++i) {
    //         const auto& ftj = arr[i];
    //         int size = ftj.value("size", (i < static_cast<int>(part.faceTextures.size()) ? part.faceTextures[i].size : 64));
    //         std::string b64 = ftj.value("pixelsB64", std::string());
    //         if (!b64.empty()) {
    //             std::vector<uint8_t> data = base64Decode(b64);
    //             if (size > 0 && static_cast<int>(data.size()) == size * size * 4 &&
    //                 i < static_cast<int>(part.faceTextures.size())) {
    //                 auto& ft = part.faceTextures[i];
    //                 ft.size = size;
    //                 ft.pixels = std::move(data);
    //                 ft.updateWholeGPU();
    //             }
    //         }
    //     }
    // }

    // Composite sub-objects — saved transforms are local offsets
    if (j.contains("subObjects")) {
        while (part.getSubObjectCount() > 0) {
            part.removeSubObject(part.getSubObjectCount() - 1);
        }
        for (const auto& sj : j["subObjects"]) {
            // Extract the local offset from the saved transform
            glm::mat4 localOffset(1.0f);
            std::vector<float> tvals = sj.value("transform", std::vector<float>{});
            if (tvals.size() == 16) {
                localOffset = vectorToMat4(tvals);
            }

            Object* sub = sj.contains("shapeKind")
                ? part.addSubObject(static_cast<Object::ShapeKind>(sj["shapeKind"].get<int>()), localOffset)
                : part.addSubObject(static_cast<Object::ShapeKind>(sj.value("geometryType", 0)), localOffset);
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
    j["height"] = body.height;

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
    if (j.contains("height")) body.height = j["height"].get<float>();

    // Match saved parts to existing parts by name, or create new parts if they don't exist
    if (j.contains("bodyParts")) {
        const auto& partsArr = j["bodyParts"];
        for (const auto& pj : partsArr) {
            std::string name = pj.value("name", std::string());
            if (name.empty()) continue;

            // Find matching part in the body
            BodyPart* existingPart = nullptr;
            for (auto* part : body.parts) {
                if (part && part->getName() == name) {
                    existingPart = part;
                    break;
                }
            }

            if (existingPart) {
                // Update existing part
                bodyPartFromJson(pj, *existingPart);
            } else {
                // Create new part from save data
                BodyPart* newPart = new BodyPart();
                bodyPartFromJson(pj, *newPart);
                body.addPart(newPart);
            }
        }
    }
}

